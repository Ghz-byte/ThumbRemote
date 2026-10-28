# MatePad HID 单击 Implementation Plan

**Goal:** 在同一dual固件中保留键盘上下滑和OPPO App路线，为MatePad试验中心单击及快速两次单击。

**Architecture:** 共享BLE服务端增加独立HID绝对坐标输入报告；实际输入严格按App/HID路由、订阅、代数和当前连接分流。HID报告引入按下/释放状态机及限时的一笔后续点击；dual方向键取消固定冷却，独立hid/touch目标保留。实板发现SDK对同一服务内重复报告UUID去重后，改为单一BLE服务端中独立键盘和数字笔两个HID服务实例，见设计修正；保留SDK不修改。设计参见 `docs/superpowers/specs/2026-10-27-matepad-tap-design.md`。

**Global constraints:** ESP32-C3、既有esp32 3.3.10-cn和DIO/80MHz/4MB；不新增系统权限、手机App、接线、自动动作、清配对或全片擦除；不删除文件、不提交推送。新GATT表的现有绑定客户端缓存刷新待实测。

**Review focus:** 双报告订阅独立、无授权/旧连接禁发；释放先于下一次按下；旧状态切换时不漏零释放或跨设备补发；用户快速点击不被冷却吞掉；App路径及键盘旧报告缓存迁移仍正常。

### Task 1: 保留基线并建立测试契约

- [ ] 在AGENTS记录本方案确认、仅dual扩展的文件职责和保存恢复产物；核对8dceb清单的全部BIN及原touch恢复清单，保留原文件。
- [ ] 在现有`firmware/tests/`添加或扩展测试：报告描述符及按下/释放编码、中心坐标、无接触启动、中途取消；先运行观察缺失功能失败。

### Task 2: 报告与触摸协调

- [ ] 在现有`firmware/dual_remote/`内独立实现可主机测试的HID点击报告和时序状态机；以真实短触为唯一触发，首笔立即发，短时后一笔有界；健康变化/过期/身份/代数失效清除，按下必须配对释放。
- [ ] 扩展`dual_touch_control`和相关主机测试：App/平板各执行一条路线、短点只发点击、上下滑仅发旧箭头、长按/斜滑/触摸异常取消；dual键盘冷却取消，独立hid目标行为不变。

### Task 3: SDK服务集成和派发

- [ ] 核对现有SDK的`BLEHIDDevice::inputReport(2)`、独立CCCD回调、报告映射和服务启动顺序；扩展`dual_ble_transport`的独立句柄、订阅及代数和发送状态，最终发送校验当前连接安全状态、App锁存、路由与代数。释放无法可靠发送时结束连接，不能遗留按压。
- [ ] 为两报告并发、订阅撤销、连接切换、发送失败、队列满/过期、App接管、中性释放编写针对真实协调代码的主机测试；广播和GATT表继续通过现有验证，未测设备行为不推定通过。

### Task 4: 构建、烧录和真机验收

- [ ] `powershell -ExecutionPolicy Bypass -File scripts/esp32.ps1 test`及`verify -Target dual`完整通过；检查准确源/二进制清单、单元测试结果和恢复材料。
- [ ] 复核唯一目标端口、芯片及授权；使用`upload -Target dual -Port COM7`更新，记录实际输出，不自动清理旧文件或绑定。被动读取串口确认启动，系统配对由用户完成。
- [ ] 用户逐项验证MatePad单击、快速双击、上下滑；切回OPPO确认App连接、点击和上下滑，再切回平板。按实际反馈记录，缓存不刷新或设备不响应时停止并分析，不将通知成功当屏幕操作成功。
