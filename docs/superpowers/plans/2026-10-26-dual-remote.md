# Dual Remote Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 用户已回复“开始”批准实施和验证后上传；本会话顺序实现，不自动提交或推送。实时任务/RED→GREEN/独立审查见WORK_LOG顶部ledger。

**Goal:** 一份固件兼容 OPPO 原 App 与 MatePad Arrow HID，自动互斥分流，日常断开再连接即可切换，不重刷。

**Architecture:** 独立 dual 目标拥有唯一 BLE server，承载原自定义服务与 HID。首次由 OPPO 的有效 App 握手保存稳定绑定身份，此后该主机只走 App，其余已确认身份按 HID 条件使用；纯路由/触摸状态机与 NimBLE 最终发送门控共同阻止双发和旧输入。

**Tech Stack:** Arduino ESP32 3.3.10-cn/NimBLE、C++11 纯逻辑、Preferences/NVS、既有 g++/PowerShell 工具链；Android Kotlin App 不改。

**Spec:** [2026-10-26-dual-remote-design.md](../specs/2026-10-26-dual-remote-design.md)，设计与本计划已随用户“开始”批准；后续执行先读设计及WORK_LOG，不重复请求已覆盖的实施/上传授权。

进度（2026-10-27）：方案已获“开始”批准，任务1–4已完成；任务5已上传并获得手机/平板基本测试成功反馈，其余未逐项确认的专项保留未验证。完成证据/错误修复及有效版本以WORK_LOG顶部为准，下面未勾选项不自动视为通过。

## Global Constraints

- 固定 esp32 3.3.10-cn/NimBLE、DIO/80 MHz/4 MB、115200；SDA=GPIO4、SCL=GPIO5、3.3V/GND。
- 唯一有效物理连接，App/HID 互斥；UP→0x51、DOWN→0x52，40ms释放、释放后500ms冷却、非零报告等待>100ms丢弃。
- 无长按/60秒/左右切模式；连接、通信或路由变化取消半笔，可信释放后才接新笔；正常非坐标包忽略。
- App UUID、5字节START和10字节READY/方向帧不变，Android无代码/权限/UI改动。
- 首次 OPPO 登记成功前无HID；输出须当前 encrypted+bonded+稳定identity。保存成功前不回READY；已登记App peer重连只走App。
- 主广播29字节、响应名称ThumbRemote；固定服务表，保留旧绑定，不自动删除/驱逐。
- 原touch/hid源码与恢复入口保留；默认touch；新文件只进已约定目录，产物只放TEMP，不自动清理。
- 不装新全局依赖、不改.env/系统配置/CI、不提交推送；实施/合并版上传等待完整设计确认，系统授权和真实触摸由用户处理。

## Review Focus

1. App停止后系统HID仍连接或自己重连：登记身份持续禁止HID，含重启（任务1/3/5）。
2. 加密/身份解析晚到、地址轮换、相同handle被复用：旧READY/报告不可发给新连接（任务1/3）。
3. 首次NVS保存断电/失败、配置损坏、已登记peer被别的App覆盖：无隐式回退HID（任务1/3）。
4. HID按下/释放单槽与App接管竞争、READY和方向竞争：不丢关键释放或让App收不到READY（任务2/3）。
5. 旧bond的GATT缓存、SDK配对失败/绑定满默认处理：不自动删bond，也不把电脑检查当迁移通过（任务3/5）。

## 任务1：登记配置与自动路由纯逻辑

**Files:** Create `firmware/dual_remote/dual_profile.h`, `dual_route_policy.h`; Test `firmware/tests/dual_route_policy_test.cpp`, `dual_profile_test.cpp`。先确认根AGENTS的预留结构，再创建目录。

