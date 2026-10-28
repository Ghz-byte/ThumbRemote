# ThumbRemote 项目规则

## 范围与推进
- 2026-10-27 用户审阅MatePad点击设计及计划后回复“动手动手”，授权按`docs/superpowers/specs/2026-10-27-matepad-tap-design.md`和对应计划实施、验证并更新唯一核对的C3。现有上下滑保留，双点不加固定冷却；新增HID绝对坐标输入只对真实MatePad触摸生效，兼容性和缓存待实测。保留8dceb固件恢复产物、touch/hid独立源码与全部绑定，不清NVS/配对、不整片擦除；系统配对用户处理。
- 2026-10-27 用户确认手机端触摸板单击屏幕中心、快速两次单击、取消遥控动作结束后固定500ms冷却的方案并说“开始吧”。允许实现、验证并在电脑验证通过后更新唯一已核对C3和唯一已授权OPPO；系统授权仍由用户操作。单击首笔无需等待双击判定；执行中允许短时有界按序等待，过期/停止/断线清除、不补发。保留独立touch/hid源码与恢复入口、既有绑定及MatePad方向键路线；不新增平板点击或权限。
- 2026-10-27 OPPO重连再次出现App扫不到，串口确认已登记App身份的加密HID连接仍占用、App session为0；用户也确认卡在扫描。按现有合并联调授权修复发现入口：仅对已登记且本次加密/绑定/identity确认的App主机，连接期间继续广播供原App扫描；其他HID主机连接时停广播，第二条连接仍拒绝，执行路由/START门控不变。此为现有重连缺陷修复，不增加并发控制或手机动作，不清绑定、不改Android。
- 2026-10-26 用户在审阅合并设计/计划和“电脑验证通过后上传到重新核对的唯一C3”范围后回复“开始”，确认按2026-10-26-dual-remote-design.md/dual-remote.md实施、验证并上传。包括首次OPPO App稳定绑定身份登记、统一BLE服务端、自动互斥路由和dual构建；系统配对仍由用户处理。保留旧源码/产物/绑定，不整片擦除。此前仅文档的等待状态解除，不重复请求本次已覆盖的实施/上传授权。
- 2026-10-26 接手TR-20261026-DUAL-01：用户要求先核对状态、制定同一固件App+HID合并设计与实施计划，并明确选择“自动分流，日常只断开再连接”。本轮仅授权核对与文档；首次App身份登记、绑定身份持久化、共享服务端、dual构建目标等细节属于待审阅方案，未授权实现/烧录。完整文档确认前不改产品代码，不沿用旧独立固件上传授权。
- 2026-10-26 用户询问能否兼容原App与HID，在获知可合并为一份固件（OPPO走App、MatePad走HID、一次控制一台、切设备不重刷）后回复“ok我换一个对话直接开始”。下一目标为合并方向，当前只保存接续材料；尚未形成/确认完整合并设计，不把旧独立固件上传授权扩大为未审阅的新合并行为。新对话先读docs/WORK_LOG.md交接区TR-20261026-DUAL-01，按大改动先出具体方案再实施；不要再次问用户在原App与HID之间二选一，也不要自动恢复/刷写独立固件。
- 2026-10-26 OPPO实际收到KEY_DOWN按下/释放而前台抖音未翻页，按原已批准HID兼容性方案的“方向键无效才测Page组”开展一次PageDown/PageUp对照；仍是连接直接使用、真实上下滑唯一触发，不增加鼠标/系统快捷键等动作。此为键盘兼容性实验，不承诺OPPO支持，不将其结果推广到已通过方向键的MatePad；保留Arrow恢复入口与原touch产物。
- 2026-10-26 长按启停在OPPO本轮未触发开启，用户随后明确批准“按这个直接使用方案改”：取消长按和60秒门控，通电并连接系统蓝牙后，确认触摸释放即可由新的真实上下滑持续遥控；断开/断电停止，重连不补发历史动作。允许实现/验证/上传唯一已核对C3。此条取代下方长按/串口arm作为HID日常输入前提；保留旧源码和touch恢复入口，不新增动作、硬件或App改动。
- 2026-10-26 用户选择“长按启停吧”，批准已展示的HID独立供电使用方案：默认待机，同一点长按3秒并松手切换遥控，物理开启后不受60秒限制；重启、连接状态变化或触摸通信异常回待机，真实上下滑仍是唯一导航来源。允许在原hid目标中实现/测试/编译并上传已核对的唯一C3，保留touch恢复入口；不新增接线、显示、电池电路或Android改动。串口arm仍保留60秒诊断窗口，断线不恢复已开启状态。精确抖动/长接触过滤由docs/superpowers/specs/2026-10-26-hid-long-press-design.md说明。
- 2026-10-26 用户明确“不，按HID来”，恢复下述已确认最小HID设计/计划及临时烧录授权，暂停状态解除。按原范围实施键盘导航试验、保留恢复原OPPO固件入口，不推进此前提出的软件界面改动。系统配对由用户亲自确认，导航键仍仅在显式arm后由真实触摸触发。
- 2026-10-25 用户随后明确“先别开发这个了，我就接着用软件”，暂停HID路线，当前继续使用原Android App/原touch_probe v3。暂停时只更新了HID设计/计划/规则，未创建HID产品代码、未改构建脚本、未编译或烧录HID。下方HID实施授权暂停生效，除非用户明确恢复，不继续HID代码、构建、上传或配对试验；保留文档，不删除文件。
- 2026-10-25 用户审阅最小BLE HID方案及其对OPPO连接的影响后回复“行，接着来吧”，授权按 docs/superpowers/specs/2026-10-25-hid-probe-design.md 实施、电脑验证、临时烧录到唯一已识别C3；原Android固件保留，测试后按用户选择恢复。仅HID键盘导航、显式串口arm后真实触摸触发，不自动发键、不代确认配对；不包括鼠标/触控模拟、原生鸿蒙App或其他动作。
- 2026-10-25 用户审阅 docs/superpowers/specs/2026-10-25-ble-control-design.md 后回复“实施”，授权上下滑遥控、connectedDevice前台服务与通知权限、跨应用即时执行；电脑端验证后安装Debug APK到唯一已授权OPPO。仅用户前台点击开始后保持BLE，左右只记录，停止/锁屏/断线/权限或辅助功能丢失结束，无自动恢复/补发。保持固件与接线，不自动开启遥控或操作系统授权。此前用户停止的测试保留未验证，不强制补测。
- 当前允许阶段A/B、阶段C触摸/串口手势测试、BLE前台收消息及上述已确认上下滑遥控；电池、显示驱动及其他手机动作不在范围内。
- 2026-10-25 用户在审阅完整设计与实施计划后回复“实现”，授权按 docs/superpowers/specs/2026-10-25-ble-receiver-design.md 实现、验证，并烧录到唯一已识别C3、安装Debug APK到唯一已授权OPPO手机。BLE仅在App前台接收/显示方向，不调用辅助功能；onStop断开，不自动重连，不补发。可新增附近设备SCAN/CONNECT权限，由用户亲自允许；不安装新全局依赖，不修改系统设置。
- 2026-10-25 早期仅授权准备BLE方案；该等待确认状态已由上条用户“实现”的明确授权取代。实际范围以已确认设计为准，不自动扩展为手机控制。
- 2026-10-24 用户同意编写并烧录 IT7259 最小触摸测试固件，知晓会替换 GetChipID 示例。仅限 SDA=GPIO4、SCL=GPIO5、3.3V/GND 的四线连接、I²C 通信诊断及触摸/坐标/松手串口输出；不写触摸芯片配置或升级固件、不主动驱动复位、不整片擦除。上传前核对唯一目标端口与芯片；连接未就绪时先完成电脑端验证，不重复请求既有授权。
- 2026-10-24 用户进一步确认软件手势识别方案，授权修改并重新烧录：沿用本轮实测朝向，上/下/左/右在松手后判定，一次接触最多一个事件；过滤短距离/轴向不明确输入，通信异常取消本次判断。只在串口显示结果，仍不实施 BLE、显示、电池或 Android 改动。
- 2026-10-21 用户已授权安装 Arduino IDE 并继续准备 ESP32 工具链；允许安装固定版本的官方开发板支持包并识别已连接芯片。此授权不视为完整阶段 C 固件、触摸接线或 BLE 实施许可；编写和烧录测试固件前先确认具体方案。
- 2026-10-22 用户在知晓会替换板上程序后明确说“开始上传”，授权将已编译的官方 GetChipID 串口示例上传至已识别的 ESP32-C3（COM7、4 MB Flash），并读取串口验证。仅覆盖此次独立开发板测试，不扩展到触摸接线、显示、BLE 或其他固件；不执行整片擦除。
- 本块开发板的串口示例实测配置为 `esp32:esp32:esp32c3:FlashMode=dio`，Flash 80 MHz / 4 MB，串口 115200；默认 QIO 写入校验通过但启动报分区表错误，改 DIO 后运行通过，证据见 docs/esp32-toolchain.md。后续不得无依据恢复默认 QIO，也不将该配置推广到其他板卡。
- 教学优先，说明关键设计；用户已于 2026-10-21 明确授权创建第一阶段 App。
- 新硬件参数必须附资料或实测依据，未知项写“待验证”，不得猜测接线或要求通电。
- 遵守上级 AGENTS.md 的删除、凭据、系统配置、数据库、推送和公开发布红线。未经用户要求不提交或推送 Git。

