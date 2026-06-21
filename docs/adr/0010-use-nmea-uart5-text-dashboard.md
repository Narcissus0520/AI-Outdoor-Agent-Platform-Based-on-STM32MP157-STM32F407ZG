# ADR-0010: Use UART5 NMEA Input and Text Dashboard Prototype

## 状态

Accepted

## 背景

Stage 1 需要为 UBLOX-M10 GNSS 模块补齐 MP157 侧真实串口输入的软件路径，并开始形成屏幕仪表盘显示能力。本 ADR 初始决策时尚未进行 UART5 上板验证，也尚未适配 7 英寸 RGB framebuffer 图形显示。

## 可选方案

### 方案 A：先实现 NMEA over UART5 和文本仪表盘

优点：

- 贴合 u-blox M10 常见 NMEA 输出方式，能复用现有 `NmeaParser`。
- 不引入 UI 框架或图形依赖，对交叉编译影响小。
- PC 侧可通过 NMEA 文件回放自动验证仪表盘内容。
- 后续可把文本渲染迁移到 framebuffer、终端 TUI 或 GUI。

缺点：

- 当前不解析 UBX 二进制协议。
- 当前仪表盘输出为文本文件，不是真正的 LCD framebuffer 绘制。

### 方案 B：直接实现 UBX 二进制协议和 framebuffer 图形仪表盘

优点：

- 更接近最终板端形态。
- 可获得更丰富的 u-blox 专有状态。

缺点：

- 未上板前风险较高，调试面大。
- 会引入更多协议状态机和屏幕绘制细节，容易拖慢 Sensor/GNSS 主链路。

## 最终决策

选择方案 A：先实现 MP157 UART5 NMEA 输入、增强 NMEA RMC/GGA/VTG/GSA/GSV 解析，并输出轻量文本仪表盘。

## 决策理由

当前阶段优先建立可编译、可测试、可展示的软件闭环。NMEA over UART5 足以覆盖定位、速度、航向、卫星数和 DOP 等仪表盘核心字段；文本仪表盘避免提前绑定具体屏幕驱动或 UI 框架。

## 风险

- UBLOX-M10 实际模块波特率可能不是默认 9600，需要上板时通过配置调整。
- MP157 UART5 对应 Linux 设备节点需上板确认；当前默认假设为 `/dev/ttySTM2`。
- 初始文本仪表盘不能代表 7 英寸 RGB 屏已完成适配；2026-06-21 后续修订已补齐轻量 fbdev 原型，但仍不代表完整 GUI/触摸交互完成。

## 后续 TODO

- 上板确认 UART5 设备节点、波特率和 NMEA 输出语句集合。
- 完成 UART5 + UBLOX-M10 真实串口验证。
- 评估是否需要 UBX 二进制协议解析。
- 已将文本仪表盘演进为 `outdoor-agent` fbdev framebuffer 仪表盘 APP 原型；后续继续评估终端 TUI 或更完整 GUI。

## 2026-06-21 后续修订

在保持方案 A 的轻量依赖原则下，`DashboardService` 已新增 Linux fbdev framebuffer 输出路径：

- 默认仍输出 `runtime/dashboard.txt`，保证 PC 自动化测试稳定。
- 板端可通过 `dashboard_output_mode = framebuffer | both` 和 `dashboard_framebuffer_device = /dev/fb0` 渲染到 7 英寸 RGB 屏。
- 新增 `dashboard_refresh_count` 和 `dashboard_refresh_interval_ms`，用于把一次性 dashboard 输出演进为可控的 `outdoor-agent` APP 刷新循环。
- 屏幕当前显示 GNSS、F407 Sensor Hub、MP157 Board IMU 和 `AI LOCAL AGENT: PLANNED` 预留区；真实 AI Agent 本地部署和交互仍未实现。

## 2026-06-22 后续修订

`DashboardService` 的 framebuffer 布局已按参考示意图升级为深色科技风户外终端仪表盘：

- 使用左侧导航、顶部状态栏、方向罗盘、大速度表、位置地图、温度/光照展示区和底部设备状态栏组织屏幕。
- 继续保持纯 fbdev 绘制，不引入 Qt、LVGL、SDL 或 Web UI 依赖。
- GNSS、F407 Sensor Hub 和 MP157 Board IMU 使用当前 Runtime 状态；光照、空气质量、电池、信号等展示项暂为 UI 占位/演示指标，等待后续真实传感器或系统状态接入。
