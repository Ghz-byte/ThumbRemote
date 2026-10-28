# MatePad mini BLE HID 键盘测试

更新：2026-10-27。当前使用同一合并固件，OPPO App和MatePad HID两端基本测试均获用户成功反馈；历史平板目标为MatePad mini/HarmonyOS7.0.0.109上的鸿蒙原生版抖音，本次具体应用版本未另行确认。完整证据和未测专项以WORK_LOG/验收表为准。

## 当前状态

- 当前上传为e3fbea199932重连修复版。2026-10-27用户确认保持OPPO系统连接时原App“能扫描，且显示可以接收”，板端connected1/advertising1/route app/session1/errors0；重连发现入口复测通过。平板控制逻辑未改，但不把旧1ed9两端反馈扩展为新版所有专项通过。下面旧版“当前板内/未验证”仅指相应历史阶段，以WORK_LOG顶部为准。

- 2026-10-27用户明确“手机端和平板端都测试成功了”，记录当前dual_remote合并固件两端基本路径成功；不再将MatePad本版核心路径写成未测。具体次数/上下映射/超过60秒/停止不回退/反复切换/断电/充电宝未逐项反馈，继续保留未验证；不因此要求重做已反馈的基本测试。以下2026-10-26过程为历史。

- 2026-10-26最新已批准并上传同一固件dual_remote，OPPO原App/MatePad Arrow HID自动分流，日常断开再连接。当前板内最新上传source1ed9b4bde4df…，586 C++/37清单、完整编译/上传通过，8秒4次空闲心跳无panic/errors；首次OPPO App登记/实际双机效果尚未验证。中途9711的NimBLE事件初始化早于BLE导致崩溃已修复，保留日志不使用该旧镜像。下方Page/Arrow独立版“当前板内”均为历史；以WORK_LOG顶部ledger为准。
- 接手预检差异：三组HID历史build目录虽列清单BIN哈希匹配，各多4个未列清单的`*_flashed.bin`，因此当前精确集合上传保护会拒绝，不能直接重传。额外文件与原BIN一致、来源未核实，保持不删；touch恢复目录完整Assert-Verified通过。下方历史verify/upload通过只指当时，不代表当前目录仍可直接上传。

- 最新兼容性结论：OPPO PLB110/抖音40.8.0上Arrow和Page两组实测均未翻页；手机getevent已确认真实KEY_DOWN及KEY_PAGEDOWN/KEY_PAGEUP按下/释放。当前板内为Page对照8ee49f4f3241（630573程序字节/RAM18816），446 C+++9清单、完整编译/上传和增量审查通过，但这不代表OPPO遥控可用。停止键盘试验，等待用户选择恢复原OPPO软件配套touch，或恢复Arrow留给MatePad；不能把MatePad原v1 Arrow成功推广到当前Page。原Arrow和touch源码/构建均保留。

- 当前板内为用户已批准的v3直接使用版：取消长按开关/60秒门控，连接就绪且触摸健康、确认释放后即可上下滑，断开或断电停止。原地长按不再作为开关。428主机检查+9清单检查、独立审查、完整verify/upload通过；630285程序字节/RAM18816，源hash8b3672a0abca9ca869f5aad4c6bbf1c4842a4647fdf51618ae0ed864f3709250，产物TEMP/thumbremote-hid-8b3672a0abca-targets-v1。OPPO上传后ready1/enabled1/errors0，实际翻页/>60秒/本版充电宝等待实测。下方v2/v1均为历史，不按其启停步骤操作当前版。

## v3直接使用

开发板USB供电，在手机/平板系统蓝牙中连接ThumbRemote-HID-Test，不打开原Android App遥控。松开硬件触摸屏，待连接就绪后，上滑发Down Arrow、下滑发Up Arrow；每笔抬手，释放后500ms冷却。无需长按或电脑指令，没有60秒截止。

断开此蓝牙设备或拔电停止。连接/通信异常取消当前笔，恢复后先确认松手，只处理新的滑动；不补发历史。当前只接受串口status，旧arm/disarm/mode命令提示不适用。设备保留键盘身份和绑定，不需要清配对；另一台设备接入前先断开当前设备。目标App能否响应键盘须实际确认，不能因连接成功推定。

## v2历史记录（已被v3替代）

- 最新v2长按启停已获用户批准并实现/烧录，源hash4c384d1a7b5d363b72ee60ff4b7e4c3ae02cef62c1fca70e1d91c40f7b63039c，产物TEMP/thumbremote-hid-4c384d1a7b5d-targets-v1，632013程序字节/RAM18824；363主机检查+9清单检查、独立审查、完整verify/upload均通过。重连后只读7秒4组ready1、armed0/persistent0、errors0；实际长按启停/超过60秒/新版本充电宝使用仍待用户验证。下方v1记录为兼容性测试历史，以此条及WORK_LOG最新状态为准。

## v2日常启停

保持原四线，用开发板USB供电，平板系统蓝牙连接原ThumbRemote-HID-Test。上电/重连默认待机；单指原地长按至少3秒（首次建议数4秒）并松手开启，再次长按松手停止。开启期间上下滑照旧；物理开启没有60秒限制，断线/通信异常回待机，不自动恢复。

长按允许每轴最多12原始坐标单位抖动，移出再移回不生效；多指、换指或未知包取消该笔。单笔持续>=3秒不发导航键，避免长按后移动导致翻页。没有状态显示，需通过实际滑动判断；阈值手感还待实物反馈。串口arm仅作为60秒诊断入口保留，不是日常启用前提。

## v1兼容性测试历史

