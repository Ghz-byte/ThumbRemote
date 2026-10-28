# 同一固件兼容 OPPO App 与 MatePad HID

日期：2026-10-26（Asia/Shanghai）；进度更新2026-10-27。已批准并完成实施/电脑验证/上传，用户明确反馈手机端和平板端均测试成功。两端基本路径已有用户反馈，具体次数、方向映射、停止/断电/反复切换等专项未逐项确认；系统配对始终用户亲自处理。

## 1. 目标、事实与边界

一份 ESP32-C3 固件，OPPO PLB110 用原 Android App，MatePad mini 用系统 BLE HID；一次控制一台，日常切换先断开再连接，不重刷、不长按、不通过左右滑切模式。只处理真实触摸，上下滑遥控，左右不得产生手机动作。

2026-10-26 接手核对结果：

| 项目 | 核对结果与证据范围 |
|---|---|
| 原 App 路线 | 验收表记录 OPPO 跨应用上下滑及原 USB 充电宝使用获用户反馈通过；不等于所有生命周期异常均通过 |
| MatePad HID | v1 Arrow 在 HarmonyOS 7.0.0.109 原生抖音获双向有效反馈；新直接使用版、合并版均未在平板验收 |
| OPPO HID | 抖音 40.8.0 对 Arrow/Page 均未翻页，系统实际收键有证据；不继续此试验 |
| 当前源码 | touch SHA256 `56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b`；hid SHA256 `8ee49f4f32413717bd0d4116975e6bde58fa2fe3a7e0539e7154f78f77a89a4c`，sketch 显式 Page |
| 板内程序 | 最后上传记录为 Page 对照版；本轮未读 Flash/复位，未独立确认当前运行镜像 |
| 恢复材料 | 四组TEMP清单列出BIN哈希均匹配；调用实际Assert-Verified：touch完整通过，Page因多BIN被拒绝。三组HID目录各有4个未列清单的`*_flashed.bin`，与对应原BIN一致，来源未证实，全部保留；HID当前不能直接重传。未逐字节重新比对Arrow历史源码快照 |
| 当前检查 | `scripts/esp32.ps1 test` 退出 0：446 C++ 检查、9 上传清单检查全部通过；仅验证现有代码 |
| 设备/仓库 | Windows 只读枚举可见 CH343 COM7，未打开串口；工作区全部未跟踪，无 commit 回滚基线 |

保持 SDA=GPIO4、SCL=GPIO5、3.3V/GND 四线和既有触摸阈值。固定 esp32 3.3.10-cn/NimBLE、DIO/80 MHz/4 MB、115200；不新增全局依赖、硬件、显示、电池驱动、鼠标、触控模拟或鸿蒙 App。原 Android 源码及两套独立固件保持可恢复，不提交/推送，不删文件或蓝牙绑定。

## 2. 方案选择与首次使用

用户已选择自动分流。显式模式或断连左右滑选择均不采用。两个服务同时广播但同一笔触摸广播两份事件会重复执行，禁止采用。

单纯采用“App 优先，否则 HID”不足以满足停止语义：App 退出并不作为固件判断系统 HID 连接已结束的依据。建议登记一台 App 主机的稳定绑定身份；其所有后续连接只允许 App 路由，包括重新上电后的连接。这里按协议与登记身份分流，不识别品牌或设备名称。

**本方案新增的一次性操作（已随完整方案批准）：首次合并使用必须先完成 OPPO 的 App 登记，再启用 MatePad HID。**

