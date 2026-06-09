# Project Design

## 项目目标

AI Outdoor Agent Platform 目标是围绕 STM32MP157 + STM32F407ZG 构建一个户外智能体平台，逐步体现 Embedded Linux、C/C++ 工程化、GNSS、Sensor Integration、IPC、Runtime Service 和 AI Terminal 能力。

## 项目范围

当前 Stage 0.5 范围：

- CMake C++17 工程骨架
- 简单日志模块与日志级别配置
- 基础 `key=value` 配置文件加载
- 输入源抽象 `IInputSource`
- NMEA 文件回放 `FileReplayInput`
- Runtime Service 生命周期接口 `IService`
- Runtime Manager 顺序启动、运行、停止服务
- GNSS 文件回放服务 `GnssReplayService`
- GNSS 基础数据模型 `GnssFix`
- 最小 NMEA Parser，支持 RMC/GGA 基础字段
- 主程序组装 Runtime，并由服务读取配置指定的 NMEA 文件和输出基础定位状态
- 文件型 Runtime 状态发布原型
- Runtime 基础状态输出
- 最小 PowerShell 验证脚本

当前不包含：

- 完整 NMEA 协议覆盖
- NMEA checksum 校验
- 串口 GNSS 输入
- HTTP API
- UI
- AI 功能

## 当前阶段

当前处于 Stage 0.5：IPC 原型与基础运行状态输出。

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
├── log::Logger
└── input::IInputSource
```

后续会逐步加入 checksum 校验、IPC 和更多硬件输入源。

## 模块划分

- `src/main.cpp`: 程序入口、命令行参数处理和当前主循环
- `include/config`, `src/config`: 应用配置结构和配置加载器
- `include/gnss`, `src/gnss`: GNSS 数据模型和最小 NMEA Parser
- `include/ipc`, `src/ipc`: 当前最小 IPC/状态发布原型
- `include/log`, `src/log`: 日志模块和日志级别过滤
- `include/runtime`, `src/runtime`: 服务生命周期接口和 Runtime Manager
- `include/services`, `src/services`: 当前 Runtime 服务实现
- `include/input`, `src/input`: 输入源接口和 NMEA 文件回放实现
- `config/runtime.conf`: Stage 0 默认运行配置
- `data/nmea_sample.txt`: Stage 0 样例 NMEA 数据
- `scripts/verify_runtime.ps1`: 当前 Windows 开发环境下的最小验证脚本

## 数据流

```text
config/runtime.conf
  -> ConfigLoader
  -> AppConfig
  -> StatusPublisher

data/nmea_sample.txt
  -> FileReplayInput::readLine()
  -> GnssReplayService::run()
  -> NmeaParser::parseLine()
  -> GnssFix
  -> RuntimeManager::run()
  -> Logger::info()
  -> stdout

RuntimeManager::status()
  -> StatusPublisher::publish()
  -> runtime/status.txt
```

当前输出原始 NMEA 文本，并对 RMC/GGA 输出基础 GNSS 定位状态。

## IPC 方案

Stage 0.5 使用文件型状态发布作为最小 IPC 原型，默认输出 `runtime/status.txt`。后续计划继续评估 Unix domain socket、pipe 或轻量本地文本协议，避免过早引入复杂依赖。

## 日志与配置方案

日志：

- 当前使用 `Logger` 输出时间戳、级别和消息
- 支持 `debug`、`info`、`warn`、`error` 最小日志级别过滤
- 当前只输出到 stdout/stderr

配置：

- 当前使用 `config/runtime.conf`
- 配置格式为简单 `key=value`
- 当前支持 `nmea_input_path`、`status_output_path` 和 `log_level`
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

## 验收用例

- 能完成 CMake 配置
- 能编译生成 `outdoor_core_runtime`
- 不传参数时能读取 `config/runtime.conf`
- 配置文件能指定默认 NMEA 输入路径
- 命令行能覆盖 NMEA 输入路径
- 命令行能覆盖日志级别
- Runtime Manager 能启动、运行并停止 GNSS 文件回放服务
- 能解析样例 RMC/GGA 并输出基础定位状态
- 能生成 Runtime 状态文件并输出基础状态字段
- 文件不存在时返回非 0 并输出错误日志

## 风险与 TODO

- 当前日志模块仍较简单，后续需要考虑文件输出和更完整的运行状态记录
- 当前配置格式只适合少量简单配置项
- 当前输入源只有文件回放，尚未覆盖串口输入
- 当前 NMEA Parser 未做 checksum 校验，也未覆盖完整 NMEA 协议
- 当前 IPC 原型是状态文件发布，不是完整交互式 IPC 服务
- 当前没有 CTest 测试用例，后续需要补充更标准的自动化测试
- 当前 Runtime Manager 仍是顺序运行模型，尚未实现并发、健康检查或服务状态输出
