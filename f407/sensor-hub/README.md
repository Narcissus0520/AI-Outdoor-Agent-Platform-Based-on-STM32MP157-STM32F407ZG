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
- USART1：PA9 TX、PA10 RX、115200、8N1、无流控；当前保留给 UART Bootloader 下载链路，并镜像应用态上行协议帧用于 COM6 诊断抓包。
- UART4：PC10 TX、PC11 RX、115200、8N1、无流控；当前作为 F407 与 MP157 USART3 的应用通信口。
- ICM42688 管脚：PB10 (`I2C2_SCL`)、PB11 (`I2C2_SDA`)、PB12 (`ICM42688_INT1`)。
- I2C2：100 kHz，共用总线包含 ICM42688 `0x69/0x68`、MMC5603 `0x30` 和 BMP390 `0x76/0x77`。
- 已接入 ICM42688 `WHO_AM_I` 检测、100 Hz accel/gyro 配置和 14 字节 burst 读取；暂未配置 DMA。
- 已接入 MMC5603 软件复位、产品 ID、SET/RESET 脉冲和 20-bit XYZ 磁场读取。
- 已接入 BMP390 地址探测、校准补偿、气压/温度读取；当前已烧录验证应用态链路，但真实模块尚未 ACK。

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

构建生成 `sensor_hub.elf`、`sensor_hub.hex`、`sensor_hub.bin` 和 `sensor_hub.map`。集成 BMP390 后的当前 Debug 固件约使用 16.4 KiB Flash 和 2.0 KiB RAM；实际大小会随功能和工具链版本变化。

当前 `main()`：

- 每 500 ms 翻转 PF9 (`LED0`)。
- 每 1000 ms 通过 UART4 发送 heartbeat。
- 每 10 ms 读取 ICM42688 并通过 UART4 发送一帧 IMU，目标采样/上报频率为 100 Hz；若 ICM42688 初始化或读取失败，则回退到 Mock IMU。
- MMC5603 每 50 ms 执行一次单次测量并发送 `sensor_magnetometer` 帧；IMU 调度采用固定 10 ms 时间基准。
- BMP390 配置为 25 Hz normal mode，每 100 ms 发送一帧 `sensor_barometer`。
- 每轮轮询 UART4 RX，解析 MP157 下发的 `command_ping`，并通过 UART4 TX 返回 `command_ack`。
- 串口输出是二进制 MCU 协议，不是可读文本。

USART1 heartbeat 和 IMU 帧曾通过 F407 上板串口抓包验证；当前应用通信口已切换为 UART4，并已通过 MP157 USART3 `/dev/ttySTM1` 上板验证。USART1 当前作为上行协议帧诊断镜像保留，方便 COM6 抓包。ICM42688 I2C 数据源曾完成 `status_flags=0x0001` 的历史验证，但当前重新整理接线后初始化失败并回退 Mock IMU。MMC5603 已完成真实读取历史验证，当前按 I2C 排障需要已断开。

## 上电时序与工作流程

硬件上电前确认：

- F407、ICM42688 和串口调试器/后续 MP157 共地。
- ICM42688 `VCC` 为 3.3 V，`AD0` 固定接 3.3 V，对应 7-bit I2C 地址 `0x69`。
- ICM42688 `SCL -> PB10`、`SDA -> PB11`，与 CubeMX `I2C2_SCL/I2C2_SDA` 配置一致。
- I2C SCL/SDA 需要上拉到 3.3 V；若模块板没有板载上拉，建议外接 4.7k 到 10k。
- `INT1 -> PB12` 当前仅作为普通输入预留，固件尚未依赖 INT1 数据就绪中断。

正常上电运行流程：

