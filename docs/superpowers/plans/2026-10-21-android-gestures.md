# Android Gesture Test Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task. User has approved creation; execute in this chat without requesting the same approval again.

**Goal:** 创建可构建、可测试、可安装的一次性手动手势 App。
**Architecture:** 服务持有会话，纯 Kotlin 状态机隔离倒计时/重复/取消/回调；Activity 仅展示和发送用户操作。
**Tech Stack:** Kotlin (AGP built-in), native Views, AGP 9.2.0, Gradle 9.4.1, JUnit 4.13.2.
**Spec:** `docs/superpowers/specs/2026-10-21-android-gestures-design.md`。

## Global Constraints
- minSdk 24 / compileSdk 37.0 / targetSdk 36 / Build Tools 36.0.0。
- 无网络/蓝牙/窗口内容读取权限，不自动授予辅助功能权限。
- 5000 ms 准备，300 ms 滑动，80 ms 单击，500 ms 冷却，执行 3000 ms 超时。
- 当前空仓库继续使用当前工作目录；不提交 Git，不安装全局依赖，不删除临时文件。

## Review Focus
- 切出 Activity 不丢失用户已发起的任务，重建后不重复执行。
- 锁屏/服务断开/取消使任务失效，迟到回调不覆盖新结果。
- 系统完成与应用响应分别表述。
- 所有触发入口仅用户点击，服务未就绪的按钮显示原因。
- edge-to-edge 在 Android 16 下正确应用系统栏 inset，按钮可触达。

### Task 1: 工程、纯 Kotlin 手势会话
Files: `android/{settings.gradle.kts,build.gradle.kts,app/build.gradle.kts,gradle/wrapper/*}`; `android/app/src/{main,test}/java/com/thumbremote/app/GestureSession*.kt`; `scripts/android.ps1`。
Interfaces: `GestureSession.arm(command, now): Boolean`, `takeDue(now): PendingGesture?`, `finish(id, now): Boolean`, `cancel(now): Boolean`; `GestureGeometry.plan(command,width,height): GesturePlan`。
- [x] 固定工具版本，准备官方 Gradle Wrapper 与项目本地缓存。
- [x] 写 JUnit 行为测试，运行观察缺少实现的失败。
- [x] 实现上述接口，运行 `scripts/android.ps1 test`，期望全部通过。

### Task 2: 辅助功能服务与界面
Files: `ThumbAccessibilityService.kt`, `MainActivity.kt`, `RemoteStatus.kt`, manifest、布局、字符串、主题与辅助功能 XML。
Consumes: Task 1 状态机与几何；Produces: 用户点击→服务倒计时→一次 dispatchGesture→结果。
- [x] 实现服务与 UI，屏幕关闭/锁定检查、3000 ms 超时、生命周期清理、不可重入。
- [x] 运行 `scripts/android.ps1 verify`，期望单元测试/Lint/assembleDebug 均通过。

### Task 3: 真机与交付
Files: `README.md`, `docs/acceptance.md`。
- [x] 检查唯一已授权设备，构建安装并启动；读取本 App 的崩溃信息，不读取个人数据。
- [x] 在未授权状态验证界面、按钮错误提示，记录截图；用户随后亲自开启权限。设置入口到授权的完整导航未单独观察。
- [x] 独立代码复核并修复关键问题；用户亲自开启权限，反馈短视频内上下滑都能切换。
- [x] 更新验收记录、关键设计和后续十次测试步骤，明确未验证部分。

Task 3 的核心交付已完成；完整阶段验收仍需补充 `docs/acceptance.md` 中的连续次数、异常场景和视觉修复复核，不据此自动进入硬件阶段。