1. 尚无 App 登记时，固件处于登记等待，任何主机均不得发 HID 导航键。
2. 用户亲自完成 OPPO 的系统配对/加密确认（已有绑定能复用则复用），在原 App 扫描选择设备并开始接收或遥控。原协议的有效 START、当前订阅、已确认稳定绑定身份同时满足后，固件保存该 App 身份并回复原 READY。
3. 原 App 仍保留 10 秒连接/服务发现与 5 秒订阅/START 超时。首次系统配对慢于超时则用户完成配对后主动重试，不延长旧会话、不自动恢复。App 接收测试页也使用 START，可完成登记，但仍只显示方向。
4. 登记成功后无需再登记。App 停止后如系统蓝牙仍连接，换设备前须由用户断开该系统连接；若旧设备反复自动抢连，可暂时关闭旧设备蓝牙，不删除绑定。
5. MatePad 在系统蓝牙连接并就绪，确认硬件松手后，新的上下滑直接生效，无长按、无 60 秒窗口。

首个成功登记身份应为本次用户正在操作的 OPPO。只有该主机连入时进行登记；第二条连接拒绝，不自动抢占。以后更换 App 手机、重置登记或增加多台 App 主机不属于本轮；不得自动覆盖登记记录。无登记而先连接平板只会等待，不会误发键。

## 3. 共享 BLE 架构与协议

新增独立 `firmware/dual_remote/`，唯一一次 `BLEDevice::init("ThumbRemote")`、`createServer`、统一 GAP/安全回调，创建 HID/DIS/库电量服务及原自定义 App 服务。不同时调用现有两个 transport 的 begin，不修改 SDK 源码。

HID 的 DIS/HID/电量三个公开服务先按原顺序注册，App 服务追加；最后唯一一次 `server.start()`。不能调用会提前启动整个服务端的 `hid.startServices()` 再加App；该SDK行为已由独立审查和失败→通过的服务顺序回归核实。尽量减少原 HID 句柄变化，但不承诺旧缓存可直接复用。GATT 表在启动后固定，不随路由删除重建。本地 SDK 已包含 `ble_svc_gatt_init()` 与 `ble_svc_gatt_changed(start,end)`；全部服务注册完成后按 SDK 时序请求全句柄范围 Service Changed，并核对对旧绑定的实际派发/确认，不在每次普通断连时重建表。若缓存/重复配对阻塞，记录并停在迁移检查，不自动删绑定、不调用隐藏 refresh 接口、不假称改名可清缓存。正常设备切换不改变 GATT 表。

| 通道 | 保持的接口 |
|---|---|
| App 服务 | `50101af6-adf1-4884-b512-754d9f980ed0` |
| App Event / START | `d71a4ee9-9da8-4672-9e8f-e6478b2ca2b0` / `b5fb20eb-73b5-4cfc-ba46-d787489c1b80` |
| App 帧 | START 5 字节，版本 1 + 非零 uint32 session；READY/方向 10 字节，原会话/序号/方向含义不变 |
| App 动作 | 固件可发送四方向；原 App 仅在用户开启遥控后执行 UP/DOWN，LEFT/RIGHT 仍只记录 |
| HID | 0x1812，report ID 1，8 字节，Report Protocol；UP→Down Arrow 0x51，DOWN→Up Arrow 0x52；不发 Page |

主广播按 AD 字段实际序列化：Flags 3 字节 + 原 128 位服务 18 字节 + HID 16 位服务 4 字节 + Keyboard Appearance 4 字节 = **29 字节**；扫描响应仅名称 `ThumbRemote`（13 字节含 AD 开销）。原 App 按 UUID 扫描，不依赖名字。应测试原始字节长度、AD 类型/UUID 内容及 SDK 配置返回值，不只看长度算式。

仅一条有效物理连接；HID主机或身份未确认的连接成功后停止广播，额外连接按其handle拒绝，不改变当前状态。当前连接断开后恢复广播。2026-10-27重连修正：已登记App主机在本次encrypted/bonded/identity确认且peer匹配时，连接期间继续广播（包括AppWaiting和AppReady）；这只保留原App的扫描入口，不允许第二条控制连接，不发导航键，不根据已恢复CCCD判定App主动开始。固件不主动连接手机，不承诺阻止主机系统自动重连；切换到另一台主机前仍要断开当前连接。

