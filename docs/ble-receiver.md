# BLE 前台接收测试

日期：2026-10-25。范围：ESP32识别方向，手机前台接收并显示；BLE事件不执行手机手势。用户已确认设计、实施、测试后烧录及安装。

## 当前交付与验证

- 固件新增独立BLE协议与传输层，保留触摸v2识别器。固定esp32:esp32 3.3.10-cn实际是NimBLE后端，使用ble_gap_conn_desc及onSubscribe，标准CCCD由库自动生成；未安装另一套库或切换后端。
- `scripts/esp32.ps1 verify`：196检查（29 BLE +128手势 +39触摸协议）全通过；完整编译退出0，程序626361字节、RAM18648字节。产物 TEMP/thumbremote-touch-56789245aac4，源码哈希56789245aac450dd1ee358c8784ea9f9ad6bb755fcc4916ea073c27f33b7f56b。
- 核对唯一CH343/COM7与ROM ESP32-C3后，upload退出0，四段哈希校验通过，无整片擦除。当前板内已为touch_probe v3 BLE收消息测试版。上传后115200串口采样8秒，4次心跳、触摸地址应答、idle、errors=0；打开串口时已经运行，没有截到启动横幅，不将此采样称为广播/手机通信已通过。
- Android `scripts/android.ps1 verify`：17项JUnit（原7+协议/状态7+权限异常回归3）通过、Lint 0 errors / 5 warnings、Debug APK成功。最后构建镜像TEMP/thumbremote-1b0bc2c17a5b；APK在android/app/build/outputs/apk/debug/app-debug.apk。
- 独立审查发现的Gatt.close和扫描结果读取权限竞态已修复；3项新增回归测试先失败后通过。权限检查与平台调用之间仍可能撤权，因此通过可测边界捕获SecurityException，清理阶段仍尝试close并完成本地状态失效，不使用Lint忽略标记。
- 唯一授权OPPO PLB110/API36已通过ADB识别，`scripts/android.ps1 install` 返回Success；冷启动Status ok，自身进程AndroidRuntime检查未见错误。重连后板端记录连接/订阅/READY，四方向各10/10的串口序号与用户手机确认对应，轻点/斜滑各3次过滤通过。未安装驱动或修改系统设置。
- 用户明确说“不用验证了”，停止后续测试。断线不补发、页面离开/锁屏、蓝牙关闭、权限拒绝/撤销、关闭辅助功能独立接收及本版手动功能回归均保留未验证；实现中的对应逻辑不等于真机已验收。

## 怎么使用（安装后由用户操作）

首轮联合采样17条UP因无就绪会话丢弃，首次断开原因未确定。重新连接后记录到连接/订阅/READY；上seq1–3与34–40、下seq4–13、右seq14–23、左seq24–33均获用户手机方向/数量确认，总数40，通信errors=0。轻点3次too_short、斜滑3次ambiguous均未发送，手机无新增。日志见WORK_LOG最新节。

1. 保持原屏幕四线及USB供电。手机开启蓝牙、打开ThumbRemote。
2. 点击“扫描开发板”，由用户亲自允许附近设备权限；如未开蓝牙，用户在系统设置开启后返回重试。
3. 在结果里点选ThumbRemote。界面依次显示连接、服务发现、订阅、会话确认，完成后为“可以接收”。显示连接不等于握手已完成。
4. 保持此前实测屏幕朝向，慢滑并松手。卡片显示方向、接收数和最近30条记录，记录中的#序号用于与开发板串口对照。轻点、轴向不明确的斜滑不产生方向。
5. 点击断开会立即结束会话；离开页面/锁屏也会断开，返回不自动重连。重连前的动作不会补发。清空记录只清列表，不重置当前去重状态或连接。
6. 当前BLE不调用辅助功能，接收测试不需要辅助功能服务；页面下方原手动按钮仍遵循已有授权和5秒倒计时规则。

## 实现关键点

协议、UUID、10字节通知与5字节启动帧详见设计文档。手机依次完成服务发现、开启本地通知、CCCD写入、会话启动写入及READY确认；写回调与READY可以乱序，两个条件齐全才开始接受方向。手机SDK31/32与33+通知回调分开，立刻复制字节。

每次连接生成非零sessionId；它是消息隔离编号，不是认证凭据。旧Gatt/generation/session消息及重复/倒序sequence丢弃；正向跳号只记录缺号、不要求补发。固件断线时不保留方向队列；连接/订阅/会话变化取消进行中的触摸接触，等完全松手后新接触才可发送。BLE回调只发布带连接身份的原子状态，不操作触摸识别器或I²C。

本轮只服务一个客户端，连接后停止广播，断开恢复广播；广播含服务UUID，名字在扫描响应中。notify日志标记为BLE_NOTIFY_ATTEMPT，只表示SDK通知尝试，手机实际收到必须由App记录证明。

App仅API31+提供BLE，minSdk24与手动功能保持。只声明SCAN(neverForLocation)/CONNECT及可选BLE硬件，不申请定位、网络、手机广播、后台常驻或前台服务权限。

## 验证命令

```powershell
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 verify
powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 upload -Port COM7
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 verify
powershell -ExecutionPolicy Bypass -File scripts/android.ps1 install
```

所有步骤用既有镜像/固定工具链，不修改永久环境变量。安装脚本只接受唯一授权真机，不选择任意一台。系统权限均由用户本人处理，不通过ADB授予。

## 尚待真机验收

- 扫描与完整握手Ready，关闭辅助功能仍能收方向。
- 上/下/左/右各10次，对照串口尝试序号与手机记录逐条登记。
- 轻点/斜滑各3次不产生手机方向。
- 主动断开后滑3次，手动重连后无旧动作，新动作可接收。
- 页面离开、锁屏、蓝牙关闭、权限拒绝/撤销均明确结束，不自动重连。
- 原手动路径回归。真实无线距离、长期稳定性、多点/故障实物恢复、固定机械朝向仍未验收。

主机门控测试与原识别器故障测试分别通过；“跨连接半笔接触”的真实无线组合流程尚未实测。审查建议的组合自动化覆盖保留后续，不把模块单测推广为实物结果。

## 当前提示说明

Lint保留5项Warning：OldTargetApi（target36对应现有Android16）、UnusedAttribute（扫描标志仅API31+，功能已有版本门控）、AndroidGradlePluginVersion（保留固定验证版本）、UseRequiresApi（使用平台@TargetApi和真实版本检查，未新增注解依赖）、DataExtractionRules（原allowBackup=false策略）。未添加跳过或抑制标记。编译的deprecated提示主要来自API31/32兼容回调/写入分支与较新SDK对connectGatt的弃用，实际API33+使用新通知value/写入接口。
