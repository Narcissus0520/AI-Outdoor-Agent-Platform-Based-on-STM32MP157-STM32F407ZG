# Project Design

## 项目目标

AI Outdoor Agent Platform 目标是围绕 STM32MP157 + STM32F407ZG 构建一个户外智能体平台，逐步体现 Embedded Linux、C/C++ 工程化、GNSS、Sensor Integration、IPC、Runtime Service 和 AI Terminal 能力。

## 项目范围

当前 Stage 1 范围：

- CMake C++17 工程骨架
- 简单日志模块与日志级别配置
- 基础 `key=value` 配置文件加载
- 输入源抽象 `IInputSource`
- NMEA 文件回放 `FileReplayInput`
- Runtime Service 生命周期接口 `IService`
- Runtime Manager 顺序启动、运行、停止服务
- GNSS 文件回放服务 `GnssReplayService`
- GNSS 基础数据模型 `GnssFix`
- 最小 NMEA Parser，支持 RMC/GGA 基础字段和 checksum 校验
- 主程序组装 Runtime，并由服务读取配置指定的 NMEA 文件和输出基础定位状态
- 文件型 Runtime 状态发布原型
- Runtime 基础状态输出
- MCU 协议常量、帧结构和 CRC16/MODBUS 校验
- `McuFrameParser`
- `McuStatus`
- Heartbeat mock 帧解析
- Mock sensor 帧解析
- Mock IMU payload 定义、F407 mock 打包和 MP157 解析
- F407 `firmware/` 固件应用骨架和 STM32CubeMX/HAL 基础工程
- F407 I2C2 读取 ICM42688 的固件数据源，已完成上板验证，失败时回退 Mock IMU
- `runtime_status.json` 输出 MCU 状态
- `runtime_status.json` 输出 IMU 状态
- `tools/frame_decoder` 十六进制 MCU 协议帧解析工具
- 最小 PowerShell 验证脚本
- F407 UART Bootloader 烧录、回读校验和 PC 侧串口帧抓取验证脚本
- MP157 板载 ICM20608 字符设备读取服务，保留 IIO 可选读取路径
- MP157 live serial 输入路径，默认按 USART3 `/dev/ttySTM1` 读取 F407 UART4 二进制 MCU 帧，并已完成上板验证
- MP157 -> F407 最小下行命令 `command_ping -> command_ack` 软件路径
- `runtime_status.json` 输出 `board_imu` 状态
- MP157 板载 ICM20608 fake character-device / fake-IIO 自动化测试和 Runtime 冒烟验证

当前不包含：

- 完整 NMEA 协议覆盖
- 串口 GNSS 输入
- MP157 反向控制 F407 的完整命令集、权限/状态机和长期可靠性策略
- 更多真实外置传感器 HAL 外设绑定
- HTTP API
- UI
- AI 功能

## 当前阶段

当前处于 Stage 1：Sensor Integration 与 STM32F407ZG Sensor Hub 协议解析。MP157 板载 ICM20608 的 Linux 用户态读取和上板验证已经完成；F407 -> MP157 串口输入已按 `F407 UART4 PC10/PC11 <-> MP157 USART3 PD9/PD8` 完成最小上板验收。MP157 -> F407 方向当前实现最小 `ping -> ack`，并已通过 MP157 shell raw 帧方式完成真实接线验证；新版 ARM Runtime `--mcu-command ping` 板端复测仍待完成。当前板上实测 ICM20608 默认入口为 `/dev/icm20608` 字符设备，IIO 路径保留为可选来源。

## 硬件平台

- STM32MP157 开发板：后续 Embedded Linux 部署目标
- STM32F407ZG 开发板：后续异构协作目标
- 7 英寸 RGB 屏：尚未适配
- UBLOX-M10 GNSS 开发板：当前未接入真实串口，仅使用 NMEA 文件样例

## 软件架构

当前软件架构保持最小可运行：

```text
Outdoor Core Runtime
├── main loop
├── config::ConfigLoader
├── runtime::RuntimeManager
├── ipc::StatusPublisher
├── runtime::IService
│   └── services::GnssReplayService
│       ├── gnss::NmeaParser
│       ├── gnss::GnssFix
│       └── input::FileReplayInput
│   └── services::McuMockService
│       ├── mcu::McuFrameParser
│       ├── mcu::ImuPayloadParser
│       └── mcu::McuStatus
│   └── services::McuSerialService
│       ├── mcu::McuCommand
│       ├── mcu::McuFrameStreamDecoder
│       ├── mcu::McuFrameParser
│       ├── mcu::ImuPayloadParser
│       └── mcu::McuStatus
│   └── services::Icm20608Service
│       ├── sensors::Icm20608CharReader
│       ├── sensors::Icm20608IioReader
│       └── sensors::BoardImuStatus
├── log::Logger
└── input::IInputSource
```

