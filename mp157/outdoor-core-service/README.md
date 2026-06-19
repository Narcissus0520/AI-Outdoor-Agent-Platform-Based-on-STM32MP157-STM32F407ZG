# Outdoor Core Service

`outdoor-core-service` 是 AI Outdoor Agent Platform 的 STM32MP157 Linux 主控侧 Runtime 服务。该模块独立使用 CMake 编译，当前包含 Stage 0 Linux Outdoor Core Runtime 能力，并承载 Stage 1 Linux Runtime 侧 MCU 协议解析和 MP157 板载 ICM20608 读取服务。

本模块不包含 STM32F407ZG 固件代码，不实现 UI、HTTP API 或 AI Agent。

## 当前能力

- C++17 / CMake Linux Runtime 工程
- 日志、配置和 Runtime Service 生命周期
- NMEA 文件回放与最小 NMEA Parser
- GNSS 基础定位状态输出
- 文件型 Runtime 状态输出：`runtime/runtime_status.json`
- MCU mock frame 解析
- `McuFrame` / `McuFrameParser` / `McuStatus`
- CRC16/MODBUS 校验
- Heartbeat、mock sensor 和 Mock IMU 帧解析
- `runtime_status.json` 输出独立 `imu` 字段
- MP157 板载 ICM20608 字符设备读取服务，输出独立 `board_imu` 字段
- ICM20608 IIO sysfs reader 保留为后续可选来源

## 目录结构

```text
outdoor-core-service/
├── CMakeLists.txt
├── README.md
├── config/
├── data/
├── include/
├── scripts/
├── src/
└── tests/
```

## 编译

在本目录执行：

```bash
cmake -S . -B build
cmake --build build
```

也可以在仓库根目录执行：

```bash
./scripts/build_mp157.sh
```

面向 MP157 板端部署时，需要使用 ARMv7 hard-float Linux 交叉编译器。2026-06-19 上板验证使用 Windows host `arm-none-linux-gnueabihf-g++ 9.2.1` 编译 `outdoor_core_runtime`；正点原子资料目录中的 `gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz` 和 OpenSTLinux SDK 脚本面向 Linux host，不能直接在当前 Windows PowerShell 环境原生运行。

## 运行

Linux / macOS:

```bash
./build/outdoor_core_runtime --config config/runtime.conf
```

Windows CMake 默认 Visual Studio 生成器:

```powershell
.\build\Debug\outdoor_core_runtime.exe --config config\runtime.conf
```

MP157 板载 ICM20608 上板验证:

```bash
modprobe icm20608
./outdoor_core_runtime --config config/runtime.conf --board-imu --board-imu-source char_device --board-imu-device-path /dev/icm20608 --board-imu-samples 5
```

已验证结果：`runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`、`source=icm20608_char`、`sample_count=5`、`last_error=""`，静置样例中 Z 轴约 `0.983g`。

## 验证

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_runtime.ps1
```

验证脚本会检查：

- NMEA 回放和 GNSS fix 输出
- NMEA checksum 错误拒绝
- MCU heartbeat mock 帧
- MCU mock sensor 帧
- MCU Mock IMU 帧和 `imu` 状态字段
- 默认 `board_imu` 状态字段
- fake character-device / fake-IIO 的 ICM20608 换算测试
- MCU CRC16 错误拒绝
- `runtime_status.json` 状态输出

## 配置

默认配置文件：

```text
config/runtime.conf
```

当前字段：

```text
nmea_input_path = data/nmea_sample.txt
mcu_mock_input_path = data/mcu_mock_frames.txt
status_output_path = runtime/runtime_status.json
board_imu_enabled = false
board_imu_source = char_device
board_imu_device_path = /dev/icm20608
board_imu_iio_path = /sys/bus/iio/devices/iio:device0
board_imu_sample_count = 5
board_imu_sample_interval_ms = 100
log_level = info
```
