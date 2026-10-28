# HID长按启停实施计划

> **For agentic workers:** 使用superpowers:executing-plans由当前会话实施；测试先行，末尾独立审查。用户已确认完整聊天行为并选择长按启停，在既有工作区完成，不再询问执行方式，不提交Git。

**Goal:** 无电脑时以3秒长按松手启停HID遥控。
**Architecture:** 新纯TouchControl复用GestureRecognizer，长接触与滑动互斥；Gate区分60秒诊断arm和无限期物理arm；sketch协调连接取消和零释放，不改传输与原App。
**Tech Stack:** 既有C++11、esp32 3.3.10-cn/NimBLE、DIO80MHz4MB，无新增依赖。
**Spec:** docs/superpowers/specs/2026-10-26-hid-long-press-design.md（用户已确认聊天方案，精确阈值为实现选择）。

## Global Constraints
- 默认待机，长按3000ms且每轴抖动<=12原始单位、松手才切换；长接触不发方向键。
- 原touch源码及恢复产物保留；串口arm保持60000ms，物理arm持续直到停止/错误/链路状态变化。
- 每笔最多一动作，故障/多点/未知包取消，先释放；无新接线/显示/Android/依赖/提交/删除。

## Review Focus
- 长按开始在ready前或取消前，不能跨连接开启。
- 达到时长后移动、移出再移回、多点换指不可变为开关或长接触方向键。
- Holding能维持计时，未知包不可当作释放。
- 无限期会话保留原40ms释放/500ms冷却及停止重建零报告，不能将旧报告跨连接发出。
- millis回绕、诊断arm与物理arm交替，期限不能串用。

### Task1：输入分类和持久Gate
Files: firmware/hid_probe/hid_touch_control.h, hid_keyboard_gate.h, firmware/tests/hid_touch_control_test.cpp, hid_keyboard_gate_test.cpp。
Interfaces: TouchControl.observe(QueryState,const touch::Report*,uint32_t)->InputResult{toggle,gesture}；cancel()；Gate.armPersistent(uint32_t)->bool。
- [x] 写长按2999/3000ms、只松手触发、重复Holding、抖动边界/移回、多点/换指/取消、启动半笔、回绕、短滑和长滑互斥、持久arm跨60秒和诊断仍超时用例，观察预期失败（59检查先20失败）。
- [x] 实现上述接口，scripts/esp32.ps1 test全部通过（363 C++检查+9清单检查）。

### Task2：sketch接线与完整验证
Files: firmware/hid_probe/hid_probe.ino。
- [x] observe先分类，toggle分支只启停不发键；链路/异常/串口控制cancel输入；unknown只取消该笔；启动横幅与日志区分来源及persistent。
- [x] 主机用例通过后，scripts/esp32.ps1 verify -Target hid完整编译；原touch恢复清单保持。程序632013字节/RAM18824，源4c384d1a7b5d前缀。

### Task3：审查、上传与实测入口
Files: docs/hid-probe.md, docs/WORK_LOG.md, docs/acceptance.md。
- [x] 独立审查，未发现Critical/Important/Minor问题；不替代真机组合验证。
- [x] 恢复产物及唯一COM7/C3复核，upload -Target hid退出0；7秒4组只读心跳ready1/armed0/persistent0/errors0，不自动发arm。
- [ ] 记录真实编译/上传/串口证据，请用户物理长按/滑动/停止；超过60秒及USB充电宝实物测试完成前留未验证。