后续会逐步加入 Unix domain socket 查询接口、串口长期服务化、更多硬件输入源和更完整的 MCU 控制命令集。

## 模块划分

- `mp157/outdoor-core-service/src/main.cpp`: 程序入口、命令行参数处理和当前主循环
- `mp157/outdoor-core-service/include/config`, `mp157/outdoor-core-service/src/config`: 应用配置结构和配置加载器
- `mp157/outdoor-core-service/include/gnss`, `mp157/outdoor-core-service/src/gnss`: GNSS 数据模型和最小 NMEA Parser
- `mp157/outdoor-core-service/include/ipc`, `mp157/outdoor-core-service/src/ipc`: 当前最小 IPC/状态发布原型
- `mp157/outdoor-core-service/include/mcu`, `mp157/outdoor-core-service/src/mcu`: MCU 协议常量、帧解析和 Sensor Hub 状态
- `mp157/outdoor-core-service/include/sensors`, `mp157/outdoor-core-service/src/sensors`: MP157 板载传感器读取抽象，当前实现 ICM20608 character-device reader 和 IIO sysfs reader
- `common/protocol`: MP157 与 F407 共享协议常量、CRC16/MODBUS 和 IMU payload 定义
- `f407/sensor-hub`: F407 侧 Mock IMU Provider、IMU 协议帧打包和真实 ICM42688 数据源
- `f407/sensor-hub/firmware`: F407 固件工作区，包含项目自有 app、BSP、协议帧构建、ICM42688 数据源和 Mock IMU fallback
- `f407/sensor-hub/firmware/stm32cube`: CubeMX 生成的 STM32F407ZG HAL/CMSIS 基础工程和 `.ioc` 配置
- `f407/sensor-hub/firmware/cmake`, `linker`, `startup`, `platform`: GNU ARM 工具链、内存布局、启动和最小 newlib 系统调用支持
- `tools/frame_decoder`: 十六进制 MCU 协议帧解析工具
- `mp157/outdoor-core-service/include/log`, `mp157/outdoor-core-service/src/log`: 日志模块和日志级别过滤
- `mp157/outdoor-core-service/include/runtime`, `mp157/outdoor-core-service/src/runtime`: 服务生命周期接口和 Runtime Manager
- `mp157/outdoor-core-service/include/services`, `mp157/outdoor-core-service/src/services`: 当前 Runtime 服务实现
- `mp157/outdoor-core-service/include/input`, `mp157/outdoor-core-service/src/input`: 输入源接口和 NMEA 文件回放实现
- `mp157/outdoor-core-service/config/runtime.conf`: Stage 0 默认运行配置
- `mp157/outdoor-core-service/data/nmea_sample.txt`: Stage 0 样例 NMEA 数据
- `mp157/outdoor-core-service/data/nmea_edge_cases.txt`: checksum、无效定位和南/西半球坐标边界样例
- `mp157/outdoor-core-service/data/mcu_mock_frames.txt`: Stage 1 MCU mock heartbeat 和 mock sensor 帧
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`: 当前 Windows 开发环境下的最小验证脚本

## 数据流

```text
mp157/outdoor-core-service/config/runtime.conf
  -> ConfigLoader
  -> AppConfig
  -> StatusPublisher

mp157/outdoor-core-service/data/nmea_sample.txt
  -> FileReplayInput::readLine()
  -> GnssReplayService::run()
  -> NmeaParser::parseLine()
  -> GnssFix
  -> RuntimeManager::run()
  -> Logger::info()
  -> stdout

RuntimeManager::status()
  -> StatusPublisher::publish()
  -> runtime/runtime_status.json

mp157/outdoor-core-service/data/mcu_mock_frames.txt
  -> McuMockService::run()
  -> McuFrameParser::parseFrame()
  -> ImuPayloadParser::parse()
  -> McuStatus
  -> RuntimeStatus
  -> runtime/runtime_status.json

MP157 live serial path
  -> F407 PC10 UART4_TX
  -> MP157 PD9 USART3_RX
  -> /dev/ttySTM1, 115200 8N1, raw mode
  -> services::McuSerialService
  -> McuFrameStreamDecoder
  -> McuFrameParser::parseFrame()
  -> ImuPayloadParser::parse()
  -> McuStatus
  -> runtime/runtime_status.json

