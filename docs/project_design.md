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
- `runtime_status.json` 输出 MCU 状态
- `runtime_status.json` 输出 IMU 状态
- `tools/frame_decoder` 十六进制 MCU 协议帧解析工具
- 最小 PowerShell 验证脚本
- F407 UART Bootloader 烧录、回读校验和 PC 侧串口帧抓取验证脚本

当前不包含：

- 完整 NMEA 协议覆盖
- 串口 GNSS 输入
- STM32F407ZG 与 MP157 的真实串口链路联调
- 真实传感器 HAL 外设绑定
- 真实 ICM42688/IMU 接入
- HTTP API
- UI
- AI 功能

## 当前阶段

当前处于 Stage 1：STM32F407ZG Sensor Hub 协议解析和状态管理。

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
├── log::Logger
└── input::IInputSource
```

后续会逐步加入 Unix domain socket 查询接口、串口输入和更多硬件输入源。

## 模块划分

- `mp157/outdoor-core-service/src/main.cpp`: 程序入口、命令行参数处理和当前主循环
- `mp157/outdoor-core-service/include/config`, `mp157/outdoor-core-service/src/config`: 应用配置结构和配置加载器
- `mp157/outdoor-core-service/include/gnss`, `mp157/outdoor-core-service/src/gnss`: GNSS 数据模型和最小 NMEA Parser
- `mp157/outdoor-core-service/include/ipc`, `mp157/outdoor-core-service/src/ipc`: 当前最小 IPC/状态发布原型
- `mp157/outdoor-core-service/include/mcu`, `mp157/outdoor-core-service/src/mcu`: MCU 协议常量、帧解析和 Sensor Hub 状态
- `common/protocol`: MP157 与 F407 共享协议常量、CRC16/MODBUS 和 IMU payload 定义
- `f407/sensor-hub`: F407 侧 Mock IMU Provider、IMU 协议帧打包和 ICM42688 占位接口
- `f407/sensor-hub/firmware`: F407 固件工作区，包含项目自有 app、BSP、协议帧构建和 Mock IMU Provider
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

f407/sensor-hub MockImuProvider
  -> McuFrameBuilder::buildImuFrame()
  -> MCU sensor_imu frame
  -> mp157/outdoor-core-service/data/mcu_mock_frames.txt
  -> MP157 Runtime mock replay

f407/sensor-hub/firmware sensor_hub_app_poll()
  -> board_get_tick_ms()
  -> mock_imu_provider_next()
  -> mcu_build_heartbeat_frame() / mcu_build_imu_frame()
  -> board_uart_send_bytes()
  -> HAL_UART_Transmit(USART1)
  -> PA9 TX, 115200 8N1
  -> PC UART capture validated by scripts/verify_f407_uart.ps1
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
- 当前支持 `nmea_input_path`、`mcu_mock_input_path`、`status_output_path` 和 `log_level`
- 命令行参数可覆盖配置文件中的输入路径和日志级别

## 构建与部署方案

构建使用 CMake：

```bash
cmake -S . -B build
cmake --build build
```

当前先面向本机 Linux 用户态构建，后续再补充 STM32MP157 交叉编译说明和工具链文件。

F407 固件采用双边界管理：

- CubeMX 负责生成芯片启动、HAL/CMSIS 和外设初始化基线。
- 项目自有业务代码保留在 `firmware/app`、`bsp`、`protocol` 和 `sensors`，避免散落到 Cube 生成目录。
- 仓库使用独立 GNU ARM CMake 构建直接引用 Cube 源文件，不把 Keil 工程作为唯一构建入口。
- 当前可生成 `sensor_hub.elf`、`.hex`、`.bin`、`.map` 并输出 Flash/RAM 使用量。
- 当前可通过 `scripts/flash_f407_uart.ps1` 使用 STM32 ROM UART Bootloader 烧录并回读校验 F407 固件，通过 `scripts/verify_f407_uart.ps1` 在 PC 侧验证 heartbeat 和 Mock IMU 帧。

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
- 能拒绝 CRC16 错误的 MCU 帧
- 能使用 `tools/frame_decoder` 解码十六进制 IMU 协议帧
- F407 firmware C 骨架能通过本机 C 语法检查
- CubeMX 基础工程的芯片、时钟、GPIO、HAL/CMSIS 文件清单有明确记录
- 能使用 GNU Arm Embedded Toolchain 编译并链接 F407 最小固件
- 能生成 ELF、HEX、BIN、map 和 size 输出
- F407 固件能通过 USART1 构建 heartbeat 和 Mock IMU 上报链路
- F407 固件能通过 UART Bootloader 烧录、回读校验，并在 PC 侧抓取 1 Hz heartbeat 和 10 Hz Mock IMU 帧
- 文件不存在时返回非 0 并输出错误日志

## 风险与 TODO

- 当前日志模块仍较简单，后续需要考虑文件输出和更完整的运行状态记录
- 当前配置格式只适合少量简单配置项
- 当前输入源只有文件回放，尚未覆盖串口输入
- 当前 NMEA Parser 未覆盖完整 NMEA 协议
- 当前 IPC 原型是状态文件发布，不是完整交互式 IPC 服务
- 当前 MP157 Runtime 仍从 mock frame 文件读取 MCU 数据；F407 板上固件已能通过 USART1 输出 heartbeat 和 Mock IMU 帧，但尚未接入 MP157 的真实 `/dev/tty*` 输入
- 当前 ICM42688 尚未到货，真实寄存器驱动未实现
- F407 USART1 和 HAL BSP 已接入，并已在 PC 侧通过 CH340 抓取验证二进制帧
- STM32CubeProgrammer 已安装；当前烧录流程采用仓库自带 UART Bootloader 脚本来控制一键下载电路时序，ST-LINK 未接入
- 当前 Runtime Manager 仍是顺序运行模型，尚未实现并发、健康检查或服务状态输出