## 4. 自动路由与停止规则

连接、当前身份未确认、配置读取未完成时均不输出。稳定身份须在本次加密且 bonded 的连接上取得并确认，用 `peer_id_addr` 的规范化类型+6字节地址，不用名称、conn_handle 或 `peer_ota_addr` 的临时地址。RPA 解析/安全事件后重读连接描述符。无法确认身份时停止输出；在真实 SDK 上验证，不能只靠字段名推定。

| 条件 | 路由结果 |
|---|---|
| 登记缺失 | 等待 App 登记；无 HID；满足第 2 节后才进入 App |
| 登记故障/损坏 | 两条路均关闭，明确状态；不当作首次使用，不覆盖旧记录 |
| 当前 peer 等于登记 App peer | App 等待：无新订阅+有效 START 则无输出；HID 即使已订阅也禁止导航 |
| 已登记、当前 peer 不等于 App peer、无 App 占用 | HID 完整就绪后候选；健康且确认释放后接收新笔 |
| 任意当前连接订阅 App Event 或发送有效 START | 立即锁定本连接为 App 意图，取消 HID 待发按下/半笔；HID 不可回退 |
| 非登记 peer 尝试 App START | 锁定为 App 等待但拒绝 READY/导航，不替换登记；换 App 手机需另行设计 |
| 登记 peer 当前有效 START | App 接收路由；App 四方向通知，HID 非零报告始终禁止 |
| App 退订/会话失效/安全丢失 | 结束发送，保留本次 App 锁定；只有新的有效订阅/START 才能再次接收；不回退 HID |
| 连接断开/重连/重启 | 取消旧笔、旧 session、待发报告；登记持久保留，重连重新分类，先确认释放 |

App 与 HID 同时 ready 时 App 意图优先，永不双发。App CCCD 关闭不抹去 App 意图；同一主机的 HID 协议/挂起/CCCD 变化不得重置 App 会话或导致重复 READY。无有效 App 意图的 MatePad 才走 HID。App订阅或有效START在NimBLE回调当场锁存`appClaimed=true`，仅当前连接代数结束才清除；不能等loop轮询，否则订阅→退订的短暂边沿会丢失。最终HID发送直接检查host的该锁存值。

HID→App 意图发生时首先作废未消费的非零报告；若已有可能发出的按下，在旧的当前安全连接发中性全零释放，释放无法可靠提交则断开连接并结束本次会话。中性释放不算导航，不等待释放期间接受新触摸，也不将旧笔转到 App。已交给 SDK 的在途报告无法撤回；保证在最终发送边界仍属于当前路由，不能承诺撤销已经发生的动作。

每笔触摸仅消费一次，结束时不可重新分发；路由/连接/相关会话代数变化立即取消半笔，要求新的可信 Idle 或零触点坐标包后才接新笔。通信错误同样取消；Holding 不当松手；正常非坐标 Flick 包忽略，不当方向、不当释放、不打断有效笔。

HID 延续 40ms 零释放、释放后 500ms 冷却，左右不输出；等待 SDK 的非零报告超过 100ms 丢弃，不积压输入。发送失败不重试旧按下，释放失败终止连接。App 不增加历史队列，保持原 session 和 sequence 校验。

## 5. 身份配置、并发与安全

只新增板端一条 App 角色配置，属于蓝牙路由配置，不是学习日志、屏幕内容或 Android 数据存储。命名空间 `tr_dual`、键 `app_peer`；不存 session、动作、按键、触摸历史，不把身份原值写串口。

配置格式固定 16 字节：ASCII `TRAP` 四字节、版本 1 一字节、规范化地址类型一字节（public=0/random static=1）、地址六字节、前十二字节 CRC32/IEEE 四字节小端。身份类型/地址须验证，不能把未解析 RPA 当静态身份。缺键表示未登记；长度/魔数/版本/CRC/读失败为故障，禁止隐式修复或覆盖。

