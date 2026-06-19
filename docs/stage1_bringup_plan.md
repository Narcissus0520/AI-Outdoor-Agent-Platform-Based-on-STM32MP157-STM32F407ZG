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
- 已包含：ICM42688 I2C2 PB10/PB11、INT1 PB12、I2C BSP、ICM42688 寄存器读取和 Mock IMU 兜底
- 已包含：修正接线后的 ICM42688 真实读数上板验证
- 未包含：MP157 Linux 真实串口输入和 F407 -> MP157 板间联调
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

当前验收：F407 每 1000 ms 输出 heartbeat、每 10 ms 输出 IMU 帧。修正 ICM42688 SCL/SDA 接线后已完成真实 ICM42688 上板验证；当前 100 Hz 版本 5 秒抓取 506 帧、heartbeat 5 帧、IMU 501 帧、`imu_rate_hz=100.2`、CRC 错误 0 帧，最后 heartbeat `status_flags=0x0001`。MP157 Runtime 真实串口输入仍待实现。

上板验证环境：

- 烧录链路：CH340 `COM3`，STM32 ROM UART Bootloader。
- Programmer：用户已安装 STM32CubeProgrammer，CLI 路径为 `D:\Program Files (x86)\ST\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`；当前仓库脚本直接实现 Bootloader 协议，以便精确控制一键下载电路的 DTR/RTS 时序。
- Bootloader 识别结果：ACK `0x79`，Bootloader version `0x31`，chip ID `0x0413`。
- 烧录命令：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3`。
- 验证命令：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5`。
- 若 Bootloader 因读保护导致擦写命令返回 NACK，再追加 `-ReadoutUnprotectFirst`；该操作会擦除用户 Flash。

当前串口接线：

```text
F407 PA9  (USART1_TX) -> MP157 UART_RX
F407 PA10 (USART1_RX) <- MP157 UART_TX
F407 GND               - MP157 GND
```

仅验证单向上报时，可以只连接 PA9、MP157 RX 和公共 GND。两端必须使用 3.3 V TTL 电平。

### 当前上电时序与运行流程

上电前硬件状态：

- F407、ICM42688 和串口调试器/后续 MP157 必须共地。
- ICM42688 `VCC=3.3V`，`AD0=3.3V`，对应 I2C 7-bit 地址 `0x69`。
- ICM42688 `SCL -> PB10`、`SDA -> PB11`、`INT1 -> PB12`，其中 PB12 当前只作为普通输入预留。
- I2C SCL/SDA 需要上拉到 3.3 V；模块板若没有板载上拉，建议外接 4.7k 到 10k。

固件启动流程：

1. F407 复位后先执行 CubeMX 生成的 HAL、时钟、GPIO、I2C2、USART1 初始化。
2. `main()` 调用 `sensor_hub_app_init()`，项目业务逻辑开始接管 Sensor Hub。
3. `sensor_hub_app_init()` 初始化 Mock IMU fallback，并对 ICM42688 执行 bank 0 选择、软复位、`WHO_AM_I=0x47` 检查、accel/gyro 量程和 100 Hz ODR 配置、`PWR_MGMT0` 使能。
4. 初始化成功后 heartbeat `status_flags=0x0001`；初始化失败则进入 Mock fallback，`status_flags=0x0006`。
5. `sensor_hub_app_poll()` 在主循环中持续运行：PF9 每 500 ms 翻转，heartbeat 每 1000 ms 发送，IMU 每 10 ms 读取 ICM42688 并发送 `sensor_imu` 帧。
6. 若运行中 I2C 读取失败，当前策略是立即回退 Mock IMU 并设置 fallback/error 标志；后续需要补充自动重新初始化策略。

烧录/验证流程：

1. `flash_f407_uart.ps1` 通过一键下载电路控制 DTR/RTS 进入 STM32 ROM UART Bootloader。
2. 写入 `sensor_hub.bin` 到 `0x08000000`，并执行逐字节回读校验。
3. `verify_f407_uart.ps1` 触发应用态复位，抓取 USART1 二进制帧并统计 heartbeat、IMU、CRC 和 `status_flags`。
4. 若刷写后第一次抓包为 0 字节，重新运行验证脚本或手动复位；该现象来自一键下载电路应用态复位状态切换。

### Step 6：ICM42688 I2C 接入

