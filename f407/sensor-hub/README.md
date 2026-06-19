# F407 Sensor Hub

本目录同时维护 PC 侧可测试模型和 STM32F407ZG 真实固件，两者共享 MCU 协议语义，但构建入口彼此独立。

## 目录

```text
sensor-hub/
├── CMakeLists.txt
├── include/
├── src/
├── tests/
└── firmware/
    ├── app/
    ├── bsp/
    ├── cmake/
    ├── linker/
    ├── platform/
    ├── protocol/
    ├── sensors/
    ├── startup/
    └── stm32cube/
```

- 根目录 `include/`、`src/`、`tests/`：PC mock C++ 工程，用于快速验证协议打包和 Mock IMU 行为。
- `firmware/app`、`bsp`、`protocol`、`sensors`：项目自行维护的纯 C 固件代码。
- `firmware/cmake`、`linker`、`startup`、`platform`：GNU ARM 工具链、链接布局、启动和最小系统调用支持。
- `firmware/stm32cube`：CubeMX 生成基线，包含 `.ioc`、Core、HAL、CMSIS 和 MDK-ARM 工程。

## 当前 Cube 基线

- MCU：`STM32F407ZGTx`，LQFP144。
- CubeMX：6.17.0。
- STM32Cube FW_F4：V1.28.3。
- 系统时钟：生成代码使用 HSI + PLL，目标 168 MHz。
- GPIO：PF9 (`LED0`) 和 PF10 (`LED1`)。
- USART1：PA9 TX、PA10 RX、115200、8N1、无流控。
- ICM42688 管脚：PB10 (`I2C2_SCL`)、PB11 (`I2C2_SDA`)、PB12 (`ICM42688_INT1`)。
- I2C2：100 kHz，7-bit 从地址 `0x69`（ICM42688 `AD0` 接 3.3 V）。
- 已接入 ICM42688 `WHO_AM_I` 检测、100 Hz accel/gyro 配置和 14 字节 burst 读取；暂未配置 DMA。

## 构建

PC mock 工程当前可构建：

```bash
cmake -S f407/sensor-hub -B f407/sensor-hub/build
cmake --build f407/sensor-hub/build
ctest --test-dir f407/sensor-hub/build -C Debug --output-on-failure
```

真实固件可在 Windows 下从仓库根目录构建：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build_f407.ps1
```

也可直接使用 CMake：

```bash
cmake -S f407/sensor-hub/firmware -B f407/sensor-hub/firmware/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build f407/sensor-hub/firmware/build
```

构建生成 `sensor_hub.elf`、`sensor_hub.hex`、`sensor_hub.bin` 和 `sensor_hub.map`。当前 Debug 固件约使用 10 KiB Flash 和 1.8 KiB RAM；实际大小会随功能和工具链版本变化。

当前 `main()`：

- 每 500 ms 翻转 PF9 (`LED0`)。
- 每 1000 ms 通过 USART1 发送 heartbeat。
- 每 100 ms 读取 ICM42688 并通过 USART1 发送一帧 IMU；若 ICM42688 初始化或读取失败，则回退到 Mock IMU。
- 串口输出是二进制 MCU 协议，不是可读文本。

USART1 heartbeat 和 IMU 帧已通过 F407 上板串口抓包验证；ICM42688 I2C 数据源已在修正 SCL/SDA 接线后通过上板验证，heartbeat `status_flags=0x0001` 表示当前 IMU 帧来自真实 ICM42688。

ICM42688 接线要求：

```text
ICM42688 AD0  -> F407 3.3V     # 7-bit I2C address = 0x69
ICM42688 SCL  -> F407 PB10     # I2C2_SCL
ICM42688 SDA  -> F407 PB11     # I2C2_SDA
ICM42688 INT1 -> F407 PB12     # 当前普通输入，后续可扩展为 EXTI
ICM42688 GND  -> F407 GND
```

注意：如果按 `SDA -> PB10`、`SCL -> PB11` 接线，则与当前 CubeMX 配置相反，I2C 无法正常通信。SCL/SDA 需要上拉到 3.3 V；模块板若没有板载上拉，建议外接 4.7k 到 10k 上拉电阻。

接线时使用 3.3 V TTL 电平：

```text
F407 PA9  (USART1_TX) -> MP157 UART_RX
F407 PA10 (USART1_RX) <- MP157 UART_TX  # 当前上报测试可暂不连接
F407 GND               - MP157 GND
```

当前已通过 CH340 `COM3` 完成 UART Bootloader 烧录和串口抓包验证：

- 5 秒抓取 55 帧。
- heartbeat 5 帧。
- IMU 50 帧。
- CRC 错误 0 帧。
- 最后一帧 heartbeat `status_flags=0x0001`，表示 ICM42688 ready。

可复现命令：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
powershell -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5
```

如果 Bootloader 因读保护返回 NACK，可在确认会擦除用户 Flash 后追加 `-ReadoutUnprotectFirst`。

不要直接修改 Cube 生成区域来承载业务逻辑；项目代码应优先放在 `firmware/app` 等项目自有目录，并通过明确的 HAL/BSP 边界接入。
