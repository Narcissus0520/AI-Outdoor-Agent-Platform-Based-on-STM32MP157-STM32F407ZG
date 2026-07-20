# ADR-0015: Use ICM42688 INT1 Data Ready and Fixed-Point Filtering

## 状态

Accepted

## 背景

F407 Sensor Hub 原先按固定 10 ms 周期轮询 ICM42688。ICM42688 已配置 100 Hz ODR，且硬件已将 INT1 接到 F407 PB12，因此固定轮询会让读取时刻与传感器数据更新时间产生偏差，也无法从状态中确认数据就绪信号是否正常。

户外仪表盘和后续姿态处理还需要抑制高频噪声，但当前 Stage 1 不适合引入动态内存、浮点 DSP 库或复杂传感器融合框架。

寄存器配置依据为 TDK InvenSense 官方 ICM-42688-P Datasheet DS-000347：`INT_CONFIG` 定义 INT1 模式/驱动/极性，`INT_CONFIG1.INT_ASYNC_RESET` 需清零，`INT_SOURCE0.UI_DRDY_INT1_EN` 将 UI data-ready 路由到 INT1，`GYRO_ACCEL_CONFIG0` 配置 accel/gyro UI LPF 带宽。

## 可选方案

### 方案 A：继续固定 10 ms 轮询

优点：实现简单，不依赖 INT1 接线。

缺点：读取与真实 ODR 不同步；主循环阻塞时会产生抖动；无法验证 INT1 链路。

### 方案 B：在 EXTI ISR 中直接读取 I2C

优点：中断到读取的延迟最小。

缺点：HAL I2C 是阻塞事务，放在 ISR 中会延长中断占用并干扰 UART、SysTick 和其他实时任务。

### 方案 C：ISR 记录事件，主循环读取最新样本

优点：ISR 只做常数时间计数；I2C 和协议发送仍在主循环；积压事件可合并，避免没有 FIFO 时重复读取同一份最新寄存器数据。

缺点：主循环过载时会丢弃中间样本；若要保证每个 100 Hz 样本都不丢，后续仍需 FIFO。

### 滤波方案

- 只使用 ICM42688 内部 LPF：成本最低，但无法统一处理 MMC5603 和 BMP390。
- 使用浮点滤波或外部 DSP 库：参数直观，但增加代码、依赖和嵌入式运行成本。
- 使用传感器内部 LPF加定点一阶 IIR：无动态内存、无第三方依赖，可覆盖所有当前传感器。

## 最终决策

选择方案 C，并采用内部 LPF加定点一阶 IIR：

- PB12 配置为上升沿 `EXTI15_10`；INT1 配置为脉冲、推挽、低空闲/高有效。
- EXTI ISR 只递增事件计数，`sensor_hub_app_poll()` 合并未处理事件并读取一次最新 ICM42688 样本。
- 最近 250 ms 内成功处理 INT1 事件时，heartbeat bit 7 `0x0080` 置位；三颗传感器 ready 且 INT1 活动时为 `0x00A9`。
- INT1 超过 250 ms 无事件时进入 Mock IMU fallback；连续读取失败或事件超时后，每 1 秒尝试重新初始化 ICM42688。
- ICM42688 内部 accel/gyro UI LPF 设为约 25 Hz。
- 软件定点 IIR 使用 `alpha = 1 / 2^shift`：IMU accel/gyro 和磁力计为 `1/4`，IMU 温度、BMP390 压力/温度为 `1/8`。
- 滤波器首样本直通；数据源切换或读取失败时重置状态，避免把旧源历史混入恢复后的数据。

MMC5603 仍按 50 ms 单次测量轮询，BMP390 仍按 100 ms 轮询。

## 决策理由

该方案把硬件数据就绪时序与阻塞 I2C 操作分离，既能证明 INT1 链路真实工作，又不扩大 ISR 职责。定点 IIR 只需要加减、移位等价除法和少量状态，适合 STM32F407 当前阶段，同时对三类传感器提供一致、可测试的平滑处理。

## 风险

- 当前没有启用 ICM42688 FIFO；主循环阻塞时会合并多个 INT1 事件，因此协议帧率可能低于传感器 100 Hz ODR。
- `alpha=1/4` 和 `alpha=1/8` 会引入一定相位延迟，不适合直接替代后续高动态姿态融合的原始数据通道。
- INT1 断线会触发 Mock fallback，但当前 heartbeat 只提供活动位，没有提供累计中断数和超时次数。

## 后续 TODO

- 根据姿态融合和数据记录需求评估 ICM42688 FIFO。
- 将滤波参数做成编译期配置，并为高动态/低噪声模式提供不同参数组。
- 增加 INT1 事件数、合并数、超时数和 I2C 恢复次数诊断。

## 参考

- TDK InvenSense, ICM-42688-P Datasheet DS-000347: https://invensense.tdk.com/wp-content/uploads/2020/04/ds-000347_icm-42688-p-datasheet.pdf
