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
- 已包含：USART1 PA9/PA10、115200 8N1 作为 UART Bootloader 下载口
- 已包含：UART4 PC10/PC11、115200 8N1、1024 字节中断 TX 队列，以及中断接收和 64 字节 RX 环形缓冲，作为 F407 与 MP157 的专用板间通信口
- 已包含：USART1 1024 字节中断 TX 队列，用于 COM6 应用态诊断镜像
- 已包含：ICM42688 I2C2 PB10/PB11、INT1 PB12 EXTI、record-count FIFO、4 条记录 watermark、批量解析、定点 IIR 和 Mock IMU 兜底
- 已包含：MMC5603 与 ICM42688 共用 I2C2，地址 `0x30`，20 Hz 磁场帧
- 已包含：BMP390 与现有传感器共用 I2C2，自动探测 `0x76/0x77`，10 Hz 气压/温度帧；已完成真实模块 ACK、协议帧和 MP157 状态验证
- 已包含：修正接线后的 ICM42688 真实读数上板验证
- 已包含：MP157 Linux 真实串口输入和 F407 UART4 -> MP157 USART3 板间联调
- 已包含：MP157 -> F407 最小 `command_ping -> command_ack` 软件路径
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

- [x] 在 `.ioc` 中配置 USART1、PA9/PA10 和 115200 8N1，用于 UART Bootloader 下载
- [x] 在 `.ioc` 中配置 UART4、PC10/PC11 和 115200 8N1，用于 MP157 板间通信
- [x] 历史基线使用阻塞式 `HAL_UART_Transmit()` 完成 UART4/USART1 双路发送
- [x] 将 UART4 和 USART1 替换为各 1024 字节固定 TX 队列和 `HAL_UART_Transmit_IT()` 完成回调
- [x] 队列空间不足时按整帧拒绝，并通过 heartbeat 区分 UART4/USART1 TX overflow
- [x] 串口抓取 heartbeat 和 Mock IMU 帧
- [x] 使用 `scripts/verify_f407_uart.ps1` 验证帧内容和 CRC
- [x] 使用 MP157 Runtime 验证真实串口链路
- [x] 当前负载先采用中断 TX 队列，DMA 仅在真实 overflow 或更高带宽出现时再评估
- [x] 对 FIFO/TX 队列版本完成烧录回读、COM6/USART1 10 秒 Mock fallback 抓取和 MP157 UART4 双向板端验收；两路 TX overflow 均为 false
- [x] 整板复电后对真实 ICM42688 FIFO 执行 COM6 60 秒验收；最终 `0x01A9`

历史验收：F407 每 1000 ms 输出 heartbeat、每 10 ms 轮询输出 IMU 帧；ICM42688 曾完成 `status_flags=0x0001` 的 100 Hz 上板验证。2026-07-19 的 INT1 直接读取固件在三颗传感器 ready 时为 `0x00A9`。最终 FIFO/TX 队列固件已完成整板复电和 60 秒验收，健康值为 `0x01A9`。

上板验证环境：

