# AI Outdoor Agent Platform

AI Outdoor Agent Platform 是一个围绕 STM32MP157 + STM32F407ZG 的户外智能体长期作品集项目，当前从 Embedded Linux 用户态 Runtime 和 GNSS 数据底座开始演进。

## 项目背景与动机

本项目面向嵌入式软件工程师作品集展示，目标是逐步体现 C/C++ 工程化、Linux Runtime Service、GNSS、Sensor Integration、IPC、异构 MCU 协作和 AI Terminal 相关能力。当前 Stage 0.1 只实现 Outdoor Core Runtime 的工程骨架与 NMEA 文件回放，不包含 NMEA Parser、串口、HTTP API、UI 或 AI 功能。

## 当前已实现功能与计划功能

- [x] CMake C++17 最小工程
- [x] 简单日志模块
- [x] 输入源抽象 `IInputSource`
- [x] NMEA 文件回放 `FileReplayInput`
- [x] 主循环从 `data/nmea_sample.txt` 逐行读取 NMEA 并打印日志
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
  -> IInputSource
      -> FileReplayInput
  -> Logger
```

主程序创建文件回放输入源，逐行读取 NMEA 文本，并通过日志模块输出。后续会在该基础上扩展 GNSS Parser、Runtime Manager、IPC 和更多输入源。

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
├── data/
│   └── nmea_sample.txt
├── docs/
│   ├── project_design.md
│   ├── stage0_plan.md
│   ├── dev_log.md
│   └── adr/
├── include/
│   ├── input/
│   └── log/
└── src/
    ├── input/
    ├── log/
    └── main.cpp
```

## 编译方法

```bash
cmake -S . -B build
cmake --build build
```

## 运行方法

使用默认样例文件：

```bash
./build/outdoor_core_runtime
```

指定 NMEA 文件：

```bash
./build/outdoor_core_runtime data/nmea_sample.txt
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

项目处于 Stage 0.1。当前只验证 Linux 用户态 C++ Runtime 的最小启动流程和 NMEA 文件回放能力，尚未接入真实硬件数据。

## 设计文档链接

- [项目设计文档](docs/project_design.md)
- [Stage 0 计划](docs/stage0_plan.md)
- [开发日志](docs/dev_log.md)
- [ADR-0001: 使用 CMake 管理 Stage 0 C++ 工程](docs/adr/0001-use-cmake.md)

## 项目亮点

- 面向 STM32MP157 + STM32F407ZG 的长期嵌入式作品集项目
- 从可编译、可运行的 Embedded Linux 用户态服务骨架开始
- 使用输入源抽象为后续 GNSS 文件回放、串口输入和测试打基础
- 明确区分已实现功能和计划功能，便于持续迭代展示

## 开源协议

当前尚未添加 LICENSE 文件。后续计划补充开源协议。
