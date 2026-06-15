# Stage 1 F407 Firmware Build Plan

本文件记录 STM32F407ZG Sensor Hub 从 CubeMX 基础工程演进到仓库自主编译、可上板验证固件的步骤。

## 最终目标

在不依赖 Keil 工程手工操作的前提下，由仓库中的 CMake 构建直接生成：

- `sensor_hub.elf`
- `sensor_hub.hex`
- `sensor_hub.bin`
- 链接 map 文件和可审查的 size 输出

目标工具链为 GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)。不新增大型第三方依赖。

## 当前基线

- Cube 工程目录：`f407/sensor-hub/firmware/stm32cube/`
- MCU：`STM32F407ZGTx`，LQFP144
- CubeMX：6.17.0
- STM32Cube FW_F4：V1.28.3
- 生成代码：HSI + PLL 168 MHz，PF9/PF10 LED GPIO
- 已包含：Core、HAL、CMSIS、`.ioc` 和 MDK-ARM 工程
- 已包含：GNU 启动、链接脚本、仓库固件 CMake、HEX/BIN/map 产物生成
- 已包含：USART1 PA9/PA10、115200 8N1 和阻塞式 HAL UART 发送
- 未包含：真实传感器外设和 MP157 Linux 串口输入
- 当前开发环境：GNU Arm Embedded Toolchain 14.2.Rel1、CMake 和 Ninja

Cube 生成代码与项目代码的边界见 `docs/adr/0008-f407-cube-source-and-cmake-build-boundary.md`。

## 目录边界

```text
f407/sensor-hub/
├── include/                    # PC mock C++ API
├── src/                        # PC mock C++ 实现
├── tests/                      # PC mock 测试
└── firmware/
    ├── app/                    # Sensor Hub 主循环
    ├── bsp/                    # HAL 板级适配
    ├── protocol/               # C 协议实现
    ├── sensors/                # Mock/真实传感器数据源
    └── stm32cube/              # CubeMX 生成内容
```

CubeMX 重新生成时，只允许更新 `firmware/stm32cube/`。业务代码不放入 Cube 目录；必须写入 Cube `USER CODE` 区域的内容保持最小，只负责调用项目入口。

## 分步实施

### Step 1：导入与基线盘点

- [x] 导入 `.ioc`、Core、HAL、CMSIS 和 MDK-ARM 工程
- [x] 将 Cube 生成内容移动到 `firmware/stm32cube/`
- [x] 记录 MCU、Cube 版本、时钟和已配置外设
- [x] 明确 Cube 代码与项目自有代码边界
- [x] 忽略 Keil 用户状态和构建输出

验收：目录和文档能够准确反映现有工程，不宣称 UART 或真实 IMU 已接入。

### Step 2：建立最小 GNU ARM/CMake 构建

- [x] 安装并确认 `arm-none-eabi-gcc`、`objcopy`、`size`
- [x] 新增 `firmware/cmake/arm-none-eabi-gcc.cmake`
- [x] 新增 STM32F407ZG GNU linker script
- [x] 新增 STM32F407xx GCC startup 实现
- [x] 新增 `firmware/CMakeLists.txt`
- [x] 编译 Cube Core、HAL 和 CMSIS 所需源文件
- [x] 生成 ELF、HEX、BIN、map 和 size 输出

验收结果：最小固件完成编译和链接，无未定义符号；Debug 基线使用 4572 B Flash、1584 B RAM。

### Step 3：最小上板基线

- [x] 在 Cube `USER CODE` 区域加入 PF9 LED 心跳逻辑
- [x] 使用 UART Bootloader 下载仓库构建的固件
- [x] 验证系统时钟、SysTick 和 PF9 LED 心跳
- [x] 记录下载命令、板卡连接和结果

验收结果：通过 `COM3` UART Bootloader 烧录并回读校验成功；LED0 心跳随应用运行。

### Step 4：接入 Sensor Hub 应用

- [x] 将 `board_get_tick_ms()` 对接 `HAL_GetTick()`
- [x] 将 `sensor_hub_app_init()` 接入初始化流程
- [x] 将 `sensor_hub_app_poll()` 接入主循环
- [x] 把 app、protocol 和 Mock IMU C 源文件加入固件构建
- [x] 保持 PC mock 测试继续通过

验收结果：业务代码在 ARM 工具链下完成编译和链接，ELF 中已确认应用、BSP、协议和 HAL UART 符号。

### Step 5：UART 与协议链路

- [x] 在 `.ioc` 中配置 USART1、PA9/PA10 和 115200 8N1
- [x] 使用阻塞式 `HAL_UART_Transmit()` 实现 `board_uart_send_bytes()`
- [x] 串口抓取 heartbeat 和 Mock IMU 帧
- [x] 使用 `scripts/verify_f407_uart.ps1` 验证帧内容和 CRC
- [ ] 使用 MP157 Runtime 验证真实串口链路
- [ ] 根据测量结果再决定是否引入 DMA 发送队列

当前验收：F407 每 1000 ms 输出 heartbeat、每 100 ms 输出 Mock IMU 帧，PC 端 5 秒抓取 55 帧且 CRC 错误 0 帧。MP157 Runtime 真实串口输入仍待实现。

上板验证环境：

- 烧录链路：CH340 `COM3`，STM32 ROM UART Bootloader。
- Programmer：用户已安装 STM32CubeProgrammer，CLI 路径为 `D:\Program Files (x86)\ST\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`；当前仓库脚本直接实现 Bootloader 协议，以便精确控制一键下载电路的 DTR/RTS 时序。
- Bootloader 识别结果：ACK `0x79`，Bootloader version `0x31`，chip ID `0x0413`。
- 烧录命令：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3 -ReadoutUnprotectFirst`。
- 验证命令：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5`。

当前串口接线：

```text
F407 PA9  (USART1_TX) -> MP157 UART_RX
F407 PA10 (USART1_RX) <- MP157 UART_TX
F407 GND               - MP157 GND
```

仅验证单向上报时，可以只连接 PA9、MP157 RX 和公共 GND。两端必须使用 3.3 V TTL 电平。

## 暂不处理

- 真实 ICM42688 寄存器驱动
- RTOS 任务模型
- UART DMA 优化
- Bootloader 和固件升级
- 量产烧录流程

这些内容在最小裸机固件构建和串口链路稳定后再推进。