- 烧录链路：F407 USB-UART，STM32 ROM UART Bootloader，使用 USART1 PA9/PA10；历史主要枚举为 `COM6`，2026-07-21 物理重连后当前枚举为 `COM3`，执行前必须按设备管理器/PnP 实际值确认。
- MP157 console：CH340 `COM9`（2026-07-18 当前枚举；Windows COM 号变化时以设备管理器为准）。
- Programmer：用户已安装 STM32CubeProgrammer，CLI 路径为 `D:\Program Files (x86)\ST\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`；当前仓库脚本直接实现 Bootloader 协议，以便精确控制一键下载电路的 DTR/RTS 时序。
- Bootloader 识别结果：ACK `0x79`，Bootloader version `0x31`，chip ID `0x0413`。
- 当前烧录命令：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3`；COM 号变化时同步替换参数。
- 当前应用态协议帧主链路走 UART4 到 MP157 `/dev/ttySTM1`，同时镜像到 USART1/COM6 供诊断抓包。
- 若 Bootloader 因读保护导致擦写命令返回 NACK，再追加 `-ReadoutUnprotectFirst`；该操作会擦除用户 Flash。

当前 F407 与 MP157 板间串口接线：

```text
F407 PC10 (UART4_TX) -> MP157 PD9 (USART3_RX, /dev/ttySTM1)
F407 PC11 (UART4_RX) <- MP157 PD8 (USART3_TX)
F407 GND              - MP157 GND
```

USART1 PA9/PA10 只保留给 F407 USB 下载/Bootloader，不再作为 F407 与 MP157 的应用通信口。两端必须使用 3.3 V TTL 电平。

2026-06-20 UART4 -> MP157 USART3 验证结果：

- F407 固件构建通过，`sensor_hub.bin` 大小 10320 B。
- 通过 `COM6` 执行 UART Bootloader 烧录和逐字节回读校验成功，Bootloader version `0x31`，chip ID `0x0413`。
- MP157 console 使用 `COM3`，确认 `/dev/ttySTM1` 存在，对应 USART3。
- 在 MP157 上执行 `stty -F /dev/ttySTM1 115200 raw -echo -ixon -ixoff -crtscts` 后，`dd if=/dev/ttySTM1 bs=1 count=512 | hexdump -C` 可看到连续 `a5 5a` MCU 帧头。
- 部署 ARM 版 `outdoor_core_runtime` 到 `/tmp/ai_outdoor_runtime_serial`，运行 serial 模式 5 秒后，`runtime/runtime_status.json` 输出 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true`、`mcu.status_flags=1`、`imu.seen=true`。
- 本次样例末帧 `last_sequence=16835`，IMU 样例约为 `accel=(-0.577, -0.043, 0.817) g`、`gyro=(-0.610, -0.274, 0.000) dps`、`temperature=24.970°C`。

### 当前上电时序与运行流程

上电前硬件状态：

- F407、ICM42688 和串口调试器/后续 MP157 必须共地。
- ICM42688 `VCC=3.3V`，`AD0=3.3V`，对应 I2C 7-bit 地址 `0x69`。
- ICM42688 `SCL -> PB10`、`SDA -> PB11`、`INT1 -> PB12`；PB12 当前配置为上升沿 EXTI12。
- I2C SCL/SDA 需要上拉到 3.3 V；模块板若没有板载上拉，建议外接 4.7k 到 10k。

固件启动流程：

1. F407 复位后先执行 CubeMX 生成的 HAL、时钟、GPIO、I2C2、USART1 和 UART4 初始化。
2. `main()` 调用 `sensor_hub_app_init()`，项目业务逻辑开始接管 Sensor Hub。
3. `sensor_hub_app_init()` 初始化 Mock IMU fallback，并对 ICM42688 执行软复位、`WHO_AM_I=0x47`、量程、100 Hz ODR、约 25 Hz UI LPF、record-count FIFO、4 条记录 watermark、threshold/full INT1 和 `PWR_MGMT0` 配置。
4. PB12 EXTI ISR 只累计 FIFO 事件；主循环以最短 30 ms 周期读取 FIFO record count，一次 burst 最多读取并解析 16 条完整组合记录。
5. 解析器保留 16 字节组合包，跳过但在时间轴中计入 8 字节单传感器包；overflow、畸形包或容量不足时 flush FIFO 并记录错误。FIFO_DATA 失败后恢复总线但不重放有副作用的读取。
6. INT1/FIFO 最近 250 ms 内成功输出样本时 heartbeat bit 7/8 置位；超时或连续读取失败时进入 Mock fallback，并每 1 秒尝试重新初始化 ICM42688。
7. UART4 IRQ 持续把接收字节写入 64 字节 RX 环形缓冲；UART4/USART1 TX 由各自 1024 字节队列和完成回调推进。
8. 若运行中 I2C 读取失败，BSP 会硬复位 I2C2、释放总线并重试；重试仍失败时设置诊断位并由 IMU 路径进入 fallback/周期重初始化。

