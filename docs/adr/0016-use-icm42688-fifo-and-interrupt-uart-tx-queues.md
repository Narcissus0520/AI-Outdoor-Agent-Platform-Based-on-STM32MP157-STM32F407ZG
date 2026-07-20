# ADR-0016: Use ICM42688 FIFO and Interrupt-Driven UART TX Queues

## 状态

Accepted

> 2026-07-19 修订：本 ADR 的 UART TX 队列决策继续有效；ICM42688 byte-count、64 字节 watermark 和可变包读取部分在真实板累计 diagnostics 出现后，已由 [ADR-0021](0021-bound-mmc5603-polling-and-use-fifo-record-count.md) 修订为 record-count、4 条记录 watermark，并同时限制 MMC5603 状态轮询。以下内容保留为当时决策和历史验收记录，不代表当前最终配置。

## 背景

ICM42688 INT1 版本在 100 Hz ODR 下直接读取最新 14 字节样本，并通过 UART4 和 USART1 依次执行阻塞发送。2026-07-19 的 60 秒实测 IMU 频率约为 `92.45 Hz`；当主循环处理共享 I2C2 或双路串口发送时，多个 data-ready 事件会被合并，无法保留积压样本。

本阶段需要在不引入大型依赖、不把 I2C/UART 阻塞事务放进 ISR 的前提下，提高 IMU 样本保留能力，并消除 UART4/USART1 应用态发送对主循环的阻塞。

## 可选方案

### ICM42688 方案 A：继续读取最新寄存器样本

优点：

- 实现简单，单次读取固定 14 字节。
- RAM 和解析开销小。

缺点：

- 主循环延迟时只能得到最新样本，期间产生的样本会丢失。
- INT1 事件计数无法恢复已经被覆盖的寄存器数据。

### ICM42688 方案 B：FIFO record-count 模式并假设固定 16 字节记录

优点：

- 读取批次时可以直接按记录数计算长度。

缺点：

- ICM42688 FIFO 在启动和状态变化阶段可能出现 8 字节单传感器包或消息头，不应假设每条记录都是 16 字节组合包。
- 错误的固定长度假设会造成后续包头错位。

### ICM42688 方案 C：FIFO byte-count 模式和可变包解析

优点：

- FIFO 计数直接表示待读取字节数，适合一次 I2C burst drain。
- 可以显式处理 16 字节 accel+gyro 包、8 字节单传感器包和消息头。
- 主循环短时延迟不会立即丢失全部积压样本。

缺点：

- 需要包头解析、批次时间戳重建和异常恢复。
- 批量读取使用更多栈空间，最坏 I2C 事务时间更长。

### UART 方案 A：继续阻塞式双路发送

优点：实现简单。

缺点：UART4 和 USART1 发送时间直接叠加到主循环延迟，容易造成 FIFO/传感器处理滞后。

### UART 方案 B：固定内存队列加 HAL 中断发送

优点：

- `HAL_UART_Transmit_IT()` 启动后立即返回，发送完成回调继续消费队列。
- 不需要配置 DMA stream，CubeMX 和中断资源改动较小。
- 固定缓冲区不使用动态内存，队列满时可以按整帧拒绝并累计诊断计数。

缺点：每字节仍由 UART 中断推进，CPU 开销高于 DMA。

### UART 方案 C：DMA 发送

优点：CPU 开销最低，适合更高持续带宽。

缺点：需要增加 DMA stream、IRQ、缓冲生命周期和错误恢复设计；当前约 4 kB/s 协议负载尚不足以证明必须承担该复杂度。

## 最终决策

选择 ICM42688 方案 C 和 UART 方案 B：

