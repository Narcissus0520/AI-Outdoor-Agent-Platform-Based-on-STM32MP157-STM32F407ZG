# AI Outdoor Agent Platform

AI Outdoor Agent Platform 是一个围绕 STM32MP157 + STM32F407ZG 的户外智能体长期作品集项目，当前从 Embedded Linux 用户态 Runtime 和 GNSS 数据底座开始演进。

## 项目背景与项目动机

本项目面向嵌入式软件工程师作品集展示，目标是逐步体现 C/C++ 工程化、Linux Runtime Service、GNSS、Sensor Integration、IPC、异构 MCU 协作和 AI Terminal 相关能力。

当前处于 Stage 0.3，只实现 Outdoor Core Runtime 的工程骨架、NMEA 文件回放、基础配置加载、日志级别控制和 Runtime Service 生命周期骨架。不包含 NMEA Parser、串口、HTTP API、UI 或 AI 功能。

## 当前已实现功能与计划功能

- [x] CMake C++17 最小工程
- [x] 简单日志模块
- [x] 日志级别配置
- [x] 基础配置文件加载
- [x] 输入源抽象 `IInputSource`
- [x] NMEA 文件回放 `FileReplayInput`
- [x] 主循环从配置指定的 NMEA 文件逐行读取并打印日志
- [x] Runtime Service 生命周期接口
- [x] Runtime Manager 顺序启动、运行、停止服务
- [x] GNSS 文件回放服务 `GnssReplayService`
- [x] 最小 Runtime PowerShell 验证脚本
- [ ] NMEA Parser
- [ ] 串口 GNSS 输入
- [ ] IPC 原型
- [ ] Runtime Service 管理机制
- [ ] Sensor Integration
- [ ] STM32F407ZG 协作通信
- [ ] AI Terminal 能力

## 系统架构说明

当前架构保持最小化：

```text
main.cpp
  -> config::ConfigLoader
  -> runtime::RuntimeManager
      -> runtime::IService
          -> services::GnssReplayService
              -> input::IInputSource
                  -> input::FileReplayInput
  -> log::Logger
```

主程序先加载 `config/runtime.conf`，再应用命令行覆盖项，随后创建 Runtime Manager 并注册 GNSS 文件回放服务。服务按 `start/run/stop` 生命周期读取 NMEA 文本，并通过日志模块输出。后续会在该基础上扩展 GNSS Parser、IPC 和更多输入源。

## 硬件平台说明

当前目标硬件与计划硬件：

- STM32MP157 开发板，作为 Embedded Linux 主控平台
- STM32F407ZG 开发板，计划用于后续 MCU 协作
- 7 英寸 RGB 屏，分辨率 1024x600，尚未适配
- UBLOX-M10 GNSS 开发板，当前只使用 NMEA 文件样例模拟数据

## 软件技术栈说明

- C++17
- CMake
- Embedded Linux 用户态服务设计
- POSIX/Linux 部署方向
- 后续计划扩展 UART、IPC、Sensor Integration 和 Runtime Service

## 项目目录结构

```text
ai-outdoor-agent-platform/
├── CMakeLists.txt
├── README.md
├── AGENTS.md
├── config/
│   └── runtime.conf
├── data/
│   └── nmea_sample.txt
├── docs/
│   ├── project_design.md
│   ├── stage0_plan.md
│   ├── dev_log.md
│   └── adr/
├── include/
│   ├── config/
│   ├── input/
│   ├── log/
│   ├── runtime/
│   └── services/
├── scripts/
└── src/
    ├── config/
    ├── input/
    ├── log/
    ├── runtime/
    ├── services/
    └── main.cpp
```

## 编译方法

```bash
cmake -S . -B build
cmake --build build
```

## 运行方法

使用默认配置：

```bash
./build/outdoor_core_runtime
```

指定配置文件：

```bash
./build/outdoor_core_runtime --config config/runtime.conf
```

覆盖 NMEA 输入文件和日志级别：

```bash
./build/outdoor_core_runtime --input data/nmea_sample.txt --log-level info
```

兼容 Stage 0.1 的直接文件路径用法：

```bash
./build/outdoor_core_runtime data/nmea_sample.txt
```

## 配置文件

当前配置文件为 `config/runtime.conf`：

```text
nmea_input_path = data/nmea_sample.txt
log_level = info
```

支持的日志级别：

- `debug`
- `info`
- `warn`
- `error`

## 验证方法

当前环境如果没有安装 CMake，可使用本地 `g++` 验证脚本：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

## 开发路线图

- Stage 0.1: Outdoor Core Runtime 工程骨架与 NMEA 文件回放
- Stage 0.2: 日志与配置模块增强
- Stage 0.3: Runtime Manager 与 Service 抽象
- Stage 0.4: GNSS mock 服务与 NMEA Parser
- Stage 0.5: IPC 原型与基础运行状态输出
- Stage 1: 接入 UBLOX-M10 串口数据
- Stage 2: Sensor Integration 与 STM32F407ZG 协作
- Stage 3: AI Terminal 能力探索

## 当前项目状态

项目处于 Stage 0.3。当前只验证 Linux 用户态 C++ Runtime 的最小启动流程、配置加载、日志级别控制、服务生命周期和 NMEA 文件回放能力，尚未接入真实硬件数据。

## 设计文档链接

- [项目设计文档](docs/project_design.md)
- [Stage 0 计划](docs/stage0_plan.md)
- [开发日志](docs/dev_log.md)
- [ADR-0001: 使用 CMake 管理 Stage 0 C++ 工程](docs/adr/0001-use-cmake.md)
- [ADR-0002: 使用简单 key=value 配置格式](docs/adr/0002-use-simple-key-value-config.md)
- [ADR-0003: 使用 Runtime Manager 管理服务生命周期](docs/adr/0003-runtime-service-architecture.md)

## 项目亮点

- 面向 STM32MP157 + STM32F407ZG 的长期嵌入式作品集项目
- 从可编译、可运行的 Embedded Linux 用户态服务骨架开始
- 使用输入源抽象为后续 GNSS 文件回放、串口输入和测试打基础
- 使用轻量配置和日志模块推进 Runtime Service 工程化
- 使用 `IService` 和 `RuntimeManager` 建立后续服务化演进基础
- 明确区分已实现功能和计划功能，便于持续迭代展示

## 开源协议

当前尚未添加 LICENSE 文件。后续计划补充开源协议。