- [x] 在 CubeMX 中配置 PB10 为 `I2C2_SCL`
- [x] 在 CubeMX 中配置 PB11 为 `I2C2_SDA`
- [x] 在 CubeMX 中配置 PB12 为 `ICM42688_INT1` 输入
- [x] 新增 PB12 输入读取 BSP 封装 `board_icm42688_data_ready()`
- [x] 新增 I2C2 初始化和 `HAL_I2C_Mem_Read/Write` BSP 封装
- [x] 移植 ICM42688 `WHO_AM_I`、100 Hz accel/gyro 配置和数据读取逻辑
- [x] F407 IMU 上报优先使用 ICM42688，失败时回退 Mock IMU 并设置 heartbeat `status_flags`
- [x] 修正实际接线为 `SCL -> PB10`、`SDA -> PB11` 后，上板验证真实 ICM42688 IMU 数据路径
- [x] 将 F407 侧 ICM42688 读取和 IMU 帧上报周期调整为 100 Hz
- [x] 上板复测 100 Hz IMU 输出，5 秒抓取 heartbeat 5 帧、IMU 501 帧、`imu_rate_hz=100.2`、CRC 错误 0 帧

当前接线要求：

```text
ICM42688 AD0  -> F407 3.3V     # 7-bit address 0x69
ICM42688 SCL  -> F407 PB10     # I2C2_SCL
ICM42688 SDA  -> F407 PB11     # I2C2_SDA
ICM42688 INT1 -> F407 PB12
ICM42688 GND  -> F407 GND
```

若接成 `SDA -> PB10`、`SCL -> PB11`，则与当前 CubeMX 配置相反，I2C 无法通信。此前错误模块资料不再作为实现依据。

上一版 10 Hz 验收结果：修正接线后通过 UART Bootloader 刷写 `sensor_hub.bin`，PC 端 5 秒抓取 55 帧、heartbeat 5 帧、IMU 50 帧、CRC 错误 0 帧；最后一帧 heartbeat `status_flags=0x0001`，表示 ICM42688 初始化和读取成功。

100 Hz 验证命令和结果：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5 -MinImuHz 80
```

验证结果：5 秒读取 17132 字节，共解析 506 帧；heartbeat 5 帧；IMU 501 帧；`imu_rate_hz=100.2`；CRC 错误 0 帧；最后一帧 heartbeat `status_flags=0x0001`。

### Step 7：F407 Sensor Hub 完成项

- [ ] 决定是否将 PB12 `ICM42688_INT1` 从普通输入升级为 EXTI 数据就绪中断；当前固件按 10 ms 周期轮询读取。
- [ ] 增加 I2C 瞬时错误后的 ICM42688 重新初始化/恢复策略；当前策略是回退 Mock IMU 并通过 heartbeat `status_flags` 标记。
- [ ] 评估是否需要 ICM42688 FIFO；当前直接从 `TEMP_DATA1` 连续读取最新 14 字节样本。
- [ ] 评估是否需要 UART DMA 或环形缓冲；当前 100 Hz IMU 帧带宽仍低于 115200 8N1 能力，但发送方式仍为阻塞式 `HAL_UART_Transmit()`。
- [ ] 清理 PC mock C++ 层 `Icm42688Driver` 占位接口，避免与真实 F407 固件数据源混淆。

### Step 8：MP157 Live Serial Integration

- [ ] 在 MP157 Runtime 新增真实 MCU 串口输入路径，默认保留 mock 文件模式
- [ ] 新增配置项：`mcu_input_mode = mock_file | serial`
- [ ] 新增配置项：`mcu_serial_device = /dev/ttySTM*`
- [ ] 新增配置项：`mcu_serial_baud = 115200`
- [ ] 新增配置项：`mcu_serial_capture_seconds = 5`
- [ ] 将 F407 PA9 (`USART1_TX`) 和 GND 接入 MP157 UART_RX/GND
- [ ] 在 MP157 上验证 `runtime_status.json` 输出真实 heartbeat 和 IMU 状态
- [ ] 根据实测串口稳定性再评估 UART DMA 发送队列和 Runtime 长期运行模型

## 暂不处理

- RTOS 任务模型
- UART DMA 优化
- Bootloader 和固件升级
- 量产烧录流程

这些内容在最小裸机固件构建和串口链路稳定后再推进。
