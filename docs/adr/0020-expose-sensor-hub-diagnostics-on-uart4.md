# ADR-0020: 在 UART4 主链路暴露 Sensor Hub 累计诊断

## 状态

Accepted

## 背景

F407 已每秒生成 `0x02 sensor_hub_diagnostics`，包含 I2C recovery/failure、FIFO overflow/malformed/empty/stall/skipped 和最近失败上下文，但此前只发往 USART1/COM6。当前一键下载硬件在打开 COM6 时会扰动 DTR/RTS 并重置 F407，因而不能在 MP157 小时长测期间安全读取累计诊断。

2026-07-19 的健康时间线捕获到周期性 `0x022E/0x03AE` FIFO fallback，但仅凭 heartbeat 无法区分 overflow、畸形包、空事件或 I2C burst 失败。

## 可选方案

### 方案 A：继续只通过 COM6 输出诊断

优点：

- 不增加 UART4 业务链路流量。
- 不需要修改 MP157 parser。

缺点：

- 当前硬件打开 COM6 会重置 F407，无法用于无人值守长测。
- 诊断和业务数据不在同一证据时间轴。

### 方案 B：扩展 heartbeat payload

优点：

- 每个 heartbeat 都携带累计计数。

缺点：

- 改变既有 heartbeat payload 长度，影响所有解析方和 fixture。
- 44 字节诊断字段会让基础存活帧承担过多职责。

### 方案 C：增加下行诊断查询命令

优点：

- 只在需要时返回，不持续占用带宽。

缺点：

- 需要新增命令、响应、超时和并发处理语义。
- 轮询失败本身会引入新的证据缺口。

### 方案 D：将现有 1 Hz `0x02` 帧同时发送到 UART4 和 USART1

优点：

- 复用现有协议和 F407 builder，不改变 heartbeat。
- MP157 可在同一串口、同一状态文件和同一时间线中保存累计诊断。
- 每秒仅增加约 54 字节，远低于 115200 8N1 可用带宽。

缺点：

- MP157 parser、状态结构和验证 fixture 需要扩展。
- UART4 主链路不再只包含业务传感器帧。

## 最终决策

采用方案 D。F407 每秒将同一个 `0x02` 帧分别入队 UART4 主链路和 USART1 诊断镜像。MP157 Runtime 解析 44 字节 payload，在 `runtime_status.json` 输出独立 `sensor_hub_diagnostics` 节点；健康时间线记录累计计数并检查单调性和测试期间增量。

ICM42688 provider 增加显式首次 reset；运行期重新初始化不再清零 FIFO 累计计数，保证诊断值在一次 F407 启动周期内单调。

## 决策理由

该方案用最小协议增量解除 COM6 物理控制线对长测诊断的依赖。新增流量约占 UART4 理论带宽的 0.5% 以下，不需要线程、动态内存或第三方依赖，也不改变既有传感器和 heartbeat 帧格式。

## 风险

- 诊断帧和高频 IMU 共享 UART4 TX 队列；现有 sticky TX overflow 位和累计时间线必须继续验收。
- 旧 F407 固件不会发送 `0x02`，新版 MP157 预检会因 `diagnostics_seen=false` 拒绝正式长测，这是预期的版本配对门槛。
- 累计计数能定位错误类型，但不能替代逻辑分析仪或 I2C 电气测量。

## 后续 TODO

- 烧录新固件并通过 MP157 确认 diagnostics seen、字段解码和计数单调。
- 使用累计计数定位周期性 FIFO fallback 的具体来源并修复。
- 完成带 diagnostics 的 60 秒和 3600 秒联合验收，确认 UART4 TX queue 无 overflow。
