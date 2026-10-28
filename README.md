# ThumbRemote

拇指触控 BLE 视频遥控器。同一份 ESP32-C3 固件支持 Android App 与 MatePad BLE HID 自动分流，一次控制一台设备。OPPO App 的上下滑、中心单击和快速双击已获本机用户测试反馈；MatePad 上下滑已获用户反馈，新增的平板中心点击仍在验证，不能保证所有设备兼容。详细证据和未测项见 `docs/WORK_LOG.md` 与 `docs/acceptance.md`。

## 同一固件怎样使用

1. 首次必须先用 OPPO：系统配对由本人确认，再从 ThumbRemote App 扫描并开始接收/遥控，固件会保存该手机的绑定身份。如果其他设备已占用连接，先断开其他设备，保留配对。
2. 登记成功后 OPPO 始终走 App；用户在 App 主动开始后才执行上下滑或中心单击，停止/锁屏等不转成 HID。接收测试页只显示接收到的事件。
3. 完整断开 OPPO，再在 MatePad 系统蓝牙连接 ThumbRemote（旧名称可能仍缓存为 ThumbRemote-HID-Test）；确认硬件松手后，真实上滑发 Down Arrow、下滑发 Up Arrow，左右不导航；无需长按/串口arm，没有60秒限制。最新固件提供试验性的平板中心点击，实际识别效果尚待验收。
4. 换回 OPPO 同样先断开 MatePad，再从 App 主动连接开始；同一时刻只一条连接，断线/重启不补发旧动作。旧主机反复自动抢连时暂时关闭旧主机蓝牙，不清绑定。

2026-10-27重连修复版已通过本轮扫描/握手复测：完成登记的OPPO即使在系统蓝牙中已连接，也可直接在原App扫描并开始接收，不用先断开手机自身的系统键盘连接。系统连接仍不等同于App已开始遥控；换到另一台主机仍先断开当前连接。初次尚未登记或另一台主机占用时按上面的首次/切换步骤操作。

原接线保持 SDA=GPIO4/SCL=GPIO5/3.3V/GND；不新增显示/电池电路。两机切换、合并版断电/充电宝与旧绑定迁移须逐项实测，不能沿用历史独立固件结果。

```powershell
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify -Target dual
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 upload -Target dual -Port COM7
```

上传前必须按项目规则核对唯一端口/芯片和有效清单；默认目标仍touch，touch/hid恢复入口保留。异常产物目录不删除或改清单放行；需要同源干净重建时显式使用同一个 `-BuildId <英文小写标识>` 做 verify/upload。

## 第一阶段手动验证（历史入口仍保留）

用户点击上滑、下滑或中心单击 → 服务倒计时 5 秒 → 用户手动切回目标应用 → 一次系统手势 → 返回 App 查看结果。

App 不联网、不截图、不读取其他应用的窗口内容。单击只在屏幕中心模拟触摸，不保证播放/暂停。系统回调完成不等于目标 App 已切换视频。

## 工程与构建

- 源码位于 `android/`。Windows 的 AGP 会拒绝中文工程路径；脚本自动复制到按源码哈希命名的英文临时目录构建，不关闭检查。构建时输出 `ASCII build snapshot` 路径，可在 Studio 打开该镜像查看，但不要在那里编辑，源码修改仍在当前仓库进行。以后若希望完全在 Studio 内开发，可经用户同意将工作区换到英文路径。
- Kotlin + 原生 Views，minSdk 24，compileSdk 37.0，targetSdk 36。
- AGP 9.2.0（内置 Kotlin），Gradle 9.4.1，Build Tools 36.0.0。
- Windows 脚本默认使用 Studio 随附 JDK、用户目录下 Android SDK；路径不同时修改脚本里的路径。
- 脚本仅在当前进程设置环境变量，Gradle 缓存保留在系统临时目录 `thumbremote-cache-<工作区路径哈希>/gradle-home`，避免中文路径导致测试 worker 无法加载类。若 Windows 已启用代理，仅为本次 Java 进程复用，不改系统配置。

在仓库根目录运行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 test
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 verify
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 install
```

`install` 只接受恰好一台已授权的 ADB 真机，不自动授予辅助功能权限。

源码位于英文路径时，也可以使用 `android/gradlew.bat :app:assembleDebug`，但需要自行配置当前终端的 JDK、SDK 和网络。脚本会将 APK 与测试报告复制回 `android/app/build/`；APK：`android/app/build/outputs/apk/debug/app-debug.apk`。

## 手机上怎样验证

1. 安装并打开 ThumbRemote。
2. 点击“打开辅助功能设置”，在系统中找到“ThumbRemote 手势控制”，阅读说明后亲自开启。
3. 返回 App，确认“授权：已开启”“服务：已连接”。厂商菜单可能将服务放在“已下载的应用”中；如系统限制侧载应用权限，按手机系统说明处理，不通过 ADB 绕过。
4. 在目标视频应用打开普通视频流，再返回 ThumbRemote。
5. 点击“测试上滑”，在 5 秒内手动切回目标应用；等待一次手势，观察视频是否切换。
6. 返回 App 查看系统结果。分别测试下滑、中心单击。
7. 重复上下各 10 次，记录成功、无响应、重复切换和目标应用版本。初始建议每个方向至少 9 次正确切换。
8. 测试取消、锁屏和关闭服务，确认任务作废且之后不会补发。

以上是原手动验证入口。硬件和BLE当前进度见顶部合并说明，不根据历史步骤扩展接线或通电范围。

## 关键设计

- `GestureSession.kt`：不依赖 Android 的会话状态机与路径几何，隔离重入、冷却、取消和迟到回调。
- `ThumbAccessibilityService.kt`：拥有待执行任务；Activity 离开不会丢失倒计时，服务重启不恢复任务。
- `MainActivity.kt`：显示状态、授权引导、用户按钮；不调用 ADB 或其他应用内部 API。
- `RemoteStatus.kt`：仅进程内保留最近结果，页面停止时解除观察者。
- 失败时不补发，不排队；每个新操作都要求再次主动点击。

实际验证状态见 `docs/acceptance.md`。