- ICM42688 使用 stream-to-FIFO，FIFO count 配置为字节数和大端格式。
- FIFO 启用 accel、gyro 和 temperature，watermark 为 64 字节；`FIFO_WM_GT_TH` 使 FIFO 高于阈值时继续按 ODR 触发，threshold/full 路由到 INT1。
- 每次最多读取 256 字节，解析最多 16 个组合样本；8 字节单传感器包保留在时间轴中但不发布，消息头跳过。
- 主循环最短每 30 ms 消费一次 FIFO 事件，ICM42688 在 MMC5603/BMP390 之后初始化，避免启动阶段积压。
- FIFO overflow、畸形包或输出容量不足时清空 FIFO，并设置当前错误；下一批有效样本到达后恢复 ready 状态。删除了把“读取后 count 未下降”当作 drain stall 的判断，因为 FIFO 可在读取期间并发增长，该判断会产生误报。
- `FIFO_DATA` 是有副作用的数据流，I2C 读取失败后只恢复总线、不重放原 burst，并清空 FIFO 重新对齐；普通寄存器读写仍允许恢复后重试一次。
- 真实 FIFO 与 Mock fallback 共用 100 Hz 发布时序器；批次整体锚定到上一已发布样本并保留批内相对间隔，避免 I2C 恢复或批次回推导致时间戳回退/大空洞。
- I2C 读取超时调整为 30 ms，以覆盖 100 kHz 下 256 字节 burst 的理论传输时间。
- UART4 和 USART1 各使用 1024 字节固定 TX 队列；主循环按整帧入队，`HAL_UART_Transmit_IT()` 与完成回调分段发送。
- UART4 继续使用 64 字节 RX 中断环形缓冲；TX/RX 队列溢出通过 heartbeat sticky bit 暴露，FIFO/I2C 位表达当前错误。累计恢复/失败信息最初由 USART1-only `0x02` 帧保留；按 ADR-0020，当前实现将同一诊断帧同时发送到 UART4/MP157。

## 决策理由

64 字节 watermark 在 100 Hz、16 字节组合包的稳态下约每 4 个样本触发一次读取，既能降低 I2C 事务次数，也把正常批次保持在较小范围。byte-count 模式允许解析实际出现的可变包，而不是把硬件行为简化为固定记录。

中断 TX 队列已经消除主循环的串口等待时间，且当前带宽远低于 115200 8N1 的有效吞吐能力。DMA 保留为后续在真实队列拥塞或更高数据率出现时的升级选项。

该实现只依赖项目自有 C 代码和 STM32 HAL，不增加第三方依赖，对 GNU ARM 交叉编译和长期维护影响较小。

## 风险

- 两个 TX 队列增加约 2 KiB 静态 RAM；当前 Release 固件 BSS 为 4424 B，仍远低于 STM32F407ZG RAM 容量。
- FIFO 中断或主循环若长时间停滞，超过 256 字节的软件批次上限时会丢弃当前 FIFO 并记录 overflow。
- UART 队列满时整帧会被拒绝；heartbeat 对应 overflow bit 为 sticky，便于验收发现问题。
- 2026-07-19 最终 Release BIN 为 19368 B；主机 CTest 5/5、ARM 构建、逐字节烧录回读、整板复电、两次最终镜像独立烧录复位、60 秒 COM6 FIFO 验收和 MP157 UART4 双向命令回包均已通过。
- 最终 60 秒抓取为 heartbeat 60、IMU 6086、磁力计 1187、气压计 595；对应约 `101.43/19.78/9.92 Hz`，最终 heartbeat `0x01A9`，CRC/协议/载荷错误、IMU 时间戳回退和 UART4/USART1 TX overflow 均为 0。

## 后续 TODO

- 完成小时级稳定性、SDA 卡住和传感器断线故障注入测试。
- 只有在真实 TX overflow 或更高输出带宽出现时，再评估 UART DMA。

## 参考资料

- [TDK ICM-42688-P Datasheet](https://invensense.tdk.com/wp-content/uploads/2020/04/ds-000347_icm-42688-p-datasheet.pdf)
- [ST UM1725: Description of STM32F4 HAL and LL drivers](https://www.st.com/resource/en/user_manual/dm00105879-description-of-stm32f4-haland-ll-drivers-stmicroelectronics.pdf)