1. F407 复位启动，执行 CubeMX 生成的 `HAL_Init()`、系统时钟、GPIO、USART1、UART4 和 I2C2 初始化；I2C2 初始化前会尝试释放总线。
2. `main()` 进入项目 USER CODE 区域，调用 `sensor_hub_app_init()`。
3. `sensor_hub_app_init()` 初始化 Mock IMU fallback，并执行 ICM42688 初始化：
   - 选择 register bank 0。
   - 写 `DEVICE_CONFIG=0x01` 触发软复位，并等待约 100 ms。
   - 读取 `WHO_AM_I`，期望值为 `0x47`。
   - 配置 accel ±4 g、gyro ±1000 dps，并把 accel/gyro ODR 都配置为 100 Hz。
   - 写 `PWR_MGMT0` 使能 accel/gyro 工作模式。
   - 初始化成功时设置 heartbeat `status_flags=0x0001`。
   - 随后初始化 MMC5603，并初始化 BMP390；BMP390 先探测 `0x76`，再探测 `0x77`。
   - BMP390 配置温度/气压 2x 过采样、IIR 系数 3 和 25 Hz normal mode。
4. 主循环持续调用 `sensor_hub_app_poll()`：
   - 每轮最多轮询 32 个 UART4 RX 字节，解析 `command_ping` 并返回 `command_ack`。
   - PF9 LED 每 500 ms 翻转一次，作为应用存活指示。
   - 每 1000 ms 发送一帧 heartbeat。
   - 每 10 ms 从 ICM42688 连续读取 `TEMP_DATA1` 起始的 14 字节样本，换算为协议定点值并发送 `sensor_imu` 帧。
   - 每 50 ms 驱动 MMC5603 单次测量并发送 `sensor_magnetometer`。
   - 每 100 ms 读取 BMP390 补偿值并发送 `sensor_barometer`。
5. 如果 ICM42688 初始化或读取失败，固件回退到 Mock IMU 数据源，并将 heartbeat `status_flags` 设置为 `0x0006`（fallback + error）。

烧录/验证时序：

1. `scripts/flash_f407_uart.ps1` 通过一键下载电路控制 DTR/RTS，使 F407 进入 STM32 ROM UART Bootloader。
2. 脚本写入 `sensor_hub.bin` 到 `0x08000000`，并执行逐字节回读校验。
3. 校验成功后发送 STM32 ROM Bootloader `GO 0x08000000` 启动应用，并释放 DTR/RTS 到运行态。
4. 当前应用态协议帧通过 UART4 输出到 MP157 USART3，同时镜像到 COM6/USART1 用于本机诊断。

ICM42688 接线要求：

```text
ICM42688 AD0  -> F407 3.3V     # 7-bit I2C address = 0x69
ICM42688 SCL  -> F407 PB10     # I2C2_SCL
ICM42688 SDA  -> F407 PB11     # I2C2_SDA
ICM42688 INT1 -> F407 PB12     # 当前普通输入，后续可扩展为 EXTI
ICM42688 GND  -> F407 GND
```

MMC5603 接线要求：

```text
MMC5603 VIN -> F407 3.3V
MMC5603 GND -> F407 GND
MMC5603 SCL -> F407 PB10
MMC5603 SDA -> F407 PB11
```

BMP390 接线要求：

```text
BMP390 VCC -> F407 3.3V
BMP390 GND -> F407 GND
BMP390 SCL -> F407 PB10
BMP390 SDA -> F407 PB11
BMP390 CSB -> F407 3.3V
BMP390 SDO -> F407 3.3V  # I2C address 0x77
             或 F407 GND # I2C address 0x76
BMP390 INT -> NC          # 当前未使用中断
```

固件会先探测 `0x76`，失败后探测 `0x77`。BMP390 使用 Bosch BMP3 Sensor API v2.0.5 的 64-bit 定点补偿，配置气压/温度 2x 过采样、IIR 系数 3 和 25 Hz ODR。当前已烧录验证应用态链路，但真实模块在 `0x76/0x77` 均未 ACK，尚未出现 `sensor_barometer` 帧。

2026-06-29 I2C 传感器重验证快照：

