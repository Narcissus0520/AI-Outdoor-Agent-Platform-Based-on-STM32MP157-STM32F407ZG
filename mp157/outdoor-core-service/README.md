# Outdoor Core Service

`outdoor-core-service` 是 AI Outdoor Agent Platform 的 STM32MP157 Linux 主控侧 Runtime 服务。该模块独立使用 CMake 编译，当前包含 Stage 0 Linux Outdoor Core Runtime 能力，并开始承载 Stage 1 Linux Runtime 侧 MCU 协议解析。

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

## 运行

Linux / macOS:

```bash
./build/outdoor_core_runtime --config config/runtime.conf
```

Windows CMake 默认 Visual Studio 生成器:

```powershell
.\build\Debug\outdoor_core_runtime.exe --config config\runtime.conf
```

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
log_level = info
```
