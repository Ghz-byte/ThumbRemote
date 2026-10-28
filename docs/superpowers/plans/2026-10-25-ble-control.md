# BLE上下滑遥控实施计划

> **For agentic workers:** 使用superpowers:executing-plans在当前会话逐项实施；用户“实施”已授权，不重复申请计划执行或安装许可。

**Goal:** 用户主动开启后，切换应用仍能用硬件上下滑控制手机。
**Architecture:** 服务持有独立Gatt，协议回调仅新事件，纯门控验证代数/时效/单任务，再调用辅助功能即时入口；接收测试模式保持独立。
**Tech Stack:** 既有Kotlin/原生Views/Android平台BLE，API31+，固定Gradle链，无新依赖。
**Spec:** docs/superpowers/specs/2026-10-25-ble-control-design.md

## Global Constraints
- 只UP/DOWN，300ms几何、500ms冷却、500ms接收到执行时效，无队列。
- connectedDevice服务、可见通知/停止入口、START_NOT_STICKY，无持久化/恢复。
- 固件/接线不改，不烧录；不自动操作手机手势/授权。
- 当前未跟踪工作区继续使用，无提交/推送/删除/全局安装。

## Review Focus
- 停止与新开始间旧回调不能清掉新请求：Task1代数测试，Task2协调检查。
- 前台服务启动/通知拒绝/启动失败不能留下假活动状态：Task3失败清理与纯启动门控测试。
- 忙/冷却拒绝消耗事件，不可延后重放：Task1消费序号测试。
- 跨页面不能让接收模式onStop关掉服务Gatt：Task3绑定所有者检查。
- 主线程堵塞后的回调应按原始接收时间丢弃：Task1边界测试，Task2传入时间。

### Task1: 纯会话门控与即时手势
Files: RemoteControlSession.kt/RemoteControlSessionTest.kt、GestureSession.kt/GestureSessionTest.kt。
Interfaces: begin():Long、ready(generation):Boolean、accept(generation,sequence,code,receivedAt,now):RemoteRequest?、finish(generation,id,now):Boolean、stop()；GestureSession.armImmediate(command,now):PendingGesture?。
- [x] 先写行为测试：无READY/停止/旧代数、只上下、重复、忙/冷却、500ms时效、迟到完成、30条上限、手动/即时互斥。
- [x] 观察失败，再实现最小逻辑，运行scripts/android.ps1 test全套通过。

### Task2: BLE新事件与辅助功能即时入口
Files: BleClient.kt、ThumbAccessibilityService.kt。
Interfaces: BleClient增加onAccepted(frame,generation,receivedElapsed)可选回调与selected目标；辅助功能executeRemote(command,complete):Boolean、cancelRemote()，原arm路径不变。
- [x] 用Task1门控测试约束语义，沿用既有协议消费回归；通知回调捕获原始单调时间，session.receive返回非null才回调，不读历史列表。
- [x] 派发共用方法，完成/超时/停止隔离请求id；onInterrupt/disconnect通知遥控结束。验证单测与编译。

### Task3: 服务、通知和页面
Files: RemoteControlService.kt、RemoteControlStatus.kt、MainActivity.kt、activity_main.xml、strings.xml、AndroidManifest.xml。
Interfaces: 本地Binder取得服务，仅状态绑定；显式START/STOP，用户选定目标参数只内存/本次Intent，无磁盘；静态运行快照与观察者无Activity泄漏。
- [x] 启动检查通知/附近设备/辅助功能/屏幕/无手动任务；用户前台点击开始后断开接收Gatt，再服务独立重握手。
- [x] 服务前台通知含停止、后台继续、锁屏/蓝牙/断线/辅助功能丢失/移除任务结束，STOP幂等，无重连。
- [x] 页面选择目标、开始/停止/状态/30条结果，手动互斥；通知权限用户操作，拒绝不启动，不在权限回调自动开始遥控。
- [x] scripts/android.ps1 verify：原测试回归、单测、Lint/APK。审查失败路径/平台限制，不添加绕过。

### Task4: 审查、安装和接续
Files: docs/ble-control.md、WORK_LOG.md、acceptance.md及此计划。
- [x] 独立整体审查，Important以上先写回归修复；仅必要时再跑verify。
- [x] 核对唯一授权PLB110/API36，scripts/android.ps1 install；不发起遥控/实际手势，权限用户亲自处理。
- [x] 记录构建/安装实际结果、未测项与操作入口，用户不愿真机操作则保留未验证，不重测上阶段清单。

## 执行记录
- 后续用户继续推进后，原Minor拒绝计数已修复：显式reject成功结束当前请求后计数一次，服务拒绝分支使用；新增2项回归先红1项再27JUnit全绿，verify与install成功（最终thumbremote-b28c3ca94042/APK939899字节）。小修复复查无发现，替代下文原“暂留”状态；详见WORK_LOG最新节。未增加权限/动作，不自动重做用户跳过的真机测试。
- 安装完成：唯一授权PLB110/API36，install返回Success，沿用最终构建镜像thumbremote-0d4547104781；APK939815字节/SHA256 0CA62FCFFA1CD7FB5016148E98A45119D8D8343DEE46205D1273E714853FB4C1。未开启遥控/代授权/模拟手机手势，跨应用效果保留未验证。
- 整体审查完成，无Critical/Important。Minor1暂留：执行器拒绝时逐条结果明确、摘要未执行数未增；不影响动作门控，说明已写ble-control.md。按inline执行流程不为显示计数继续扩大改动/测试，后续由用户决定是否补齐。无审查修复代码，首次verify结果仍对应当前产品源码。
- Task1：6项新增测试预期失败后实现，23项JUnit全通过。Task2/3：服务/通知/绑定/UI已接，启动条件及票号2项测试先失败后通过；首次整体verify通过，25项JUnit、Lint0errors/6warnings、APK生成。独立整体审查进行中，尚未安装本版或遥控真机。
- Ruling: 状态票号与前置条件抽成可测试进程对象/数据类 — 防重复开始与过期启动Intent恢复会话 — 成本为两个小型纯接口，无新持久化。Android平台连接/通知真实能力仍留真机结果，不用纯测试代替。
- 服务类型尚未创建时曾运行一次编译失败；补齐编译类型后，两项启动测试确实失败，随后实现并通过。红绿依据为后者，不是编译错误。
- 计划自查：纯门控/平台/生命周期/UI/安装均有任务；无新目录/依赖。复用当前工作区是因为现有文件无Git基线且用户未要求隔离，避免丢失已验证进度。按用户授权本会话实施，不再次询问执行方式。
