# ThumbRemote 工作日志

## 2026-10-28 MatePad 双HID服务实板验证进行中

- 新源码 `61f1da63d7a7e1e0cfb63d183dfadd3c71f687f6abb64a83ae7e306a974cf2a5` 的编译产物、完整manifest及主机测试运行输出已存在；原运行会话因跨日不可恢复，不能宣称重新观察到该verify退出码。构建日志程序642679字节、RAM19432，manifest绑定完整二进制及源；`upload -Target dual -Port COM7`验证源码和清单、touch恢复产物、唯一CH343及ROM C3后退出0，四段写入校验通过。未整片擦除或清绑定。
- 被动串口启动日志`serial-composite-startup.log`：GATT_READY，App event=52/start=55、keyboard=29、digitizer=42，两个输入句柄不同；profile=registered，最初advertising1，随后一个安全绑定连接route=hid_arrow、hid_ready1、digitizer_ready0、errors0。该连接身份未确认，不能据此说是MatePad或数字笔被识别。已请用户在MatePad系统蓝牙确认连接后测试上下滑、单击与指针；等待反馈，不删除配对。
- GitHub任务：用户明确选择Ghz-byte账户公开新建ThumbRemote，并授权结束后上传。当前仓库无提交/remote；尚未创建GitHub仓库或推送。发布前已将两份历史文档中的本机用户名绝对路径改为工作区或环境变量，静态扫描未发现常见凭据、MAC或邮箱；准备首次Git提交时使用GitHub no-reply提交邮箱，避免公开本机真实邮箱。

## 2026-10-27 MatePad 点击实施（正在排查启动）

- 本轮新功能电脑端最新完成38项数字输入、57项派发、6项双线程串行、38项路由、66项触摸等全套测试及37项清单校验；`verify -Target dual`退出0，641317程序字节/RAM19360，source `2eb2e8dd6de369e5124a218346338c8577e0100563ea3097e370872c5fee4af7`。独立审查提出取消竞态、订阅变化半笔和第二笔抢占三项Important，均补回归并修复；硬件结果不由电脑测试推定。
- 唯一CH343/COM7及ROM C3、touch恢复清单核对后，2eb2版upload退出0、四段写入校验通过；未整片擦除、不清绑定。启动后串口status为route none、advertising0；RTS复位捕获`DUAL_GATT_TABLE_FAILED; no advertising`，触摸ACK，表明当前板上2eb2版蓝牙未就绪，不可视为平板通过。日志`serial-matepad-restart.log`保存在该版本TEMP目录。
- 已按系统化调试增加启动失败的各项句柄/返回码诊断输出，保留原校验条件，不猜测原因、不绕过检查；新源码正在完整verify，尚未上传或获取诊断。8dceb已获用户OPPO单点双点和MatePad翻页反馈的产物清单已核对并保留。
- aa895诊断版完整verify/upload后复位串口显示 `event_rc=0 start_rc=0 hid_rc=0 app=39/39 start=42/42 keyboard=29 pen=65535`，确定第二报告未注册。核对SDK `BLEService::addCharacteristic` 的NimBLE分支：同服务重复UUID只恢复原对象、并未登记新对象；与SDK公开issue #12951相符。独立审查先前“可保留重复UUID”判断遗漏该分支，已更正。按蓝牙HOGP多HID实例能力修订设计，采用同一BLE服务器中的两个HID服务实例，以独立对象和特征句柄区分；平台实际枚举仍待验证。
- 当前源码手建第二个HID服务，不重复创建DIS/Battery；键盘和数字笔分别有自己的报告映射、订阅和模式/挂起状态。主机测试先因服务启动接口和状态分离缺失失败，修复后通过；串行取消/状态变化/快速输入回归保留。完整verify正在运行，当前板内仍为aa895诊断版，蓝牙尚未恢复，不能作为用户可用版本。
- 用户额外授权Git提交及任务结束后上传GitHub，选择账户Ghz-byte下公开`ThumbRemote`新仓库；项目原无任何commit、remote。公开前检查凭据、日志和机器路径；实际上传须以测试结果和实际命令输出记录，未上传不标成功。

- 用户审阅具体方案后回复“动手动手”，方案、电脑验证和唯一C3更新已获授权；不重复询问。8dceb清单中五个构建BIN均重新核对匹配，原文件保留。使用现有全部未跟踪的工作区，不创建缺少现有文件的Git worktree；未提交、删除或清绑定。实施进度以本节后续记录为准。

- 用户要求“现在开始做平板”。在请用户先切到MatePad并验证当前上下滑后，用户反馈“可以翻页，但是无法收到单点的结果”。按8dceb现有实现，平板短点不发送点击；此反馈证明本轮平板翻页基本有效及单点当前未实现，不推定双击或其他专项。
- 用户目标结合前文为同一固件增加MatePad中心单点、快速两点，保留OPPO路径和方向键、不重刷切设备；已完成只读核对并制定`docs/superpowers/specs/2026-10-27-matepad-tap-design.md`和对应计划。设计提议新增标准HID绝对坐标报告，HarmonyOS实际支持/位置/视频响应及GATT缓存均待真机验证；不自动删除配对。当前只写文档，未改产品代码、编译或烧录。
- 本轮新增蓝牙输入类别和服务表；用户已审阅方案并确认实施。保留8dceb固件及OPPO App，不把先前手机功能授权当作平板新增HID报告的完整方案确认。

## 2026-10-27 手机中心单击、快速连续操作（OPPO本轮反馈通过）

- 用户确认触摸板单击对应OPPO屏幕中心、两次快速轻点连续点击、移除遥控动作结束后的固定500ms冷却，并说“开始吧”。按所述方案实现、验证、更新唯一设备；不扩展MatePad点击，不新增权限。
- `firmware/dual_remote/dual_touch_control.h`复用原方向识别器，仅AppReady路由识别完整短接触：释放时输出tap，观察起点至所有点最大轴偏移小于40且时间不超过350ms，长按/移动后返回/多点/故障取消；两阈值是软件选择，真实手感待测。独立touch/hid源码保留。事件码5为单击；方向事件码1–4和配对/GATT结构保持。
- 短时顺序发送：固件App最多4条、100ms过期，READY优先，路由变化取消；原单槽新回归4处失败后修复。App最多4条、500ms过期，当前手势完成后继续，无固定冷却；屏幕锁定/停止/断线清除，不补发。手动测试冷却保留。双击是立即执行两次单击，不对第一下等待双击窗口；实测速度、蓝牙延迟及目标App双击判定窗口未量化。
- 验证：Android `verify`退出0，30个JUnit测试0失败、Lint和Debug APK构建通过；`install`确认唯一已授权PLB110并返回Success，APK SHA256 A467221E0093B354226A4E9F8764C7A2A578D67AFF63FDD65919D560570784F6。固件最新主机测试含Dual touch 55、dispatch 51及其余测试均0失败，清单37检查通过；`verify -Target dual`完整编译退出0，程序639087字节、RAM19288，source SHA256 8dceb250509ae1ca605e6c0d27d3907f93a300f414d6a3bc3a48b4a7b189e2fa。测试增强发生于固件编译期间，随后独立重跑主机测试全通过，固件源码未变。
- `upload -Target dual -Port COM7`核对唯一CH343、ROM ESP32-C3及touch恢复清单后退出0，四段校验通过，不整片擦除、不清绑定。启动后6秒被动串口 `serial-tap-startup.log`：profile=1、connected=1、encrypted/bonded/identity=1、route=hid_arrow、errors=0；未推断主机身份，未发生手势，不能视为OPPO点击或平板回归通过。串口已关闭。
- 邀请用户亲自测试后，另采30秒`serial-tap-user-test.log`仍为route=hid_arrow、app_session=0、gestures=0、errors=0，未观察到测试动作；这只是采样窗口空闲，不能当作测试失败。串口已关闭。
- OPPO本轮反馈：在明确要求依次测试单击、快速双击和上下滑后，用户答复“都可以了”。据此记录这三项在当前8dceb版及新App上的基本测试获用户反馈通过；未独立观察画面、具体应用和点击作用，未推定次数、长期稳定性或其他机型适配。
- 最新板内固件为8dceb250509a，手机为本轮新App；最新版MatePad方向键回归、平板单击、两台来回切换和异常专项仍未验证，不沿用旧版双端反馈替代新版验证。

## 2026-10-27 OPPO再次扫不到：重连发现入口修复