- `flash_f407_uart.ps1 -PortName COM6` 通过，Bootloader version `0x31`，chip ID `0x0413`，逐字节回读校验成功，并通过 `GO 0x08000000` 启动应用。
- COM6 USART1 协议镜像抓包结果：`frames=1014`、`heartbeat=10`、`imu=1004`、`magnetometer=0`、`barometer=0`、`crc_bad=0`。
- 最后 heartbeat `status_flags=0x0056`，表示当前 ICM42688 仍为 fallback/error，MMC5603 因排障断开为 error，BMP390 仍未 ready。

2026-06-25 MMC5603 上板验证：5 秒收到 100 帧，`19.8 Hz`，平均 XYZ 约
`(-35.60, -25.18, 18.16) μT`，合成磁场约 `47.24 μT`。

注意：如果按 `SDA -> PB10`、`SCL -> PB11` 接线，则与当前 CubeMX 配置相反，I2C 无法正常通信。SCL/SDA 需要上拉到 3.3 V；模块板若没有板载上拉，建议外接 4.7k 到 10k 上拉电阻。

接线时使用 3.3 V TTL 电平：

```text
F407 PC10 (UART4_TX) -> MP157 PD9 (USART3_RX, /dev/ttySTM1)
F407 PC11 (UART4_RX) <- MP157 PD8 (USART3_TX)
F407 GND              - MP157 GND
```

USART1 PA9/PA10 继续用于 F407 USB-UART 下载和 ROM Bootloader，不参与当前板间应用通信。

上一版 10 Hz 固件已通过 CH340 `COM3` 完成 UART Bootloader 烧录和串口抓包验证：

- 5 秒抓取 55 帧。
- heartbeat 5 帧。
- IMU 50 帧。
- CRC 错误 0 帧。
- 最后一帧 heartbeat `status_flags=0x0001`，表示 ICM42688 ready。

当前 100 Hz 版本上板验证结果：

- 5 秒抓取 506 帧。
- heartbeat 5 帧。
- IMU 501 帧，`imu_rate_hz=100.2`。
- CRC 错误 0 帧。
- 最后一帧 heartbeat `status_flags=0x0001`，表示 ICM42688 ready。

可复现命令：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM6
```

如果 Bootloader 因读保护返回 NACK，可在确认会擦除用户 Flash 后追加 `-ReadoutUnprotectFirst`。

当前 UART4 -> MP157 USART3 验证命令在 MP157 上执行：

```bash
stty -F /dev/ttySTM1 115200 raw -echo -ixon -ixoff -crtscts
timeout 5 dd if=/dev/ttySTM1 bs=1 count=512 | hexdump -C
cd /tmp/ai_outdoor_runtime_serial
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5
```

2026-06-20 验证结果：MP157 raw capture 可见连续 `a5 5a` 帧头；Runtime serial 模式输出 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true`、`mcu.status_flags=1`、`imu.seen=true`。

MP157 -> F407 最小下行 ping/ack 验证命令：

```bash
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5 --mcu-command ping
```

预期：`runtime/runtime_status.json` 中 `mcu.command_ack_seen=true`、`mcu.command_ack_status=0`，并且 `mcu.command_ack_request_type=128`。

2026-06-21 已完成 raw shell 级别双向验证：在 MP157 shell 中直接向 `/dev/ttySTM1` 写入 `command_ping` 帧 `a5 5a 01 80 01 00 04 00 47 4e 49 50 c3 43`，F407 返回 `command_ack` 首帧 `a5 5a 01 81 f1 99 08 00 80 00 00 00 47 4e 49 50 f1 59`。这证明 F407 UART4 RX、命令 decoder 和 ack 回包逻辑已在真实接线上工作；新版 ARM Runtime 的 `--mcu-command ping` 板端复测仍需更可靠的部署方式。

不要直接修改 Cube 生成区域来承载业务逻辑；项目代码应优先放在 `firmware/app` 等项目自有目录，并通过明确的 HAL/BSP 边界接入。
