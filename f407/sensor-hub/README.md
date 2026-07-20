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
- ICM42688 管脚：PB10 (`I2C2_SCL`)、PB11 (`I2C2_SDA`)、PB12 (`ICM42688_INT1/EXTI12`)。
- I2C2：400 kHz，共用总线包含 ICM42688 `0x69/0x68`、MMC5603 `0x30` 和 BMP390 `0x76/0x77`；运行期事务失败时会硬复位 I2C2、释放总线并重试一次。
- 已接入 ICM42688 `WHO_AM_I`、100 Hz accel/gyro、约 25 Hz UI LPF、record-count FIFO、4 条记录 watermark、threshold/full INT1 和批量解析。
- UART4/USART1 各使用 1024 字节固定 TX 队列和 `HAL_UART_Transmit_IT()`；当前未使用 DMA，队列满时按整帧拒绝并记录诊断。
- 已接入 MMC5603 软件复位、产品 ID、SET/RESET 脉冲和 20-bit XYZ 磁场读取。
- 已接入 BMP390 地址探测、校准补偿、气压/温度读取；2026-07-18 已完成真实 ACK、气压计协议帧和 MP157 状态解析验证。

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

共享 I2C2 单器件隔离时可生成独立、非生产的 ICM42688-only 镜像；默认值为 `OFF`，不会改变正式构建：

```powershell
cmake -S f407/sensor-hub/firmware -B f407/sensor-hub/firmware/build-icm-only `
  -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake `
  -DSENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC=ON
cmake --build f407/sensor-hub/firmware/build-icm-only
```

该镜像完全跳过 MMC5603/BMP390 的初始化和周期轮询，并在 heartbeat 设置 `0x8000`，防止被误当作三传感器正式固件。完成隔离后必须重新刷回默认构建。

没有电气测量工具、需要在相同 ICM-only 条件下做 I2C2 降速 A/B 时，可额外生成 100 kHz 非生产镜像：

```powershell
cmake -S f407/sensor-hub/firmware -B f407/sensor-hub/firmware/build-icm-only-100k `
  -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake `
  -DSENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC=ON `
  -DSENSOR_HUB_I2C2_CLOCK_HZ=100000