烧录/验证流程：

1. `flash_f407_uart.ps1` 通过一键下载电路控制 DTR/RTS 进入 STM32 ROM UART Bootloader。
2. 写入 `sensor_hub.bin` 到 `0x08000000`，并执行逐字节回读校验。
3. 校验成功后脚本发送 STM32 ROM Bootloader `GO 0x08000000` 启动应用，并释放 DTR/RTS 到运行态。
4. 当前应用态协议帧通过 UART4 输出到 MP157 USART3，同时镜像到 USART1，便于通过 COM6 抓包诊断。
4. 若刷写后第一次抓包为 0 字节，重新运行验证脚本或手动复位；该现象来自一键下载电路应用态复位状态切换。

### Step 6：ICM42688 I2C 接入

- [x] 在 CubeMX 中配置 PB10 为 `I2C2_SCL`
- [x] 在 CubeMX 中配置 PB11 为 `I2C2_SDA`
- [x] 在 CubeMX 中配置 PB12 为 `ICM42688_INT1` 上升沿 EXTI12
- [x] 新增 PB12 INT1 事件计数、合并消费和清空 BSP 封装
- [x] 新增 I2C2 初始化和 `HAL_I2C_Mem_Read/Write` BSP 封装
- [x] 移植 ICM42688 `WHO_AM_I`、100 Hz accel/gyro 配置和数据读取逻辑
- [x] F407 IMU 上报优先使用 ICM42688，失败时回退 Mock IMU 并设置 heartbeat `status_flags`
- [x] 修正实际接线为 `SCL -> PB10`、`SDA -> PB11` 后，上板验证真实 ICM42688 IMU 数据路径
- [x] 将 F407 侧 ICM42688 读取和 IMU 帧上报周期调整为 100 Hz
- [x] 上板复测 100 Hz IMU 输出，5 秒抓取 heartbeat 5 帧、IMU 501 帧、`imu_rate_hz=100.2`、CRC 错误 0 帧
- [x] 将轮询改为 INT1 data-ready 触发；60 秒抓取 IMU 5547 帧、`imu_rate_hz=92.45`、最终 `status_flags=0x00A9`
- [x] 配置 ICM42688 约 25 Hz UI LPF，并为三类传感器增加定点 IIR 与主机单元测试
- [x] 启用 FIFO；历史 byte-count/可变包方案按 ADR-0021 修订为 record-count、4 条记录 watermark、threshold/full INT1 和最多 16 条完整记录批量读取
- [x] 为 FIFO 包头、无效样本、截断、16/8/消息混合流、批次时间轴、provider 寄存器配置和异常 flush 增加主机测试
- [x] 整板断电恢复后完成 FIFO 真实传感器 60 秒回归

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

