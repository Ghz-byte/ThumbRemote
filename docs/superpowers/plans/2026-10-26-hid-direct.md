# HID直接使用实施计划

> **For agentic workers:** 当前会话按executing-plans执行；行为已经用户明确批准，不重复询问执行方式。末尾一次增量独立审查，不提交/删除。

**Goal:** 系统蓝牙连接后真实上下滑直接遥控，移除长按与60秒期限。
**Architecture:** DirectControl协调Gate与原GestureRecognizer，确认释放才进入可用；sketch维护串口状态与传输。旧长按源码保留不接入。
**Tech Stack:** 原固定C++11/ESP32-C3/NimBLE与DIO80MHz4MB。
**Spec:** docs/superpowers/specs/2026-10-26-hid-direct-design.md。

## Global Constraints
- 真实上下滑唯一导航来源，无长按/串口arm要求，无超时；40ms释放与500ms冷却。
- 上电/重连/通信异常后确认释放；不重放历史动作，保留绑定及touch恢复产物。
- 不改原touch/Android/传输库，不新增依赖/硬件，不删除或提交。

## Review Focus
- 手指压着时连接或恢复健康，不得接受半次滑动。
- 等待释放时键盘中性报告未发送完，不能丢掉释放或自动重发旧键。
- 恢复健康后原接触不可被当作新接触。
- 超过60秒、millis回绕持续可用，左右及斜滑仍过滤。
- 非坐标包不当释放/坐标，未知包也不作为启停来源。

### Task1：纯直接控制协调
Files: firmware/hid_probe/hid_direct_control.h, firmware/tests/hid_direct_control_test.cpp。
Interfaces: DirectControl.update(bool ready,uint32_t generation,bool healthy,uint32_t now); observe(QueryState,const touch::Report*,uint32_t now,Report&)->bool; tick(uint32_t,Report&)->bool; cancel(uint32_t); enabled()->bool。
- [x] 先写连接、初次释放、上下报告/零释放/冷却、断线半笔、健康失败恢复、长接触不再开关、>60秒、回绕和过滤测试并观察失败（65检查先32失败）。
- [x] 实现协调，全部scripts/esp32.ps1 test通过（428 C+++9清单）。

### Task2：接入、审查与交付
Files: firmware/hid_probe/hid_probe.ino，当前文档。
- [x] 用DirectControl替换Gate+TouchControl，串口只status，未知包恢复v1忽略策略；更新日志。
- [x] 完整verify hid通过，独立审查无重要问题；唯一COM7/C3和touch恢复清单校验后upload通过。程序630285字节/RAM18816、源8b3672a0abca前缀。
- [ ] 只读串口检查，邀请用户OPPO抖音上下滑；未测项留未验证。
