# 最小触摸串口测试 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 ESP32-C3 上诊断 IT7259 通信并输出触摸坐标及松手，用户已在本聊天确认实施与烧录。

**Architecture:** Arduino Wire 负责带超时的缓冲区读取，独立 C++ 头文件负责查询分类、14 字节报告解析及触摸状态。主机测试与目标固件共用协议代码。

**Tech Stack:** 既有 esp32:esp32 3.3.10-cn、Arduino CLI 1.5.1、C++、PowerShell；不安装依赖。

**Spec:** 本聊天已批准的最小方案；协议来源 docs/hardware-evidence.md 及卖家 IT7259 User Programming Guide V0.11 第 10、15–18、48–49 页。

## Global Constraints

- SDA=GPIO4、SCL=GPIO5；屏幕绿色/黄色/红色/黑色分别为 SDA/SCL/3V3/GND，来源为用户近照与反馈。接线是否完成尚未独立确认。
- FQBN esp32:esp32:esp32c3:FlashMode=dio，80 MHz/4 MB，串口 115200。
- 首轮选择 100 kHz I²C、50 ms 事务超时、20 ms 轮询；这些为实现选择，不是实测结果。
- 默认地址 0x46；只探测该地址，失败提示排查，不随意写其他设备。
- 查询 0x80 后仅在 bit7=1 时读 0xE0 的 14 字节。00 无触摸；01 保留触摸且无新包；读取失败、未知报告均不产生松手。
- 不主动复位、不写命令配置、不用卖家混合寄存器初始化、不做显示/BLE。
- 上拉实装未知：读取空闲电平并提示；Wire 自带内部弱上拉不作为总线电气合格依据，不以 ACK 证明信号裕量。若失败再核对外部上拉。
- 产物只放 TEMP；不提交、推送、删除或改系统配置。

## Review Focus

- 高位坐标不得丢失：手工字节 fixture 验证 12 位组合。
- 01 无新包、I²C 失败不得当作松手。
- 短包与非坐标报告不得覆盖有效状态。
- 多点有效位不得错误使用 point0；首版完整解析三个点。
- 未连接或线路低电平时应有明确诊断，不无限等待，不报告测试通过。

### Task 1: 协议与状态

**Files:** firmware/tests/touch_protocol_test.cpp、firmware/touch_probe/touch_protocol.h。
**Interfaces:** `classifyQuery(uint8_t)`、`decodeReport(const uint8_t*, size_t, Report&)`、`TouchState::apply(QueryState, const Report*)`，Event 区分 None/Down/Move/Up。

- [x] 先写断言：12 位、三点、短包、未知报告、首次触摸、移动、00 松手、01 保持、无效读取不改状态。
- [x] 运行并记录缺失实现导致的失败；实现后同一测试全部通过。

### Task 2: Arduino 测试入口与可复现验证

**Files:** firmware/touch_probe/touch_probe.ino、scripts/esp32.ps1、docs/touch-probe.md。

- [x] 实现显式引脚、事务超时、先查询后读包、错误节流及每 2 秒心跳，输出未知报告原始字节。
- [x] 脚本将源文件按哈希复制到 TEMP 英文路径；调用既有镜像工具链完整编译，写 manifest 绑定源哈希/配置/产物。
- [x] 主机测试、目标完整编译通过；检查上传端口。未连接则请用户连接，不虚报烧录或触摸通过。
- [x] 更新硬件证据、工作日志、验收状态，说明未测项和后续操作。

## 执行记录

- 已有同意覆盖此次小测试实现与烧录；继续本会话执行，不重复询问。上级规则禁止未经要求提交/删除，故不使用技能中的 commit/cleanup 步骤。
- Task 1: complete — 主机测试 35 checks：19 failures RED → 0 failures GREEN。
- Task 2: 电脑端、上传与独立板心跳完成，触摸验证待连接 — 最终 verify 39 checks / 0 failures，完整编译退出 0，307502 bytes / RAM 14236 bytes；随后 upload 退出 0，8 秒 4 次 STATUS。
- Final review: 独立只读审查未发现 Critical/Important；诊断的 idle 文案问题补测试先红 3 项再绿，并绑定平台上传 boot_app0 哈希；最终重新完整编译通过。已按授权上传，屏幕断开状态下取得心跳；未完成实际触摸验证。
- 上板更新：接屏后取得 0x46 ACK；动作 30 秒采样 5 组完整接触、36 条移动，另有窗口末第 6 次按下；追加 6 秒确认空闲，累计通信错误 0。最小通信/坐标/松手验收已取得实测证据；全屏方向、范围、上拉与长期稳定性不在本轮通过结论内。