**Interfaces:** 命名空间 `dual_remote`。
- `PeerIdentity { uint8_t type; std::array<uint8_t,6> address; }`，type规范化0/1，支持相等比较。
- `enum class ProfileStatus { Missing, Valid, Fault }; struct Profile { ProfileStatus status; PeerIdentity appPeer; };`
- `using ProfileBytes=std::array<uint8_t,16>; ProfileBytes encodeProfile(const PeerIdentity&); Profile decodeProfile(const uint8_t*,size_t);`，缺键由存储层返回Missing，不把空/坏字节解码为Missing。
- `enum class Route { None, AppWaiting, AppReady, HidReady, Blocked };`
- `LinkSnapshot` 字段：`uint16_t handle; uint32_t connectionEpoch, appEpoch, hidEpoch; bool connected, identityReady, encrypted, bonded, appSubscribed, appClaimed, hidReady; PeerIdentity peer; uint32_t appSession;`。appClaimed由host回调收到App订阅/有效START时当场锁存，仅断开换连接代数清除。
- `RoutePolicy::update(const LinkSnapshot&,const Profile&) -> void`；`route() -> Route`、`generation() -> uint32_t`、`needsEnrollment() -> bool`。由host单线程逐事件调用；loop仅消费快照，不能持另一份权威路由。generation只在影响当前输出的变化递增，不因无关HID变动重发App READY。