## 目录与命名
- `firmware/dual_remote/dual_digitizer.h`：MatePad HID绝对坐标报告及单点状态机；`dual_keyboard_gate.h`：dual专用无额外冷却方向键门控。仅扩展现有dual目录，测试仍放`firmware/tests/`，独立hid/touch恢复源码不改。
- 合并设计草案预留 `firmware/dual_remote/`：独立合并sketch、纯路由/触摸协调、统一BLE适配与App身份配置存储；确认设计后才创建。仅复用原touch协议/手势/App编码及HID纯门控头文件的原字节副本，不复制两套独立transport同时初始化；测试仍放 `firmware/tests/`，计划目标为显式 `-Target dual`，默认touch及原touch/hid恢复入口保留。此项只定义未来目录用途，不构成实施授权。
- `android/`：Android Gradle 工程。Kotlin 包名 `com.thumbremote.app`；类用 PascalCase，方法用 camelCase，Android 资源用 snake_case。
- `docs/`：中文设计、阶段验收、硬件证据及进度；计划放 `docs/superpowers/plans/`，设计放 `docs/superpowers/specs/`。
- `docs/third-party-notices.md`：项目源码使用的第三方许可和版权声明。
- `docs/WORK_LOG.md`：工作日志与跨会话接续入口，记录已完成工作、验证依据、未测项及下次起点；暂停前更新，继续工作时先读本文件和验收表。
- 测试状态仅按明确反馈或工具证据记录。“可以了”“都 OK”不能自动推广到未逐项确认的测试；用户纠正后同步修正日志、进度和验收表。
- `scripts/`：项目本地 PowerShell 构建、测试、安装入口，不修改永久环境变量。
- `firmware/`：已授权的阶段C源码；`firmware/touch_probe/` 保存sketch、触摸协议、gesture_recognizer.h及独立ble_event_protocol.h/ble_transport.h/.cpp，协议/识别/传输分离；`firmware/tests/` 保存主机端C++测试。文件使用英文snake_case。产物仅放TEMP，不自动清理。
- `firmware/hid_probe/`：独立HID测试sketch、hid_keyboard_gate.h纯报告/arm/释放门控及hid_keyboard_transport.h/.cpp的NimBLE适配；原touch_probe源码不改。构建HID镜像时按原字节副本复制共享touch_protocol.h/gesture_recognizer.h并纳入源码哈希，测试放已有firmware/tests。scripts/esp32.ps1显式Target=touch|hid，默认touch；清单绑定目标/源码/构建选项/产物，HID上传前校验touch恢复产物。允许仅HID目标增加链接包装ble_gap_adv_start以拒绝SDK自动删除旧bond的重复配对路径，不改SDK源码或跳过官方构建。
- `firmware/hid_probe/hid_touch_control.h`：纯触摸输入分类与长按启停识别，复用原手势识别器，隔离启停接触与方向手势；对应测试仍放`firmware/tests/`，不新建目录。
- `firmware/hid_probe/hid_direct_control.h`：当前直接使用模式的纯连接/健康状态/释放/手势协调，复用Gate与原GestureRecognizer；旧长按头文件和测试保留作为历史，不再接入sketch，不删除文件。
- `android/app/src/main/java/com/thumbremote/app/ble/`：BLE协议、纯接收会话、Android平台Gatt适配，不引用辅助功能执行模块；对应 `android/app/src/test/java/com/thumbremote/app/ble/` 保存JUnit测试。文件名PascalCase，与Kotlin类对应；仅内存最近30条记录，无数据库或文件日志。
- 现有Android app包内的 `RemoteControlSession.kt` 保存纯遥控门控与最近30条内存结果，`RemoteControlService.kt` 保存connectedDevice服务/绑定/通知和BLE至辅助功能协调，`RemoteControlStatus.kt` 保存进程内展示快照与观察者；对应测试放现有app测试包。BLE适配只回调新接受事件，不引用执行模块；服务持有独立Gatt，接收测试页面仍离开即断开。
- Gradle 缓存、构建产物、机器 SDK 路径、IDE 本地状态、截图和临时日志不得入 Git。临时文件仅放系统临时目录或被忽略的构建目录；需要清理时先取得用户删除授权。
- Windows AGP 拒绝中文工程路径。`scripts/android.ps1` 将 `android/` 的输入文件（排除缓存/产物）按内容哈希复制到系统临时目录 `thumbremote-<hash>/`，在英文路径构建，APK 与报告复制回 `android/app/build/`。镜像是生成文件，不在其中编辑源码；哈希变化产生新目录，不自动删除旧目录。
- Gradle 的测试 worker 同样要求缓存路径可正确编码。构建脚本使用系统临时目录 `thumbremote-cache-<工作区路径哈希>/gradle-home` 作为该工作区专用缓存；首次可复制已下载的 `.tools/gradle-home` 中的 caches/wrapper 以避免重复下载，不移动或删除原缓存。
- 创建新目录前在本文件或其目录级规则中先说明用途；后续结构变更先改规则。
- 卖家资料静态检查使用系统临时目录 `thumbremote-seller-review-<压缩包哈希前12位>/`，仅保存选取的 PDF、源码副本、提取文本和原理图预览；保持资料相对目录，不执行包内程序或脚本，不自动清理。结论和来源记录在 docs/hardware-evidence.md。
- ESP32 工具链遇到 EFS 源文件复制失败时，允许在系统临时目录 `thumbremote-esp32-toolchain-<版本>/` 创建生成的构建镜像：平台源码与目标芯片 SDK 以内容副本保存并逐文件校验，其他官方工具目录可用 junction 只读复用现有安装。通过仅当前进程的 `ARDUINO_DIRECTORIES_DATA` 指向镜像，构建产物另放临时目录；不在镜像编辑源码、不跳过官方构建步骤、不自动删除源安装或镜像、不修改永久环境变量。原安装的 EFS 属性修复失败时如实保留该限制。

