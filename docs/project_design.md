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
- `runtime_status.json` 输出 MCU 状态
- 最小 PowerShell 验证脚本

当前不包含：

- 完整 NMEA 协议覆盖
- 串口 GNSS 输入
- 真实 STM32F407ZG 固件
- 真实 IMU 接入
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
  -> McuStatus
  -> RuntimeStatus
  -> runtime/runtime_status.json
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

构建系统选型记录见 `docs/adr/0001-use-cmake.md`。
配置格式选型记录见 `docs/adr/0002-use-simple-key-value-config.md`。
Runtime 服务架构记录见 `docs/adr/0003-runtime-service-architecture.md`。
最小 NMEA Parser 记录见 `docs/adr/0004-minimal-nmea-parser.md`。
文件型 IPC 原型记录见 `docs/adr/0005-file-status-ipc-prototype.md`。
Unix domain socket 状态查询接口评估见 `docs/adr/0006-evaluate-unix-domain-status-query.md`。
MCU 帧协议记录见 `docs/adr/0007-mcu-frame-protocol.md`。

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
- 能拒绝 CRC16 错误的 MCU 帧
- 文件不存在时返回非 0 并输出错误日志

## 风险与 TODO

- 当前日志模块仍较简单，后续需要考虑文件输出和更完整的运行状态记录
- 当前配置格式只适合少量简单配置项
- 当前输入源只有文件回放，尚未覆盖串口输入
- 当前 NMEA Parser 未覆盖完整 NMEA 协议
- 当前 IPC 原型是状态文件发布，不是完整交互式 IPC 服务
- 当前 MCU 数据来自 mock frame 文件，不是 STM32F407ZG 真实固件
- 当前没有 CTest 测试用例，后续需要补充更标准的自动化测试
- 当前 Runtime Manager 仍是顺序运行模型，尚未实现并发、健康检查或服务状态输出