- [ ] 先写失败用例：Missing+HID ready无导航；Missing+有效App请求给登记意图但无READY；保存Valid后同peer AppReady；Fault始终Blocked；已登记App peer只有HID订阅时AppWaiting；不同peer纯HID才HidReady；同时ready仅App；App退订后仍不回HID；不同peer App START不可替换登记。
- [ ] 配置测试断言：长度16/魔数TRAP/版本1、类型0或1、往返、单字节篡改CRC失败、未知版本/非法身份/短帧均Fault。地址仅在已确认identity层传入，RPA不能按临时地址登记。
- [ ] 增加重连/重启快照、handle复用、旧appEpoch、加密丢失后恢复、新START与相同START幂等、无关HID变化不改App代数的测试；模拟两次loop之间App订阅→退订，最终host appClaimed仍真，待发HID非零必须被拒绝。
- [ ] 用现有测试约定构造输入，运行 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test`，先确认新用例因接口/行为缺失失败，再实现并全绿；保留原446检查。不要通过跳过新文件获得绿灯。

## 任务2：统一触摸输入与报告投递门控

**Files:** Create `firmware/dual_remote/dual_touch_control.h`, `dual_dispatch_gate.h`; Test `firmware/tests/dual_touch_control_test.cpp`, `dual_dispatch_gate_test.cpp`。引用原纯头文件，不改独立固件。

**Interfaces:**
- `Action { Route route; touch::Direction direction; hid_probe::Report report; uint32_t generation; };`
- `TouchControl::update(Route,uint32_t generation,bool healthy,uint32_t now)`、`observe(touch::QueryState,const touch::Report*,uint32_t now,Action&) -> bool`、`tick(uint32_t now,Action&) -> bool`、`cancel(uint32_t now)`。复用单个GestureRecognizer和Arrow Gate，App路径不套HID冷却。
- `TxRequest { Route route; uint16_t handle; uint32_t connectionEpoch, routeGeneration, channelEpoch; bool readyFrame; remote_ble::Packet app; hid_probe::Report hid; };`
- `DispatchGate::accepts(const TxRequest&,const LinkSnapshot&,Route,uint32_t routeGeneration) -> bool`：最终身份/handle/代数/订阅/路由检查；允许当前安全连接的中性HID释放跨路由，禁止旧连接释放。

- [ ] 先写失败测试：ready但未释放无动作；半笔切路由不分发；Holding/正常非坐标忽略不释放；通信故障恢复等新释放；App四方向各一个Action；HID左右无Action；40ms零释放、500ms冷却、跨60秒仍有效。
- [ ] 先写最终发送用例：HID pending press在App接管后拒绝；旧handle/代数/身份拒绝；当前中性释放允许、旧连接中性拒绝；两路同时ready仍单路；准备好的App方向在退订/新session后拒绝。另测READY优先保留，方向不覆盖控制帧；HID零释放覆盖待发按下及100ms过期边界沿用原Delivery语义。
- [ ] 运行上述全套命令看到预期失败，再实现两个纯头文件，运行全套测试0失败；对“可能按下已发出”的中性释放/断开要求保留任务3的平台验证，不用纯单测冒充SDK结果。

## 任务3：统一BLE服务、身份存储与最终发送

**Files:** Create `firmware/dual_remote/dual_ble_transport.h/.cpp`, `dual_profile_store.h/.cpp`; 使用任务1/2纯逻辑。平台检查在TEMP保留日志，不新增全局库。

**Interfaces:**
- `ProfileStore::load() -> Profile`、`saveFirst(const PeerIdentity&) -> bool`，Preferences `tr_dual/app_peer`，仅缺键允许首次保存、写16字节并读回校验；不覆盖有效不同peer，不修复损坏，不erase。
- `DualBleTransport::begin()`、`poll()`、`state() const -> LinkSnapshot`、`profile() const -> Profile`、`route() const -> Route`、`generation() const -> uint32_t`、`send(const Action&) -> bool`、`cancelInput()`。ProfileStore归transport协调；poll在Arduino loop执行登记I/O，再排队host确认；send只表示接受当前请求，不承诺手机效果。

- [ ] 以已核对的固定SDK为基准，验证identity resolved事件、peer_id_addr与bonded/IRK关系；NVS persist=1/MAX_BONDS=3。通过 `BLEDevice::setDeviceCallbacks` 覆写 `onStoreStatus`，OVERFLOW/FULL返回 `BLE_HS_ENOMEM`，禁止调用默认 `ble_store_util_status_rr`；用桩事件验证两分支无删除调用。服务注册完成后按SDK有效时序调用 `ble_svc_gatt_changed(1,0xffff)`，记录实际派发/确认供任务5检查；不能清bond后假称兼容。
- [ ] 创建唯一BLE初始化/服务器/安全与GAP回调；host拥有RoutePolicy，App意图当场锁存而非loop推断；保留REPEAT_PAIRING包装及其他事件委托；在同handle上同时承载两服务，非当前handle回调不能改状态；额外连接不影响当前会话。按照设计实现29字节原始AD及扫描响应，检查SDK返回值。
- [ ] App按原订阅/START握手；相同START幂等，新session新代数；加密完成前可暂存本次控制请求但无方向历史。先核对稳定身份再登记；NVS保存/读回成功并在host重新核对token后才READY。不同peer或存储故障明确等待/错误，不能转HID。
- [ ] 复用原HID报告描述与安全策略，改为Arrow；只读Input Report恒零，不发布虚构电量；HID状态变化与App通道代数隔离。
- [ ] 两路实际通知都改为host事件队列显式payload+handle：发送前用DispatchGate和当前SDK描述符再次核对；READY独立控制优先级，方向忙丢弃；HID待发按下作废后完成中性释放再处理App接管，失败则断开，不能取消释放后继续接受App方向。
- [ ] 存储适配用可替换读写接口或测试桩注入：不存在、读取错误、短写/读回错、旧profile不同peer、保存途中断线/同handle新连接。断言无错误READY、无HID回退、无自动覆盖；故障用例追加现有dual_profile/dispatch测试。真实Preferences原子性和断电恢复留任务5实物检查。
- [ ] 完成静态自审：没有旧transport begin、全局notify广播、peer地址/密钥日志、自动清bond、回调里阻塞写NVS；运行全套主机检查。完整Arduino编译依赖任务4接入，未编译不声明平台通过。

## 任务4：sketch、构建闭包与恢复保护

**Files:** Create `firmware/dual_remote/dual_remote.ino`; Modify `scripts/esp32.ps1`, `scripts/test_esp32_manifest.ps1`，按需更新根AGENTS验证命令。

**Interfaces:** `scripts/esp32.ps1 verify|upload -Target dual`，默认仍touch。dual输入=新目录普通文件+原touch三个共享头+hid_keyboard_gate.h，无原独立transport/sketch；目标、链接选项、全部输入和每个BIN绑定清单。

- [ ] 先扩清单测试，用实际函数校验dual正常包、拿touch/hid包冒充dual、缺BIN/多BIN、错源码/共享头、漏wrapper、构建中变更和恢复清单失效均拒绝；拒绝发生在任何设备访问前。分别保持touch/hid原正例，不能只测dual负例。将本次`*_flashed.bin`额外文件作为拒绝回归；上传staging独立性测试：模拟CLI在staging写辅助BIN不污染build，下一次仍能从原清单复制验证，新复制/源被篡改均拒绝。
- [ ] 添加显式目标映射dual→dual_remote，拒绝未知目标；dual和hid均用 `-Wl,--wrap=ble_gap_adv_start`；dual上传前验证touch恢复材料。每次upload从完整通过Assert-Verified的build复制清单BIN到唯一TEMP `upload-<GUID>/`并逐个校验，CLI只用该input-dir，不放宽build精确集合检查。若重构脚本函数便于测试，保持旧目标哈希计算原样并对比本次核对值。
- [ ] sketch沿用hid直接版四线与20ms轮询、I2C错误/健康恢复和非坐标忽略策略；统一TouchControl产出一次Action，使用transport当前代数；tick零释放不被串口流饿死；串口只支持status，状态不泄露地址。
- [ ] 运行 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test` 全绿，然后 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify -Target dual` 完整编译，确认exit0与最终manifest；检查ELF确有wrapper且实际调用受保护，广播编码静态检查通过。构建失败找根因，不加跳过标记。
- [ ] 核对touch源码哈希仍56789245aac4…、hid仍8ee49f4f3241…，touch完整恢复预检通过；三组历史HID额外BIN保留并标为不能直接重传，不声称其完整预检通过。没有HID恢复需求时不为形式重建历史版本；如获准恢复需另建干净TEMP根完整编译验证，不能删额外BIN或改清单放行。共享输入若变化须解释并重新验证恢复目标。
- [ ] 代码/清单/文档自审并修复重要问题；记录新主机检查数量、内存/程序字节、完整源hash与产物路径。Android无改动不重复构建。此时仍不能声称合并版硬件可用。

## 任务5：经确认上传、旧绑定迁移与两机验收

**Files:** Update `docs/acceptance.md`, `docs/WORK_LOG.md`, `docs/hid-probe.md`、必要时README；不删除历史。

**Interfaces:** 用户批准的完整设计与上传范围 + 任务4 VERIFIED dual产物；现行停止/权限边界不变。

- [ ] 若完整设计尚未获确认，停在本计划；确认实施但未覆盖合并版上传时，先完成可审阅产物后只补问上传权限。不得沿用旧独立hid上传授权。
- [x] 上传前重新核对唯一CH343端口及ROM C3；确认dual与touch恢复清单/产物无变；按获批范围执行 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 upload -Target dual -Port COM7`（端口变动重新核对，不盲用），记录完整日志与写入校验；不整片擦除。
- [ ] 用户处理系统配对；按设计第7节先验证未登记时不发HID，再OPPO登记/READY和原App效果；旧bond服务发现异常先记录Service Changed/缓存证据，阻塞就停，不删绑定或反复重刷碰运气。
- [ ] 验证App停止/锁屏后系统HID仍订阅无导航，OPPO系统重连仍只App等待；测试App与HID同时ready无双执行，重新遥控只能用户再次开始。用户主动操作手机，助手不ADB注键/授权。
- [ ] 断开OPPO再MatePad，用户确认上下双向效果、左右不导航、无长按且超过60秒仍工作；两机来回至少两轮不重刷。另测第二设备不能抢占、半笔断线无补发、按下临界断线不粘键。
- [ ] 用户安排USB重新上电与合并版充电宝测试：登记仍保留；OPPO未开始无HID、MatePad重新就绪后只接新笔。首次登记断电故障用可控存储故障覆盖，真断电测试仅用户愿意且获具体安排后实施，不为测试清旧配置/绑定。
- [x] 更新逐项验收和日志：源码/构建/上传/连接/用户效果反馈分开，未测项明确保留；不用notify rc0代替用户反馈，不把原App/v1 Arrow结果移植为合并验收。保留恢复入口，回滚只有用户选择后执行。

## 执行入口与检查清单

- [x] 用户确认完整设计（含首次OPPO登记/绑定身份存储）及本计划；“开始”覆盖电脑验证后上传重新核对的唯一C3。
- [ ] 任务1→2→3→4顺序执行，每个纯逻辑用例先失败后通过；无需新worktree，当前无提交基线，不能用Git回滚替代恢复产物。
- [ ] 任务5按实物可用情况逐项确认，缺平板则平板/切换项保持未验证，不用OPPO键盘试验替代。

本轮已进行计划自审：设计第1–2节对应任务1/5，第3节对应任务3/4，第4节对应任务1/2/3，第5节对应任务1/3，第6节对应任务4，第7节对应任务5。纯逻辑测试不能覆盖的SDK身份/存储/旧缓存假设明确留在任务3/5验证，不暗示已通过。