- 2026-10-26方向键实测获用户明确反馈：鸿蒙原生抖音“上滑下一条，下滑上一条，都有效”。板端采到UP3/DOWN1，对应方向键与每次零释放通知rc0、errors0；实际切换结论来自用户反馈，不把通知日志当主机确认。箭头有效，未测试Page。当前仍为串口arm后60秒的兼容性测试版，尚不能直接断电脑后长期遥控；保留HID或恢复touch等待用户选择。

- 最终verify与upload均退出0，HID程序631011字节/RAM18816，源码SHA256 1ed8e5ab95cb7a42cdcdc0094d498a249985352a3beffac5f35d6570f00cace7，产物TEMP/thumbremote-hid-1ed8e5ab95cb-targets-v1。已核对ELF包装函数的实际调用；上传前touch恢复清单及唯一COM7/C3均匹配、四段写入哈希通过。板内现为HID测试版。
- 6秒4次空闲心跳armed0/ready0/idle/errors0，用户系统配对待确认，尚未arm或测试视频切换。新增取消零释放组合回归通过，原审查问题已修复。
- 独立firmware/hid_probe已实现，只发送标准键盘导航，不是鼠标或绝对触控。Android App/原touch_probe源码未改，原App暂停作为HID连接入口。
- 304项C++主机检查通过（原196+HID108），包括报告/arm/释放、串口完整行、旧代数丢弃与单槽释放组合；另9项真实上传清单校验通过，测试未访问设备。
- 原touch目标已完整编译并校验，源码SHA256保持56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b，程序626361字节/RAM18648。恢复产物位于TEMP/thumbremote-touch-56789245aac4-targets-v1。旧生成目录保留，未删除或覆盖。
- HID首版有POSIX link同名编译冲突，已改sharedLink。初审2项Important：发送与连接回调竞态、可读Input Report缓存旧键；改为同NimBLE host队列实际发送、明确conn/代数、可读值恒零。新增24检查先12失败后通过。复查又发现取消待发零释放的组合问题，新增13检查先1失败、修复disarm重建零释放后通过。实际SDK/平台效果仍待验证。
- 一个修复中途快照编译出程序631011字节，但源变更检查拒绝生成最终清单，未上传该中间产物；之后稳定源码的最终编译、上传已通过，见本节首项。
- 用户已亲自配对，平板显示键盘设备；串口generation=3、connected/subscribed/encrypted/ready均为1，配对期间armed=0。原生抖音视频页已打开，方向键试验等待真实触摸和用户效果反馈；配对通过不等于视频导航通过。

## 电脑命令与恢复

```powershell
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify -Target touch
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify -Target hid
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 upload -Target hid -Port COM7
```

默认目标仍为touch。上传HID前同时核对touch恢复清单和所有BIN；清单绑定目标/源码/选项，任何不一致都在芯片写入前停止。COM变化时重新核对，不盲用COM7。两项目标保持DIO/80MHz/4MB，无整片擦除。

恢复原App配套固件：用户明确选择恢复后，重新核对唯一C3，再执行 `scripts/esp32.ps1 upload -Target touch -Port COM7`。若原清单/产物失效，先重新verify touch。不删除平板/开发板绑定，系统里必要的用户操作由用户亲自处理。

## HID配对与输入窗口

1. 保持原四线，开发板USB连电脑；OPPO原App停止遥控。HID固件期间原App配套BLE服务不可用。
2. 用户在MatePad系统蓝牙里搜索ThumbRemote-HID-Test并亲自确认配对。无需通过卓易通里的App开无障碍；配对成功不等于原生抖音支持按键切视频。
3. 串口115200，以完整行发送 `status`；须看到connected/subscribed/encrypted/ready均1。默认armed=0，连接本身不发导航键。
4. 用户在原生抖音视频页准备好后，串口 `arm` 开60秒窗口。先等3秒，再由用户真实硬件上滑/下滑并抬手，每笔间隔约1秒。
5. arrow模式：硬件上滑发Down Arrow（下一条候选），下滑发Up Arrow（上一条候选）。40ms后全零释放；左右不发，忙/冷却不排队。
6. 箭头有效就结束本轮；无效才先 `disarm`，待释放后 `mode page`，确认模式，再由用户准备后 `arm`。Page模式对应PageDown/PageUp。模式命令不发键。
7. `disarm`、窗口到期、触摸错误、连接/订阅/加密/协议/挂起状态变化结束输入，重连不恢复窗口。已交给SDK的在途报告无法撤回；取消未消费槽并请求零释放，失败则停止/断开，不重放输入。

日志HID_NOTIFY_ATTEMPT只代表SDK尝试，HID_TX_STATUS不代表目标App已响应；最终切换结果需用户明确反馈。主机单测不冒充蓝牙主机实测。

## 固定库与范围限制

- 固定3.3.10-cn NimBLE，HID服务0x1812/报告ID1/8字节键盘。显式payload通知走NimBLE host事件队列，Input Report可读缓存恒零，避免旧按键被新连接读出。
- 单个SDK分发槽只容纳当前报告，零释放可替换尚未消费的按下；非零报告在SDK消费前超过100ms丢弃，不积压动作。与GAP/ATT状态变更同线程处理，实际通知绑定明确连接。无修改BLE库源码或后端。
- 标准加密绑定、无输入输出能力，不配置默认PIN、不自动确认用户安全弹窗。仅HID目标链接包装ble_gap_adv_start，拦截SDK重复配对自动删bond分支并拒绝重配，其余事件完整委托原库。遇到不支持的配对交互明确拒绝，不清空绑定。
- 库创建电量服务，但没有电池测量，本版未发布虚构电量。Boot Protocol不支持；需要报告模式，主机兼容仍待测。
- 没有添加鼠标滚轮/拖拽/绝对触控、原生鸿蒙App、显示或电池。若两组键盘导航均不适用，记录结果后再决定新方案，不自动扩大功能。