首次有效登记在 Arduino loop 执行 NVS 写入并读回校验，避免阻塞 NimBLE host 回调；提交结果回到 host 队列时重新核对连接代数/身份/App 请求代数。**必须保存成功后才回复 READY 和允许 App 方向。** 若保存期间断线，可保留确已写入的 App 身份，但不能给下一连接回复旧 READY。保存失败不启用任一路由；不自动 erase 或驱逐 SDK bond。已有一致记录不重复写入。

NimBLE host单线程拥有RoutePolicy、appClaimed及路由权威状态，相关回调逐事件更新，不由loop基于快照重建丢失的边沿。回调只接受当前handle的事件；Arduino loop只读受锁保护的导出快照，负责触摸与配置I/O，保存完成作为带代数的不可变结果交回host。所有实际App/HID通知在同一host队列中，使用显式payload + conn_handle + 连接/路由/通道代数，发送前重新校验。禁止复用原App对所有订阅者的无目标`notify()`。HID单槽沿用既有Delivery，零释放可覆盖待发按下；App使用独立当前帧槽，READY控制帧不可被方向覆盖，忙则丢弃方向，绝不排队历史。

App START 触发的 READY 是幂等会话控制帧；可以等待本次配对/配置完成，但不保存导航事件。重复相同 START 不重置当前序号、不重复触摸；新 session 才新建 App 代数。纯 HID 的无关状态变化不重置 App 代数。

延续无输入输出配对、SC bonding、无固定 PIN、不代确认手机安全提示；合并目标保留 `--wrap=ble_gap_adv_start` 防 SDK REPEAT_PAIRING 自动删除旧 bond，并将身份解析等其他事件完整交给原库。当前固定 SDK 已核对 NVS persist=1、MAX_BONDS=3，`BLEDeviceCallbacks::onStoreStatus` 默认调用 `ble_store_util_status_rr`。合并目标须通过 `BLEDevice::setDeviceCallbacks` 安装自有回调：OVERFLOW/FULL 返回明确存储错误（如 `BLE_HS_ENOMEM`），不委托默认轮换、不删除最旧绑定；注册回调/日志均不可泄露身份或密钥。真实失败分支和两机旧绑定保留尚需实施时验证，未完成不得上传。

## 6. 文件边界与构建

| 文件（批准后创建/调整） | 职责 |
|---|---|
| `firmware/dual_remote/dual_remote.ino` | 唯一 setup/loop、原四线触摸轮询、协调 transport/control；串口仅 status |
| `dual_route_policy.h` | 无 Arduino 依赖：首次登记/身份与互斥路由、App 占用锁定、代数 |
| `dual_touch_control.h` | 无 Arduino 依赖：统一单笔识别、可信释放、App方向/HID Gate协调 |
| `dual_profile.h`、`dual_profile_store.h/.cpp` | 纯身份/配置编码与校验；Preferences持久化适配与读回 |
| `dual_ble_transport.h/.cpp` | 唯一BLE生命周期、服务/安全回调、广播、host队列最终发送门控；若超过易审阅范围可按服务拆实现但不能拆所有权 |
| `firmware/tests/dual_*_test.cpp` | 路由、配置、输入和最终发送请求的纯主机回归 |
| `scripts/esp32.ps1`、`test_esp32_manifest.ps1` | 显式增加dual、正确闭包与清单、恢复校验及失败保护 |

构建时原字节复制 touch 的 `touch_protocol.h`、`gesture_recognizer.h`、`ble_event_protocol.h`，hid 的 `hid_keyboard_gate.h`；独立 transport 和 sketch 不纳入。共享输入纳入源哈希，拒绝重名、缺文件和构建中源变动。默认仍 touch，dual 链接同 HID 保护；dual 上传前校验当前 dual 与 touch 恢复产物。旧 `touch`、`hid` 目标与哈希保持不变。

