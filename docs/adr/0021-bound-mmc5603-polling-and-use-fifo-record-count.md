# ADR-0021: 限制 MMC5603 状态轮询并使用 ICM42688 FIFO record-count

## 状态

Accepted；软件决策保留，完整复电后的板端稳定性验收未通过，TRB-024 继续分析物理总线/从机隔离问题。

## 背景

UART4 diagnostics 首次让 MP157 在不打开 COM6 的情况下看到累计证据。30 秒测试中逐秒 heartbeat 全为 `0x01A9`，但 I2C recovery 增加 204、最终 failure 增加 2，FIFO overflow/malformed/skipped 分别增加 10/8/9281。

TDK ICM42688-P 数据手册定义 `HEADER_MSG=1` 表示 FIFO 已空，旧 parser 却继续逐字节扫描。第一轮 record-count/400 kHz 修复将 skipped 降至 290，但 recovery/failure 未消失、overflow 反增。最近失败横跨三颗从机，进一步检查发现 MMC5603 单次测量后的 `STATUS1` 轮询没有任何间隔，会在最长 20 ms 内持续占用共享 I2C2。

MEMSIC MMC5603NJ Rev.B 数据手册规定默认 `BW=00` 测量时间为 6.6 ms，I2C Fast Mode 上限为 400 kHz。ICM42688-P 同样支持 Fast Mode 以上速率；当前共享总线的三颗器件均可在 400 kHz 工作。

## 可选方案

### 方案 A：保留忙轮询，只增加 I2C 重试

优点：改动最小。

缺点：会继续制造事务风暴，并用恢复计数掩盖根因；提高总线速率还会增加轮询次数。

### 方案 B：Take Measurement 后固定等待，再有界检查状态

优点：符合器件时序；把每次测量的状态读取从不确定的高频循环收敛到最多 3 次；实现小且无需新增线程或动态内存。

缺点：每 50 ms 会阻塞约 7 ms；后续更复杂调度仍可演进为非阻塞状态机。

### 方案 C：MMC5603 使用非阻塞状态机或连续测量模式

优点：主循环最坏延迟最小，适合更高采样率。

缺点：需要扩展 provider 生命周期、错误状态和调度测试；当前 20 Hz 目标尚不要求一次扩大到该复杂度。

### 方案 D：只修 ICM42688 parser，不处理共享总线

优点：能减少虚假的 skipped 计数。

缺点：第一轮板测已证明 recovery/failure/overflow 仍存在，不能形成闭环。

## 最终决策

采用方案 B，并同时收敛 ICM42688 FIFO 读取：

- ICM42688 使用 record-count、大端计数和 4-record watermark；当前 accel/gyro/temp 同 ODR，稳定数据包为 16 字节。
- parser 遇到 FIFO empty message 后立即结束，不再扫描后续 filler。
- EXTI 已证明 threshold/full 事件来源，运行期只读取 FIFO count，不再额外读取 `INT_STATUS`。
- I2C2 使用 400 kHz Fast Mode，降低批量 FIFO 事务占用时间。
- MMC5603 Take Measurement 后等待 7 ms，再最多检查 3 次状态，检查间隔 1 ms。
- 单次 ICM FIFO 失败仍累计诊断，但连续 3 次失败或 250 ms 无成功事件才进入 Mock fallback/reinit。

## 决策理由

该方案直接针对两类已取得证据的根因：empty-header 解析放大，以及 MMC5603 无间隔状态轮询造成的共享总线事务风暴。它不增加第三方依赖、线程或动态内存，保持 Stage 1 固定资源设计，并用已有累计 diagnostics 进行前后对比。

## 风险

- 7 ms 同步等待仍占用主循环，依赖 ICM FIFO 和 UART RX 环形缓冲吸收短时延迟。
- record-count 依赖当前 accel/gyro/temp 同 ODR 的 16 字节包配置；未来启用单传感器或 high-resolution 包时必须同步调整。
- 软件修复不能排除上拉、线长、边沿或电源导致的物理层 NACK；若复电后仍有跨设备失败，需要示波器/逻辑分析仪证据。

