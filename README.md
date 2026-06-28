# AI Outdoor Agent Platform

AI Outdoor Agent Platform 是一个围绕 STM32MP157 + STM32F407ZG 的户外智能体长期作品集项目。当前仓库采用 Monorepo 结构，将 MP157 Linux 主控侧程序、F407 Sensor Hub 固件、公共协议和工具脚本分目录管理。

## 当前状态

- `mp157/outdoor-core-service/`: Linux Outdoor Core Runtime，已完成 Stage 0，并在 Stage 1 支持 GNSS 文件回放、UART5 GNSS/NMEA 软件输入路径、`outdoor-agent` 文本和 framebuffer 仪表盘 APP、SD 卡目录持久化输出、MCU heartbeat、mock sensor、Mock IMU 协议帧解析、MP157 板载 ICM20608 的读取服务和上板验证、F407 live serial 输入路径，以及最小 MP157 -> F407 `command_ping` 下行命令。
- `f407/sensor-hub/`: STM32F407ZG Sensor Hub 软件目录，包含 PC mock C++ 工程，以及 `firmware/` 下的项目自有 C 代码和 STM32CubeMX/HAL 工程。当前固件通过 UART4 PC10/PC11 对接 MP157 USART3，发送 heartbeat、IMU、MMC5603 磁力计和 BMP390 气压计协议帧，并可回复 MP157 下发的 `command_ping`。MMC5603 已完成 I2C2 真实数据历史上板验证，当前按 I2C 排障需要已断开；BMP390 已完成软件集成和烧录验证，但当前未在 I2C2 上 ACK；ICM42688 当前重新接线后仍处于 Mock fallback。
- `common/protocol/`: MP157 与 F407 共享的协议常量、CRC16/MODBUS 和各传感器 payload 定义。
- `tools/`: PC 调试工具，当前包含 `frame_decoder` 十六进制 MCU 协议帧解析工具。
- `scripts/`: 放置仓库级构建脚本。

当前不包含 HTTP API 或真实 AI Agent 推理/交互能力。Sensor Integration 已完成 MP157 板载 ICM20608、F407 侧 MMC5603 和 F407/MP157 串口链路的真实上板验证。UBLOX-M10 已确认从 MP157 UART5 `/dev/ttySTM2` 以 38400 8N1 输出有效 NMEA；当前环境无卫星信号，因此定位仍无效。`outdoor-agent` 当前可在 7 英寸 RGB 屏 `/dev/fb0` 显示仪表盘和 AI Agent 预留区。

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

## MP157 板载 ICM20608

MP157 侧新增 `icm20608_service`，参考 正点原子 STM32MP157 ICM20608 示例读取板载 ICM20608。当前实测板上系统通过 `modprobe icm20608` 加载 `/lib/modules/.../drivers/char/icm20608.ko`，并生成 `/dev/icm20608` 字符设备；IIO sysfs 读取路径保留为后续可选来源。默认 PC 开发配置关闭该服务，避免没有真实设备时影响自动化验证。

2026-06-19 已完成真实 MP157 上板验证：使用 Windows host 的 `arm-none-linux-gnueabihf-g++ 9.2.1` 交叉编译 ARMv7 hard-float 可执行文件，经 CH340 串口传输到 `/tmp/ai_outdoor_runtime` 后运行通过。板端 `runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`、`source=icm20608_char`、`sample_count=5`、`last_error=""`；静置样例中 Z 轴约 `0.983g`，温度约 `36.5°C`。

本地资料目录 `F:\BaiduNetdiskDownload\【正点原子】STM32MP157开发板（A盘）-基础资料\05、开发工具\01、交叉编译器` 中的 `gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz` 和 OpenSTLinux SDK 脚本是 Linux host 工具链，不能直接在当前 Windows PowerShell 环境原生使用；本次验证改用同版本 Windows host 工具链完成交叉编译。

默认配置项位于 `mp157/outdoor-core-service/config/runtime.conf`：

```text
board_imu_enabled = false
board_imu_source = char_device
board_imu_device_path = /dev/icm20608
board_imu_iio_path = /sys/bus/iio/devices/iio:device0
board_imu_sample_count = 5
board_imu_sample_interval_ms = 100
```

在 MP157 板上启用验证：

```bash
cd mp157/outdoor-core-service
modprobe icm20608
./build/outdoor_core_runtime --board-imu --board-imu-source char_device --board-imu-device-path /dev/icm20608 --board-imu-samples 5
```

预期 `runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`，并输出 `accel_*_g`、`gyro_*_dps` 和 `temperature_celsius`。如果后续切换到 IIO 驱动，可使用 `--board-imu-source iio` 并以板上 `/sys/bus/iio/devices/` 枚举结果为准。

## UBLOX-M10 GNSS 与仪表盘

MP157 Runtime 当前新增 GNSS 输入模式：

```text
gnss_input_mode = file | serial
gnss_serial_device = /dev/ttySTM2
gnss_serial_baud = 38400
gnss_serial_capture_seconds = 5
dashboard_enabled = true
dashboard_output_path = runtime/dashboard.txt
dashboard_output_mode = text | framebuffer | both
dashboard_framebuffer_device = /dev/fb0
dashboard_refresh_count = 1
dashboard_refresh_interval_ms = 1000
```

默认仍使用 `data/nmea_sample.txt` 文件回放，方便 PC 自动化测试；后续上板时可切换到 UART5：

```bash
./outdoor_core_runtime --config config/runtime.conf --gnss-input-mode serial --gnss-serial-device /dev/ttySTM2 --gnss-serial-baud 38400
```