当前HID历史产物额外文件不删除，也不放宽精确BIN集合校验。批准后为每次新上传创建独立TEMP staging，仅复制已通过完整清单的BIN并逐个校验，Arduino CLI `--input-dir`指向staging，上传辅助文件不能回写已验构建目录；重新上传仍从未污染的构建材料派生新staging。历史被额外文件阻塞的HID目录不得冒充可用，只能在另一个干净TEMP产物目录按对应源码完整重建/验证，不能擅自恢复或只改清单掩盖异常。

实施细化：verify编译前拒绝未知BIN，编译后要求固定SDK产出的五个名称（应用/bootloader/partitions/merged/boot_app0）；上传清单同样拒绝未知名称，即使未知文件被写入清单也不能放行。可选 `-BuildId <英文小写标识>` 为同源另建TEMP根，verify/upload使用同一标识，默认入口不变。

## 7. 验证、实施顺序与通过条件

先纯逻辑回归，再双服务完整编译/链接/清单，再经用户批准上传唯一重新核对的 C3；按下列顺序真机验收，不自动触摸、注键或确认授权。

1. 首次登记：未登记时先接平板无导航；换 OPPO 完成配对、App START、保存与 READY。记录握手是否被原超时阻塞；需要用户主动重试时明确说明。
2. OPPO：原 App 接收页四方向、遥控 UP/DOWN；系统 HID 同时订阅仍零导航，无双执行。停止/锁屏/关闭 App 后无动作，系统保持连接与重新连接两种情况均验证；重新遥控须用户再次开始。
3. MatePad：完整断开 OPPO 后接入；直接上下滑双向效果、左右无导航、无长按，跨 60 秒可用。效果由用户确认，通知 rc0 不代表翻页。
4. 来回切换至少两轮且期间不重刷；旧设备仍连接时拒绝第二台，释放旧设备后可连接；半笔切换/按下临界断线不补发、不粘键。
5. USB 重新上电仍按登记分流；合并版充电宝供电及持续使用单独验证，不沿用旧反馈。旧绑定缓存能否迁移也要单列结果。

完成标准包括两机真实效果、停止不回退、切换不重刷及恢复入口完整。任一未测写“未验证”；无法确认 stable identity、配置保存、旧缓存迁移时停止该阶段，不自动放宽规则或要求删除绑定。

## 8. 依据与尚需实测的假设

- 本地固定 SDK `libraries/BLE/src/BLEServer.cpp`、`BLEAdvertising.cpp`、`BLEHIDDevice.cpp`、`BLEDevice.cpp/.h`、`BLESecurity.cpp/.h`、C3 `dio_qspi/include/sdkconfig.h` 与 NimBLE 头文件：已核对统一回调、额外连接处理、两类服务 AD 字段追加方式、默认 ENC|ID 分发、3个绑定/NVS持久化、重复配对自动删 bond 路径和可覆写的存储满回调。`ble_svc_gatt_changed` 已确认有接口，尚未验证旧主机的实际接收。
- [Apache NimBLE GAP 参考](https://mynewt.apache.org/master/network/ble_hs/ble_gap.html)：区分 identity 与 over-the-air 地址，连接安全状态提供 encrypted/bonded；本方案的持久身份分流是工程设计，仍须在固定 SDK 与两台真机上验证解析和持久性。
- [Bluetooth Core 5.4 GATT 规范](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host/generic-attribute-profile--gatt-.html)：服务表变化须考虑客户端缓存与 Service Changed；不能把一份新固件的编译成功当作旧绑定迁移成功。

具体选择：首次 OPPO 登记优先、仅一台持久 App 主机、所有输出先确认已绑定身份、配置格式与故障关闭策略、双服务统一广播，均随用户本轮“开始”批准。身份解析、缓存迁移与两机真实效果仍需验证，不因批准变为已验收。
