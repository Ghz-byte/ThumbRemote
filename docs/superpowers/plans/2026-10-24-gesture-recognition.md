# 串口软件手势识别 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 用户已批准修改并烧录：每次单指接触松手后输出至多一个四方向事件，过滤短距离、轴向不明确及通信中断。

**Architecture:** 纯 C++ GestureRecognizer 独立于 I²C；消费成功的查询/坐标报告，fault 时锁住直到确认全松手。Arduino 保留原始输出，额外输出 GESTURE 或 GESTURE_REJECT，不发送手机消息。

**Tech Stack:** 既有 Arduino / esp32:esp32 3.3.10-cn、C++11 主机测试、scripts/esp32.ps1。

**Spec:** 本聊天用户“是”已确认的方案；实测映射与原始记录在 docs/touch-probe.md、docs/WORK_LOG.md。

## Global Constraints

- 不改接线。GPIO4/5、3.3V、DIO/80 MHz/4 MB、115200 保持；只读触摸，不主动复位/写触摸配置。
- 四方向：原始 X减=上、X增=下、Y减=右、Y增=左，仅适用于最近实测的握持朝向；不猜机械方向，不实现自动旋转。
- 本次实现选择：主轴位移 >=40 原始单位，且主轴位移 >=2×次轴位移，等号接受；判据为起点至最后有效坐标的净位移，无速度限制。
- 初始先等已知松手；01 无新包保留已有接触，01但未捕获起点时等松手；未知非坐标报告忽略，不能因芯片常见 Flick 包取消所有正常滑动。
- I²C 错误/短读、多个有效点或同次接触点号切换均取消，直到查询00或有效坐标包mask0才可恢复；不把故障当松手，不中途重开接触。
- 不提交/删除/推送，不扩展 BLE、显示或 Android。无新依赖。

## Review Focus

- 一次按下输出不得早于松手，重复空闲不得重复输出。
- Fault后即使恢复到有效坐标，也不能把残余半次接触误识别；下一次完整接触恢复。
- 正常非坐标/手势报告不得破坏当前滑动，01不得触发松手。
- 阈值/比例边界、三点有效位/换点号、4095坐标跨度与往返归零。
- 固件实际接入取消路径，不能仅纯算法通过而I²C失败仍沿用旧起点。

### Task 1: 识别状态机与主机验证

**Files:** firmware/touch_probe/gesture_recognizer.h、firmware/tests/gesture_recognizer_test.cpp、scripts/esp32.ps1。
**Interfaces:** `GestureRecognizer::observe(QueryState,const Report*) -> GestureResult`；`cancel()`；结果 outcome( None/Accepted/Rejected )、direction、reason、raw dx/dy。

- [x] 添加测试入口与声明，先观察失败；覆盖四方向、阈值39/40、轴比边界、静止、回原位、01、故障取消/恢复、多点与换点号。
- [x] 使用12组既有实测的起止坐标作为独立fixture验证映射，不复制计算规则作为期望。
- [x] 实现状态机，两个主机测试程序全部通过。

### Task 2: 集成与上板

**Files:** firmware/touch_probe/touch_probe.ino、docs/touch-probe.md、docs/WORK_LOG.md、docs/acceptance.md。

- [x] 将原始成功query/有效报告同时交给状态机，错误走cancel；未知非坐标包保持忽略。
- [x] 增加版本横幅、阈值说明及方向/拒绝结果；完整verify并独立审查。
- [x] 核对COM7唯一板与身份，按已授权方案上传；记录真实结果，不重问授权。
- [x] 用户准备好后做四方向实测与短触/不明确输入，保存日志；未实测项明确保留。

## 执行记录

- 参数40/2倍由实现者选择，已在进度消息说明；依据上一轮成功滑动主轴位移95–191，作为保守初值，不声称已验证体验。
- 本次为已批准的小范围改动，方案已在聊天确认；不另开大项目，不重复确认同一方案。
- Task 1: 2026-10-25 主机手势测试128检查先40失败，再0失败；原协议39检查也通过，合计167检查。重放fixture不代替新固件上板测试。
- Task 2: verify/upload均退出0，程序308584字节、RAM14260字节；独立审查无Critical/Important，mask4/5/6/7额外组合覆盖为非阻塞Minor，后续按需补充。v2空闲3次心跳正常；动作验证待用户准备好。
- 上板验收更新：四方向各3条事件、6次轻点too_short拒绝、单独3次斜滑ambiguous拒绝，本轮通信errors=0。方向窗口有2笔起点未覆盖，不称12笔完整接触；故障恢复/多点与长期稳定性不在已通过结论内。完整证据见WORK_LOG的v2节。
