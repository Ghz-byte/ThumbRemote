# ThumbRemote BLE 前台收消息设计

日期：2026-10-25。状态：**用户已明确回复“实现”，按本方案实施中；实际验证进度见WORK_LOG。**

## 目标与交付边界

把已验证的四方向串口事件，通过 BLE 送到现有 ThumbRemote Android App，并在手机页面显示方向、收到时间和本次接收数量。

成功的具体表现：用户打开 App、点击扫描并亲自允许附近设备权限，选中开发板连接；页面显示“可以接收”，触摸板松手后手机出现对应方向。关闭辅助功能服务仍应能完成收消息测试。本阶段的蓝牙事件不调用辅助功能或发送系统手势。

阶段一只验证收消息；真正控制短视频是之后的独立方案。后台常驻、切出 App 后保持连接、自动重连、配对绑定、OTA、显示驱动、电池与省电均不实施。本次无需改四根线、补焊或购买材料。

## 已有事实与实现选择

- 当前板内为 touch_probe v2：167项主机检查通过；四方向各记录3条正确事件，6次轻点和3次斜滑均按规则过滤，通信errors=0。详见 WORK_LOG 的v2节。
- 保持 SDA=GPIO4、SCL=GPIO5、3.3V/GND；DIO/80 MHz/4 MB，115200串口。GPIO、供电和屏幕轴向不重设。
- 当前握持朝向下 X减/增对应上/下，Y减/增对应右/左。只传最终方向，不传原始坐标；旋转屏幕需另行对应，不做自动旋转。
- Android现有 Kotlin+原生Views，minSdk24、targetSdk36、compileSdk37；已有OPPO PLB110/Android16真机。原手动手势路径与辅助功能权限不因接收BLE被调用。
- 选择自定义 GATT 服务而非 HID：两端协议可检查，适合先在现有App中验证方向；HID会引入另一套系统输入映射，不作为本轮路线。
- 固件复用固定 esp32:esp32 3.3.10-cn 包内自带 BLE 库；Android使用平台 BluetoothGatt，不新增全局工具、SDK或第三方BLE依赖。当前本地BLEServer/Characteristic/Descriptor API已只读核对，完整编译和运行能力仍待实施后验证。

## 使用流程与页面

在现有 MainActivity 的原生布局中增加“蓝牙接收测试”卡片，放在手动手势测试区域之前，沿用现有样式。卡片包含：

1. 扫描/停止扫描按钮、连接状态和错误说明。
2. 本轮匹配设备列表，名称缺失时显示“未命名设备”，用户选中后才连接。
3. 断开按钮、最近方向、收到时间、当前会话接收数。
4. 最近30条会话内记录，包含正常方向与丢弃原因计数；仅内存保留。清空记录不触发重连或任何手势。

用户可继续点击原手动测试按钮，但BLE接收路径没有到 ThumbAccessibilityService 的调用。该卡片明确标注“仅显示收到的方向”。不展示UUID、包字节等实现细节。

扫描10秒自动停止；按服务UUID过滤，不凭设备名称自动连接。连接目标后停止扫描，避免持续扫附近设备。离开页面/锁屏导致 onStop 时停止扫描、断开并close；回到页面不自动恢复，用户再次扫描/连接。进程重启不恢复会话或历史记录。

## 权限与平台

- App minSdk24保持；本轮BLE功能仅在API31+开放，旧系统显示说明且不调用蓝牙API，原手动测试可用。
- 新增 BLUETOOTH_SCAN（neverForLocation）与 BLUETOOTH_CONNECT，点击扫描时请求附近设备运行时权限。无需手机广播，不申请 BLUETOOTH_ADVERTISE；不申请定位、INTERNET、后台定位、通知或前台服务权限。
- android.hardware.bluetooth_le 声明 required=false；运行时检查硬件能力、适配器状态和扫描器可用性。
- 拒绝权限显示明确结果；不循环请求、不跳过检查、不通过ADB授予。用户可再次点按钮主动重试。系统蓝牙开关由用户处理，不替其改变设置。
- 用户开启蓝牙或授权流程返回后，仅在当前页面仍有效、原请求仍有效时继续；若页面已停止则不恢复，提示重新点扫描。
- 权限撤销、蓝牙关闭、SecurityException、扫描错误等均结束本次会话；清理本地状态即使平台disconnect失败也必须完成。