- [x] MMC5603 `VIN/GND/SCL/SDA` 接入 3.3 V、GND、PB10、PB11
- [x] 实现软件复位、产品 ID 检查、SET/RESET 脉冲和 20-bit XYZ 读取
- [x] 新增 20 Hz `sensor_magnetometer` 帧并完成 MP157 UART 抓包验证
- [x] 集成 Bosch BMP3 Sensor API v2.0.5 和 `bmp390_provider_c`
- [x] 实现 `0x77/0x76` 地址探测、64-bit 补偿、25 Hz normal mode 和 10 Hz `sensor_barometer`
- [x] 完成 F407 ARM、MP157 Runtime、交叉编译和 frame decoder 构建验证
- [x] 烧录 BMP390 软件路径并确认 F407 应用态协议链路正常
- [x] 完成 BMP390 真实 I2C ACK、协议帧和 MP157 `barometer_seen=true` 上板验证
- [x] 将 PB12 `ICM42688_INT1` 升级为 EXTI data-ready；ISR 只记录事件，I2C 读取保留在主循环。
- [x] 增加 I2C 瞬时错误后的 I2C2 外设硬复位、最多 18 个 SCL 释放脉冲、STOP 和单次事务重试策略；2026-07-19 已完成 60 秒三传感器持续验证。
- [x] ICM42688 FIFO 已由早期 byte-count/可变包方案修订为 record-count、4 条记录 watermark 和完整 16 字节记录解析，异常路径会 flush 并设置诊断位。
- [x] UART4/USART1 TX 已改为固定内存中断队列；当前无需引入 DMA。
- [x] PC mock C++ 层未使用的 `Icm42688Driver` 占位接口已删除。
- [x] 新增 UART4/USART1 TX overflow、UART4 RX overflow、ICM init 和 I2C 运行期 failure 诊断位。
- [x] 将每秒 `0x02` 累计 diagnostics 同时发送到 UART4/MP157 与 USART1/COM6，并升级为兼容 legacy 44 字节的 48 字节 schema 1；MP157 解析、主机测试、实板零值和 RX drop 2231 压力注入均已验证。
- [x] 完成 FIFO/TX 队列版本烧录回读、COM6/USART1 镜像和 MP157 UART4 双向验收。
- [x] 整板复电后完成真实 ICM42688 FIFO 60 秒和多次独立复位验收。

MMC5603 上板验收（2026-06-25）：

- 地址 `0x30` 初始化成功，heartbeat MMC5603 ready 位有效。
- 5 秒收到 100 个磁力计帧，频率 `19.8 Hz`。
- 平均磁场约 `(-35.60, -25.18, 18.16) μT`，合成强度约 `47.24 μT`。
- 该次 heartbeat 为 `0x000E`：MMC5603 ready，同时 ICM42688 处于 fallback/error；2026-07-18 当前接线最终 heartbeat 已更新为 `0x0029`。

BMP390 当前接线方案和验收边界：

```text
BMP390 VCC -> F407 3.3V
BMP390 GND -> F407 GND
BMP390 SCL -> F407 PB10
BMP390 SDA -> F407 PB11
BMP390 CSB -> F407 3.3V
BMP390 SDO -> 3.3V (0x77) 或 GND (0x76)
BMP390 INT -> NC
```

2026-07-18 当前接线已完成真实 ACK 和 `sensor_barometer` 帧验证：heartbeat bit 5 `0x0020` 置位、bit 6 `0x0040` 未置位，MP157 输出 `barometer_seen=true`。2026-07-19 的 60 秒采集得到平均气压 `80383.7 Pa`、平均温度 `28.00 °C`，10.03 Hz 且无异常样本；室外对照测量和小时级稳定性仍需继续验证。

### Step 8：MP157 Live Serial Integration

