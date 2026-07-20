# ADR-0023: Version the Sensor Hub Diagnostics Extension

## 状态

Accepted

## 背景

F407 UART4 RX 已使用中断和 64 字节环形缓冲，并在缓冲满时累计 `board_uart_rx_drop_count()`。heartbeat bit 13 能表明本次启动中曾发生 RX 丢字节，但 MP157 无法取得具体累计次数，长测也无法判断是否发生多次丢失或量化运行期增量。

现有 `sensor_hub_diagnostics` 为 44 字节固定载荷，最后 1 字节保留为 0。直接改变固定长度会让旧 MP157 Runtime 拒绝新帧；仅复用 1 字节又无法提供长期运行所需的 32 位单调计数。

## 可选方案

### 方案 A：继续只使用 heartbeat bit

优点：

- 无协议变更。
- 当前健康门槛已经会拒绝 bit 13。

缺点：

- 只能表达零/非零，不能量化次数或增量。
- 无法和 I2C/FIFO 等累计 diagnostics 使用相同时间线审计。

### 方案 B：为现有 diagnostics 增加带版本标记的尾部扩展

优点：

- 复用既有每秒诊断帧和 MP157 状态链路。
- 保留 44 字节 legacy 解析，同时用原 reserved 字节区分扩展 schema。
- 48 字节载荷仍小于协议 64 字节上限。

缺点：

- 旧 MP157 Runtime 仍会拒绝 48 字节新帧，需要先部署兼容解析器再刷 F407。
- 后续扩展必须继续维护 schema 与长度的一致性。

### 方案 C：新增独立 UART link diagnostics 帧类型

优点：

- 与 I2C/FIFO 诊断解耦，扩展空间独立。
- 旧 Runtime 可忽略未知类型。

缺点：

- 每秒新增一帧，增加协议类型、发送和审计分支。
- 当前只有一个 32 位字段，独立帧的复杂度和带宽收益不足。

## 最终决策

采用方案 B。

- legacy payload 保持 44 字节，offset 43 为 0；新 MP157 Runtime 将其解析为 `schema_version=0`、`uart4_rx_drop_count=0`，用于兼容历史 fixture 和旧固件证据。
- 当前 payload 为 48 字节，offset 43 为 extension schema `1`，offset 44..47 为 little-endian `uart4_rx_drop_count`。
- MP157 只接受 44 字节 legacy 或 48 字节 schema 1；未知长度和未知扩展版本明确拒绝。
- 正式板端预检与耐久审计要求 schema 1、最终 RX drop 为 0、运行期增量为 0；legacy 兼容用于读取历史数据，不作为新正式测试放行条件。
- 部署顺序固定为先更新兼容 MP157 Runtime，再更新 F407 固件，避免中间阶段主控无法解析新 diagnostics。

## 决策理由

该方案用最小协议增量把已有 ISR 计数变成 MP157 可持久化、可量化的长期可靠性证据，同时保留历史 fixture 和旧 44 字节数据的可读性。显式 schema 比“根据长度隐式猜版本”更易于后续审计和面试说明。

## 风险

- heartbeat bit 和累计计数在同一 F407 启动周期内均为粘性证据；真实溢出后需要重启 F407 才能开始新的零基线正式验收。
- 32 位计数在极端长期运行中可能回绕；当前小时级验收只检查单调性和零值，回绕风险可忽略但协议语义需保留。
- 已用 40960 字节连续下行在真实板触发 drop 2231 并验证预检拒绝；单次故障注入已闭环，但更长持续过载、不同命令帧分布和环形缓冲容量边界仍待测量。

## 后续 TODO

- [已完成] 在独立测试启动周期中执行可控 UART4 下行压力测试，证明非零计数、heartbeat bit 13、MP157 状态和预检拒绝一致；F407 应用复位后计数归零。
- 扩展完整下行命令集前测量主循环最坏消费延迟，再决定是否扩大 RX ring 或引入 DMA/idle-line 接收。