MP157 downlink command path
  -> --mcu-command ping / mcu_command = ping
  -> services::McuSerialService
  -> mcu::buildPingCommandFrame()
  -> /dev/ttySTM1, 115200 8N1, raw mode
  -> MP157 PD8 USART3_TX
  -> F407 PC11 UART4_RX
  -> board_uart_receive_byte()
  -> mcu_command_decoder_feed_byte()
  -> mcu_build_command_ack_frame()
  -> F407 PC10 UART4_TX
  -> MP157 PD9 USART3_RX
  -> McuFrameParser::applyFrame()
  -> runtime_status.json command_ack_*

MP157 onboard ICM20608 path
  -> modprobe icm20608
  -> /dev/icm20608
  -> sensors::Icm20608CharReader
  -> services::Icm20608Service
  -> sensors::BoardImuStatus
  -> runtime/runtime_status.json board_imu

Optional MP157 onboard ICM20608 IIO path
  -> /sys/bus/iio/devices/iio:device*/in_accel_*_raw
  -> /sys/bus/iio/devices/iio:device*/in_anglvel_*_raw
  -> /sys/bus/iio/devices/iio:device*/in_temp_*
  -> sensors::Icm20608IioReader
  -> services::Icm20608Service
  -> sensors::BoardImuStatus
  -> runtime/runtime_status.json board_imu

f407/sensor-hub MockImuProvider
  -> McuFrameBuilder::buildImuFrame()
  -> MCU sensor_imu frame
  -> mp157/outdoor-core-service/data/mcu_mock_frames.txt
  -> MP157 Runtime mock replay

f407/sensor-hub/firmware sensor_hub_app_poll()
  -> board_get_tick_ms()
  -> icm42688_provider_read() or mock_imu_provider_next()
  -> 10 ms IMU poll period, nominal 100 Hz
  -> board_uart_receive_byte() polls command bytes
  -> mcu_command_decoder_feed_byte() handles command_ping
  -> mcu_build_heartbeat_frame() / mcu_build_imu_frame() / mcu_build_command_ack_frame()
  -> board_uart_send_bytes()
  -> HAL_UART_Transmit(UART4)
  -> PC10 TX, 115200 8N1
  -> MP157 USART3 /dev/ttySTM1 live serial validation

ICM42688 I2C sensor path
  -> PB10 I2C2_SCL
  -> PB11 I2C2_SDA
  -> PB12 ICM42688_INT1 input
  -> sensor_hub_app_init()
  -> register bank 0
  -> DEVICE_CONFIG soft reset
  -> HAL_I2C_Mem_Read/Write(I2C2, 0x69 << 1)
  -> WHO_AM_I 0x47 check
  -> ACCEL_CONFIG0/GYRO_CONFIG0 range + 100 Hz ODR
  -> PWR_MGMT0 accel/gyro enable
  -> sensor_hub_app_poll()
  -> 10 ms burst read from TEMP_DATA1
  -> 100 Hz accel/gyro sample
  -> MCU sensor_imu frame

Validated MP157 live serial board link
  -> F407 PC10 UART4_TX to MP157 PD9 USART3_RX
  -> F407 PC11 UART4_RX from MP157 PD8 USART3_TX
  -> common GND
  -> /dev/ttySTM1 raw capture shows A5 5A MCU frame headers
  -> runtime_status.json: heartbeat_seen=true, imu_seen=true, status_flags=1