权限依据：[Android Bluetooth permissions](https://developer.android.com/develop/connectivity/bluetooth/bt-permissions)。API31+使用附近设备权限与neverForLocation是官方提供的路径；是否在本机稳定扫描仍需真机验证。

## BLE 服务与数据协议 v1

固定自定义UUID（本次生成，两个端点与测试共用）：

| 对象 | UUID / 属性 |
|---|---|
| 服务 | `50101af6-adf1-4884-b512-754d9f980ed0` |
| 事件 | `d71a4ee9-9da8-4672-9e8f-e6478b2ca2b0`，Notify，无Read/Write |
| 会话启动 | `b5fb20eb-73b5-4cfc-ba46-d787489c1b80`，Write with response |
| 事件CCCD | 标准0x2902，通知值0x0001 |

设备名 ThumbRemote。广播包携带服务UUID与必要flags；名字放扫描响应，避免将128位UUID和完整名字挤入同一传统广播包。一次只服务一个客户端；连接后停止广播，额外连接若出现应拒绝；断开后恢复广播，但App不自动重连。

会话启动写入固定5字节：version=1（1字节）＋非零sessionId（4字节，little-endian）。sessionId由App为每次连接生成，与上次不同；它用于区分旧消息，**不是身份凭据或认证机制**。本轮是不驱动手机的单设备桌面通信验证，未来控制方案另行评估绑定/认证，不将UUID/名字当身份认证。

事件固定10字节：

| 字节 | 内容 |
|---|---|
| 0 | version=1 |
| 1–4 | sessionId，uint32 little-endian |
| 5–8 | sequence，uint32 little-endian |
| 9 | code：0 READY，1 UP，2 DOWN，3 LEFT，4 RIGHT |

READY的sequence固定0；方向sequence在本会话从1开始递增，uint32回绕按模比较。code决定消息种类，回绕后的方向sequence=0不是READY。测试向量：sessionId=0x12345678、sequence=1、UP → `01 78 56 34 12 01 00 00 00 01`。

接收严格检查长度、版本、sessionId和code；当前会话中的重复/旧sequence丢弃，正向跳号保留当前方向并记录缺号，不补发。正向定义为无符号差值1..0x7fffffff；差值0为重复、其余为旧/不明确。第一次有效方向可接受任意sequence，若不为1记录起始缺号。READY重复不能重置方向计数或sequence。

通知不是端到端交付确认：固件记录“尝试通知”和序号，手机记录真正收到；不把 notify() 被调用等同于手机收到。本轮不做历史缓存、补发、应用层ACK重传或额外MTU协商。

## 连接与新会话握手

Android按顺序完成：连接 → 服务发现 → 验证目标服务、事件/控制特征属性和CCCD → 开启本地通知 → 写CCCD → CCCD成功回调 → 写会话启动值 → 写成功及匹配READY均收到 → “可以接收”。

仅调用 setCharacteristicNotification 不代表外设已订阅；CCCD步骤不可省。依据：[Android Transfer BLE data](https://developer.android.com/develop/connectivity/bluetooth/ble/transfer-ble-data)。

- 会话写成功与READY通知可能先后到达，状态机独立记录两个条件；两者均满足才Ready。未Ready的方向消息直接丢弃，不缓存后展示。
- Android API33+使用带byte[] value的通知回调；API31/32旧回调立即复制字节。两路径按API互斥处理，不能双重计数，回调数据必须在转交UI前复制。
- 所有GATT操作顺序执行，前一步成功才做下一步。连接10秒、服务发现10秒、CCCD5秒、会话写入+READY5秒超时，均关闭本次连接并给出原因。扫描10秒，无无限等待。
- App每次扫描/连接/断开维护递增generation，并绑定实际Gatt实例。回调到达时与UI处理前都验证generation；旧Gatt和旧sessionId消息不能更新当前页面。
- 主动断开、页面停止、权限丢失、蓝牙关闭都会立即使generation失效、取消定时器、清除待处理事件、close当前Gatt。状态变化与回调统一串行处理。

固件只有收到当前客户端的合法启动写入、CCCD已启用时才建立sessionId并发READY。连接/订阅/会话变化必须使进行中的触摸接触失效，等完全松手后才开始新的一次；避免连接前开始的半笔滑动在连接后发送。

BLE回调不直接调用非线程安全的触摸识别器，也不操作I²C；只发布带连接generation的原子状态。主循环处理变更并cancel现有识别。生成方向时绑定会话代数，通知前再次核对；任何旧代数事件不得在新连接中发出。即使极端竞态使旧通知跨过SDK边界，旧sessionId也不能在新手机会话被接受。

断线、取消订阅或新会话启动时立即失效旧sessionId；只有新握手可恢复。无连接/无订阅/无Ready时的方向直接丢弃，只保留串口诊断计数，不排队。普通触摸读取失败沿用v2取消策略，不生成BLE方向。

## 文件与结构（确认后才创建代码）

| 文件 | 职责 |
|---|---|
| firmware/touch_probe/ble_event_protocol.h | 无Arduino依赖的10字节协议/会话发送门控，与手势识别分离 |
| firmware/touch_probe/ble_transport.h/.cpp | 内置BLE库适配、单连接、CCCD/启动写入、广播及会话代数 |
| firmware/touch_probe/touch_probe.ino | 保留触摸v2逻辑，绑定会话代数并把已接受方向交给BLE适配 |
| firmware/tests/ble_event_protocol_test.cpp | 协议字节、握手/代数、断线丢弃与不补发测试 |
| android/.../app/ble/BleEventProtocol.kt | 数据解析、sessionId/sequence过滤，无Android依赖 |
| android/.../app/ble/BleReceiveSession.kt | 纯状态机、generation/超时/Ready条件与30条记录 |
| android/.../app/ble/BleClient.kt | Android扫描/Gatt适配与权限/资源清理 |
| android/.../app/MainActivity.kt及布局/strings.xml | 前台接收卡片、用户权限动作和生命周期绑定 |
| AndroidManifest.xml | 附近设备权限与可选BLE能力声明 |
| android/.../test/.../app/ble/*Test.kt | 共享fixture、握手乱序、旧回调、断开和日志上限测试 |
| docs/ble-receiver.md | 实施后的实际操作、验收、限制及日志来源 |

不新增固件顶层工程或替换现有构建入口；脚本已能编译sketch内.h/.cpp并自动发现firmware/tests/*_test.cpp。创建Android ble/目录前先在AGENTS.md定义用途。

## 验证与完成标准

1. 纯逻辑：相同十六进制fixture在C++/Kotlin一致；畸形包/旧会话/重复/倒序/回绕有测试。握手的两种到达顺序、超时后迟到回调、断线中跨会话接触和没有历史补发有测试。
2. `scripts/esp32.ps1 verify`：原167项检查保持通过，新协议测试通过，完整BLE固件编译成功。检查产物容量与分区，不自行切换Flash模式或跳过步骤；容量/编译问题先定位再调整方案。
3. `scripts/android.ps1 verify`：原7项测试回归、新协议/状态测试、Lint、Debug APK通过。真实Android蓝牙适配不能用纯测试冒充验收。
4. 代码审查重点：手机接收路径不调用辅助功能，旧session/generation过滤，非线程安全对象不被BLE回调直接访问，Gatt关闭与权限撤销覆盖。
5. 确认实施范围后构建/烧录/安装；权限与系统蓝牙提示由用户本人确认，不ADB授予。保持原连线，USB供电。
6. 真机：可以扫描/连接/握手；App显示Ready。四方向各10次（每次间隔约1秒）对照串口、手机方向/序号，逐条登记，不把一次成功推广为40次。
7. 轻点与斜滑各3次，不应产生手机方向；沿用已验证识别器也要做BLE端回归。
8. 用App主动断开，然后在断开期间滑动3次，再手动重连。重连后不显示断开期间或订阅前的动作，新滑动可以接收。先观察断连状态再滑动，不带电拔触摸线。
9. 离开页面、锁屏、关蓝牙、拒绝/撤销附近设备权限都有明确结果，恢复后不自动连接/不补发。原手动测试保持原规则；关闭辅助功能仍可收消息。
10. 测试结束记录实际结果/未测项/日志、释放Gatt与串口。单次短测不等于后台、远距离、抗干扰、续航或长期稳定性通过。

Android扫描限时依据：[Find BLE devices](https://developer.android.com/develop/connectivity/bluetooth/ble/find-ble-devices)。官方建议找到目标后停止扫描、扫描设置上限；本设计选择10秒。

## 已确认的具体范围

用户已明确回复“实现”，允许先更新项目范围与目录/权限规则，再实现上述两端通信；验证通过后可将接收测试版固件烧录至已识别C3、将Debug APK安装到唯一已授权OPPO手机。手机系统权限仍由用户亲自允许。

确认不覆盖BLE控制手机、后台常驻、生产部署、绑定/认证、全局依赖安装或任何用户红线操作；需要扩展时先提出具体方案。