- [x] 在 MP157 Runtime 新增真实 MCU 串口输入路径，默认保留 mock 文件模式
- [x] 新增配置项：`mcu_input_mode = mock_file | serial`
- [x] 新增配置项：`mcu_serial_device = /dev/ttySTM1`
- [x] 新增配置项：`mcu_serial_baud = 115200`
- [x] 新增配置项：`mcu_serial_capture_seconds = 5`
- [x] 将 F407 UART4 PC10/PC11 和 GND 接入 MP157 USART3 PD9/PD8/GND
- [x] 在 MP157 上验证 `runtime_status.json` 输出真实 heartbeat 和 IMU 状态
- [x] F407 当前使用中断 TX 队列消除阻塞；正常业务负载未见 TX overflow。UART4 RX 已完成故意饱和注入，DMA 是否需要采用仍按真实命令负载的最坏延迟和容量测量决定
- [x] 将 MP157 Runtime 演进为单线程协作式服务调度，使持续串口采集、板载 IMU、状态发布和常驻 dashboard 可交错运行
- [x] 通过 Runtime Manager 单元测试、本机一秒持续模式和 ARMv7 hard-float 交叉构建验证协作调度实现
- [x] 使用 `scripts/run_board_long_validation.sh` 验证独立 SD 证据目录和独占保护；竞争进程及 COM6 控制线复位污染的数据均已作废，复电后已完成健康预检和 60 秒冒烟
- [x] 增加 `run_board_health_preflight.sh`，并让小时长测和异常恢复在 F407 `0x01A9`、diagnostics schema 1、UART4 RX drop 0、四类 MCU 数据、ping ACK、GNSS NMEA、Board IMU、Logger 健康与核心错误门槛通过前拒绝启动
- [x] 真实板连续两次通过 `0x01A9` 预检，首个 60 秒联合冒烟结束审计通过；第二次启动的 PID `4868` 已因发现瞬态 FIFO fallback 主动作废
- [x] PID `4868` 早期出现 `0x022E/0x03AE` 后自恢复，证明最终快照不足；任务已停止作废并新增逐秒健康时间线，等待从零重新计时
- [x] 20 秒真实健康时间线冒烟正确拒绝 5 个降级样本和最终 `0x03AE`；UART4 diagnostics 后续已刷板完成两轮累计 I2C/FIFO 失败归因，并以 schema 1 发布 UART4 RX drop
- [x] 连续 40960 字节下行使 heartbeat 进入 `0x302E`、UART4 RX drop 达 2231，预检正确拒绝；F407 应用复位后 drop 归零
- [x] 完整断电重上电恢复 `0x01A9` 初始化，但 diagnostics 在约 1–3 分钟内继续累计 recovery/overflow/malformed，第二次预检以 `0x03A9` 拒绝；100 kHz 和 fixed partial-read A/B 均已证伪并撤回
- [x] 新增默认关闭、heartbeat bit `0x8000` 标识的 ICM-only 编译模式，以及要求 `0x8181` 和 I2C/FIFO/init/drop 全零的 MP157 预检 profile；诊断镜像已刷写回读
- [ ] COM6 关闭后的 400 kHz ICM-only `XMWvGC` 仍以 recovery/failure `232/45` 和 FIFO 累计异常失败；单器件 100 kHz 完整复电 A/B 更退化为 `0xD206/init step 1`，recovery/failure 速率为 400 kHz 的 `3.29/3.67` 倍。F407 USB-UART 改枚举为 COM3 后，400 kHz ICM-only 基线已重新烧录并逐字节回读通过。先取得供电/上拉/接触/波形证据并通过 8/30/60 秒零累计门槛，再刷回正式镜像按 MMC5603、BMP390 顺序逐颗接回
- [x] 将 ARM Runtime、配置和验证脚本部署到持久化 `/opt/outdoor-agent`，软重启后完成 SHA256、权限和 shell 语法复验
- [x] 增加 GNSS fix 限时验收脚本和合法 MCU fixture；室内 5 秒负向测试正确拒绝零卫星/无 fix，室外正向测试待执行
- [ ] 在真实 MP157 上完成 GNSS/MCU 双串口、板载 IMU、framebuffer 小时级长期运行验证

### Step 9：MP157 Downlink Command Prototype

- [x] 新增 `command_ping` / `command_ack` 帧类型，保持既有 SOF、version、sequence、payload_length 和 CRC16/MODBUS 帧格式
- [x] MP157 Runtime 新增 `mcu_command = none | ping` 配置项和 `--mcu-command none|ping` 命令行参数
- [x] MP157 serial 模式将 `/dev/ttySTM1` 以读写方式打开，发送 ping 后继续读取 F407 上行帧和 ack
- [x] F407 新增 UART4 RX 中断接收和 64 字节环形缓冲 BSP
- [x] F407 新增 C 版命令帧 decoder，当前只接受 4 字节 nonce 的 `command_ping`
- [x] F407 收到 ping 后返回 `command_ack`，payload 包含 `request_type`、`status`、reserved 和原 nonce
- [x] MP157 `runtime_status.json` 输出 `command_ack_seen`、`command_ack_request_type`、`command_ack_status` 和 `command_ack_nonce`
- [x] 本机和交叉编译构建通过，单元测试覆盖 ping 构帧和 ack 解析
- [x] 在真实板上使用 MP157 shell 向 `/dev/ttySTM1` 发送 raw `command_ping`，确认 F407 返回 `command_ack`
- [x] 在真实板上执行当前 ARM Runtime `--mcu-command ping`，确认 `command_ack_seen=true`、`command_ack_status=0`

