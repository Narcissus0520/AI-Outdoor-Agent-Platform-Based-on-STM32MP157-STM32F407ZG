# AI Outdoor Agent Platform

AI Outdoor Agent Platform 是一个围绕 STM32MP157 + STM32F407ZG 的户外智能体长期作品集项目。当前仓库采用 Monorepo 结构，将 MP157 Linux 主控侧程序、F407 Sensor Hub 固件、公共协议和工具脚本分目录管理。

## 当前状态

- `mp157/outdoor-core-service/`: Linux Outdoor Core Runtime，已完成 Stage 0，并开始 Stage 1 Linux Runtime 侧 MCU 协议解析准备。
- `f407/sensor-hub/`: STM32F407ZG Sensor Hub 固件目录，目前只保留目录结构，暂不实现真实固件。
- `common/protocol/`: 后续放置 MP157 与 F407 共享的协议、公共头文件和 CRC 说明。
- `tools/`: 后续放置 PC 调试工具。
- `scripts/`: 放置仓库级构建脚本。

当前不包含 UI、HTTP API、AI Agent，也不假设真实 STM32F407ZG 固件或真实 IMU 已完成接入。

## 快速编译 MP157 Runtime

在仓库根目录执行：

```bash
./scripts/build_mp157.sh
```

也可以单独进入模块目录编译：

```bash
cd mp157/outdoor-core-service
cmake -S . -B build
cmake --build build
```

## 目录结构

```text
ai-outdoor-agent-platform/
├── README.md
├── AGENTS.md
├── common/
│   └── protocol/
├── docs/
├── f407/
│   └── sensor-hub/
├── mp157/
│   └── outdoor-core-service/
├── scripts/
└── tools/
```

详细说明见 [docs/repo_structure.md](docs/repo_structure.md)。

## 设计文档

- [仓库结构说明](docs/repo_structure.md)
- [项目设计文档](docs/project_design.md)
- [Stage 0 计划](docs/stage0_plan.md)
- [Stage 1 计划](docs/stage1_plan.md)
- [MCU 协议文档](docs/mcu_protocol.md)

## 开源协议

当前尚未添加 LICENSE 文件。后续计划补充开源协议。
