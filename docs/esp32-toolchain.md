# ESP32-C3 工具链与验证

更新日期：2026-10-25（Asia/Shanghai）。本文件主要保存工具链准备及历史串口示例证据。最新板内程序为获授权的 touch_probe v3 BLE前台收消息版，GetChipID 不再运行。

BLE联测收尾：重连后连接/订阅/READY获板端证据，四方向各10条与手机确认对应，轻点/斜滑各3次过滤通过。用户明确停止后续验证，断线不补发、生命周期/权限异常及长期稳定性仍未验证。完整实测与下次起点见WORK_LOG最新节，不重复安装或烧录。

2026-10-25 BLE当前更新（优先于下方v2/v1历史）：用户已授权BLE前台收消息版，touch_probe v3已编译并上传，DIO/80MHz/4MB不变，程序626361字节/RAM18648字节，196项C++检查通过。固定包实际CONFIG_NIMBLE_ENABLED，使用包内BLE包装层的NimBLE回调与自动CCCD，无新增依赖。最终产物TEMP/thumbremote-touch-56789245aac4；8秒串口4次正常触摸空闲心跳、errors=0，未截到启动横幅，手机无线功能未验证。Android APK已安装到唯一授权PLB110/API36并冷启动成功，用户反馈手机连接后“可以接收”；方向传输待联测。完整交付与证据见docs/ble-receiver.md和WORK_LOG最新节。

2026-10-25 当前版本补充（优先于下方v1记录）：板内已更新为 touch_probe v2，包含获授权的软件手势识别，DIO/80 MHz/4 MB不变。`scripts/esp32.ps1 verify` 与 `upload -Port COM7` 均退出0，167项主机检查通过，程序308584字节、RAM14260字节。四方向各3条正确事件，6次轻点、3次斜滑均按规则拒绝，实测通信errors=0；两个方向窗口首笔起点在窗口前，不推广成全部完整采样或长期准确率。当前产物及全部v2日志为 TEMP/thumbremote-touch-ac895c144276，完整证据见WORK_LOG最新v2节。

最新上传：`scripts/esp32.ps1 verify` / `upload -Port COM7` 均退出 0，DIO/80 MHz/4 MB；39 项主机检查通过，程序 307502 字节、RAM 14236 字节。独立开发板（用户明确屏幕四线断开）115200 串口采样 8 秒收到 4 次 STATUS，无本次检查的启动/崩溃关键字。device=not_found/NAK 是该断开状态下的观测，不能视为触摸通过。上传工具曾报告 XMC flash boot-up failure、自动 startup flow 成功，原始日志保留。产物、manifest、compile/upload/serial-board-only 日志在 TEMP/thumbremote-touch-6f5f78cacbcd。实际触摸方案与后续验收见 docs/touch-probe.md、docs/WORK_LOG.md。

## 已验证结果

- Arduino IDE 2.3.10，内置 Arduino CLI 1.5.1。
- 乐鑫官方国内镜像的 `esp32:esp32@3.3.10-cn`，FQBN 为 `esp32:esp32:esp32c3`。
- esptool 5.3.0 经 COM7 连接 ROM，实测芯片 ESP32-C3 QFN32 revision v0.4、40 MHz 晶振、4 MB Flash。此为此前实测，不保证端口始终存在。
- 2026-10-22：官方随包 `GetChipID` 示例在临时工具链镜像中完整编译成功，退出码 0。程序大小 277068 字节，动态内存 13196 字节。
- 最初仅构建电脑端产物；下述后续上传已在用户授权后完成，当前板内运行官方串口示例。

后续上传更新（同日）：用户明确授权“开始上传”后，已上传官方示例。默认 QIO 配置上传校验通过但启动失败；改为 `esp32:esp32:esp32c3:FlashMode=dio`（保持 Flash 80 MHz、4 MB）后编译、上传与运行验证均通过。12 秒串口采样收到 4 组完整芯片信息，无本次检查的启动错误；不等于长期稳定性或断电重启测试已通过。

## EFS 问题与处理边界

