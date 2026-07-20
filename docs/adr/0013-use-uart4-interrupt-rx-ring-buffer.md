# ADR-0013: Use UART4 Interrupt RX Ring Buffer

## 状态

Accepted

## 背景

F407 Sensor Hub 通过 UART4 向 MP157 持续发送 100 Hz IMU、20 Hz 磁力计、10 Hz 气压计和 1 Hz heartbeat 帧，同时需要接收 MP157 下发的 `command_ping`。

原实现由主循环调用零超时 `HAL_UART_Receive()` 轮询单个字节。UART4 和 USART1 的阻塞式协议帧发送会占用主循环，连续 14 字节 `command_ping` 可能在这段时间到达并触发 UART overrun。2026-07-18 实测表现为 F407 -> MP157 上行稳定，但 MP157 连续发送三个合法 ping 仍没有 `command_ack`。

## 可选方案

### 方案 A：保留主循环轮询，增加接收超时

优点：

- 修改量小。
- 不需要新增中断状态。

缺点：

- 主循环会产生额外阻塞，影响固定周期传感器调度。
- 仍可能在阻塞发送期间发生连续字节 overrun。
- 随后续命令帧增长，可靠性继续下降。

### 方案 B：UART4 单字节中断接收加固定环形缓冲

优点：

- 接收不依赖主循环恰好在字节到达时轮询。
- 固定 64 字节缓冲不使用动态内存，成本明确。
- 保持主循环中的命令 decoder 和业务处理不变。
- 对当前短命令和低下行流量足够。

缺点：

- 需要维护 IRQ、HAL callback 和生产者/消费者索引。
- 缓冲满时仍可能丢字节，需要后续增加可观测计数。

### 方案 C：UART4 RX DMA 加循环缓冲

优点：

- 更适合持续高吞吐和较长命令流。
- CPU 接收开销更低。

缺点：

- DMA、空闲线检测和缓冲边界处理明显增加当前 Stage 1 的复杂度。
- 当前下行仅有低频短 `command_ping`，收益不足以抵消实现和验证成本。

## 最终决策

采用方案 B：UART4 使用 HAL 单字节中断持续接收，将字节写入固定 64 字节环形缓冲；主循环通过既有 `board_uart_receive_byte()` 消费缓冲并解析命令。

UART4 TX 和 USART1 诊断镜像暂时继续使用阻塞式发送，避免在同一变更中扩大到 TX DMA 重构。

## 决策理由

该方案直接解决真实板上复现的命令丢失，同时保持内存、CPU、依赖和代码边界可控。中断和环形缓冲位于项目自有 BSP，命令协议与 Sensor Hub 应用逻辑无需依赖 STM32CubeMX 生成代码。

2026-07-18 重刷后，MP157 连续发送三个 raw `command_ping`，COM6 诊断镜像收到三个合法 `command_ack`。当前 ARM Runtime 进一步确认 `command_ack_seen=true`、`command_ack_status=0` 和 nonce `0x50494E47`。

## 风险

- 缓冲满时会丢弃新字节；初始版本只发布 heartbeat bit，累计计数已由 ADR-0023 的 diagnostics schema 1 后续补齐。
- HAL UART callback 当前由 UART4 BSP 持有；后续新增其他中断 UART 时需要明确 callback 路由。
- UART4 TX 与 USART1 镜像仍为阻塞式发送，后续扩大数据量时可能需要 DMA 或发送队列。

## 后续 TODO

- [x] 增加 UART4 RX overflow 累计计数并发布到 diagnostics schema 1；MP157 预检和长测审计要求为零。
- 进行长时间双向压力测试和重复冷启动验证。
- 仅在下行吞吐或命令长度明显增加时评估 RX DMA。
- 根据传感器帧吞吐评估 UART4 TX DMA 或发送队列。

2026-07-19 后续实现与兼容方案见 [ADR-0023](0023-version-sensor-hub-diagnostics-extension.md)。当前 48 字节 diagnostics 已在真实 F407 -> MP157 链路确认 `schema_version=1`、`uart4_rx_drop_count=0`；真实非零 overflow 压力注入仍待独立启动周期验证。