- 用户反馈“手机又连不上了”，进一步明确为App扫描不到。串口serial-phone-reconnect-20261027-094031.log确认原1ed9版启动GATT_READY、DUAL_STARTED registered/29字节；随后generation2、route app_waiting、profileValid、connected/encrypted/bonded/identity/app_sub/app_claim/hid_ready均1、app_session0、errors0。没有有效本次START，不是登记丢失；广播因连接而关闭。
- 根因：OPPO系统HID自动重连让旧“任何连接均停广播”策略隐藏原App扫描入口。上次手动断开只能暂时恢复，恢复CCCD也不代表App建立新会话。本轮先改规则/设计，再以纯发现策略回归复现/修复；保留原单连接、App/HID互斥及显式START，不改Android、不清绑定。
- 修复范围：仅已登记App peer在本次加密/绑定/稳定identity确认时连接期间保留广播（AppReady也保留，避免App退出后系统仍连导致再次卡住）；其他主机连接仍不广播。host事件负责广告状态同步，补status广告可见性；现有实施/验证/上传授权覆盖此缺陷修复，不重复询问。
- RED→GREEN：新增12个发现策略检查；将旧!connected策略提取后实际3处失败（登记手机等待App、AppReady、App会话归零但系统仍连）。修复后全598 C++/37清单通过，verify完整编译进行中。新发现策略不改变RoutePolicy输出路由/连接代数，额外连接仍按handle拒绝；host state事件按实际isAdvertising同步，失败1秒退避，日志增加advertising字段。
- 依据：[NimBLE广播指南](https://mynewt.apache.org/latest/tutorials/ble/bleprph/bleprph-sections/bleprph-adv.html)说明建立连接后广播需显式恢复；[AOSP GATT实现](https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/heads/main/system/stack/gatt/gatt_main.cc)维护同peer ACL的多个应用持有者。本修复推断原App可在发现设备后附着到现有手机链路，OPPO实际行为须修复版实测，不据此承诺所有系统。
- 独立只读增量审查完成：无Critical/Important/Minor；确认host同步、MAX_CONNECTIONS=3支持本次广播、默认断连自动广告关闭、包装回调仍委托同server、第二连接拒绝不改link、路由/START/最终门控不受广告变化影响。实际手机附着、控制器失败/连续抢连/host reset仍未验证。完整编译TEMP/thumbremote-dual-e3fbea199932-targets-v1进行中，成功后按已批准范围核对并上传；不重试旧1ed9作为修复。
- 最终verify -Target dual退出0：598 C++/37清单检查，638225程序字节/RAM19144；source e3fbea199932149f063336b13fee766e4dbc28f5d670a8fedbd6ea8eb06b9787，TEMP/thumbremote-dual-e3fbea199932-targets-v1；ELF包装实际调用核对通过。upload重新核对唯一CH343/COM7和ROM C3、touch恢复清单后exit0，四段哈希通过；不整片擦除/清绑定。
- 上传后10秒采样serial-discovery-fix-startup.log：6组DUAL_STATUS，profileValid、connected0/advertising1、touch idle/errors0/unknown0/gestures0、panic0；说明当前未连接时广播正常且登记仍保留，不冒充“连接期间广播”已实测。串口已关闭；已请用户先让系统蓝牙连接ThumbRemote并保持连接，再从原App扫描/选择/开始接收，等待结果。
- 本轮复测通过：用户在“保持OPPO系统蓝牙已连接、不先断开，再从原App扫描/选择/开始接收”问题下明确答复“能扫描，且显示可以接收”。e3fbea版该重连发现入口与接收握手记录通过，不再要求手动断开系统连接绕过。5秒串口serial-discovery-user-pass-20261027-120635.log有3组generation4、route app、profileValid、connected1/advertising1、加密/绑定/identity/App订阅/session/HID ready均1、errors0；证明连接与广播实际并存且App接管。
- 本次采样gestures14/unknown14只作累计状态，未逐笔捕获方向/目标应用效果，不记14次翻页或成功率。退出重进多轮、停止不回退、两台抢连和断电专项仍未由本次成功推定。日志已保存，串口关闭；本轮无代码修改/编译/烧录。

## 2026-10-27 合并固件两端基本测试反馈

- 用户明确反馈：“手机端和平板端都测试成功了”。按当前同一合并固件联调上下文，记录OPPO App路径、MatePad Arrow HID路径的基本测试均获用户成功反馈；不再把平板核心路径记为未测试，不要求重复同一轮基本测试。
- 对应当时两端基本测试的上传版本为 `dual_remote`，source `1ed9b4bde4df405e68e8cdb6b8f3eedbb88358a6d00cf883aebe5e7c1e0053d4`；该次反馈期间未改源码或重刷。之后重连缺陷修复版为顶部e3fbea版本，不能把此处旧版两端反馈直接标为新版复测通过。此前板端已有OPPO profile Valid、route app、HID同时ready、errors0的证据。
- 用户未逐项描述手机操作页面/视频变化、上下映射、成功次数、双端来回两轮、超过60秒、停止/锁屏不回退、半笔断连、断电后登记保持、充电宝或缓存迁移过程。只记录两端基本成功，不把上述专项或稳定性记为通过；此前手机接收/真实翻页范围的追问不再扩大此次反馈。
- 本轮仅更新工作日志、验收、进度和使用文档，回读核对；未运行新编译、烧录、串口或手机操作。之前586 C++/37清单与完整编译/上传是2026-10-26证据，不伪装成本轮重跑。
- 下次起点：当前合并固件继续使用，换设备先断开当前连接，OPPO从原App主动开始、MatePad从系统蓝牙连接。用户若要继续专项验收，从未测项选取，不恢复旧独立固件、不清绑定、不重复配对/基本测试。

## 2026-10-26 合并实施 ledger — plan: docs/superpowers/plans/2026-10-26-dual-remote.md

- 授权：用户在完整方案和验证后上传范围后回复“开始”；现实施/验证/唯一C3上传获准，首次OPPO登记等设计细节一并确认。系统授权仍用户处理，不删文件/绑定、不提交推送。
- Task 1: complete — 新配置35/路由23检查，先缺模块失败后实现，全504 C++/9清单通过（test exit0）。配置CRC/身份/故障关闭及App跨重连锁定已覆盖，SDK实际identity仍未测。
- Task 2: complete — 单笔/释放/路由切换32检查、发送/READY优先29检查通过；新增模块/取消仅方向接口先失败后实现。全主机用例0失败。
- Task 3: complete（电脑端）— 唯一BLE服务端、host路由、NVS存储完整编译；store15/广播1、FULL/OVERFLOW虚回调注入1检查通过。host当场锁存App意图；显式handle/payload/代数；固定SDK3个bond，满时返回错误，保留重复配对包装。真实身份解析/存储满载/缓存迁移仍待实物。
- Task 4: complete — 最新重连修复e3fbea199932完整verify退出0，598 C++/37清单，638225程序字节/RAM19144；source及产物见顶部。原1ed9的586 C++/37检查与两端基本成功为历史基线，发现重连入口问题后不能当作修复版。旧9711为崩溃历史，不恢复；原touch恢复入口保留。
- Task 5: 两端基本测试获用户成功反馈（2026-10-27），专项验收尚未齐全 — 1ed9b4bde4df版upload退出0，唯一CH343/COM7/ROM C3和touch恢复清单核对、四段写入哈希通过；无整片擦除/清绑定。此前8秒空闲正常，之后OPPO登记与App接管有串口证据。本次手机/平板成功依据用户反馈，具体次数/方向映射未细分；停止不回退、两轮切换、断电等专项仍未验证。最新反馈与下次起点见顶部2026-10-27段。
- 首次App扫描反馈“扫描不到设备”：只读串口status确认generation2/connected1/encrypted1/bonded1/identity1/hid_ready1，App sub0/claim0/session0，profileMissing、route app_waiting、errors0。证实当前已有HID主机占用且按设计停广播；不推断品牌或App握手通过。日志serial-scan-failure.log保存于最新TEMP根，未重刷/重置/删绑定。已请用户在系统蓝牙断开ThumbRemote（旧名ThumbRemote-HID-Test），保留配对并直接回App扫描，不在系统页重新连接；必要时断开旁边MatePad。等待操作反馈，先不改代码。
- 用户随后明确反馈“App已能找到设备”，按系统断开后App扫描恢复记录通过；不扩大为连接、START/READY、身份登记或实际遥控通过。已请用户在接收测试选择ThumbRemote并开始接收，暂不滑动；本轮串口仅status/被动采样，不代操作App或系统配对。
- 板端登记/路由新证据：20秒采样serial-app-enroll-20261026-210605.log见generation9、route app、profile1(Valid)、connected/encrypted/bonded/identity/app_sub/app_claim/app_session/hid_ready均1，touch idle/errors0/gestures0。此前profileMissing本次转Valid，按实现已完成NVS写入与读回；HID同时ready仍锁App。未捕获保存/READY具体通知边沿，不把状态当App收到READY确认；App“可以接收”反馈仍待取得。未发方向或导航指令。
- Task 5硬件首轮：9711ef40a039版upload退出0，唯一COM7/C3与touch恢复均核对，四段写入哈希通过；8秒被动串口采样见重复Load access fault，不能记为运行通过。serial-dual-startup.log保留。addr2line/反汇编定位MEPC0x420014d4到ble_npl_event_init，npl_funcs尚为null：错误地在BLEDevice::init前初始化NimBLE事件。没有配对/导航或清绑定。
- 硬件bug修复：补启动runtime顺序与BLE初始化失败分支回归，保留原错误顺序时2处失败；改为BLEDevice::init成功后才ble_npl_event_init。重新完整verify/upload仍在本次已批准范围，不重复问权限；旧9711产物标为不可用，未自动恢复独立固件。
- 上述修复后586 C++/37清单通过，完整编译TEMP/thumbremote-dual-1ed9b4bde4df-targets-v1进行中，尚未声明新版编译/上传成功。
- 上传隔离实测：9711构建目录上传后extra_bins=0，upload-3cf9d20ebada49a3b2130e2c126d569b内出现4个*_flashed.bin，证明本轮工具辅助产物留在stage；不再推断这些是Flash读回，也不把旧9711镜像记为运行可用。
- Ruling: 复用当前无提交基线的用户工作区，ledger保存在既定WORK_LOG而非新建.git依赖的sdd目录；计划已明确不建新worktree/不commit，不运行会自动删除或提交的技能脚本。成本：不能用Git提交恢复，只依靠保留源码、产物与本ledger。
- Pre-flight: T1 Profile/LinkSnapshot/RoutePolicy→T2 control+dispatch与T3 host，host唯一拥有RoutePolicy，loop只读快照；T2 Action→T3 send→T4 sketch，代数统一；T4构建源闭包包括四个原字节共享头，旧独立目标不改。新dual_dispatch_gate.h用途已在计划规定，测试继续现有firmware/tests。
- Ruling: 主机测试编译入口提前增加原hid头文件include路径（原属任务4的构建接入），以保证新纯头与Arduino扁平镜像使用相同include名称；不复制或修改共享头。成本：测试脚本有提前的一行必要变更，旧sketch输入哈希不变。
- Ruling: 平台存储用SDK原生NVS接口而非Preferences，以明确区分“键不存在”和读取故障，不把损坏当首次登记；保留批准的16字节格式/命名空间/只写一次语义。成本：需核对NVS返回值，已加纯I/O失败/读回故障测试。
- Ruling: 新增dual_advertising.h纯字节编码，避免只测算式不测实际载荷；新增TransportSnapshot整体getter避免loop分别读route/gen时撕裂；健康取消仅清方向不清READY（补取消控制帧竞态测试）。成本：少量新增接口，路径/动作/授权不扩大。
- 首次完整verify -Target dual退出0：637197程序字节/RAM19136，源ecfcbc51cc30a75cb3f279558be16c7ca6bcb4e2943f41607fabdcc6c54d44c2，TEMP/thumbremote-dual-ecfcbc51cc30-targets-v1；581 C++/28清单检查通过。独立只读完整审查进行中，未上传。清单测试独立运行曾因Get-SketchName加载晚而失败，已调整函数提取顺序后独立exit0；产品源码未因此变化。
- Final review: 独立只读审阅完成，2项Important、无Critical/Minor；旧ecfcbc产物不可上传。Important1固定SDK HID startServices提前冻结整个表，Important2重复verify可能把旧额外BIN重签清单。
- Final: fixed 服务注册时序 — dual_service_setup测试保留旧顺序时1处失败，再改为公开DIS/HID/电量服务按旧顺序注册、追加App、唯一server.start；2检查通过，增加真实SDK handle启动检查，不符合则不广播。
- Final: fixed 产物污染洗白 — 清单测试缺Assert-BuildBinaries先失败，增加编译前/后固定SDK精确名称白名单、manifest未知名称拒绝，以及污染被写入清单后仍拒绝；37清单检查通过。可选-BuildId只用于另建干净TEMP产物根，默认旧路径/目标保留，绝不删除或改旧清单放行。
- Final: Ruling: 审阅列出未实测身份/RPA/旧缓存/翻页/断电项；这些必须保留未验证，先以获批上传取得运行证据，不以主机测试冒充实物通过。store满载不填满/删除真实bond；补同一生产回调模板的FULL/OVERFLOW注入与虚派发无删除测试，编译绑定实际SDK基类；真实NVS满载仍未验证。成本：平台集成边界仍需真实用户会话证据，失败时停止不放宽。
- Ruling: HID报告UUID存在多个实例，启动检查不假定第一个0x2a4d一定是input；核对HID服务注册和SDK分配的真实input handle，App独有UUID则逐个对比handle。成本：HID通知订阅仍须真实主机验证。

更新日期：2026-10-27（Asia/Shanghai）。当前板内为App重连发现修复版e3fbea199932，598 C++/37清单、完整编译/上传通过；用户已确认保持OPPO系统连接时App“能扫描，且显示可以接收”，串口connected1/advertising1/route app/session1/errors0相符。旧1ed9两端基本测试成功仍按当时版本保留，不扩展为新版全部专项通过。原touch/Android源码、绑定及接线保持，换到另一台设备仍先断开当前有效主机；手机自身的系统HID连接不再要求先断开才能从App扫描。

<!-- TR-HANDOFF-BEGIN TR-20261026-DUAL-01 -->
## 新对话交接：TR-20261026-DUAL-01

- 状态：已接手。2026-10-26用户在本对话指定该编号继续，关键源码/恢复产物/授权边界核对完成；已开始并保存方案和计划草案。源/接手对话ID未取得，不伪造；未调用新建对话工具。原源对话的串口/手机采样已结束，本轮未打开串口/操作手机；只读设计审查不拥有业务修改权。
- 项目路径：当前工作区根目录。交接时工作区全部未跟踪，无可用于回滚的Git基线；当时没有commit/push。不要因git diff空就判断没有代码。

### 目标、授权和未决项

用户最新原话：“两个不能同时用是吗”→解释可合并一份固件，OPPO原App、MatePad系统HID、一次只控制一台、切换设备不用重刷→用户“ok我换一个对话直接开始”。已同意合并方向，不是要求同时遥控两台。不要再问App/HID二选一，不恢复已被用户放弃的长按启停，不重复摸索OPPOArrow/Page。

本轮最新决定：用户已选择自动分流，不采用左右滑选模式；在完整方案与“电脑验证通过后上传重新核对的唯一C3”范围后回复“开始”，确认docs/superpowers/specs/2026-10-26-dual-remote-design.md和docs/superpowers/plans/2026-10-26-dual-remote.md，包括首次OPPO登记、仅一台持久App身份、所有输出确认绑定身份、配置故障关闭等细节。实施与上传进度看顶部ledger，不重复请求已覆盖授权。用户系统授权/配对仍本人处理；禁止删文件/绑定、全局依赖/系统配置变更、git提交推送等越界行为。

### 确定事实与当前版本

1. 原方案：`firmware/touch_probe/` + Android App，在OPPO PLB110/API36上已验证硬件上下滑翻抖音，原USB充电宝方案也获反馈通过。App有用户主动开始的connectedDevice前台服务、通知停止，只有UP/DOWN执行，左右记录，锁屏/断线等停止、不自动恢复/补发。原源码未改。
2. MatePad mini/HarmonyOS7.0.0.109：卓易通运行Android App不能取得系统辅助功能，不要再反复要求开权限。原生鸿蒙抖音曾在HID v1 Arrow实测“上滑下一条、下滑上一条，都有效”。只证明该次Arrow核心路径，不是所有版本/应用或当前Page固件都通过。
3. OPPO当前抖音40.8.0(versionCode400801)：HID Arrow和Page两组均明确反馈不翻页。手机系统getevent已实际收到KEY_DOWN及KEY_PAGEDOWN/KEY_PAGEUP按下/释放，焦点在com.ss.android.ugc.aweme。不能通过反复改阈值/配对声称修复，也不推广为所有Android均不支持。下一步OPPO用原App协议。
4. 用户先选择长按启停，v2实物未开启，已明确放弃；随后批准连接直接使用、无长按/60秒。当前`DirectControl`是ready+健康+确认释放后自动可用，异常/重连取消半笔，恢复后只收新输入。不要把旧日志的arm60秒要求当现行需求。
5. 板内目前`hid_probe` Page对照版，源码SHA256 `8ee49f4f32413717bd0d4116975e6bde58fa2fe3a7e0539e7154f78f77a89a4c`，630573程序字节/RAM18816，产品sketch里显式`DirectControl control(hid_probe::Mode::Page)`。合并HID路线应恢复已在MatePad有效的Arrow，不能照搬当前Page默认值。
6. 当前最终主机检查446 C++（BLE29/gesture128/direct83/gate108/longpress59/touch39）+9上传清单检查通过；完整verify/upload均exit0，独立审查无重要问题。无合并版代码/构建/验证，也没有双机并发验证。

### 必读文件与第一步

- 根`AGENTS.md`：授权/目录/隐私/构建纪律；`docs/acceptance.md`：哪些已通过/无效/未测。
- `firmware/touch_probe/ble_transport.cpp/.h`和`ble_event_protocol.h`：原App的自定义服务UUID、START会话、事件序号、通知门控；`android/.../ble/`：扫码过滤、握手与订阅；`RemoteControlService.kt`等：已验证的原App启停，优先保持兼容。
- `firmware/hid_probe/hid_keyboard_transport.cpp/.h`：HID、安全/订阅/ready代数、NimBLE host事件队列显式payload、中性可读缓存、重复配对保护包装；两边现在各自调用BLEDevice::init/createServer并安装回调，不能简单同时begin。
- `firmware/hid_probe/hid_keyboard_gate.h`、`hid_direct_control.h`及`hid_probe.ino`：真实触摸、门控与释放；旧`hid_touch_control.h`保留但不接入，不删除。
- `scripts/esp32.ps1`：默认touch，显式hid；源码/选项/每个BIN清单；HID上传校验touch恢复产物。增加合并目标或结构前先改规则，不能覆盖旧恢复入口。
- `docs/hid-probe.md`、本日志后续历史和三个HID设计：已有兼容性证据，遇到旧“编译中”等以顶部最终状态为准。

**下一步：**两端基本测试已获用户成功反馈，继续使用当前合并版；用户需要进一步验收时只推进顶部列出的未测专项，不重新要求基本配对/双端测试，不自动恢复旧固件。不得把双端基本成功扩展为停止/断电/稳定性全部通过；登记App身份跨重连/重启禁止HID仍是实现要求。

### 构建、硬件和证据入口

- Windows PowerShell，固定esp32:esp32 3.3.10-cn/NimBLE；唯一CH343/ESP32-C3，最近COM7，115200。DIO/80MHz/4MB（QIO这块板曾启动失败），每次上传先复核端口/芯片，不整片擦除。
- 接线固定：SDA绿GPIO4，SCL黄GPIO5，红3.3V，黑GND。四线保持，不加显示/电池电路/鼠标/触控模拟。正常非坐标Flick包应忽略、不当松手且不取消整笔；依据docs/touch-probe.md，v2曾错误cancel。
- 仅本机TEMP产物根：`%TEMP%`。Page `thumbremote-hid-8ee49f4f3241-targets-v1`，Arrow直接版 `thumbremote-hid-8b3672a0abca-targets-v1`，原touch恢复 `thumbremote-touch-56789245aac4-targets-v1`（完整源hash56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b，626361程序字节）。旧HID v1 `thumbremote-hid-1ed8e5ab95cb-targets-v1`也保留。不是跨机器可用的资源，新对话核对存在性，不自动删除。
- Page目录的`compile.log`、`upload.log`、`serial-oppo-page.log`、`oppo-page-key-events.log`；Arrow目录的`oppo-input-devices.txt`、`oppo-hid-key-events.log`、`serial-oppo-diagnose-20261026-150424.log`。手机getevent主动timeout90退出1是限定采样，不是权限失败。没有通过ADB注入导航/授予权限/操作安全弹窗。
- 命令：`powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test`；`verify -Target hid|touch`；`upload -Target ... -Port COM7`。Android必要改动才运行`scripts/android.ps1 verify`，无改动不重复构建。合并未测不标通过，notify rc0不当作App翻页确认。
<!-- TR-HANDOFF-END TR-20261026-DUAL-01 -->

## 2026-10-26 合并方案核对与计划（本轮仅文档）

- 本轮按project-handoff恢复流程核对指定交接，未创建新聊天。git status仍全部未跟踪，无commit/push；不能以空diff判无代码。
- 当前touch完整源hash仍56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b，hid仍8ee49f4f32413717bd0d4116975e6bde58fa2fe3a7e0539e7154f78f77a89a4c。四组本机TEMP产物（touch/Page/直接Arrow/v1Arrow）的manifest与所有列出BIN哈希匹配；不宣称新Flash读回或真机回归通过。
- 更严格预检补充：实际提取并调用现有Assert-Verified，touch完整通过；Page报Unexpected binary set。三组HID build各多4个`*_flashed.bin`（boot_app0、应用、bootloader、partitions），都与对应原BIN哈希一致，但生成来源未核实，不能称读回文件；前述“列出BIN哈希匹配”不等于目录可直接上传。文件全部保留，原保护不改。计划为未来上传使用独立TEMP staging避免辅助文件污染已验构建目录，历史HID若要恢复需另建干净根完整重建/验证。
- 运行powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test退出0：BLE29、gesture128、direct83、gate108、longpress59、touch39，共446 C++检查0失败，另9清单检查通过。测试仅写TEMP夹具，无设备访问；夹具thumbremote-manifest-test-6b9adabbb9274c6aaabb3378db40451d保留。
- 只读枚举仍见USB-Enhanced-SERIAL CH343 (COM7)，未握手ROM、未打开串口/复位或读取手机。最后板内上传记录为Page版，合并HID使用Arrow的选择只在文档。
- 用户答复“自动分流，日常只断开再连接”。草案设计新增首次OPPO App登记与稳定绑定身份保存，以避免停止App后系统HID续连/重连变为导航；首次登记细节待确认，不扩大现有上传授权。系统配对/授权由用户亲自处理。
- 只修改规则与文档，预留dual_remote目录用途但未创建产品目录；未改Android、固件、构建脚本或SDK，未编译/烧录合并版。旧缓存迁移、身份解析、首次配对超时、两机切换/实际效果均未验证。
- brainstorming要求的只读builder审查已完成：指出App订阅→退订在两次loop之间发生会丢占用边沿；已在设计和计划补为host回调立即锁存appClaimed、host单线程拥有权威路由、最终发送直接检查，并加该事件顺序回归。该项由作者明确，不需用户重新选择路由；审阅者未指出其他重要设计缺口。写入后本地文档引用/编码/产品目录边界已检查；等待用户完整方案确认，未自动进入实施。

## 最新接续：2026-10-26 OPPO两组键盘导航均未翻页，停止对照等待取舍

- Page对照最终verify -Target hid退出0：630573程序字节/RAM18816，源hash8ee49f4f32413717bd0d4116975e6bde58fa2fe3a7e0539e7154f78f77a89a4c，TEMP/thumbremote-hid-8ee49f4f3241-targets-v1。touch恢复清单、唯一CH343/COM7与ROM C3核对后upload退出0，四段哈希通过，无整片擦除/清绑定。原Arrow构建8b3672a0abca和原touch恢复56789245aac4保留。
- 上传后初始connected0，已请用户亲自从系统蓝牙重新连接，用户反馈已连接；serial-oppo-page.log确认generation3、ready1/enabled1、mode direct_page、errors0，并采到0x4e/0x4b与零释放。手机event8再次getevent读到KEY_PAGEDOWN和KEY_PAGEUP真实按下/释放（oppo-page-key-events.log，限定90秒主动结束码1），用户明确“仍然完全不动”。板端与系统事件均已证明发键，不当作应用响应证明。
- 本轮具体结论：OPPO PLB110、抖音40.8.0当前视频页对本次Arrow/Page键盘导航均未实现翻页；不推广为所有OPPO、所有Android或其他应用均不支持。不能继续通过调触摸阈值/重配对声称修复。原OPPO App+touch方案此前已通过，可按用户选择恢复。
- 本轮不继续鼠标/触控模拟，不自动切固件或清绑定。待用户选择恢复原touch以继续OPPO软件遥控，或恢复直接Arrow供MatePad后续测试。当前板内仍Page试验版，不能直接叫用户在MatePad照旧用，因为该平板只验证过v1 Arrow。

## 最新接续：2026-10-26 OPPO方向键已送达但抖音未响应，Page对照编译中

- 手机USB调试由用户亲自接入允许，ADB唯一PLB110。event8的ThumbRemote-HID-Test使用Generic.kl/kcm，getevent日志oppo-hid-key-events.log实际含11对KEY_DOWN按下/释放（部分系统批量入队时间接近，其他约47ms）；手机焦点com.ss.android.ugc.aweme，versionName40.8.0/versionCode400801。用户明确“已完成，视频仍不动”。不因SDK rc0直接认定手机收到，而是以上实际系统事件证明；未自动发送ADB keyevent或操作手机。
- 按原已批准最小HID设计“箭头无效才试Page组”，新DirectControl构造参数Mode默认Arrow，本轮sketch显式Page；真实硬件上滑PageDown0x4e、下滑PageUp0x4b，零释放/冷却/连接自动可用保持。无需长按或串口arm，旧Arrow构建8b3672a0abca保留，不清绑定。新增18项协调检查先2失败后全绿，总446 C+++9清单通过；原review_hid_direct仅复查增量未发现问题。
- 完整verify hid正在进行；未标Page编译/上传/OPPO响应通过。若Page也无效，应记录该设备当前键盘导航均不适用，不擅自扩展鼠标/触控或恢复原App固件；下一步另按用户取舍决定。

## 最新接续：2026-10-26 直接使用v3已烧录，等待OPPO上下滑反馈

- 用户实际反馈直接版“不动”，进一步复测仍不动。serial-oppo-diagnose-20261026-150424.log记录generation8、ready1/enabled1/errors0，手势计数15→26，含UP/DOWN及0x51/0x52与每次零释放notify rc0。此后用户用USB接OPPO，ADB确认唯一授权PLB110；dumpsys input将ThumbRemote-HID-Test识别为event8标准键盘，焦点为com.ss.android.ugc.aweme，getevent实际取得KEY_DOWN按下/释放。只证明系统收到下方向键，不能说抖音消费或所有方向均手机端验证。事件文件oppo-hid-key-events.log保存构建TEMP目录；timeout90结束码1为有意限定采样，不是ADB权限失败。
- 问题已缩小到手机侧导航键处理/应用兼容性，尚未确认是哪一层忽略，不能说硬件坏或所有Android不支持。按原已批准最小HID方案，在箭头无效时只补PageDown/PageUp导航对照；不自动注入按键、不加鼠标，不要求重新开启辅助功能。Arrow构建与原touch恢复产物均保留。Page对照实际效果待后续记录。

- 最终verify -Target hid退出0，程序630285字节(48%)/RAM18816，源hash8b3672a0abca9ca869f5aad4c6bbf1c4842a4647fdf51618ae0ed864f3709250，产物TEMP/thumbremote-hid-8b3672a0abca-targets-v1。上传前旧OPPO被动串口采样正常结束，touch恢复清单、唯一CH343/COM7与ROM C3身份核对通过；upload退出0、四段哈希通过，无整片擦除/清绑定。
- 上传后新被动采样serial-oppo-direct-20261026-145908.log，取得4组generation3、connected/subscribed/encrypted/ready/enabled均1、direct_arrow、idle、errors0/gestures0/unknown0。未发串口arm，证明连接释放后自动可用；不把enabled或notify当实际切视频。
- 已提示用户在手机抖音直接上滑一次停2秒下滑一次，不长按；正等待用户结果。>60秒与充电宝本版使用、平板v3尚未实测，不要求用户重复旧路线已完成项目。

- 用户在完整直接使用方案问题中明确选“按这个直接使用方案改”，已先更新AGENTS与设计/计划2026-10-26-hid-direct；该授权取代长按/显式arm/60秒前提，不能再重复要求长按。行为为系统蓝牙ready+触摸健康+确认松手后自动可用，恢复连接/通信后只接新笔；断电或断开停止。
- 新增DirectControl纯协调（连接代数/健康/释放/原GestureRecognizer/Gate持久会话），sketch v3只接此输入，串口仅status；旧长按文件和测试保留但不接入，不删除。新增65项主机用例先32处预期失败再实现，全428 C++与9清单检查通过。独立review_hid_direct增量只读审查无Critical/Important/Minor问题。
- 根因依据：docs/touch-probe.md已有实测/资料记录正常非坐标报告包含芯片Flick，应忽略，不因它们取消正常滑动。v2每个非坐标包都调用cancel，与OPPO日志unknown增长/接触取消相符；v3恢复v1忽略策略，既不当松手也不当方向。真实I2C错误仍取消，测试新增恢复健康后的半笔不可发送。
- 最终verify -Target hid进行中，构建TEMP/thumbremote-hid-8b3672a0abca-targets-v1；未声称编译成功/上传或OPPO实际翻页通过。下一步成功后校验恢复清单、唯一COM7/C3并上传，用户准备好再实际滑动。

## 最新接续：2026-10-26 长按启停v2已烧录，等待用户物理测试

- 最新用户实测反馈“长按后仍不能翻页”，随后提出“要不不弄长按启停了”。本轮日志显示ready1但armed0/persistent0、gestures0，unknown增加且接触被取消，没有物理开启记录；只能确认长按未开启，不能据此判定OPPO不支持方向键。立即停止继续修复/测试长按路线，不自动发串口arm。提出替代行为：通电并连接后真实上下滑直接生效、断开/断电停止、无历史补发；尚待用户确认新默认开启行为，未改代码或烧录。板内仍v2默认待机。

- OPPO系统蓝牙连接HID：用户反馈“手机已连接，显示键盘或输入设备”；本轮只读串口generation8、connected/subscribed/encrypted/ready均1、armed0/persistent0、idle、errors0/gestures0/unknown12，未发arm。已开启被动日志serial-oppo-long-press-20261026-144616.log并请用户在手机抖音测试待机滑动、长按开启、上下滑、长按停止；效果待反馈，不将连接就绪当启停通过。

- 用户补充平板不在手边，改用手机测试。按既有OPPO作为默认目标，通过系统蓝牙连接HID，不使用原App的自定义BLE入口；无需改固件。OPPO的HID及长按行为尚未实测，不把此前原App/平板方向键通过推广到本轮。若旧平板仍占连接，需用户在旧设备可操作时断开或移出范围，不删bond/不自动清配对。

- 最终verify -Target hid退出0：程序632013字节(48%)/RAM18824，源hash4c384d1a7b5d363b72ee60ff4b7e4c3ae02cef62c1fca70e1d91c40f7b63039c，产物TEMP/thumbremote-hid-4c384d1a7b5d-targets-v1。upload前touch恢复清单匹配，唯一CH343/COM7与ROM C3身份核对通过；上传exit0、四段哈希通过，无整片擦除/清绑定。
- 上传后7秒只读采样serial-long-press-idle.log取得4组心跳：generation3、connected/subscribed/encrypted/ready均1、armed0/persistent0、arrow/idle、errors0/gestures0/unknown0，端口已关闭。没有发arm或代触摸；证明重连后默认待机，不宣称长按实物可用。
- 下一步请用户在原生抖音视频页：待机滑动无动作，原地长按约4秒松手后上下滑，再长按松手后滑动应停止。此为新增启停实物验证；60秒以上持续和充电宝使用尚未验证。板内已是v2，前文待编译为历史过程。

- 用户审阅聊天中的完整日常启停方案后选择“长按启停吧”，已先更新AGENTS授权和目录约定，保存设计/计划2026-10-26-hid-long-press。此项为已确认行为的小规模增量，当前会话继续实现；没有把批准理解为自动通电开启。
- Ruling: 单指每轴最大12原始单位抖动、3000ms后松手切换，>=3秒整笔接触不发导航；阈值为实现选择，不是实测毫米。Holding保持计时，多点/换指/未知包/取消整笔失效、等待释放。串口arm仍60秒，物理armPersistent无超时、原40ms释放与500ms冷却保持。
- 新增hid_touch_control.h和hid_touch_control_test.cpp，Gate增加armPersistent/persistent，sketch v2接入，未改原传输或touch源码。新增59检查先20处预期失败，再实现全绿；全套363 C++检查+9清单检查通过。独立review_hid_long_press只读审查未发现Critical/Important/Minor问题；未执行sketch硬件组合流程，实测仍必要。
- 最终verify -Target hid正在构建TEMP/thumbremote-hid-4c384d1a7b5d-targets-v1，尚未取得成功输出，不宣称已上传或长按可用。不自动发串口arm；下一步完整编译通过、恢复清单及唯一COM7/C3验证后上传，观察默认待机，再由用户长按测试。

## 最新接续：2026-10-26 HID已烧录配对，原生抖音上下导航通过

- 本轮日志serial-hid-arrow-ready-20261026-094915.log保存在最终HID构建TEMP目录；采样结束已显式disarm、零释放rc0，脚本exit0并关闭串口。板内保留HID测试版，未恢复touch、未修改产品源码。本轮仅补实测证据，不重复构建；日常使用的启停方式尚未设计/批准，不直接取消arm门控。

- 用户确认上一轮“没赶上，还没滑”，不记失败。另行确认准备后启动55秒窗口并arm，提示开始；本轮采到UP、DOWN、UP、UP，共4个手势，键值0x51/0x52，每次均有零释放、notify rc0，连接/加密/订阅维持ready1、errors0。用户明确反馈“上滑下一条，下滑上一条，都有效”，据此记录这台平板原生抖音方向键核心路径通过；不推定4/4成功率、其他应用或长期稳定性。不再测试Page，不改固件。当前仍需串口arm且60秒到期关闭输入，下一步先确认保留HID或恢复touch；不能自动扩大为持久arm/自动发键方案。

- 用户亲自配对后反馈“已找到并连接，显示键盘设备”；serial-hid-pair-20261026-043801.log确认generation3、connected/subscribed/encrypted/ready均1、armed0。用户确认鸿蒙原生抖音视频页已准备好。
- 上一方向键窗口serial-hid-arrow-20261026-043926.log结束并执行disarm、零释放rc0，未见方向手势，不视为兼容性失败。用户说“继续”后重开45秒采样并显式arm，已提示硬件上滑一次、停2秒、下滑一次；等待本轮采样与明确效果反馈。旧条目中的待编译/配对描述为历史过程，以本节最新证据为准。
- 本轮serial-hid-arrow-retry-20261026-093438.log已结束：连接/加密/订阅保持ready1，errors0、gestures0，未采到方向手势；finally已发送disarm并取得零释放通知rc0，串口关闭。尚无本轮用户效果反馈，不能判定方向键在抖音是否有效；后续先确认是否实际赶上窗口，不直接切换Page模式或改固件。

- 最终HID源码稳定后verify通过：304项C++检查+9项清单离线校验，完整程序631011字节(48%)/RAM18816；产物TEMP/thumbremote-hid-1ed8e5ab95cb-targets-v1，源码hash1ed8e5ab95cb7a42cdcdc0094d498a249985352a3beffac5f35d6570f00cace7。ELF nm确认__wrap_ble_gap_adv_start及原函数均存在，objdump确认调用指令jal跳至包装，清单target=hid/linkFlags匹配。
- 上传前脚本重新校验touch恢复产物、唯一CH343/COM7与ROM ESP32-C3身份，upload -Target hid -Port COM7退出0、四段写入哈希通过，无整片擦除。板内现为hid_probe，原App配套touch v3源码/产物保留但目前不在板内；不得继续说OPPO原App现在可按旧协议连接。恢复需显式touch目标上传（已授权按用户选择恢复）。
- 上传后6秒串口4次HID_STATUS：generation0、未连接/未订阅/未加密/ready0、armed0、arrow、touch idle、errors0、gestures0。证明默认不发键及本轮空闲运行；未截启动横幅，不推广为广播/配对通过。serial-hid-idle.log保存最终构建目录，端口释放后正在为用户平板配对采样。
- 已请用户在MatePad系统蓝牙亲自搜索/配对ThumbRemote-HID-Test，暂不滑动，不用Android App。尚未确认配对/CCCD/encrypted/ready，未发送arm，不得自动发导航键。后续以最新用户反馈及串口采样为准。
- 用户说“继续来吧”后助手先沿软件路线理解，用户纠正“不，按HID来”，明确恢复原完整HID设计/计划及临时烧录授权；已先更新AGENTS和方案状态，不能再按下文暂停记录停止，也不重复确认已有授权。没有推进软件界面改动，README未修改。
- 用户已反馈停止OPPO遥控、开发板回接电脑USB，当前只读枚举唯一CH343/COM7/OK。用户另确认测试目标是鸿蒙原生版抖音，MatePad mini/HarmonyOS7.0.0.109，不是卓易通版抖音。
- 新增firmware/hid_probe独立sketch、纯Gate/CommandReader/Delivery及NimBLE键盘适配；复用原touch_protocol/gesture_recognizer的哈希副本，不改原touch源码。scripts/esp32.ps1显式Target touch|hid、默认touch，目标/源码/选项/BIN清单校验；HID上传必须同时验证touch恢复产物。新scripts/test_esp32_manifest.ps1只加载实际校验函数并用TEMP夹具验证9种有效/错误清单与产物条件，不访问硬件，不清理文件。
- TDD：HID门控61检查先28失败后通过；串口行解析新增10检查先5失败再通过；发送单槽新增24检查先12失败再通过；取消中性释放组合新增13检查先1失败再通过。当前C++共304检查全过，另9项清单检查全过。未修改Android，不重复跑Android或刷旧硬件验证。
- 独立整体审查发现2Important（发送与GAP竞态、Input Report可读旧值），已改实际发送到NimBLE默认host队列，明确conn/generation/payload，读值恒零；复查发现取消待发零释放的组合问题，已用回归证明并改disarm始终重建零释放。无Critical。修复后只按必要范围验证，不扩大输入功能。
- touch恢复目标完整编译通过：源hash56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b、626361程序字节/RAM18648，产物TEMP/thumbremote-touch-56789245aac4-targets-v1。原产物目录保留不覆盖。默认仍DIO80MHz4MB。
- HID首编译因全局link与POSIX link同名冲突失败，已改sharedLink；修复中间快照编译到631011字节但源码变更检查拒绝生成最终清单，未上传。当前固定最终源码正在重新verify -Target hid（exec session 见当前工具返回，后续以完成输出为准），未烧录/配对/实际发键。原板内仍v3，Android App不需重装。
- 下一步：等待最终HID verify成功，核对ELF里__wrap_ble_gap_adv_start确实被调用、恢复清单有效、唯一COM7/C3后临时上传。只读串口确认armed=0；用户系统配对并准备好后才发串口arm，让真实触摸触发箭头组。箭头有效即结束，无效才disarm改Page组。不要把SDK notify尝试当视频切换；测试结束disarm释放串口，用户选择是否恢复touch。完整操作见docs/hid-probe.md。未删除/提交/推送/改系统配置或依赖。

## 最新接续：用户暂停HID，继续原软件路线（2026-10-25）

- 用户曾说“行，接着来吧”授权最小HID试验，已先更新AGENTS、设计状态并写计划docs/superpowers/plans/2026-10-25-hid-probe.md；只读检查了固定SDK HID/安全回调及脚本。发现SDK重复配对默认会删旧bond，曾在计划提出仅HID链接包装拒绝该路径，但没有实现任何代码或构建脚本修改。
- 用户随后明确“算了，先别开发这个了，我就接着用软件吧”，立即暂停HID，更新规则与文档状态。firmware仍只有原touch_probe及既有tests，无hid_probe产品目录，没有HID编译/上传，原v3及恢复产物不需重刷。保留设计/计划，不删除文件。
- 用户此前已反馈OPPO停止遥控、开发板由充电宝改接电脑USB；记录为用户反馈连接状态。本轮未打开串口或写入芯片，不宣称当前硬件心跳新实测通过。用户可继续在OPPO的ThumbRemote扫描选择、主动开始；已安装拒绝计数修复版APK（939899字节）保持。
- 下次按软件使用/实际问题继续，不自动恢复HID、购买键鼠、推进装配或重做已跳过的验收。MatePad mini/HarmonyOS7.0.0.109的卓易通辅助功能限制仍未解决，不能称这台平板已可遥控；原OPPO与USB充电宝核心反馈保持。用户明确恢复HID时才重新核对该计划及当时源码/端口。

## 最新接续：其他设备辅助功能被系统限制（2026-10-25）

- 用户没有蓝牙键盘/鼠标，不要求购买。已先更新AGENTS为HID仅设计范围，准备docs/superpowers/specs/2026-10-25-hid-probe-design.md：现有C3临时模拟键盘，触摸上下映射Down/Up Arrow，必要时显式改PageDown/PageUp对照；首轮不做鼠标或绝对触控。配对不自动发键，用户准备后串口arm开60秒、仍由真实触摸触发，每笔按下后释放，断线/到期/故障取消。当前工具链已只读核对BLEHIDDevice头文件明确支持NimBLE，但尚未编译或实测。
- HID测试会临时替换原Android配套BLE程序，原OPPO App在该期间不能沿用原协议；设计要求独立目标/产物校验及明确恢复入口。独立builder-check返回“nothing important left”，现等待用户审阅完整范围后确认实施/临时烧录与可选恢复。本轮只改规范、设计、日志，没有HID代码/编译/上传，不扩大现有授权。
- 用户补充设备为华为MatePad mini、HarmonyOS 7.0.0.109，App经卓易通运行。已取得具体型号/版本，不再重复索要。官方检索未找到这条精确构建的卓易通无障碍放开方法；当前结论仍以截图的权限阻止为准，不推论所有版本。
- 备选方向为BLE HID：华为官方说明手机/平板可直接配对蓝牙键鼠（https://consumer.huawei.com/cn/support/content/zh-cn16053504/）；Espressif官方BLE HID示例列ESP32-C3为支持目标（https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/ble_hid_device_demo/README.md），本机固定3.3.10-cn亦有BLEHIDDevice.h/.cpp。但未编译HID、未验证本机NimBLE具体适配或MatePad抖音键盘/滚轮响应，不能声称可直接翻视频。
- 下一步建议先确认用户有无现成蓝牙键盘/鼠标，在目标应用验证方向键/滚轮是否有效，再选择最小HID报告与设计；无该外设也不要求立即购买，可在完整方案确认后另作最小固件试验。当前没有HID或原生鸿蒙实施/烧录授权，保持原OPPO方案和固件，不擅自替换。
- 用户已确认新设备上的App通过卓易通运行。结合明确系统提示，当前环境不能给本App开启所需系统无障碍权限，现有Android辅助功能执行路径在该设备未可用；BLE接收ready不代表遥控可用。不能将此推广为所有鸿蒙/卓易通版本的永久限制，也不声称重装/改包名/重刷当前固件能解决。当前先保留原OPPO已通过方案；如果继续适配该设备，需补型号与HarmonyOS/卓易通版本，另查可行路线后确认设计，不直接实施原生鸿蒙或HID。
- 用户明确暂不固定/装配，转而希望在其他设备使用。已说明安装同一APK、原设备停止释放连接、新设备允许权限并主动开启；当前固件仅单客户端，不需为另一台Android重刷固件。
- 用户提供ea267204d68d9cb51f6989ada105dda8.jpg截图：BLE卡片显示“可以接收”，辅助功能授权未开启/服务未连接，系统提示“为保障您的隐私安全，应用暂不支持获取系统无障碍权限”。说明截图时BLE握手已到页面ready，但执行所需辅助功能被系统限制，不能记新设备遥控已通过。设备型号/系统版本/是否运行于卓易通等兼容环境未确认，不凭界面认定设备。
- 已查Google官方受限设置说明（https://support.google.com/android/answer/12623953?hl=zh-Han），普通Android13+部分侧载应用可能有“允许受限制的设置”入口；卓易通官方FAQ说明其应用权限另有管理路径（https://www.droitong.com/CommonQues.html），但未找到该精确提示的官方解释，不能直接断言可开启或完全不支持。下一步只补设备型号、系统版本和安装方式后按对应平台排查，不先改代码/重刷板/替用户改系统设置或代授权。

## 最新接续：充电宝USB独立供电获用户反馈通过（2026-10-25）

- 用户按充电宝USB替代电脑供电、重新连接开启遥控并试上下滑的步骤，明确反馈“可以正常控制”。据此记录充电宝USB独立供电下的核心遥控获用户反馈通过；无电脑串口采样，不推算运行时长或扩大为续航、自动关断、长期稳定性通过。无代码/固件/安装变更，屏幕四线保持原样；仍未实施锂电池、充电模块、显示驱动或外壳装配。
- 用户已明确有带USB输出的充电宝，先使用开发板现有USB接口替代电脑供电；不涉及锂电池/充电模块接线，不改屏幕四线/代码/固件。拟由用户先断开开发板与电脑USB，再用充电宝连接开发板USB，手机重新扫描选择并主动开始遥控。尚未反馈实际接好或独立供电运行，不能记为脱离电脑已通过；充电宝型号、持续供电与低电流自动关断表现未确认。
- 用户在计数修复后继续要求“继续”。助手建议先整理脱离电脑的USB供电与装配路径，未将此简短请求扩大为电池接线、显示驱动或新硬件实施授权。
- 已核对hardware-evidence：现有记录只有开发板USB供电、面包板/跳线/面包板电源模块；没有电池、充电模块或外壳的已确认型号。优先确认用户手边是否有USB输出充电宝，或只有电池模块/尚未准备独立电源，避免重复索要已知开发板/触摸型号，也不盲选购物清单或猜锂电接线。
- 已发出电源现状问题，等待用户回答后确定具体方案。本轮只读资料及更新此接续，无产品代码/构建/安装、接线或通电动作，不重做旧验收。当前App/固件和已通过的核心效果保持上一节结论。

## 最新接续：执行器拒绝计数已修复并更新（2026-10-25）

- 用户在核心双向通过后再次说“接着”，本轮收尾已知Minor计数缺口。提前说明更新会结束当前遥控，之后需用户重新开启。没有扩大手机动作、权限或硬件范围。
- 根因：服务在executeRemote返回false时调用普通finish，只更新逐条结果，不增skipped。新增明确reject接口：仅当前代数/请求成功结束后skipped加1；服务拒绝分支改用该接口，正常系统结果保持finish。不按结果文字猜测拒绝类别。
- 新增2项回归：拒绝只计一次、迟到完成不覆盖、冷却/重复不补发；停止/旧代数拒绝不影响新请求。先以无计数实现运行，27项中1项按预期失败（摘要期望1实际0），实现后scripts/android.ps1 verify退出0，27项0失败/错误、Lint0错误/6提示、APK通过。镜像TEMP/thumbremote-b28c3ca94042。复查该小修复无发现，没有重复全部功能审查。
- 核对唯一授权PLB110/API36后scripts/android.ps1 install返回Success。最终APK939899字节、SHA256 4282E173D1ADE938A9163E2AFAB9066EB2D21450C6FA153AC60D73449285BE7D，位于android/app/build/outputs/apk/debug/app-debug.apk。未改固件/连线、未刷板、未代授权或开启遥控。未删除/提交/推送、改系统配置或依赖。
- 更新后打开MainActivity返回Status ok、LaunchState WARM；进程存活，自身AndroidRuntime错误过滤无输出。不是冷启动、真实拒绝场景或视频切换验收。
- 下次起点：本版已更新，用户回App重新选择开发板并主动开始即可；旧版双向切换反馈保留，修复版真实跨应用效果及异常测试未重新执行，不据构建或安装推广为真机全部通过。前述Minor已修复，不再作为未解决事项；其他阶段仍按原授权范围。

## 最新接续：BLE遥控抖音双向切换获用户反馈通过（2026-10-25）

- 用户要求“接着”后，只核对唯一授权PLB110连接与自身进程错误日志，未自动开启遥控或授予系统权限。用户亲自操作并明确反馈页面“遥控已开启，可切换到目标应用”。
- 按“切到抖音、等3秒、硬件上滑1次、停2秒、下滑1次”发起最小实际效果确认，用户明确回答“上滑切到下一条，下滑回到上一条”。据此记录本轮双向实际效果用户反馈通过，不推广成BLE遥控上下各10次、长期成功率或系统回调结果已独立核验。
- 同期45秒串口日志TEMP/thumbremote-touch-56789245aac4/serial-remote-first-20261025-182658.log：窗口初始2条UP因无ready会话BLE_DROP；随后generation1连接/2订阅/3ready、READY尝试成功，3条UP分别seq1–3/code1通知尝试。窗口未截到DOWN，不能把这3条UP逐条对应用户要求的那一对动作；实际下滑效果依据用户明确反馈，不虚构DOWN串口证据。窗口出现固件启动横幅，启动原因未查，不归因于某项故障。
- 最初服务快照仅绑定、startRequested=false，后续用户完成操作后的同包服务快照确认RemoteControlService isForeground=true、foregroundId=30、types=0x10、startRequested=true，通知channel=remote_control、actions=1。证明当前connectedDevice前台服务运行；不代替用户已看到/点击停止按钮的测试。自身进程AndroidRuntime检查无本次错误输出。
- 已同步acceptance与ble-control使用说明，本轮只保存记录，无代码/构建/安装/烧录变更，采样已结束并释放COM7。没有主动停止用户正在使用的遥控会话；需要结束时用户可点App或通知停止。
- 下次从这里继续：核心硬件→BLE→Android执行→抖音双向效果已获反馈，不重复安装/刷板/入门测试。优先响应用户实际使用问题。此前停止/锁屏/断线不补发及通知/权限异常真机测试仍保留未验证，不自动重新要求整套验收；左右仍只记录，电池/显示/其他动作未授权。摘要计数Minor仍见下一节，未修复。

## 最新接续：BLE上下滑遥控已实现并安装（2026-10-25）

- 用户审阅完整设计后回复“实施”，再要求“继续”；先更新AGENTS授权与文件职责，写实施计划docs/superpowers/plans/2026-10-25-ble-control.md后按本会话执行。无需再次请求安装或实施授权。方案见docs/superpowers/specs/2026-10-25-ble-control-design.md。
- 新增RemoteControlSession纯门控、RemoteControlStatus内存快照/启动票号、RemoteControlService connectedDevice前台服务；服务持有独立BleClient，新sessionId重新握手，页面仅绑定状态。BleClient回调新接受帧及主线程入队前的单调接收时间，不回放历史。BLE包仍不引用辅助功能。
- 辅助功能新增即时执行入口，原5秒手动路径保持，复用GestureSession单任务互斥/几何/系统派发。只UP/DOWN执行，左右记录；500ms完成冷却与500ms接收时效、忙/旧代数丢弃不排队。停止先失效代数，清理Gatt和服务；屏幕/蓝牙/权限/辅助功能异常结束，无自动恢复或重连。
- Manifest新增已批准FGS/CONNECTED_DEVICE/POST_NOTIFICATIONS；常驻通知有停止按钮。通知被拒绝/通道关闭不启动，系统授权用户亲自处理，授权回调不自动开始。原接收测试模式仍页面onStop断开自己的Gatt。App说明明确控制当前应用，并非只控制抖音。
- 测试先红再绿：纯门控5项+即时1项先6失败、实现后23JUnit全绿；启动条件/重复票号2项先2失败、实现后25JUnit全绿。中途缺少尚未创建的Service类型导致一次编译失败，补齐类型后才观察到2项真实行为失败；未将编译失败冒作红绿证据。
- 最终 `scripts/android.ps1 verify` 退出0，25项JUnit0失败/错误，Lint0错误/6提示，Debug APK生成；镜像TEMP/thumbremote-0d4547104781。新增一个UseRequiresApi同类提示，其他固定SDK/Gradle/API31兼容弃用提示保留，不加忽略标记，不安装依赖。固件未改，未重复编译/烧录或接线。
- 独立整体只读审查无Critical/Important。暂留Minor1：新会话中执行器若因旧会话冷却拒绝，逐条结果写明未执行，但摘要“未执行”计数只统计门控丢弃，未增加该项。该显示计数不影响禁止执行/无补发，后续可再补明确拒绝结果和回归；未为Minor追加改动。详见docs/ble-control.md。
- 核对唯一授权PLB110/API36后 `scripts/android.ps1 install` 返回Success；APK939815字节，SHA256 0CA62FCFFA1CD7FB5016148E98A45119D8D8343DEE46205D1273E714853FB4C1。随后打开MainActivity返回Status ok、LaunchState WARM，进程存活，自身进程AndroidRuntime过滤无错误；不是冷启动/通知/遥控验收。首次打开检查命令因PowerShell参数表达式解析失败，修正参数后上述命令成功，无手机副作用。
- 未自动开启遥控、代授通知/辅助功能或发出手机手势。本版真实跨应用上下滑、通知展示/停止、锁屏等Android平台能力尚未获用户反馈，保留未验证；不自动恢复用户已跳过的整套验证。没有提交/推送/删除、改凭据/系统配置或全局安装。
- 下次起点：本版已安装，无需重复构建/刷板。用户可在接收区扫描选择开发板，亲自确认辅助功能/通知，主动点“开始遥控”；新握手显示“遥控已开启”后自行切换抖音并上下滑。先记录实际效果及具体错误，不把系统完成当视频切换；如用户不想实测，直接保持本版未验证状态。操作说明docs/ble-control.md。

## 最新接续：准备BLE上下滑遥控方案（2026-10-25）

- 用户在停止后续验收后说“接着下一步”，转为准备手机控制方案，不重新要求补做旧测试。先更新AGENTS记录仅设计，再保存 docs/superpowers/specs/2026-10-25-ble-control-design.md。尚未确认完整方案，不修改产品代码/Manifest、不安装新版本、不自动启动遥控。
- 建议范围：现有固件/四线保持，只将UP/DOWN接现有辅助功能即时滑动；LEFT/RIGHT仅记录。用户前台点开始，带常驻通知的connectedDevice服务持有独立Gatt；离开页面可继续，停止/锁屏/断线/权限或辅助功能丢失立即结束，不自动恢复、不排队。新增前台服务和通知权限仍待用户确认。
- 已核对原onStop断开与手动5秒路径，Android官方后台BLE、FGS类型和通知权限说明；FGS并非系统强制要求先授通知权限，本方案为保持停止入口可见而选择通知不可见时不启动。文档内链接可核查。独立builder-check已返回“nothing important left”，现交用户确认完整范围，确认后再写实施计划。
- 本轮仅规则/设计/日志变更，未构建或实机测试；不将上一轮暂跳的异常测试标为通过，不扩大为电池、显示、设备认证或新硬件。

## 最新接续：BLE前台接收核心链路通过，用户停止后续验证（2026-10-25）

- 用户在收到断线不补发测试步骤后明确说“不用验证了”，停止后续测试，不将尚未回答的断线测试记为通过。未完成项：断线期间动作/重连不补发与新动作、离开页面/锁屏/蓝牙关闭/权限拒绝或撤销、关闭辅助功能独立接收、原手动功能本版回归、长期稳定性。后续先以本节和acceptance为准，不重复工具链/安装/四方向；是否继续验收或提出真正BLE遥控方案由用户决定，本轮不新增功能或扩展范围。
- 上滑补7条seq34–40，连同首轮3条达到上10/10；下/左/右此前各10/10，合计40条两端方向与计数对应。3次轻点均too_short(dx=dy=0)，3次斜滑均ambiguous((-164,-107)/(-167,-91)/(-163,-115))，过滤期间无通知。用户逐项确认总数40、过滤期间无新增。日志TEMP/thumbremote-touch-56789245aac4/serial-ble-up-filter-20261025-112520.log；本轮40条不是长期准确率保证。
- 左右各10次：RIGHT count33–42/seq14–23/code4，LEFT count43–52/seq24–33/code3，通信errors=0。用户逐项确认总数33、左右各10条正确。日志TEMP/thumbremote-touch-56789245aac4/serial-ble-right-left-20261025-112407.log；当前上3/3、下/左/右各10/10，上滑补7次与过滤待测。
- 紧接下滑10次：串口DOWN count23–32，seq4–13、code2连续，无通信错误。用户逐项确认“10条下，总数13”，本轮下滑10/10两端对应。日志TEMP/thumbremote-touch-56789245aac4/serial-ble-down-20261025-112314.log。当前已确认上3/3、下10/10，其余未测。
- 重新连接复测：串口记录generation6连接、7订阅、8会话ready，READY_ATTEMPT attempted=1；随后UP count20–22分别发送seq1/2/3、code1。用户明确确认手机新增“上”#1/#2/#3，故本轮上滑3/3获两端对应证据。日志TEMP/thumbremote-touch-56789245aac4/serial-ble-reconnect-20261025-112206.log；此前无会话窗口不能据此改为通过，首次断开原因尚未确定。其余方向、各10次及不补发仍待测。
- 首轮BLE上下55秒联合窗口未通过：串口仅取得17条UP（count3–19），每条紧随BLE_DROP，另1次too_short，通信errors=0；未见DOWN或BLE_NOTIFY_ATTEMPT。用户此前反馈“可以接收”与板端无就绪会话冲突，扫描/握手暂不能记为已确认，正核对用户所看卡片与前台状态，不据此猜测或改代码。日志TEMP/thumbremote-touch-56789245aac4/serial-ble-up-down.log；窗口外count1–2不计样本。
- 用户看过完整设计/计划后明确说“实现”，又要求“继续”；根AGENTS已先更新范围、Android ble/测试目录与权限例外。允许验证后烧录C3、安装唯一已授权OPPO；系统权限仍由用户本人确认。没有触发BLE手机手势、后台常驻、自动重连或历史补发。
- 新增两端协议、C++SendGate、NimBLE传输、Kotlin纯接收状态机、平台Gatt及前台卡片；协议10字节/控制5字节、双条件READY、sessionId/sequence/generation过滤，离开页面断开。手机BLE包无辅助功能执行依赖；Manifest仅新增SCAN(neverForLocation)/CONNECT及可选BLE能力。
- TDD：C++新增29检查先红15项再绿，原128手势+39协议保持，合计196检查。Android首次14JUnit红6项再绿，之后审查权限异常回归3项先红3项再绿，最终17JUnit全部通过。曾遗漏end默认参数导致一次Kotlin编译失败，已恢复默认值并全套通过；不掩盖失败。
- 首版BLE编译因错误使用Bluedroid回调失败；实际sdkconfig为CONFIG_NIMBLE_ENABLED。已依据本地固定库改为ble_gap_conn_desc/onSubscribe，自动CCCD，不切换后端、不装依赖。最终 `scripts/esp32.ps1 verify` 退出0，程序626361字节、RAM18648字节；TEMP/thumbremote-touch-56789245aac4，源码SHA256 56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b。
- 独立审查发现2项Important：Gatt.close与扫描结果地址读取在撤权竞态下可抛SecurityException；新增可测BlePlatformBoundary并保护两处，保证旧会话先失效、disconnect失败也尝试close、UI收尾仍执行。3项回归红→绿。审查Minor：跨握手接触组合自动化覆盖尚缺（模块测试分别有）；UI序号已加入便于联测。无Critical。
- 最终 `scripts/android.ps1 verify` 退出0：17tests、0失败/错误；Lint0 errors/5 warnings；构建镜像TEMP/thumbremote-1b0bc2c17a5b。APK为android/app/build/outputs/apk/debug/app-debug.apk，923403字节，SHA256 AB3E77B2990E0013175ED6E3E831D6BD9F787B203D94EAF0F08C93B19DC68786。5项Lint及SDK弃用兼容提示解释见docs/ble-receiver.md，未添加跳过/抑制标记。
- 核对唯一CH343/COM7、ESP32-C3后执行 `scripts/esp32.ps1 upload -Port COM7`，退出0、四段哈希通过，未整片擦除。板内现为v3 BLE版，不能再称仅v2串口版。随后8秒串口取得4次STATUS：触摸ACK、idle、errors=0；启动横幅未截到，无本次检查的panic/brownout/分区错误。serial-ble-startup.log在最终固件目录；此证据不代表广播/手机通信通过。端口已释放。
- 用户反馈手机已插线，随后反馈有USB用途通知、将检查并允许调试；最新ADB仍为空。已说明必须开启USB调试并允许本机，而不是仅选文件传输。未操作手机系统权限、未安装驱动/改系统设置、不推断设备坏或线一定有问题。
- 最新：用户确认USB调试后，ADB已识别唯一授权PLB110/API36；`scripts/android.ps1 install` 返回Success，冷启动Status ok，自身进程AndroidRuntime检查未见错误。用户亲自扫描连接后反馈“可以接收”，握手按用户反馈通过。第一步为方向消息联合采样，不重复安装/烧录或申请既有授权。此前USB未枚举是历史诊断状态。
- 本节前部联测记录优先：安装、重连握手、四方向各10次、轻点/斜滑各3次已完成；其余真机项目按用户要求停止并保留未验证，不得将本轮核心成功推广为全部异常场景通过。
- 操作说明与交付：docs/ble-receiver.md；设计及计划已保存。未提交/推送、未删文件、未改全局依赖或系统配置。

## 最新接续：BLE方案待确认（2026-10-25）

- 用户问“下一步怎么做”，助手提出先打通ESP32→手机BLE消息，之后另行接辅助功能控制；明确本次先做大改动方案，确认后写代码。用户“开始吧”按准备方案理解，不视为完整方案已批准。
- 已保存设计 docs/superpowers/specs/2026-10-25-ble-receiver-design.md 与计划 docs/superpowers/plans/2026-10-25-ble-receiver.md；根AGENTS新增“仅设计、待确认”记录，未开放实施权限。
- 方案：自定义GATT，现有App前台扫描/用户选择/订阅/会话启动/READY后显示四方向；本轮不调用辅助功能。10字节通知含版本/sessionId/sequence/code，5字节启动写入，断线无队列无补发；App页面onStop即断开、不自动重连，旧Gatt/generation/session过滤。
- 实现选择已告知用户：沿用平台Kotlin与现有esp32内置BLE库；BLE功能仅API31+，现有Android16/OPPO验收，App minSdk24及原手动功能保持；仅附近设备扫描/连接权限，由用户亲自允许，不申请定位/网络/后台权限。此项仍是待确认方案，不是Manifest已修改。
- 已只读核对本地固定BLE库的Server/Characteristic/Descriptor/CCCD API，Android官方权限、限时扫描和CCCD流程；来源链接在设计文档。最初猜测BLE_notify示例路径不存在，改为直接查真实头文件/源码；不以缺示例推断库不可用，也不声称BLE已编译。
- 独立builder-check审阅返回“nothing important left”，认为握手乱序、跨会话接触、前台生命周期、旧回调与验收标准完整。设计/计划路径、UUID格式、10字节fixture已做文档一致性检查；未运行构建/测试，因为本轮没有产品代码变更，未连接或写入设备。
- 待用户确认的具体范围：按这两份文档实现前台收消息版，在电脑验证/审查后可烧录C3及安装Debug APK到唯一已授权OPPO，系统权限仍用户操作。确认后先更新根AGENTS的阶段、目录与权限例外，再创建代码；不重复询问已涵盖的实现/安装/烧录授权。
- 下次第一步：先检查用户是否已明确确认该完整方案；尚未确认则继续只讨论文档。板内仍为v2、手机仍为原手动版，无需重测触摸。真正遥控手机、后台常驻、生产部署和绑定/认证属于后续范围。

## 最新接续：软件手势识别 v2（2026-10-25，优先于下文历史记录）

- 用户已确认“四方向松手判定、一次接触一个方向、短距离和不明确过滤、通信异常取消”的方案，授权修改并重新烧录；已更新根规则和 docs/superpowers/plans/2026-10-24-gesture-recognition.md（讨论开始日期24，执行跨至25）。不重复问相同范围授权。
- 新增 firmware/touch_probe/gesture_recognizer.h，与协议分离；新增 firmware/tests/gesture_recognizer_test.cpp；scripts/esp32.ps1 test/verify 改为运行全部 *_test.cpp。不改接线、显示、BLE 或 Android。
- 实现选择：主轴净位移>=40，且主轴>=2倍次轴；X减/增=上/下，Y减/增=右/左，沿用前轮实测握持朝向（非自动旋转）。先等确认松手；01保留已知接触，无起点则等下次；故障、多点、换点号取消并等完全松手。正常非坐标芯片手势包忽略，不取消整段正常滑动。
- TDD：手势128检查先红40项，再绿0失败；原协议39检查全绿，合计167；12组已有硬件起止坐标回放验证映射。电脑端回放不等于新固件实测成功。
- 最终 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify` 退出0，两程序合计167检查0失败；完整编译程序308584字节、RAM14260字节。产物 TEMP/thumbremote-touch-ac895c144276，源码哈希 ac895c1442765c6c87d5c984ed66e4f91053f9171ada5dbd6e132f484e952811；compile.log 与 manifest.json 同目录。
- 独立只读审查未发现 Critical/Important；指出一项非阻塞 Minor：现有手势测试覆盖mask3多点与mask2单点，可后续补mask4/5/6/7。对应分支审查正确，本次不追加同构覆盖；不把未覆盖的组合写成已测。
- 核对唯一CH343/COM7与ESP32-C3身份后，`powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 upload -Port COM7` 退出0，四段哈希验证通过。板内现为v2；未整片擦除、不改接线。本次上传输出未出现之前的XMC startup警告，不据此判断根因已解决。
- COM7/115200、DTR/RTS=false，空闲采样6秒取得3次 STATUS：device=ack_seen、link=ok、touch=idle、errors=0、gestures=0、rejected=0；证明新字段固件运行及当前空闲通信，不证明方向识别实测。日志 serial-v2-idle-20261025-000331.log（最终构建目录），串口已关闭释放。
- 首次上下方向35秒窗口无动作、无坐标包；用户明确反馈“还没开始或错过了采样时间”，仅记空闲窗口，不记识别失败。日志 serial-v2-up-down-20261025-001042.log。
- 重采上下：记录UP×3、DOWN×3，顺序正确、每条均紧跟raw UP输出；窗口内方向事件6条、拒绝0、ERROR0。首笔在打开串口前已开始，窗口只有MOVE/UP，原始DOWN不在本记录内；另5笔有完整DOWN/MOVE/UP。不把全部6笔写成完整窗口接触。方向累计count2–7，count1未被采样，不计额外通过次数。日志 serial-v2-up-down-retry-20261025-001123.log。
- 左右：记录RIGHT×3、LEFT×3，顺序正确、每条紧跟raw UP；拒绝0、ERROR0。首笔同样起点在窗口前、其余5笔完整；累计count9–14，count8未被采样，不计额外通过次数。日志 serial-v2-right-left-20261025-001158.log。所有方向位移都符合当前40单位/2倍轴比规则；原始映射仅针对本轮握持朝向。
- 首轮过滤窗口只记录6次完整轻点、位移均dx=0/dy=0，全部too_short拒绝、方向输出0、ERROR0。不能据用户“ok了”推定斜滑也通过，已明确区分并另请其补测。日志 serial-v2-filter-20261025-001239.log。
- 用户同意单独斜滑补测，按提示慢滑3次，均完整DOWN/MOVE/UP，净位移分别(-163,-113)、(-149,-108)、(-147,-128)，全部ambiguous拒绝、方向输出0、通信ERROR0。停止操作后连续空闲心跳，累计通信errors=0。累计rejected从24到27，窗口外其他操作未记录，不将累计数当作本轮验收样本数。完整35秒日志 serial-v2-diagonal-20261025-001555.log 已保存于最终构建目录，串口已关闭释放。
- 当前阶段结论：软件方向事件与短触/斜向过滤已取得本轮实测证据；并非长期准确率保证。故障取消、多点/换点号恢复仅主机测试，未要求用户带电拔线。串口采样结束后均释放，不需重复烧录/接线。
- 下次起点：先读本节和验收表；软件手势v2已在板内，不再重复工具链、上传或四方向测试。后续若进入BLE需先提出并确认具体传输/Android集成方案，当前不自动扩展该范围。固定屏幕机械朝向、全范围/多点、故障实物恢复与长期稳定性仍待验证。

## 最新接续：最小触摸测试（2026-10-24，优先于下方历史起点）

- 用户确认排针已焊好，提供屏幕插头及 ESP32 引脚近照；线色与针脚依据见 hardware-evidence.md 最新节。不再要求重复整套照片。SDA 绿→IO04、SCL 黄→IO05、红→3.3V、黑→GND；随后按断电接线/再插 USB 的指令反馈“接好了”，并已取得下述真实通信证据。
- 用户明确同意已说明的“编写并烧录触摸串口测试固件、替换 GetChipID”方案；规则与计划已先更新，再创建 firmware/touch_probe/、firmware/tests/ 与 scripts/esp32.ps1。此授权不含显示、BLE、电池，不再重复请求相同方案授权。
- 功能：显式 Wire GPIO4/5，100 kHz、50 ms 超时、20 ms 轮询；只探测 0x46，先查询 0x80 再取 0xE0 14 字节，解析三点 12 位坐标；01 不松手，NAK/短读/未知报告不松手；每 2 秒心跳。没有触摸配置写入、主动复位或升级。
- TDD：35 检查先红 19 项，再全部通过；独立只读审查未发现 Critical/Important，指出“首次01却显示idle”和平台 boot_app0 实际上传路径未单独绑定。已补回归测试（39 检查先红 3 项）并修正状态描述；额外绑定平台 boot_app0 文件哈希。不遗漏审查发现。
- 首次完整编译退出 0：程序 307450 字节、动态内存 14236 字节，TEMP/thumbremote-touch-dba06f91bd2f；这是状态提示修正前产物，不能用于最终上传。最终 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify` 退出 0，39 checks / 0 failures；程序 307502 字节、动态内存 14236 字节。最终产物 TEMP/thumbremote-touch-6f5f78cacbcd，源码 SHA-256 6f5f78cacbcdbaba30f5195736704cf9dbeafef0cb4f0a85f6aae6fc46e5b02a，compile.log 与 manifest.json 同目录。无跳过构建标记。
- Android 回归：`powershell -ExecutionPolicy Bypass -File scripts/android.ps1 test` 退出 0，BUILD SUCCESSFUL，24 tasks up-to-date；未改 Android 源码，本次复用了既有测试结果。
- 初始 .NET 串口枚举只有 COM3–COM6；随后用户明确反馈“开发板 USB 已接好，屏幕线已断开”。核对唯一 CH343（VID_1A86/PID_55D3）位于 COM7、ROM 身份 ESP32-C3 后，执行 `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 upload -Port COM7`，退出 0，四段写入哈希均验证通过；GetChipID 已被 touch_probe 替换。未整片擦除。上传日志见最终构建目录 upload.log。
- 上传时 esptool 提示 XMC flash chip boot-up failure 并自动执行 XMC25QHxxC startup flow，随后报告 fix successful；保留原始提示，不推断板卡故障根因或称上传全程无警告。之后 115200/8N1、DTR/RTS=false 采样 8 秒，4 次 STATUS，bus=ready、device=not_found、address_0x46 code=2；与用户明确断开屏幕的状态一致。检查的 invalid magic/panic/brownout/Guru Meditation/分区错误计数 0；没有启动横幅（打开串口前已运行），不能推广为长期稳定。serial-board-only.log 同目录；端口已关闭释放。
- 额外入口检查：upload 未传 Port 时按预期拒绝，未访问设备。Android XML 7 tests / 0 failures / 0 errors / 0 skipped（本次 Gradle UP-TO-DATE）。
- 接屏幕后（用户“接好了”），COM7/115200 先采样 8 秒：取得 touch_probe 启动横幅，GPIO4/5 内部弱上拉下空闲电平 1/1、地址 0x46 ACK；3 次 STATUS 均 link=ok、touch=idle、query=0x00、errors=0。日志 serial-touch-idle-20261024-235244.log。
- 随后请用户按住约 1 秒→缓慢滑动→松开，在 30 秒记录中取得 6 次 DOWN、36 次 MOVE、5 次 UP。按时序核对为 5 组完整接触、0 次异常 DOWN/UP 转移，第 6 次 DOWN 位于采样结束处，不能据缺少 UP 推断卡住或补算为完整一组。坐标样例从 (140,230) 经 (126,197)、(122,166) 等变化，UP 来源为有效 point_packet。ERROR 行数 0，累计 errors=0；日志 serial-touch-actions-20261024-235327.log。
- 已让用户停止/移开手指，追加 6 秒采样，3 次 STATUS 均 touch=idle、query=0x00、errors=0；第 6 次松开的具体 UP 不在记录窗口内，不虚构补齐。日志 serial-touch-release-20261024-235341.log。串口均已关闭释放。
- 程序 unknown 计数动作窗口末为 6、追加末为 7，表示未处理的非坐标报告，不是 I²C 错误。首个实际记录为 `80 22 ...`，按本地指南第 49/54 页是手势格式/0x22 Flick；仅该首包有完整日志，不将其余包统一判为 Flick。首版有意只输出坐标、不处理芯片手势；未知包不触发 UP。
- 上述三份日志均在 TEMP/thumbremote-touch-6f5f78cacbcd。之后开始方向核对：以用户当时正常握持朝向，要求从下往上滑 3 次。20 秒实测 3 组完整接触，分别 (208,132)→(60,136)、(166,119)→(70,131)、(172,126)→(53,125)，即 ΔX=-148/-96/-119、ΔY=+4/+12/-1；支持“本轮朝向下，上滑主要为原始 X 减小”。固定机械朝向尚未登记，不能把该映射推广到旋转后的任意握法。日志 serial-direction-up-20261024-235519.log。
- 紧接要求朝向不变、从左往右滑 3 次，但该 20 秒窗口仅有空闲心跳，packets 始终 95、unknown=11、errors=0，无触摸事件。日志 serial-direction-right-20261024-235546.log；不记为右滑失败或通过。随后改为先询问是否准备好，再开采样，避免操作错过窗口。
- 用户明确准备好后重采右滑 25 秒，取得 3 组完整接触：(128,173)→(143,36)、(158,226)→(152,35)、(123,214)→(148,36)，ΔY=-137/-191/-178，ΔX=+15/-6/+25。本轮朝向右滑主要为原始 Y 减小；日志 serial-direction-right-retry-20261024-235651.log。
- 用户再确认准备好，按“先下滑3次、停3秒、再左滑3次”采样35秒，6组接触完整。下滑 ΔX=+95/+122/+126、ΔY=-9/+11/+7；左滑 ΔY=+140/+143/+153、ΔX=+16/+26/+44。日志 serial-direction-down-left-20261024-235751.log。最后连续空闲心跳，累计 errors=0，unknown=20。非坐标报告计数不作通信错误；未逐包记录，不能推定每个非坐标包的手势类型。所有串口已关闭释放。
- 四方向各 3 组有完整 DOWN/MOVE/UP，与指定的动作顺序对应，支持当前握持朝向的映射：上 X减、下 X增、右 Y减、左 Y增。该证据不是程序已识别手势，固件当前仍只输出原始触摸。未拍照固定屏幕机械方向，旋转屏幕后不得直接复用映射。
- 当前下一步：提出软件手势识别最小方案（坐标方向适配、滑动距离/轴向判定、一次接触只出一次事件、通信失败取消本次判定），确认后再实现；用户本轮“来”和准备好仅授权方向实测，不自动扩为新手势固件或 BLE。无需重复上传/接线。多点、边缘范围、断连恢复及长期稳定性未验收。
- 上拉仍为待验证；内部弱上拉不是外部上拉合格证据。若触摸失败先依据诊断排查，不盲改电压或把没数据当松手。

## 当前进度

阶段 A 工具链已可用，阶段 B Android 手动手势测试 App 已创建并安装。用户已亲自开启辅助功能，并确认短视频内上滑、下滑均能切换，取消待执行操作也已测过且正常。

阶段 B 的完整验收尚未结束。接续锁屏测试后，用户明确反馈“解锁后不会补发”，不补发行为按用户反馈通过，任务最终结果提示待确认；用户按上下各 10 次测试要求反馈全部成功，记录上滑 10/10、下滑 10/10；服务断开测试按用户选择暂跳过，保留未验证。2026-10-24 获授权的 ESP32/IT7259 最小触摸测试已完成本轮通信、坐标与松手采样；显示、BLE 及端到端遥控尚未实现。

## 已完成工作及依据

| 项目 | 结果与依据 |
|---|---|
| 项目规则与结构 | 已建立 AGENTS.md、android/、docs/、scripts/；2026-10-24 授权后新增 firmware/touch_probe/ 与 firmware/tests/ |
| Android 环境 | Android Studio 已安装；JDK 25.0.3、SDK Platform 37.0、Build Tools 36.0.0、Platform Tools 37.0.1 可用 |
| ESP32 环境 | Arduino IDE 2.3.10 / CLI 1.5.1；esp32:esp32 3.3.10-cn；2026-10-22 官方 GetChipID 示例在临时镜像中完整编译通过（退出码 0），详见 docs/esp32-toolchain.md |
| 开发板识别 | esptool 5.3.0 经 COM7 与 ROM 握手，实际芯片 ESP32-C3 QFN32 revision v0.4，40 MHz 晶振、4 MB Flash；后续串口示例上传状态见下一行 |
| 开发板串口运行 | 历史官方 GetChipID 示例通过；2026-10-24 已按新授权替换为 touch_probe，DIO/80 MHz 上传校验与串口心跳通过 |
| IT7259 最小触摸 | 接屏幕后 0x46 应答；动作窗口 5 组完整 DOWN/UP、36 条 MOVE，窗口末另一次 DOWN；追加确认空闲；本轮通信 errors=0，详见最新接续 |
| App 技术方案 | Kotlin + 原生 Views；包名 com.thumbremote.app；minSdk 24、targetSdk 36、compileSdk 37；AGP 9.2.0 / Gradle 9.4.1 |
| 用户界面 | 上滑、下滑、中心单击、取消按钮；授权与服务状态、结果和错误提示 |
| 手势执行 | 用户点击后倒计时 5 秒，手动切回目标应用，服务仅执行一次手势；防重复、冷却、取消和超时已实现 |
| 隐私边界 | App 不申请网络、蓝牙、截图、悬浮窗或读取窗口内容能力；辅助功能由用户亲自开启 |
| 构建与自动化 | 最新完整 verify 成功；7 个 JUnit 测试通过，0 失败/错误/跳过；Lint 0 errors / 2 warnings |
| 真机安装 | OPPO PLB110 / Android 16 / API 36；APK 安装 Success、冷启动 Status ok |
| 辅助功能连接 | 用户亲自授权后，只读检查确认启用且系统绑定 hasBound=true / received=true；这是此前检查结果，接续时需重新确认连接 |
| 崩溃检查 | 上次读取 App 自身进程保留的 AndroidRuntime 日志，未发现 FATAL EXCEPTION；不等于长期稳定性验证 |
| 独立代码审查 | 未发现 Critical/Important 缺陷；不替代真机生命周期测试 |

系统手势回调完成不等于目标应用已切换；上/下切换成功依据用户的实际反馈。中心单击不保证播放/暂停。

## 真机验收的准确状态

| 项目 | 当前结论 |
|---|---|
| 短视频内上滑切换 | 用户明确反馈通过 |
| 短视频内下滑切换 | 用户明确反馈通过 |
| 取消待执行操作 | 用户确认已测试，按其前述反馈记录通过 |
| 锁屏及解锁后不补发 | 接续测试后用户明确反馈“解锁后不会补发”；任务最终结果提示待确认 |
| 连续滑动稳定性、上下各 10 次成功率 | 用户反馈本轮上滑 10/10、下滑 10/10；不代表长期稳定性 |
| 关闭服务取消任务、重启不重放 | 用户选择暂跳过，保留未验证 |
| 单击播放/暂停 | 未验证 |
| 最终结果卡自动显露、状态栏裁剪修复 | 已构建安装，未完成独立视觉复核 |
| 目标短视频 App 名称与版本 | 抖音（用户确认）；版本未记录 |

纠正记录：助手曾把“都OK了”解释得过宽。用户随后说明“锁屏啥的没测”“取消测过了”。现已同步修正进度与验收表，此前撤回无依据的锁屏和连续滑动通过判断；本次新增测试反馈以表内最新记录为准，不重复要求已明确完成的测试。

## 关键文件与构建入口

- `docs/hardware-evidence.md`：从原始聊天恢复的硬件清单与来源；ESP32-C3 SuperMini 类开发板、1.28 英寸 240×240 圆屏、GC9A01、IT7259 等已由用户提供，不重复索要整套信息；电气参数仍按证据核实。
- `docs/esp32-toolchain.md`：工具链版本、EFS 限制、临时镜像及重复编译命令、DeepSeek 接手后的核对范围。

- `android/app/src/main/java/com/thumbremote/app/GestureSession.kt`：纯 Kotlin 会话状态机及坐标几何，处理重入、取消、冷却与迟到回调。
- `android/app/src/main/java/com/thumbremote/app/ThumbAccessibilityService.kt`：持有倒计时任务并调用 dispatchGesture；离开页面后可完成一次已主动发起的任务，服务/进程重启不恢复任务。
- `android/app/src/main/java/com/thumbremote/app/MainActivity.kt`：原生界面、状态和授权引导。
- `android/app/src/test/java/com/thumbremote/app/GestureSessionTest.kt`：7 项本地测试。
- `scripts/android.ps1`：本地构建、测试和安装入口，不修改永久环境变量。
- `docs/acceptance.md`：逐项验收状态；`docs/progress.md`：开发过程记录。
- `docs/superpowers/specs/2026-10-21-android-gestures-design.md` 与 `docs/superpowers/plans/2026-10-21-android-gestures.md`：现有设计与实施计划。

在工作区根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 test
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 verify
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 install
```

APK：`android/app/build/outputs/apk/debug/app-debug.apk`（最新文件 900775 字节）。测试 XML 位于 `android/app/build/test-results/testDebugUnitTest/`，Lint 报告位于 `android/app/build/reports/`。

Windows AGP 与 Gradle 测试 worker 无法使用当前中文路径。脚本按源码哈希复制到英文临时镜像，并使用英文临时缓存路径，再将 APK 和报告复制回工作区。源码仍在原工作区修改，不编辑临时镜像，不自动删除旧镜像或缓存。当前两项 Lint 提示为 OldTargetApi 和 DataExtractionRules，说明见验收表。

## 下次从哪里继续

2026-10-22 转接板照片接续：收到圆屏背面，确认丝印 TK0128F25K V1.2。按用户原图方向，右上插座旁从上到下为 GND/3V3/SCL/SDA，右下为 MOSI/DC/SCK/CS；功能与原理图对应。接下来查看配套四线连接线两端，并确认 ESP32 排针/可靠连接方式；SDA/SCL 上拉仍待核对。无需重复索要整套实物照片。未接线、通电或开展新固件。

2026-10-22 实物照片接续：收到开发板正反面，可读 ESP32C3、IO4/IO5、3.3V/GND 等丝印；实物是蓝色长条板，不套用此前 SuperMini 排针图。照片未见已焊排针，连接方式待确认。还缺触摸转接板正反面照片，下一步只补该部分，不重复要求开发板整套照片；未形成接线表或实施新固件。详情见 docs/hardware-evidence.md 实物照片补充。

2026-10-22 资料包已取得并静态检查：本地 ZIP 路径、哈希及详细依据见 docs/hardware-evidence.md“卖家 ZIP 静态核对结果”。已读取 IT7259 协议相关页、目视核对转接板原理图、检查 ESP32 例程，确认其配置目标为 S3 且存在混用寄存器/坐标只取低字节等限制。图纸确认 P2 编号 1/2/3/4 为 SDA/SCL/3.3V/GND，RST 与显示共用 RC，INT 引至 TP2；图中无 SDA/SCL 外部上拉，实物仍待核对。下一步取得两块板正反面照片核对版本、插座朝向和 GPIO，再确认最小触摸串口测试方案。尚未编写/编译/烧录新固件、接线或通电测试；原官方串口示例保持不变。仅更新规则和文档，未运行 Android 构建。

2026-10-22 最新接续：用户已提供客服百度网盘资料链接，分享页确认文件为 `TK0128F25K整理资料.zip`（约 8 MB），见 docs/hardware-evidence.md。普通下载触发账号登录，尚未取得压缩包；已关闭本次浏览器会话。下一步取得用户下载的 ZIP 或其本地路径并静态检查原理图、触摸源码；不重复索要客服链接，不将资料包内容视为已核实。此次未修改代码、接线或烧录，仅更新文档，未运行构建。

1. 先读根 AGENTS.md、本日志与 docs/acceptance.md，保留上述用户纠正，不重新创建 App 或重复安装工具链。
2. ESP32-C3 官方串口示例已上传并验证运行，不重复安装或测试。先读 docs/esp32-toolchain.md，继续使用现有临时镜像及 FlashMode=dio（80 MHz/4 MB）配置；原目录 EFS 解密经授权尝试后被 Windows 拒绝。下一步核实触摸转接板资料，不因串口通过自动授权触摸接线或 BLE。若返回 Android 真机验收，再确认手机 USB 调试及服务连接；授权由用户本人处理。
3. 锁屏后解锁不补发已有用户明确反馈，不必重复要求；任务最终结果提示仍待确认。助手不代替用户点击发起实际手势。
4. 上下各 10 次已获用户全部成功反馈，不重复测试；服务断开与重启按用户选择暂跳过，保留未验证。目标 App 已确认是抖音，版本未记录；单击可独立测试。
5. 上一阶段验收通过后，再确认进入阶段 C。硬件实施前核对实物和资料；本次暂停时不开展固件、BLE 或接线。

硬件仍待核实：ESP32 实际芯片型号与开发板排针/GPIO；屏幕转接板具体型号、连接器方向及引脚顺序；触摸供电和逻辑电平、上拉；IT7259 I²C 地址、协议/寄存器、复位和中断时序、连续坐标与释放事件能力；可用驱动的来源、许可证与 ESP32-C3/Arduino 兼容性。尚未确认任何可供照接的接线表。

## 本次收尾

接续更新（2026-10-21）：已恢复验收。只读执行 ADB devices 未检测到连接设备；随后用户反馈锁屏测试“解锁后不会补发”。已同步验收表与进度，仅记录明确反馈，不推断结果提示通过。下一项为服务断开与重启不重放。此次仅修改文档，未重新构建或操作手机。以下为此前暂停时的收尾记录。

本次只写日志、修正文档并复核已有报告，未修改 App 源码，未重新运行构建或真机测试，未删除文件、提交/推送 Git、改系统配置或连接硬件。不根据聊天间隔推算工作时长。用户表示稍后继续，当前不再推进测试或下一阶段。

最新验收更新（2026-10-21）：用户选择暂跳过服务关闭与重开测试，该项保留未验证。解释由 ThumbRemote 发起上下各 10 次后，用户反馈“都成功了，十次成功十次”，按本轮要求记录上滑 10/10、下滑 10/10。已同步验收表和进度，目标 App 名称及版本未记录；未修改源码或重新构建，未进入阶段 C。

- 2026-10-21 应用信息补充：用户确认本轮上下滑各 10/10 的目标应用为抖音；版本未记录。已同步验收表，仅修改文档。

- 2026-10-21 历史信息恢复：用户要求自行查看此前讨论。已只读查阅原始任务书所在本地聊天记录，恢复硬件清单并写入 docs/hardware-evidence.md，区分已提供的商品信息与尚未实物核验的电气参数。不再要求用户重复整理整套资料；未进入阶段 C、未接线或通电。

- 2026-10-21 图片资料补充：用户提供屏幕尺寸/24 针排线定义、LCD 参数表、转接板商品照片。已记录至 docs/hardware-evidence.md：排线 17～21 为触摸信号，下方连接器旁可读 SDA/SCL/3V3/GND；商品图不代表实物确认。LED 背光电压不作为触摸供电依据。仍缺转接板原理图/触摸例程及协议，未给出可通电接线方案，仅修改文档。

- 2026-10-21 商品说明补充：新图片明确标注触摸 IC 为 IT7259，卖家声明提供 ESP32 参考程序、转接板原理图和 PCB 源文件。已补充硬件证据；资料包尚未收到，程序是否含触摸驱动及是否支持 C3 待检查。下一步取得配套资料包，不重复索要芯片型号。

- 2026-10-21 客服等待期间调查：用户要求先开展工作。已检索并静态检查 MIT 许可的 FT-tele/IT7259_Touch_driver（提交 33a76ceb69a50ffbb0489db9a2e73a42e8b514ca、元数据 1.0.1），确认存在实际 Arduino/ESP32 接口和坐标解析，同时记录初始化恒返回成功等限制；Rust 实现只做协议交叉参考。公开编程指南 PDF 返回 403，未读正文；转接板原理图仍未核实。命令与常见位置未发现 Arduino/PlatformIO 工具链，未安装。细节和来源已写 docs/hardware-evidence.md；仅文档变更，未编译固件、接线、烧录或进入阶段 C 实施。

- 2026-10-21 USB 连接进展：用户确认开发板到手并连接。此前仅有 COM3～COM6 蓝牙串口；最新只读检查新增 USB 串行设备 COM7，Windows 状态 OK，硬件 ID VID_1A86/PID_55D3，设备自报 USB Single Serial。仅确认 USB 串口已被系统识别，尚未确认实际 ESP 芯片型号、串口通信或烧录能力；未打开串口、安装工具或烧录。

- 2026-10-21 Arduino IDE 安装：用户在明确告知安装步骤后说“你来吧”，授权本次 IDE 安装。已从 Arduino 官方 CDN 取得 2.3.10 MSI，SHA-256 为 aebbd1efeac5cfb02a6cad0d93af8221054fc983a5b3c5ce6da8a6bfb9425165，与 winget 官方发布包清单匹配，Arduino SA Authenticode 签名 Valid。MSI 退出码 0，安装于用户目录 AppData/Local/Programs/arduino-ide，EXE 版本 2.3.10.0；内置 CLI version 输出 1.5.1。安装日志位于系统临时目录 thumbremote-arduino-install.log，安装包亦只保留在临时目录。
- 首启验证：初次加载时并行 CLI 检查遇到串口工具下载文件占用，停止额外 CLI 检查、正常关闭空白 IDE 后带日志重开；最新启动日志确认内置 serial-discovery/serial-monitor 已加载，截图确认已进入 sketch_oct21a 代码编辑界面。尚未验证编译和上传，不将 IDE 已安装视为完整固件工具链就绪。CLI board list 列出 COM7；最新 Windows 设备名为 USB-Enhanced-SERIAL CH343 (COM7)，状态 OK，仍未确认板载 ESP 芯片型号。
- 当前界面有 mdns-discovery.exe 的 Windows 防火墙提示，未替用户点击安全提示。已请用户自行点“取消”，当前准备 USB 串口测试，无需开启网络发现。ESP32 平台支持包尚未安装，未创建固件目录、接线或烧录。下一步先由用户处理系统提示，再确认实物芯片并准备 ESP32 支持包；不重复安装 IDE。

- 2026-10-21 ESP32 支持包准备：用户“OK，接着来”后继续。官方稳定索引当时为 3.3.12，首次安装在 GitHub 工具包 HEAD 请求超时而失败，未安装成功；改用乐鑫官方安装文档所列极狐镜像，固定安装 3.3.10-cn。所有依赖与平台安装结束退出码 0，core list 显示 esp32:esp32 3.3.10-cn，board details 能解析 ESP32C3 Dev Module / esp32:esp32:esp32c3；保留官方校验，无跳过标记。仅为这次 CLI 进程复用既有代理，未改系统代理或永久环境变量。
- 芯片实测：官方 esptool 5.3.0 对 COM7 执行 --no-stub chip-id 与 flash-id，均退出码 0，确认 ESP32-C3 (QFN32) revision v0.4、40 MHz 晶振、4 MB Flash。没有向 Flash 写入/擦除，也没有上传 RAM stub；工具默认通过 DTR/RTS 复位进入 ROM，读取结束硬复位。结果已同步 docs/hardware-evidence.md。COM7 曾一次未枚举，随后恢复，原因未确认。
- 编译验证失败及根因证据：对随包官方 libraries/ESP32/examples/ChipID/GetChipID 使用 `arduino-cli compile --fqbn esp32:esp32:esp32c3 --build-path <TEMP>/thumbremote-esp32c3-toolchain-check`，失败退出码 1。verbose 日志定位到平台原有 COPY 命令复制 tools/partitions/default.csv，Windows 返回 The specified file could not be encrypted。Get-Item 证实 Arduino15、packages、ESP32 平台及 default.csv 均为 Encrypted；临时构建目录不是加密目录；AVR 平台文件亦有此属性。能够读取源文件正文，但 COPY 保留 EFS 属性时失败。加密属性来源尚未确认，不推断为 ESP32 包缺陷。
- 当前没有修改 EFS 属性或系统安全设置，没有添加忽略标记或跳过构建步骤。完整编译日志位于 TEMP/thumbremote-esp32c3-compile.log，构建产物仅在临时目录。拟请用户批准仅解除 `C:/Users/Mr.G/AppData/Local/Arduino15` 公开工具链目录的文件加密，然后按相同官方流程重新编译；此项属于安全属性变更，先问用户。未创建项目固件或烧录。

## 2026-10-22 接续

- 用户已明确允许解除 Arduino15 文件加密，并要求外出期间尽量自主推进。已在解析路径与授权范围完全一致后运行 cipher /d /h /b /s，Windows 在根目录返回 Access is denied，退出码 1、解密数量 0；未修改 ACL、证书或扩大权限。原 EFS 属性仍在。文件夹界面未完成修复，不能记为已解密。
- 对公开分区表进行内容副本实验：原文件与临时明文副本 SHA-256 相同，普通 COPY 成功。随后按先更新的项目规则创建 TEMP/thumbremote-esp32-toolchain-3.3.10-cn 镜像：平台与 C3 SDK 的 5757 个文件逐文件哈希一致且不带 Encrypted；其他官方工具通过 junction 读取原安装。通过当前进程 ARDUINO_DIRECTORIES_DATA 和 ARDUINO_BUILD_CACHE_PATH 使用镜像及临时缓存，未改永久环境变量或 IDE 持久设置。
- 官方 GetChipID 示例使用 esp32:esp32:esp32c3 在 TEMP/thumbremote-esp32c3-mirror-check 完整编译通过，退出码 0；程序 277068 字节、动态内存 13196 字节。verbose 日志确认分区表复制、编译、链接、生成分区与打包步骤均成功，无 recipe 覆盖或忽略标记。只生成电脑端文件，没有烧录或擦除 Flash。
- 用户提醒 DeepSeek 曾临时接手后要求恢复。已核对当前 Android 输入与上次最终 UI 构建快照双向逐文件一致，无新增/缺失/哈希变化。官方 ESP32 平台归档哈希匹配索引，关键构建文件与归档匹配。项目仍无已跟踪的 Git 基线，不将空 diff 或上述有限检查推广为全部系统状态已还原；不将 EFS 来源归咎于其他代理。没有执行覆盖或回滚。
- 已写 docs/esp32-toolchain.md 保存重复编译命令和限制。当前阶段边界不变：未创建项目固件、未烧录、未做触摸接线或 BLE；下一步先确定最小串口输出固件方案及上传范围。手机消息没有独立发送通道，需用户亲自处理的事项在当前聊天说明。

## 2026-10-22 官方串口示例上传

- 用户明确说“开始上传”，此前已告知会替换板内现有程序。先更新规则记录此次独立开发板测试授权，核对 COM7 身份及示例来源后上传；未创建项目 firmware/ 目录、未进行触摸接线。
- 默认 QIO/80 MHz 配置首次上传退出码 0、各段哈希校验通过，但运行报分区表 invalid magic 0xffed、反复重启，不能记为运行通过。随后读回 0x8000 分区表，与预期 3072 字节逐字节一致；读取安全状态确认 Secure Boot/Flash Encryption 均 Disabled，未改安全配置。
- 按官方模式排障建议，仅将 FlashMode 改为 dio，保留 Flash 80 MHz、4 MB 与原分区，重新编译并上传。最终 FQBN esp32:esp32:esp32c3:FlashMode=dio，编译/上传退出码均 0；程序 277092 字节、动态内存 13196 字节，写入哈希验证通过。未执行整片擦除，只执行上传需要的目标扇区擦写。
- COM7/115200/8N1 运行采样 12 秒，取得 4 组 ESP32-C3 Rev 4、单核与 Chip ID 信息；检查的分区错误、panic、brownout 等计数为 0。串口已关闭释放。不将 12 秒通过推广为长期或断电重启稳定性，不推断 QIO 失败的具体电气原因。
- 产物与日志留在 TEMP/thumbremote-esp32c3-dio80-check 及对应 dio80-compile/upload/serial.log；细节与重复命令已同步 docs/esp32-toolchain.md。下一步核实 IT7259 转接板供电、针序和协议，触摸与 BLE 仍未实现。