原安装位于当前用户的 `%LOCALAPPDATA%\Arduino15`，目录及内部文件存在 EFS 加密属性。原样编译在平台自带的 Windows COPY 命令复制分区表时失败，错误为 `The specified file could not be encrypted`。

用户已明确授权仅对此目录解除加密。2026-10-22 运行 scoped `cipher /d /h /b /s:<Arduino15>` 后，Windows 在根目录即返回 Access is denied，退出码 1、解密数量 0，随后复查仍有 Encrypted 属性。未扩大权限、未修改 ACL、证书或系统加密策略。文件加密来源尚未确认，不能归因于 DeepSeek。

经验证，以 ReadAllBytes/WriteAllBytes 把公开的分区表内容写入临时文件后，SHA-256 与原文件相同，普通 COPY 成功。因此采用内容相同的临时工具链镜像，并通过 Arduino 官方支持的 `ARDUINO_DIRECTORIES_DATA` 选择该目录。

配置依据：[Arduino CLI Configuration](https://docs.arduino.cc/arduino-cli/configuration)。只设置当前进程环境变量，未修改 IDE 的持久配置或永久环境变量。IDE 默认数据目录直接构建的 EFS 问题仍然存在，不能写成“全局解密已修复”。

## 镜像位置与内容

根目录：`%TEMP%\thumbremote-esp32-toolchain-3.3.10-cn`。

- 以文件内容复制平台 `packages/esp32/hardware/esp32/3.3.10-cn` 与 `packages/esp32/tools/esp32c3-libs`，5757 个文件均逐文件比对 SHA-256，并确认副本无 Encrypted 属性。
- 其他已安装工具目录通过临时目录内的 junction 引用原安装，仅用于读取；不要在这些目录编辑或执行清理，以免影响原安装。
- 复制公开的包索引、库索引及 inventory 元数据，以便 CLI 正常发现已安装平台与工具。
- 构建目录：`%TEMP%\thumbremote-esp32c3-mirror-check`；构建缓存目录：`%TEMP%\thumbremote-esp32-build-cache`。
- 编译日志：`%TEMP%\thumbremote-esp32c3-mirror-compile.log`。此前失败日志和解密失败日志分别为 `thumbremote-esp32c3-compile.log`、`thumbremote-arduino-decrypt.log`。

镜像及构建目录均为生成文件，可能被系统清理；缺失时按 AGENTS.md 的内容副本与哈希校验约定重建，不在镜像修改源码，不自动删除原安装或旧镜像。

## 重复编译命令

在镜像仍完整且原工具安装仍存在的情况下，使用 PowerShell 执行：

```powershell
$previousArduinoData = $env:ARDUINO_DIRECTORIES_DATA
$previousArduinoCache = $env:ARDUINO_BUILD_CACHE_PATH
try {
    $env:ARDUINO_DIRECTORIES_DATA = Join-Path $env:TEMP 'thumbremote-esp32-toolchain-3.3.10-cn'
    $env:ARDUINO_BUILD_CACHE_PATH = Join-Path $env:TEMP 'thumbremote-esp32-build-cache'
    $arduinoCli = Join-Path $env:LOCALAPPDATA 'Programs\arduino-ide\resources\app\lib\backend\resources\arduino-cli.exe'
    $example = Join-Path $env:ARDUINO_DIRECTORIES_DATA 'packages\esp32\hardware\esp32\3.3.10-cn\libraries\ESP32\examples\ChipID\GetChipID'
    & $arduinoCli compile --fqbn 'esp32:esp32:esp32c3:FlashMode=dio' --build-path (Join-Path $env:TEMP 'thumbremote-esp32c3-dio80-check') $example
    if ($LASTEXITCODE -ne 0) { throw 'ESP32-C3 example compile failed' }
} finally {
    $env:ARDUINO_DIRECTORIES_DATA = $previousArduinoData
    $env:ARDUINO_BUILD_CACHE_PATH = $previousArduinoCache
}
```

正式成功验证使用同一命令并加 `--verbose`，以确认平台原有复制、编译、链接、分区生成与打包步骤均执行成功；没有覆盖平台 recipe 或添加忽略标记。

上句描述最初 QIO 配置的电脑端编译验证。后续推荐命令已改为上述 DIO 配置，其普通编译命令和上传、串口验证均实际通过；最终程序大小 277092 字节，动态内存 13196 字节，应用 bin 文件 277248 字节。

## 上传与运行验证（2026-10-22）

用户在知道会替换开发板现有程序后明确授权“开始上传”。先核对 COM7 的 VID_1A86/PID_55D3、产物 FQBN 和官方示例源码一致，再按 Arduino 官方 upload 命令写入。未使用整片擦除、--force 或修改安全 eFuse。

首次上传使用默认 QIO/80 MHz。工具退出码 0、各段写入哈希验证通过，但运行采样反复出现 partition 0 invalid magic number 0xffed、Failed to verify partition table，示例输出 0 组。不能把此次上传成功记为运行通过。

排查证据：

- 编译产物分区表前两字节为 AA 50，长度 3072 字节；从开发板 0x8000 读回 4096 字节，其中对应的 3072 字节与产物逐字节一致，差异数 0。
- get-security-info 实测 Secure Boot Disabled、Flash Encryption Disabled。未改动安全配置；此芯片 Flash 加密与 Windows EFS 是不同事项。
- board details 确认默认 FlashMode=qio、FlashFreq=80、FlashSize=4M；构建 SDK 配置使用 QIO。选择官方 FlashMode=dio，仅改变模式、保持频率和分区，重新完整编译上传。
- [乐鑫官方排障说明](https://docs.espressif.com/projects/esptool/en/latest/esp32/troubleshooting.html#wrong-flash-mode)说明某些设备会出现 QIO 写入成功但运行无法读 Flash 的情况，可检查 DIO。这里只采用模式排查原则，不照搬该页 ESP32 的 GPIO 编号到 C3。

DIO/80 MHz 编译退出码 0、上传退出码 0、写入哈希校验通过。使用 COM7、115200、8N1，DTR/RTS=false，采样 12 秒，取得 4 组如下格式，检查的启动错误数 0：

```text
ESP32 Chip model = ESP32-C3 Rev 4
This chip has 1 cores
Chip ID: [omitted]
```

最终板内程序为官方 GetChipID 示例，每约 3 秒输出一次；只报告芯片信息，没有触摸、显示、BLE 或 GPIO 外设控制。串口读取已在 finally 中关闭并释放。此结果支持在本板当前环境使用 DIO，不证明 QIO 失败的具体电气原因。

最终构建目录为 `%TEMP%\thumbremote-esp32c3-dio80-check`；日志分别为 `thumbremote-esp32c3-dio80-compile.log`、`thumbremote-esp32c3-dio80-upload.log`、`thumbremote-esp32c3-dio80-serial.log`。首次失败串口日志保留在临时目录，未删除。

## DeepSeek 接手后的核对

用户于 2026-10-22 告知前一晚曾由 DeepSeek 临时接手，随后要求恢复。项目目前没有已跟踪的 Git 提交基线，不能用空 git diff 证明恢复完整。

- 当前 Android 所有输入文件与上次最终 UI 构建镜像 `%TEMP%\thumbremote-0e0103877b56` 双向比较：无新增、缺失或内容哈希差异。排除了 build、.gradle、.kotlin、.idea 与机器 local.properties。
- 官方缓存 `esp32-core-3.3.10.zip` 的 SHA-256 与乐鑫索引匹配；当前安装的 platform.txt、boards.txt、默认分区表和 GetChipID 示例与该归档内容逐项匹配。
- 当前 IDE 配置的附加开发板索引列表为空，界面设置仅见原生标题栏选项；未根据时间戳猜测修改者。

这些是明确检查范围内的结果，不证明所有系统设置或历史操作都已还原。本次没有覆盖或回滚其他代理的改动。

## 下次起点

独立开发板的编译、上传及串口运行验证已完成，无需重复下载安装或重复此项测试。下一步回到触摸资料核实：转接板供电、针序、复位/中断接法及 IT7259 协议仍待确认，暂不连接触摸屏或电池，也不提前实施 BLE。