```

当前输出原始 NMEA 文本，并对通过 checksum 校验的 RMC/GGA 输出基础 GNSS 定位状态。

## IPC 方案

Stage 0.5 使用文件型状态发布作为最小 IPC 原型，Stage 1 后默认输出 `runtime/runtime_status.json`。Unix domain socket 状态查询接口已完成方案评估，暂不在 Stage 0 实现，后续在 STM32MP157 Linux 部署阶段继续推进。

## 日志与配置方案

日志：

- 当前使用 `Logger` 输出时间戳、级别和消息
- 支持 `debug`、`info`、`warn`、`error` 最小日志级别过滤
- 当前只输出到 stdout/stderr

配置：

- 当前使用 `config/runtime.conf`
- 配置格式为简单 `key=value`
- 当前支持 `nmea_input_path`、`mcu_input_mode`、`mcu_mock_input_path`、`mcu_serial_device`、`mcu_serial_baud`、`mcu_serial_capture_seconds`、`mcu_command`、`status_output_path` 和 `log_level`
- 当前支持 `board_imu_enabled`、`board_imu_source`、`board_imu_device_path`、`board_imu_iio_path`、`board_imu_sample_count` 和 `board_imu_sample_interval_ms`，默认关闭板载 IMU，MP157 上板时显式开启
- 命令行参数可覆盖配置文件中的输入路径和日志级别

## 构建与部署方案

构建使用 CMake：

```bash
cmake -S . -B build
cmake --build build
```

当前本机开发构建使用 CMake；MP157 上板验证已使用 Windows host 的 `arm-none-linux-gnueabihf-g++ 9.2.1` 交叉编译 ARMv7 hard-float 可执行文件，并通过串口传输到板端运行。正点原子资料目录中的 `gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz` 与 OpenSTLinux SDK 脚本面向 Linux host，当前 Windows PowerShell 环境不能直接原生使用。

F407 固件采用双边界管理：

- CubeMX 负责生成芯片启动、HAL/CMSIS 和外设初始化基线。
- 项目自有业务代码保留在 `firmware/app`、`bsp`、`protocol` 和 `sensors`，避免散落到 Cube 生成目录。
- 仓库使用独立 GNU ARM CMake 构建直接引用 Cube 源文件，不把 Keil 工程作为唯一构建入口。
- 当前可生成 `sensor_hub.elf`、`.hex`、`.bin`、`.map` 并输出 Flash/RAM 使用量。
- 当前可通过 `scripts/flash_f407_uart.ps1` 使用 STM32 ROM UART Bootloader 烧录并回读校验 F407 固件；应用态 Sensor Hub 帧通过 UART4 输出到 MP157 USART3，PC 侧 `verify_f407_uart.ps1` 仅适用于仍从 USART1 输出协议帧的历史固件。

F407 上电后工作流程：

- CubeMX 生成代码负责 HAL、时钟、GPIO、I2C2、USART1 和 UART4 初始化。
- 项目 USER CODE 区域只调用 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()`，业务逻辑保留在 `firmware/app`、`bsp`、`protocol` 和 `sensors`。
- `sensor_hub_app_init()` 完成 ICM42688 软复位、`WHO_AM_I=0x47` 检查、accel/gyro 100 Hz ODR 配置和工作模式使能。
- `sensor_hub_app_poll()` 负责 UART4 RX 命令轮询、500 ms LED 心跳、1 Hz heartbeat 帧、100 Hz IMU burst read 和 `sensor_imu` 帧发送。
- ICM42688 初始化或读取失败时，当前回退到 Mock IMU 并通过 heartbeat `status_flags` 暴露状态。

构建系统选型记录见 `docs/adr/0001-use-cmake.md`。
配置格式选型记录见 `docs/adr/0002-use-simple-key-value-config.md`。
Runtime 服务架构记录见 `docs/adr/0003-runtime-service-architecture.md`。
最小 NMEA Parser 记录见 `docs/adr/0004-minimal-nmea-parser.md`。
文件型 IPC 原型记录见 `docs/adr/0005-file-status-ipc-prototype.md`。
Unix domain socket 状态查询接口评估见 `docs/adr/0006-evaluate-unix-domain-status-query.md`。
MCU 帧协议记录见 `docs/adr/0007-mcu-frame-protocol.md`。
F407 Cube 源码与 CMake 构建边界见 `docs/adr/0008-f407-cube-source-and-cmake-build-boundary.md`。

## 验收用例

