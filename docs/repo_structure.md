# Repository Structure

本仓库采用 Monorepo 结构，但 MP157 Linux 程序和 STM32F407ZG 固件分目录独立管理、独立编译。

## `mp157/`

`mp157/` 用于 Linux 主控侧程序。

当前模块：

```text
mp157/outdoor-core-service/
```

该目录包含 Outdoor Core Runtime 的独立 CMake 工程，内部保留：

- `include/`
- `src/`
- `config/`
- `data/`
- `tests/`
- `CMakeLists.txt`

当前 MP157 Runtime 已包含 `include/sensors` 和 `src/sensors`，用于主控侧板载传感器读取。当前实现为 ICM20608 字符设备 reader 和 IIO sysfs reader；运行时服务位于 `include/services/icm20608_service.h` 和 `src/services/icm20608_service.cpp`，默认关闭，上板时通过配置或 `--board-imu` 启用。真实 MP157 板上已通过 `/dev/icm20608` 字符设备完成 Runtime 读取验证。

当前 MP157 Runtime 也包含 `McuSerialService` 和 `McuFrameStreamDecoder`，用于从 `/dev/ttySTM1` 读取 F407 UART4 输出的 MCU 二进制帧；该路径已通过 `F407 PC10 UART4_TX -> MP157 PD9 USART3_RX` 上板验证。

## `f407/`

`f407/` 用于 STM32F407ZG Sensor Hub 固件。

当前目录：

```text
f407/sensor-hub/
```

当前提供两类代码：

- PC mock C++ 工程：可在本机编译验证 Mock IMU Provider 和 MCU 协议帧打包。
- `firmware/`：真实 F407 固件工作区，包含项目自有的 app、BSP、协议、ICM42688 数据源和 Mock IMU fallback。
- `firmware/stm32cube/`：STM32CubeMX 6.17.0 基于 STM32Cube FW_F4 V1.28.3 生成的 `STM32F407ZGTx` HAL 基础工程。

```text
f407/sensor-hub/
├── CMakeLists.txt              # PC mock C++ 构建
├── include/                    # PC mock C++ 头文件
├── src/                        # PC mock C++ 实现
├── tests/                      # PC mock CTest
└── firmware/
    ├── app/                    # 项目自有固件应用层
    ├── bsp/                    # 板级适配接口
    ├── cmake/                  # GNU Arm Embedded Toolchain 配置
    ├── linker/                 # STM32F407ZG 内存布局
    ├── platform/               # 最小 newlib 系统调用
    ├── protocol/               # C 版协议实现
    ├── sensors/                # 固件传感器数据源
    ├── startup/                # Cortex-M4 启动与中断向量表
    └── stm32cube/              # CubeMX 生成代码、HAL/CMSIS 和 .ioc
```

`firmware/stm32cube/` 当前配置系统时钟、PF9/PF10 LED GPIO、USART1 PA9/PA10、UART4 PC10/PC11、I2C2 PB10/PB11，以及 ICM42688 INT1 PB12。USART1 保留为 UART Bootloader 下载口；UART4 是 F407 与 MP157 的应用通信口。目录中保留 MDK-ARM 工程作为 CubeMX 生成基线；仓库使用 GNU Arm Embedded Toolchain 和 CMake 生成 ELF/HEX/BIN/map，不依赖 Keil 工程完成编译。

## `common/`

`common/` 用于 MP157 与 F407 共享内容。

当前目录：

```text
common/protocol/
```

当前放置 MP157 与 F407 共享的协议常量、CRC16/MODBUS 和 `ImuSample` / IMU payload 定义。

## `tools/`

`tools/` 用于 PC 调试工具，例如协议帧生成器、串口调试工具或数据回放辅助工具。

当前工具：

```text
tools/frame_decoder/
```

`frame_decoder` 用于解析十六进制 MCU 协议帧，并在 `sensor_imu` 帧中展开 IMU payload 字段。

## `scripts/`

`scripts/` 用于仓库级统一构建脚本。

当前脚本：

```text
scripts/build_mp157.sh
scripts/build_f407.ps1
scripts/flash_f407_uart.ps1
scripts/verify_f407_uart.ps1
```

`build_mp157.sh` 构建 MP157 Runtime；`build_f407.ps1` 使用 GNU Arm Embedded Toolchain 和 Ninja 构建 F407 固件；`flash_f407_uart.ps1` 通过 STM32 ROM UART Bootloader 烧录并回读校验 F407 固件；`verify_f407_uart.ps1` 是历史 PC 侧 USART1 协议帧抓取脚本，当前 UART4 板间验证以 MP157 `/dev/ttySTM1` raw capture 和 Runtime serial 模式为准。
