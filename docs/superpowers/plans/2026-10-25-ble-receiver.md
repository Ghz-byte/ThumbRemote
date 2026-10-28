# BLE前台接收 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 手机前台接收并显示ESP32方向事件，断线与页面离开后不补发，BLE路径不执行手机手势。

**Architecture:** 固件在现有v2手势识别后增加独立BLE协议/适配；Android纯协议与会话状态机驱动平台Gatt，MainActivity只负责用户动作和前台展示。用CCCD+会话启动写入+READY分离物理连接与可接收状态。

**Tech Stack:** Kotlin+原生Views、平台BluetoothGatt、Arduino固定esp32:esp32 3.3.10-cn自带BLE库、C++主机测试、JUnit4.13.2。无新依赖。

**Spec:** docs/superpowers/specs/2026-10-25-ble-receiver-design.md（用户已确认，实施中）。

## Global Constraints

- 先获得完整方案确认，再更新AGENTS.md范围/目录规则，之后创建代码。不得把“开始准备方案”记成实施授权。
- minSdk24/targetSdk36/compileSdk37保持；BLE仅API31+，BLUETOOTH_SCAN neverForLocation与BLUETOOTH_CONNECT，由用户允许。不申请定位、网络或后台权限。
- 保持GPIO4/5、3.3V、DIO/80MHz/4MB、115200及原40单位/2倍轴比识别；不改触摸配置。
- UUID、10字节通知、5字节会话写入、code映射、sequence模比较，严格遵守设计表。
- 扫描10秒、连接10秒、发现10秒、CCCD5秒、会话启动/READY5秒。单连接、无自动重连、无历史队列。
- Android onStop停止/断开/close并失效generation；本轮BLE路径不调用辅助功能。保留手动测试。
- 不提交、推送、删除、改凭据/CI，不安装全局依赖，不部署。

## Review Focus

- 旧Gatt/旧session或握手完成前消息不得污染当前记录。
- CCCD成功不等于Ready；会话写回调与READY先后顺序都应正确。
- 断线期间/跨握手的一次接触不能在重连后发出；无历史补发。
- 非法包/sequence重复、倒序、跳号和回绕不误计；缓存字节不能被平台后续修改。
- 权限撤销与页面停止时，即使蓝牙清理抛异常也要清除本地会话；接收路径不得执行系统手势。

### Task 1: 两端协议与会话状态

**Files:** 创建firmware/touch_probe/ble_event_protocol.h、firmware/tests/ble_event_protocol_test.cpp、android/app/src/main/java/com/thumbremote/app/ble/BleEventProtocol.kt、BleReceiveSession.kt；对应test/.../ble/BleEventProtocolTest.kt、BleReceiveSessionTest.kt。

**Interfaces:**
- C++ `encodeReady(uint32_t sessionId)`、`encodeDirection(uint32_t sessionId,uint32_t sequence,Direction)`均返回10字节固定数组；控制写入解析器只接受version1/非零sessionId/5字节。
- Kotlin `BleEventProtocol.decode(ByteArray): BleFrame?`；`BleFrame`包含sessionId/sequence/code，uint32表示为Long以避免有符号比较。
- `BleReceiveSession`由串行事件驱动：start(generation,sessionId)、服务/CCCD/写入结果、frame、timeout、stop；输出状态与接受/丢弃结果，只保留最近30条。具体sealed class名称交实施者，但操作事件必须带generation。
- 固件纯发送门控保存连接generation/sessionId/ready，只在事件绑定代数与当前一致时允许通知，不缓存事件。

- [x] 先更新根规范，明确新增Android ble/与test ble/职责以及本阶段权限例外；只在已确认后执行。
- [x] 写共享字节fixture测试：`01 78 56 34 12 01 00 00 00 01`解析为session0x12345678、seq1、UP；长度9/11、版本2、code5、session0均拒绝。READY code0/seq0，非零seq READY拒绝。
- [x] 写sequence测试：同seq不重复；5→4拒绝；5→7接受并记1缺号；0xffffffff→0方向接受；旧session丢弃且不重置当前sequence。
- [x] 写状态测试：READY先于写回调/写回调先于READY均两条件齐才Ready；旧generation与握手前方向丢弃；stop后迟到成功无效；记录31条只保留最新30条。
- [x] 写发送门控测试：未订阅/未启动/代数变化的事件不发送；重连无待发记录；跨握手的旧接触不能沿用；新完整接触恢复。
- [x] 观察测试失败后实现最小逻辑，`scripts/esp32.ps1 test`与`scripts/android.ps1 test`全部通过；不得仅测新文件。

### Task 2: 固件BLE适配与编译

**Files:** 创建firmware/touch_probe/ble_transport.h/.cpp，修改touch_probe.ino；必要时只完善既有scripts/esp32.ps1的产物检查，不改工具链安装。

**Interfaces:** `BleTransport::begin()`初始化；`poll()`向主循环交付ready/代数变化；`sendDirection(Direction,capturedGeneration)`仅当前Ready代数尝试notify；接口返回“已尝试/已丢弃”，不声称手机收到了。主循环在代数变化时cancel识别，触摸开始时捕获代数；其余触摸协议代码不变。

- [x] 使用已安装BLEDevice/BLEServer/BLECharacteristic与标准CCCD（实际NimBLE自动创建，不手动加BLE2902），明确单客户端、停止/恢复广播，服务UUID与扫描响应中的名称。
- [x] 回调只发布连接/CCCD/控制写入状态，绑定连接身份；不跨线程修改识别器或I²C。错误控制包不启动会话。
- [x] 主循环处理握手和失效，再处理触摸；接受方向仍保留串口GESTURE，加上带session/sequence的通知尝试/丢弃诊断。
- [x] 编译前检查本地库实际签名；`scripts/esp32.ps1 verify`通过并核对容量。实际BLE编译/栈能力不以头文件存在代替。
- [x] 独立审查旧代数跨连接、非线程安全调用和单客户端过滤；修正实际问题并回归。不在此任务提前上传未审阅产物。