- 能完成 CMake 配置
- 能编译生成 `outdoor_core_runtime`
- 不传参数时能读取 `mp157/outdoor-core-service/config/runtime.conf`
- 配置文件能指定默认 NMEA 输入路径
- 命令行能覆盖 NMEA 输入路径
- 命令行能覆盖日志级别
- Runtime Manager 能启动、运行并停止 GNSS 文件回放服务
- 能解析样例 RMC/GGA 并输出基础定位状态
- 能拒绝 checksum 错误的 NMEA 语句
- 能生成 Runtime 状态文件并输出基础状态字段
- 能解析 MCU heartbeat 帧并更新 `McuStatus`
- 能解析 MCU mock sensor 帧并输出到 `runtime_status.json`
- 能解析 Mock IMU 帧并输出 `runtime_status.json` 的 `imu` 字段
- 能从 MCU 字节流中重组前导噪声、分片帧和连续帧，再复用现有 MCU parser
- 默认 PC 配置下能输出 `runtime_status.json` 的 `board_imu` 字段，且 `enabled=false`
- 能通过 fake character-device 文件验证 ICM20608 character reader 的 accel/gyro/temp 换算
- 能通过 fake-IIO 目录验证 ICM20608 IIO reader 的 accel/gyro/temp 换算
- 在启用 `--board-imu` 且提供 `/dev/icm20608` 字符设备路径时，能读取 ICM20608 并输出 `board_imu.seen=true`；当前已在真实 MP157 板上验证 `sample_count=5`、`last_error=""`、静置 Z 轴约 `0.983g`
- 能拒绝 CRC16 错误的 MCU 帧
- 能使用 `tools/frame_decoder` 解码十六进制 IMU 协议帧
- F407 firmware C 骨架能通过本机 C 语法检查
- CubeMX 基础工程的芯片、时钟、GPIO、HAL/CMSIS 文件清单有明确记录
- 能使用 GNU Arm Embedded Toolchain 编译并链接 F407 最小固件
- 能生成 ELF、HEX、BIN、map 和 size 输出
- F407 固件能通过 UART4 构建 heartbeat 和 IMU 上报链路，并通过 MP157 USART3 `/dev/ttySTM1` 读取
- MP157 Runtime serial 模式能构建并发送 `command_ping`，F407 固件能解析该命令并返回 `command_ack`；当前该能力已通过软件构建和单元测试，物理链路已通过 MP157 shell raw ping/ack 验证，新版 ARM Runtime 板端复测仍待完成
- F407 固件能通过 USART1 UART Bootloader 烧录和回读校验；USART1 当前保留为下载链路
- MP157 Runtime serial 模式能读取真实 F407 UART4 帧，并输出 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true` 和真实 IMU 字段
- F407 固件能通过 I2C2 初始化 ICM42688，读取 100 Hz accel/gyro/temp 数据并打包为 `sensor_imu` 帧；若 I2C 初始化或读取失败，回退 Mock IMU 并通过 heartbeat `status_flags` 标记。当前已通过 `status_flags=0x0001` 的串口抓包确认真实 ICM42688 数据路径。
- 文件不存在时返回非 0 并输出错误日志

## 风险与 TODO

- 当前日志模块仍较简单，后续需要考虑文件输出和更完整的运行状态记录
- 当前配置格式只适合少量简单配置项
- 当前 MCU 输入源支持 mock 文件和 Linux 串口两种模式；F407 UART4 -> MP157 USART3 最小上板验证已完成，MP157 -> F407 方向已完成 raw ping/ack 上板验证，后续需要补充新版 ARM Runtime `--mcu-command ping` 板端复测、长期稳定性和完整命令集设计
- 当前 NMEA Parser 未覆盖完整 NMEA 协议
- 当前 IPC 原型是状态文件发布，不是完整交互式 IPC 服务
- 当前 MP157 板载 ICM20608 读取服务已实现并通过 fake character-device / fake-IIO 验证；真实 MP157 板上已确认设备树、SPI 设备和 `modprobe icm20608` 可识别 `ICM20608 ID = 0XAE`，并用正点原子 `icm20608App` 以及项目自有 ARM Runtime 通过 `/dev/icm20608` 读到真实 accel/gyro/temp 数据
- 当前 MP157 Runtime 默认仍从 mock frame 文件读取 MCU 数据；serial 模式已按 `/dev/ttySTM1` 完成 F407 UART4 真实接线验证
- 当前 ICM42688 I2C 固件路径已实现，并在修正 SCL/SDA 接线后完成 F407 上板串口抓包验证
- F407 USART1 保留为 UART Bootloader 下载口；HAL BSP 当前使用 UART4 发送应用态二进制帧
- 当前 100 Hz IMU 上报版本已完成上板抓包验证，5 秒抓取 IMU 501 帧，`imu_rate_hz=100.2`
- ICM42688 INT1 当前只作为普通输入封装，尚未配置传感器数据就绪中断和 EXTI 驱动
- ICM42688 FIFO 当前未启用；固件直接按周期 burst 读取最新样本
- I2C 瞬时错误后的恢复策略仍较简单，当前会回退 Mock IMU 并通过 heartbeat `status_flags` 标记
- STM32CubeProgrammer 已安装；当前烧录流程采用仓库自带 UART Bootloader 脚本来控制一键下载电路时序，ST-LINK 未接入
- 当前 Runtime Manager 仍是顺序运行模型，尚未实现并发、健康检查或服务状态输出