## Android 行为与隐私
- Kotlin + 原生 Views，minSdk 24。固定兼容的 AGP/Gradle/Kotlin 版本，不采用动态版本。
- 不申请INTERNET、定位、后台蓝牙、悬浮窗、截图或读取窗口内容能力，不采集屏幕内容和个人数据。已批准的前台BLE例外仅API31+的BLUETOOTH_SCAN（neverForLocation）/BLUETOOTH_CONNECT；旧系统保留手动测试。手机授权由用户亲自确认，不通过ADB授予。
- 已批准遥控新增FOREGROUND_SERVICE、FOREGROUND_SERVICE_CONNECTED_DEVICE和POST_NOTIFICATIONS；后者由用户亲自允许，通知不可见时不启动遥控。API31+支持。用户在前台点击开始后，切换应用可保持该会话；锁屏/断线等结束，不自动重连、不恢复历史会话。
- 原手动操作必须由用户点击发起；遥控例外仅在用户主动开启的会话中接收硬件UP/DOWN和单击，LEFT/RIGHT不执行。两条路径互斥，手势执行串行；遥控取消固定冷却，执行忙时可短时有界按序等待，过期/停止/锁屏/断线清除、不跨会话补发；手动测试原有冷却保持。倒计时、取消、锁屏、服务断开和超时必须有明确结果。
- 页面离开后允许一次性倒计时继续；服务重启或进程重启不恢复待执行任务。
- 系统手势回调“完成”不代表目标 App 已切换视频。单击不保证播放/暂停。
- 辅助功能授权与手机安全提示由用户亲自确认，不通过 ADB 自动授予。

