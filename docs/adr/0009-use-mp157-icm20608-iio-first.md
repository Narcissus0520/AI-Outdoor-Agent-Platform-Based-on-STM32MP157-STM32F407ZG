# ADR-0009: Use MP157 Onboard ICM20608 First

## 状态

Accepted

## 背景

决策提出时，F407 侧 ICM42688 已完成 100 Hz 真实数据上报验证，但 F407 -> MP157 的真实串口链路仍未联调。MP157 开发板自身带有 ICM20608，正点原子资料中提供了字符设备、regmap 和 IIO 三类 SPI 示例。

为了尽快推进 MP157 Linux Runtime 的真实 Sensor Integration 能力，需要先在 MP157 本机完成一个可上板验证的传感器读取闭环，再把 F407 作为外部 Sensor Hub 接入。

2026-06-19 上板确认：当前板上系统的设备树包含 `/soc/spi@44004000/icm20608@0`，SPI 设备枚举为 `spi0.0`；`CONFIG_MEMS_ICM20608=m`，`modprobe icm20608` 加载 `/lib/modules/5.4.31-g886e225be/kernel/drivers/char/icm20608.ko` 后内核打印 `ICM20608 ID = 0XAE`。当前运行镜像实际可用入口是 `/dev/icm20608` 字符设备，而不是 IIO sysfs 设备。

同日继续确认：项目自有 `outdoor_core_runtime` 已使用 Windows host `arm-none-linux-gnueabihf-g++ 9.2.1` 交叉编译为 ARMv7 hard-float 可执行文件，并通过串口部署到 MP157。运行 `--board-imu --board-imu-source char_device --board-imu-device-path /dev/icm20608 --board-imu-samples 5` 后，`runtime_status.json` 输出 `board_imu.enabled=true`、`board_imu.seen=true`、`source=icm20608_char`、`sample_count=5`、`last_error=""`。

2026-06-20 后续确认：在完成 MP157 板载 ICM20608 验证后，F407 -> MP157 已按 `F407 UART4 PC10/PC11 <-> MP157 USART3 PD9/PD8` 完成最小串口联调。MP157 Runtime serial 模式读取 `/dev/ttySTM1` 后，`runtime_status.json` 输出 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true`、`mcu.status_flags=1` 和 `imu.seen=true`。

## 可选方案

### 方案 A：继续优先做 F407 -> MP157 串口联调

优点：

- 能直接连接 F407 Sensor Hub 与 MP157 Runtime。
- 延续当前 F407 ICM42688 里程碑。

缺点：

- 需要同时处理 Linux 串口配置、板间接线、协议流读取和运行时状态更新。
- 如果联调失败，难以快速区分是 MP157 Runtime、串口链路还是 F407 固件问题。

### 方案 B：先使用 MP157 板载 ICM20608 完成本机传感器读取，默认适配当前字符设备驱动

优点：

- 先验证 MP157 Linux 用户态读取真实传感器、转换数据和发布状态的能力。
- 不依赖 F407 串口链路，调试变量更少。
- 与当前板上运行镜像匹配，加载 `icm20608.ko` 后即可通过 `/dev/icm20608` 读取。

缺点：

- 与最终 F407 Sensor Hub 协作链路不同，后续仍需实现真实串口输入源。
- 正点原子字符设备驱动的 `read()` 返回 0，但仍会 copy 7 个 `signed int` 到用户缓冲，应用层需要显式兼容。
- IIO sysfs 路径更贴近 Linux 通用传感器抽象，当前只能作为后续可选路径保留。

### 方案 C：应用层直接使用 `/dev/icm20608` 字符设备

优点：

- 与正点原子字符设备示例一致，读取数据结构简单。

缺点：

- 应用层需要绑定自定义驱动返回的 `int[7]` 布局。
- 不如 IIO sysfs 适合作为 Linux 用户态传感器抽象的长期入口。

## 最终决策

采用方案 B。

MP157 Runtime 新增 `Icm20608CharReader`、`Icm20608IioReader` 和 `Icm20608Service`，默认配置关闭，MP157 上板时通过 `board_imu_enabled=true` 或 `--board-imu` 开启。默认 `board_imu_source=char_device`，读取 `/dev/icm20608`；IIO 可通过 `board_imu_source=iio` 显式切换。

状态输出新增 `board_imu` 字段，保持与 F407 MCU 协议帧输出的 `imu` 字段分离。

F407 -> MP157 真实串口对通暂后移到 MP157 板载 ICM20608 上板验证之后；该后续步骤已在 2026-06-20 完成最小闭环。

## 决策理由

该方案能更快证明 MP157 侧具备真实传感器读取、单位换算、Runtime Service 接入和状态发布能力，同时降低当前阶段调试耦合度。由于当前运行镜像已经提供 `drivers/char/icm20608.ko` 并能识别硬件 ID，先适配字符设备是最小可验证路径；IIO 路径作为更通用的 Linux 传感器抽象保留。

## 风险

- 字符设备驱动需要先 `modprobe icm20608`，否则 `/dev/icm20608` 不存在。
- 正点原子资料目录中的交叉编译器包面向 Linux host；当前 Windows PowerShell 环境需要使用 Windows host 版 `arm-none-linux-gnueabihf` 工具链，或改用 WSL/Linux SDK。
- 如果后续切换 IIO 驱动，真实 MP157 板上的 IIO 设备可能不是 `iio:device0`。
- 当前实现是有限样本读取，不是长期守护式采集循环。

## 后续 TODO

- 将当前手动交叉编译和串口部署流程沉淀为脚本或文档化命令。
- 已完成 F407 -> MP157 串口输入源最小验证；后续继续实现 MP157 -> F407 下行控制协议、长期运行采集和稳定性验证。