cmake --build f407/sensor-hub/firmware/build-icm-only-100k
```

`SENSOR_HUB_I2C2_CLOCK_HZ` 只接受 `100000` 或 `400000`，默认仍为 `400000`。100 kHz 产物只用于受控 A/B；烧录后必须完整断电重上电，验证结束后必须恢复 400 kHz 基线。2026-07-21 单器件实板 A/B 已确认 100 kHz 不改善且显著增加 recovery/failure 速率，因此不能把该选项当作修复配置。

也可直接使用 CMake：

```bash
cmake -S f407/sensor-hub/firmware -B f407/sensor-hub/firmware/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build f407/sensor-hub/firmware/build
```

构建生成 `sensor_hub.elf`、`sensor_hub.hex`、`sensor_hub.bin` 和 `sensor_hub.map`。撤回 100 kHz、fixed partial-read 和临时包头诊断后的当前 Release 固件 BIN 为 19384 B，链接统计为 text 19364 B、data 20 B、BSS 4408 B，SHA256 为 `f0addc29eec272be28aee46a063e95fd878b5681be83df0dc6e430f7687b9d1e`；实际大小会随配置和工具链版本变化。

当前 `main()`：

- 每 500 ms 翻转 PF9 (`LED0`)。
- 每 1000 ms 通过 UART4 发送 heartbeat。
- ICM42688 INT1 threshold/full 上升沿只在 ISR 中累计事件，主循环批量读取最多 256 字节 FIFO，解析并发送最多 16 个 IMU 样本；若初始化、FIFO 或 INT1 超时失败，则回退到 Mock IMU。
- MMC5603 每 50 ms 执行一次单次测量并发送 `sensor_magnetometer` 帧；触发后先等待 7 ms，再最多检查 3 次状态且每次间隔 1 ms。
- BMP390 配置为 25 Hz normal mode，每 100 ms 发送一帧 `sensor_barometer`。
- IMU accel/gyro 和磁力计使用 `alpha=1/4` 定点一阶 IIR；IMU 温度、气压和气压计温度使用 `alpha=1/8`，首样本直通且错误后重置。
- I2C2 读写首次失败时执行外设硬复位，根据 SDA 状态发送最多 18 个 SCL 脉冲并生成 STOP，然后重试原事务一次。
- FIFO_DATA 突发读取不可重放；失败时只恢复总线并 flush FIFO，避免把有副作用的数据流当作普通寄存器重读。每秒向 UART4/MP157 和 USART1/COM6 同时发送一帧 `0x02` Sensor Hub diagnostics；当前 48 字节 schema 1 在旧 44 字节载荷尾部追加版本和 UART4 RX 累计丢弃字节数，使长测不打开 COM6 也能取得 I2C/FIFO 与接收过载证据。
- UART4 RX 中断持续接收字节并写入 64 字节环形缓冲；环满时拒绝新字节并累计 `uart4_rx_drop_count`，主循环消费缓冲并解析 `command_ping`。UART4/USART1 TX 队列通过发送完成中断持续 drain，主循环不再等待双路串口发送。
- 串口输出是二进制 MCU 协议，不是可读文本。

USART1 当前作为上行协议帧诊断镜像保留，方便当前 F407 USB-UART 抓包；应用通信口 UART4 已通过 MP157 USART3 `/dev/ttySTM1` 双向验证。此前 19368 B FIFO/TX 队列镜像已完成烧录回读、整板复电、60 秒真实传感器和 MP157 双向验收；之后健康时间线捕获到周期性 FIFO fallback。三传感器条件下的 I2C2 100 kHz 与固定 128 B/64 B partial FIFO 读取均未改善并已撤回；一次性诊断抓到候选包头 `06 FF FF FF`。正式默认镜像保持 19384 B、SHA256 `f0addc29...b9d1e`，400 kHz ICM-only 基线保持 15072 B、SHA256 `c07e235a...2f80a`。COM6 关闭后的 400 kHz `XMWvGC` 在约 58 秒累计 recovery/failure `232/45`；单器件 100 kHz `wB2xhs` 在相近窗口退化为 `0xD206/init step 1`，recovery/failure 速率为 400 kHz 的 `3.29/3.67` 倍。回滚时原 COM6 未枚举，用户确认设备改为 COM3；COM3 检查正常后，400 kHz ICM-only 基线已用 Bootloader `0x31`、chip ID `0x0413` 完成全擦、写入、逐字节回读和 GO。当前 Flash 可信，但隔离门槛仍未通过，不能计为 I2C/FIFO 修复验收。

## 上电时序与工作流程

硬件上电前确认：

- F407、ICM42688 和串口调试器/后续 MP157 共地。
- ICM42688 `VCC` 为 3.3 V，`AD0` 固定接 3.3 V，对应 7-bit I2C 地址 `0x69`。
- ICM42688 `SCL -> PB10`、`SDA -> PB11`，与 CubeMX `I2C2_SCL/I2C2_SDA` 配置一致。
- I2C SCL/SDA 需要上拉到 3.3 V；若模块板没有板载上拉，建议外接 4.7k 到 10k。
- `INT1 -> PB12` 必须连接；固件使用上升沿 EXTI12 作为 ICM42688 data-ready 事件源。

正常上电运行流程：

1. F407 复位启动，执行 CubeMX 生成的 `HAL_Init()`、系统时钟、GPIO、USART1、UART4 和 I2C2 初始化；I2C2 初始化前会尝试释放总线。
2. `main()` 进入项目 USER CODE 区域，调用 `sensor_hub_app_init()`。
3. `sensor_hub_app_init()` 初始化 Mock IMU fallback、MMC5603 和 BMP390，最后执行 ICM42688 初始化，避免其他传感器启动事务造成 FIFO 启动积压：
   - 选择 register bank 0。
   - 写 `DEVICE_CONFIG=0x01` 触发软复位，并等待约 100 ms。
   - 读取 `WHO_AM_I`，期望值为 `0x47`。
   - 配置 accel ±4 g、gyro ±1000 dps，并把 accel/gyro ODR 都配置为 100 Hz。
   - 配置 accel/gyro UI LPF 约 25 Hz，FIFO count 为大端记录数，启用 accel/gyro/temperature，watermark 为 4 条记录。
   - INT1 配置为脉冲、推挽、高有效，并把 FIFO threshold/full 路由到 INT1。
   - 写 `PWR_MGMT0` 使能 accel/gyro 工作模式。
   - 成功解析 FIFO 样本后设置 ready、INT1 active 和 FIFO active；三颗传感器同时 ready 且无错误时为 `0x01A9`。
   - BMP390 配置温度/气压 2x 过采样、IIR 系数 3 和 25 Hz normal mode。
4. 主循环持续调用 `sensor_hub_app_poll()`：
   - UART4 IRQ 把接收字节放入 64 字节环形缓冲；每轮最多消费 32 字节，解析 `command_ping` 并返回 `command_ack`。
   - PF9 LED 每 500 ms 翻转一次，作为应用存活指示。
   - 每 1000 ms 发送一帧 heartbeat。
   - PB12 EXTI ISR 只累计事件；主循环最短每 30 ms 批量读取 FIFO，按可变包对齐，将批次连续锚定到 100 Hz 发布时间轴，执行定点 IIR 并发送 `sensor_imu`。
   - 每 50 ms 驱动 MMC5603 单次测量并发送 `sensor_magnetometer`。
   - 每 100 ms 读取 BMP390 补偿值并发送 `sensor_barometer`。
5. 如果 ICM42688 初始化或 FIFO 读取失败，固件回退到 Mock IMU；基础 fallback/error 为 `0x0006`，初始化失败会额外设置 `0x1000`，FIFO 异常会额外设置 `0x0200`。

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
ICM42688 INT1 -> F407 PB12     # EXTI12 rising edge, required
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

固件会先探测 `0x76`，失败后探测 `0x77`。BMP390 使用 Bosch BMP3 Sensor API v2.0.5 的 64-bit 定点补偿，配置气压/温度 2x 过采样、IIR 系数 3 和 25 Hz ODR。2026-07-18 当前接线已出现真实 `sensor_barometer` 帧，MP157 Runtime 输出 `barometer_seen=true`。

2026-06-29 I2C 传感器重验证快照：

- `flash_f407_uart.ps1 -PortName COM6` 通过，Bootloader version `0x31`，chip ID `0x0413`，逐字节回读校验成功，并通过 `GO 0x08000000` 启动应用。
- COM6 USART1 协议镜像抓包结果：`frames=1014`、`heartbeat=10`、`imu=1004`、`magnetometer=0`、`barometer=0`、`crc_bad=0`。
- 最后 heartbeat `status_flags=0x0056`，表示当前 ICM42688 仍为 fallback/error，MMC5603 因排障断开为 error，BMP390 仍未 ready。

2026-07-18 双板与 I2C 传感器验证快照：

- 使用 COM6 将 18156 字节固件写入 F407，Bootloader version `0x31`、chip ID `0x0413`，逐字节回读校验通过。
- COM6 诊断镜像 3 秒抓取 378 帧，其中 heartbeat 3、IMU 285，CRC 错误 0；后续 MP157 Runtime 验证同时收到真实磁力计和气压计帧。
- MP157 COM9 上的当前 ARM Runtime 通过 `/dev/ttySTM1` 输出 `heartbeat_seen=true`、`imu_seen=true`、`magnetometer_seen=true`、`barometer_seen=true`、`status_flags=0x0029`。
- Runtime `--mcu-command ping` 输出 `command_ack_seen=true`、`command_ack_request_type=128`、`command_ack_status=0`、nonce `0x50494E47`。

2026-07-19 I2C2 长时间稳定性验证快照：

- 修复前 30 秒抓取在末尾出现 `status_flags=0x0056`，三颗共享 I2C2 的传感器同时停止真实数据，确认运行期总线错误无法自行恢复。
- 修复后烧录 18808 字节固件并逐字节回读通过；60 秒抓取 7532 帧，heartbeat 60、IMU 5666、磁力计 1204、气压计 602。
- 实测频率为 IMU `94.43 Hz`、磁力计 `20.07 Hz`、气压计 `10.03 Hz`；CRC、协议版本、载荷长度和数据合理性错误均为 0，最后 heartbeat 为 `0x0029`。
- 随后两次独立复位的 30 秒回归也均通过，最后 heartbeat 均为 `0x0029`。
- 验收命令：`powershell -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM6 -Seconds 60`。

2026-07-19 ICM42688 INT1 与滤波验证快照：

- COM6 ROM Bootloader 写入 20356 字节固件并逐字节回读通过；Bootloader version `0x31`、chip ID `0x0413`。
- 60 秒抓取 7412 帧：heartbeat 60、IMU 5547、磁力计 1203、气压计 602；最终 heartbeat `0x00A9`，`icm42688_int1_active=true`。
- 实测频率为 IMU `92.45 Hz`、磁力计 `20.05 Hz`、气压计 `10.03 Hz`；CRC、协议版本、载荷长度、时间戳和物理范围检查均通过。
- 当时主机侧 CTest 2/2 通过；加入 FIFO parser/provider、MMC5603 provider、UART TX queue 和 diagnostics schema 1 frame-builder 测试后，当前为 7/7 通过。
- 随后三次独立复位的 10 秒抓取均通过，最终 heartbeat 均为 `0x00A9`；IMU、磁力计、气压计滤波输出均非冻结值，CRC 和协议错误均为 0。

2026-07-19 ICM42688 FIFO 与中断 TX 队列最终验证快照：

- 整板断电上电后恢复真实 ICM42688；最终 Release BIN 为 19368 B，COM6 ROM Bootloader 烧录和逐字节回读通过。
- 最终镜像独立烧录复位 10 秒回归通过，三路频率约 `101.5/19.9/10.0 Hz`，最大 IMU 时间戳间隔 10 ms，最终 heartbeat `0x01A9`。
- 再次独立烧录复位后的 60 秒抓取通过：7988 帧，heartbeat 60、IMU 6086、磁力计 1187、气压计 595；频率约 `101.43/19.78/9.92 Hz`。
- 最终 heartbeat `0x01A9`；CRC、协议版本、载荷长度、IMU 时间戳回退和 UART4/USART1 TX overflow 均为 0。
- USART1 diagnostics 累计记录 I2C recovery 100、最终失败 1、FIFO overflow/malformed 各 10，但最终当前错误位均已清除；仍需小时级共享 I2C2 稳定性和故障注入。
- MP157 `/dev/ttySTM1` 最终复验看到 heartbeat/IMU/magnetometer/barometer，`status_flags=0x01A9`，并收到 status 0、nonce `0x50494E47` 的 command ACK。

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
powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
```

如果 Bootloader 因读保护返回 NACK，可在确认会擦除用户 Flash 后追加 `-ReadoutUnprotectFirst`。Windows COM 号会随 USB 重新枚举变化；COM3 是 2026-07-21 当前值，执行前须先核对 F407 USB-UART 的实际端口。

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

2026-06-21 已完成 raw shell 级别双向验证：在 MP157 shell 中直接向 `/dev/ttySTM1` 写入 `command_ping` 帧 `a5 5a 01 80 01 00 04 00 47 4e 49 50 c3 43`，F407 返回 `command_ack` 首帧 `a5 5a 01 81 f1 99 08 00 80 00 00 00 47 4e 49 50 f1 59`。2026-07-18 进一步修复零超时轮询在高频传感器发送期间丢失连续命令的问题，并由当前 ARM Runtime 完成状态 JSON 级双向验收。

不要直接修改 Cube 生成区域来承载业务逻辑；项目代码应优先放在 `firmware/app` 等项目自有目录，并通过明确的 HAL/BSP 边界接入。