## 板端回归结果

完整断电重上电恢复了 ICM42688 初始化，首个 8 秒预检为 `0x01A9`；但该时点已经累计 I2C recovery 234、FIFO overflow 56、malformed 19。约 73 秒后的第二次预检以 `0x03A9` 拒绝正式测试，期间 recovery/failure/overflow/malformed/empty/skipped 分别增长 `190/2/58/15/16/531`，最近失败为 ICM42688 `FIFO_COUNTH(0x2E)` 读取。

因此，本 ADR 的 record-count、empty-marker、400 kHz 和 MMC5603 有界轮询仍是相对旧实现的有效软件收敛，但“已解决运行期共享 I2C2 根因”的验收结论被撤销。100 kHz 对照未改善；固定 128 B/64 B partial FIFO 读取反而增加 malformed/empty，也已撤回。临时原始包头证据为 `06 FF FF FF`，进一步把排查推进到单器件隔离、上拉/供电/布线和总线波形层面。完整数据与固件哈希见 TRB-024。

首次目标为 ICM-only 的人工接线复验仍出现 `0x30/STATUS1` 事务和 BMP390 帧，且正式固件会按三传感器设计访问已初始化的从机，无法形成可重复隔离条件。为此增加默认关闭的编译诊断模式：完全跳过 MMC5603/BMP390 初始化与轮询，并以 heartbeat bit `0x8000` 防止误作生产镜像。MP157 预检保留默认 `full` 门槛，只在显式 `icm42688_only` profile 下接受 `0x8181`，同时要求其他两类传感器 absent 和全部 I2C/FIFO/init/drop 累计值为零。该模式是诊断工具，不改变正式架构决策；隔离完成后必须刷回默认镜像。

## 后续 TODO

- ICM-only 诊断镜像的 `gsVcum` 8 秒 profile 得到 `0x8181` 和 init step 0，但 recovery/failure `660/105`、HAL timeout `0x20` 与 FIFO 累计异常使门槛失败；用户随后确认 COM6 未关闭，因此该结果只保留为受污染异常证据，不能作为干净完整复电结论。
- COM6 全程关闭后的 `XMWvGC` 干净复验仍在 diagnostics uptime 58298 ms 累计 recovery/failure `232/45`，最后失败为 `0x69/FIFO_DATA` 112 B read、HAL error `0x04 (ACK failure)`；因此 COM6 打开和其他从机周期访问均不是异常成立的必要条件。
- 当前没有万用表，已实现默认 400 kHz、只接受 100/400 kHz 的构建参数，并完成单器件 100 kHz 完整复电 A/B。`wB2xhs` 在相近窗口退化为 `0xD206/init step 1`，recovery/failure 速率为 400 kHz 的 `3.29/3.67` 倍，因此单纯降速方案被证伪。原 COM6 未枚举后用户确认设备改为 COM3；COM3 PnP/占用检查正常，400 kHz ICM-only 基线随后以 chip ID `0x0413` 完成写入、逐字节回读和 GO。
- 后续取得工具时仍需测量 ICM VCC/AD0、SCL/SDA 空闲电平、有效上拉、短线和共地，并抓取 `SIGNAL_PATH_RESET/FIFO_COUNTH/FIFO_DATA` 波形；100 kHz A/B 只排除了“400 kHz 单纯过快”，不能替代电气证据。
- 电气条件确认并通过单器件 8/30/60 秒零累计审计后，再刷回正式镜像逐颗接回从机。
- 短测零 failure/overflow/malformed 后，从零启动 3600 秒长测。
- 若 7 ms 主循环延迟影响后续功能，再以独立 ADR 评估 MMC5603 非阻塞状态机。
