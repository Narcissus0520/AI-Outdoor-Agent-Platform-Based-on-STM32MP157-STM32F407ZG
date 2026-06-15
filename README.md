# AI Outdoor Agent Platform

AI Outdoor Agent Platform 是一个围绕 STM32MP157 + STM32F407ZG 的户外智能体长期作品集项目。当前仓库采用 Monorepo 结构，将 MP157 Linux 主控侧程序、F407 Sensor Hub 固件、公共协议和工具脚本分目录管理。

## 当前状态

- `mp157/outdoor-core-service/`: Linux Outdoor Core Runtime，已完成 Stage 0，并在 Stage 1 支持 MCU heartbeat、mock sensor 和 Mock IMU 协议帧解析。
- `f407/sensor-hub/`: STM32F407ZG Sensor Hub 软件目录，包含 PC mock C++ 工程，以及 `firmware/` 下的项目自有 C 代码和 STM32CubeMX/HAL 工程。当前固件通过 USART1 发送 heartbeat 和 Mock IMU 二进制协议帧，尚未接入真实 IMU。
- `common/protocol/`: MP157 与 F407 共享的协议常量、CRC16/MODBUS 和 IMU payload 定义。
- `tools/`: PC 调试工具，当前包含 `frame_decoder` 十六进制 MCU 协议帧解析工具。
- `scripts/`: 放置仓库级构建脚本。

当前不包含 UI、HTTP API、AI Agent，也不假设真实 STM32F407ZG 固件或真实 ICM42688/IMU 已完成接入。

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

## F407 固件状态

STM32CubeMX 生成的基础工程位于：

```text
f407/sensor-hub/firmware/stm32cube/
```

仓库已使用 `arm-none-eabi-gcc` 和 CMake 独立生成 F407 的 ELF/HEX/BIN/map，不依赖 Keil 工程完成编译。当前固件包含 PF9 LED 心跳，并已在 F407 板上通过 USART1 PA9/PA10、115200 8N1 验证 heartbeat 和 Mock IMU 二进制帧上报。真实 IMU 和 MP157 侧 `/dev/tty*` 串口输入仍待接入。

详细步骤见 [F407 固件构建计划](docs/stage1_bringup_plan.md)。

Windows 开发环境可在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build_f407.ps1
```

构建产物位于 `f407/sensor-hub/firmware/build/`。

通过 UART Bootloader 烧录和验证：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3 -ReadoutUnprotectFirst
powershell -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5
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
│       └── firmware/
│           └── stm32cube/
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
- [Stage 1 Bring-up 计划](docs/stage1_bringup_plan.md)

## 开源协议

当前尚未添加 LICENSE 文件。后续计划补充开源协议。