## 验证与交付
- 合并版显式 `scripts/esp32.ps1 verify -Target dual` / `upload -Target dual -Port COM7`；默认仍touch。dual上传校验touch恢复清单，全部目标上传用独立TEMP staging副本，原build的精确BIN集合保护不放宽。`dual_advertising.h`保存经主机测试的29字节广播与扫描响应；`dual_dispatch_gate.h`保存最终发送纯校验与App控制帧优先槽，文件均放已约定dual_remote目录。
- `firmware/dual_remote/dual_service_setup.h`：可主机验证的服务启动协调，保证App服务在HID一次性启动整个GATT表前注册；避免SDK提前冻结服务/通知表。回归放现有firmware/tests，不新建目录。
- `firmware/dual_remote/dual_bond_callbacks.h`：与SDK回调基类绑定的保留bond存储回调；FULL/OVERFLOW不委托默认驱逐。主机以最小SDK替身注入两类事件验证虚派发和无删除副作用，不伪装真实NVS满载测试。
- `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test`：主机端协议与状态测试；`verify`：测试并完整编译最小触摸固件。`upload -Port COM7`：仅向已核对的唯一 CH343/ESP32-C3 上传已构建产物；串口变动时不得盲选。沿用 DIO/80 MHz/4 MB，115200 串口。
- HID显式使用 `scripts/esp32.ps1 verify -Target hid` 和 `upload -Target hid -Port COM7`；恢复用 `verify -Target touch` / `upload -Target touch -Port COM7`。上传不整片擦除、不自动删除任何绑定或确认系统安全弹窗。
- `powershell -ExecutionPolicy Bypass -File scripts/android.ps1 test`：本地单元测试。
- `powershell -ExecutionPolicy Bypass -File scripts/android.ps1 verify`：单元测试、Lint、Debug APK 构建。
- `powershell -ExecutionPolicy Bypass -File scripts/android.ps1 install`：构建并安装到唯一已授权真机，禁止自动选择多台设备中的任意一台。
- 每次修改说明变更、构建命令、实测结果及限制。构建/测试失败必须排查原因，不加跳过或绕过标记。
- 未实际完成真机步骤，验收记录必须标记“未验证”，不得宣称硬件或视频切换已可用。