当前 NMEA parser 支持 RMC/GGA/VTG/GSA/GSV，可输出经纬度、海拔、速度、航向、卫星数、fix quality/type 和 DOP。`outdoor-agent` 仪表盘默认输出到 `runtime/dashboard.txt`，也可通过 `--dashboard-output-mode framebuffer|both --dashboard-framebuffer-device /dev/fb0` 直接渲染到 7 英寸 RGB 屏 framebuffer。当前 framebuffer 界面已按深色科技风参考图实现左侧导航、顶部状态栏、方向罗盘、大速度表、位置地图、温度/光照展示区和底部状态栏；其中光照、空气质量、电池、信号等暂为 UI 占位/演示指标，尚未接入真实传感器或电源管理数据。真实 AI Agent 本地部署或交互仍未实现。

## MP157 SD 卡持久化输出

MP157 插入并挂载 SD 卡后，Runtime 可将状态、文本仪表盘和日志写入 SD 卡目录：

```bash
./outdoor_core_runtime --config config/runtime.conf --storage-root /run/media/mmcblk1p1/outdoor-agent
```

当前 MP157 实测 SD 卡自动挂载为 `/run/media/mmcblk1p1`；`/mnt/sdcard` 当前不存在。启用后默认生成 `status/runtime_status.json`、`dashboard/dashboard.txt`、`logs/outdoor_core_runtime.log`，并预建 `data/` 与 `captures/` 目录给后续采集数据使用。当前只是文件/目录型持久化基础设施，不包含日志轮转、数据库或长期采样记录服务。

## F407 固件状态

STM32CubeMX 生成的基础工程位于：

```text
f407/sensor-hub/firmware/stm32cube/
```

仓库已使用 `arm-none-eabi-gcc` 和 CMake 独立生成 F407 的 ELF/HEX/BIN/map，不依赖 Keil 工程完成编译。当前固件包含 PF9 LED 心跳；USART1 PA9/PA10 保留给 COM6 UART Bootloader 烧录，应用态 Sensor Hub 协议帧由 UART4 PC10/PC11、115200 8N1 输出到 MP157 USART3，同时镜像到 USART1 方便 COM6 诊断抓包。F407 已配置 ICM42688 相关管脚：PB10 (`I2C2_SCL`)、PB11 (`I2C2_SDA`)、PB12 (`ICM42688_INT1`)；ICM42688 I2C 读取路径曾在修正 SCL/SDA 接线后完成 `status_flags=0x0001` 历史上板验证。当前重新整理接线后，固件会尝试 `0x69` 和 `0x68`，但实测仍回退 Mock IMU，最近一次抓包 heartbeat 为 `status_flags=0x0056`。MP157 侧 live serial 输入路径默认按 USART3 `/dev/ttySTM1`、115200 8N1 读取 F407 二进制帧；2026-06-20 已完成真实接线验证，`runtime_status.json` 中 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true`、`status_flags=1`。当前还新增了 MP157 侧 `--mcu-command ping` 和 F407 侧 UART4 RX 命令解析，用于验证 MP157 -> F407 反向链路。

MMC5603 与 ICM42688 共用 I2C2：`SCL -> PB10`、`SDA -> PB11`，MMC5603 地址为 `0x30`。固件使用 20 Hz 单次测量并发送 `sensor_magnetometer` 帧；IMU 调度使用固定 10 ms 时间基准，测量结束后可追赶周期。MMC5603 已完成真实磁场读取历史验证，当前为隔离 ICM42688/BMP390 I2C 问题已断开，heartbeat 中会出现 magnetometer error 位。

BMP390 同样复用 I2C2。固件启动时优先探测 `0x76`，再探测 `0x77`，使用 Bosch BMP3 Sensor API v2.0.5 的 64-bit 定点补偿，配置温度/气压 2x 过采样、IIR 系数 3 和 25 Hz ODR，并以 10 Hz 发送 `sensor_barometer` 帧。该路径已通过 F407、MP157 和工具链构建测试，也已烧录验证应用态链路；当前 BMP390 在 `0x76/0x77` 均未 ACK，尚未出现真实 `sensor_barometer` 帧。

详细步骤见 [F407 固件构建计划](docs/stage1_bringup_plan.md)。

Windows 开发环境可在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build_f407.ps1
```

构建产物位于 `f407/sensor-hub/firmware/build/`。

通过 UART Bootloader 烧录 F407：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM6
```

当前应用态协议帧主链路走 UART4 到 MP157，同时镜像到 COM6/USART1 便于本机诊断；板间验证在 MP157 上通过 `/dev/ttySTM1` 执行。

MP157 发送最小下行 ping 并等待 F407 ack：

```bash
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5 --mcu-command ping
```

预期 `runtime/runtime_status.json` 中 `mcu.command_ack_seen=true`、`mcu.command_ack_status=0`，`mcu.command_ack_nonce` 与默认 ping nonce 一致。当前已通过 MP157 shell raw 写帧验证 F407 返回 `command_ack`，Runtime 板端复测后续补充。

如果 Bootloader 因读保护返回 NACK，可在确认会擦除用户 Flash 后追加 `-ReadoutUnprotectFirst`。

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
- [ADR-0009: MP157 板载 ICM20608 优先验证](docs/adr/0009-use-mp157-icm20608-iio-first.md)
- [ADR-0010: UART5 NMEA 输入与文本仪表盘原型](docs/adr/0010-use-nmea-uart5-text-dashboard.md)
- [ADR-0012: MP157 SD 卡文件持久化](docs/adr/0012-use-sd-card-file-storage.md)

## 开源协议

当前尚未添加 LICENSE 文件。后续计划补充开源协议。