### Task 3: Android平台适配与前台页面

**Files:** 创建android/app/src/main/java/com/thumbremote/app/ble/BleClient.kt，修改MainActivity.kt、res/layout/activity_main.xml、res/values/strings.xml、AndroidManifest.xml。不改ThumbAccessibilityService执行路径。

**Interfaces:** `BleClient.startScan()`/`connect(selectedDevice)`/`stop()`；由MainActivity的用户动作与生命周期驱动，状态更新送入Task1纯会话逻辑；notify payload先复制，API33+与31/32路径互斥。

- [x] BLE能力/SDK31检查在所有BLE调用之前；点击扫描申请附近设备权限，拒绝/蓝牙关闭结果明确。按设计限定服务UUID与10秒扫描，不自动连接首个结果。
- [x] connectGatt autoConnect=false；按服务发现→本地通知→CCCD→会话写入顺序，只在匹配READY和写成功都到齐后显示可接收。各阶段超时/错误关闭，GATT操作不并发乱序。
- [x] Gatt与generation双校验；回调到达与UI提交两次核对。onStop/权限撤销/蓝牙关闭立即失效本地会话，清理异常不导致旧事件恢复。
- [x] 接收卡片展示状态/选择设备/方向/数量/最近记录，沿用现有布局样式；更新旧“无需蓝牙”文字以对应手动区；调试记录显示设备序号用于对照，不显示UUID/原始包。
- [x] 增加关键平台边界的适配测试或可测事件入口；真实Gatt硬件流程仍记待真机，不以mock通过冒充。保留原7个手动会话测试。
- [x] `scripts/android.ps1 verify`通过；审查BLE代码无辅助功能/dispatchGesture调用、无多余权限，记录Lint实际结果。

### Task 4: 授权范围内安装与联合验收

**Files:** 新建docs/ble-receiver.md；更新docs/WORK_LOG.md、docs/acceptance.md、docs/esp32-toolchain.md。仅在批准后扩充AGENTS.md的实施范围。

- [x] 完成两端审查及完整verify；核对唯一CH343/C3/COM口和OPPO真机，调用既有upload/install。无整片擦除，不自动授予权限。
- [ ] 用户亲自允许权限并开启蓝牙；测试扫描、选择、握手Ready，关闭辅助功能仍可收消息。
- [x] 用户准备好后，四方向各10次，对照串口序号/手机显示登记实际样本；不足/失败保留原始证据，不用最终“OK”概括全部。
- [ ] 轻点/斜向各3次无手机方向；App断开期间动作3次，手动重连后不补发，新动作可收。
- [ ] 页面离开/锁屏、蓝牙关闭、权限拒绝/撤销均明确结束，不自动恢复连接；回归原手动按钮。
- [ ] 更新真实验收与未测项，释放串口/Gatt。后台、距离、长期稳定性与真正控制手机留后续方案。

## 执行前确认

用户已确认设计和此计划，当前按任务顺序实施并作独立审查；实际完成状态看复选框及WORK_LOG，不另开用户聊天，不推送或部署。

文档审阅：2026-10-25，独立builder-check返回“nothing important left”；另已核对UUID格式、设计路径与10字节fixture。未执行本计划中的任何产品代码任务，未运行BLE编译或设备操作。

## 实施记录（优先于上文准备阶段状态）

- 最新收尾：重连握手有板端连接/订阅/READY证据；四方向各10/10、轻点/斜滑各3次已由串口与用户分项确认，总数40。用户随后明确“不用验证了”，停止后续验收；断线不补发、页面离开/锁屏/蓝牙关闭/权限异常、关闭辅助功能独立接收、本版手动功能回归均保留未验证。Task4为部分完成，不再自动推进测试或扩大功能。
- 用户“实现”已授权全部计划范围；先更新规范后创建BLE文件。Task1：C++29检查先红15项后绿；Android首次14JUnit红6项后绿（原7项保留）。合计C++196检查。曾遗漏end方法默认参数导致一次Kotlin编译错误，已补回后完整主机套件通过。
- 首次目标编译确认sdkconfig实际CONFIG_NIMBLE_ENABLED，而非Bluedroid；按已安装BLE包装库的ble_gap_conn_desc/onSubscribe修正回调，CCCD由NimBLE自动生成。未切换后端或增加依赖，属于库适配细节，不改变协议；完整目标编译进行中。
- Android平台适配与页面已接入，未修改辅助功能执行路径。完整Androidverify与独立实现审查进行中；设备安装/烧录待通过后执行，手机当前未枚举，已请用户连接。
- 最终验证更新：固件196检查、完整编译与上传均通过，程序626361字节/RAM18648字节；独立审查2项Important权限竞态已修复，3项新增回归先红再绿。Android最终17JUnit通过，Lint0 errors/5 warnings，APK923403字节。
- Task2/3完成。Task4部分完成：板内v3已上传，8秒4次空闲心跳正常；唯一授权PLB110/API36已识别，install返回Success、冷启动Status ok、自身进程AndroidRuntime检查无错误。用户反馈扫描连接后“可以接收”；方向及断线不补发待实测，关闭辅助功能独立接收尚未确认。不重复请求实施/安装/烧录授权。
- 审查剩余验证缺口：跨连接接触组合自动化尚未直接覆盖，门控与取消模块分别有测试；无线组合行为保持待实测。UI已显示设备序号方便对照。未将这些缺口称为通过。