真实板上验证命令：

```bash
cd /tmp/ai_outdoor_runtime_serial
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5 --mcu-command ping
cat runtime/runtime_status.json
```

2026-06-21 raw shell 验证命令：

```bash
stty -F /dev/ttySTM1 115200 raw -echo -ixon -ixoff -crtscts
exec 3<>/dev/ttySTM1
printf '\xA5\x5A\x01\x80\x01\x00\x04\x00\x47\x4E\x49\x50\xC3\x43' >&3
timeout 3 dd bs=1 count=1024 <&3 2>/dev/null | hexdump -C
```

2026-06-21 raw 验证结果：回包首帧为 `a5 5a 01 81 ... 80 00 00 00 47 4e 49 50 ...`，表示 F407 已识别 `command_ping`，`command_ack.status=0`，并原样返回 nonce `0x50494E47`。当时通过串口 console 上传大包遇到 input overrun，因此未完成 Runtime 状态级复测。

2026-07-18 使用新增 `scripts/send_xmodem.ps1` 以 XMODEM-CRC 通过 MP157 COM9 上传当前 ARM Runtime；本地和板端 SHA256 一致。UART4 RX 从零超时轮询修复为中断接收和 64 字节环形缓冲后，连续 3 个 raw ping 均返回 ACK；当前 Runtime 最终输出 `heartbeat_seen=true`、`imu_seen=true`、`magnetometer_seen=true`、`barometer_seen=true`、`status_flags=0x0029`、`command_ack_seen=true`、`command_ack_status=0` 和 nonce `0x50494E47`。

### Step 10：MP157 SD History and Log Retention

- [x] 新增 GNSS、MCU IMU、磁力计、气压计和 Board IMU 独立 CSV History Recorder
- [x] 通过状态计数/uptime 去重，保留 host UTC、MCU uptime 和协议原始整数单位
- [x] 新增 `history_enabled`、`history_output_path`、`history_flush_interval_ms` 和 `--history` / `--history-output`
- [x] 新增 `storage_log_max_bytes` 和 `storage_log_backup_count`，按 `.1` 到 `.N` 轮转日志
- [x] History/Logger 单元测试、Runtime storage/history 组合脚本和 ARMv7 hard-float 交叉构建通过
- [x] 真实 MP157 60 秒双串口/Board IMU/framebuffer/SD history 冒烟通过，最终 state stopped、四类 MCU seen、ping ACK status 0
- [x] 修复串口批次内快照覆盖后，20 秒 framebuffer history 得到 MCU IMU 约 98.1 Hz、磁力计约 20.1 Hz、气压计 10.0 Hz
- [x] 30 秒 info 日志测试生成 718 KiB 活动日志和 1.0 MiB `.1` 备份，验证 VFAT SD 卡按大小轮转
- [ ] 在真实 MP157 SD 卡上执行小时级联合记录、日志轮转、掉电恢复和文件增长验收

板端待验证命令：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --storage-root /run/media/mmcblk1p1/outdoor-agent --history \
  --gnss-input-mode serial --gnss-serial-capture-seconds 0 \
  --mcu-input-mode serial --mcu-serial-capture-seconds 0 \
  --board-imu --board-imu-samples 0 \
  --dashboard-output-mode both --dashboard-refresh-count 0 \
  --runtime-run-seconds 3600
```

## 暂不处理

- RTOS 任务模型
- UART DMA 优化
- Bootloader 和固件升级
- 量产烧录流程

这些内容在最小裸机固件构建和串口链路稳定后再推进。
