# Dev Log

## 2026-06-19 - F407 100 Hz IMU Sampling Target

### 本次完成

- 将 F407 `sensor_hub_app_poll()` 的 IMU 读取和上报周期从 100 ms 调整为 10 ms，对齐 ICM42688 accel/gyro 100 Hz ODR 配置。
- 增强 `scripts/verify_f407_uart.ps1`，输出 `frame_rate_hz` 和 `imu_rate_hz`，并新增 `-MinImuHz` 参数；默认要求 IMU 帧率不低于 80 Hz。
- 复盘 F407 与 ICM42688 之间尚未完成的 Sensor Hub 项：INT1/EXTI、I2C 错误恢复、FIFO、UART DMA/环形缓冲、PC mock C++ 占位接口清理。
- 补充 F407/ICM42688 上电时序、初始化寄存器流程、运行工作流和 UART Bootloader 烧录/验证流程文档。

### 验证结果

- 已执行 100 Hz 上板验证：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5 -MinImuHz 80
```

- F407 固件构建成功，Debug 固件使用 10176 B Flash、1768 B RAM。
- UART Bootloader 识别成功：Bootloader version `0x31`，Chip ID `0x0413`。
- 固件写入和逐字节回读校验成功。
- 第一次抓包读到 0 字节，重新触发应用复位后恢复正常输出。
- 第二次 5 秒读取 17132 字节，共解析 506 帧。
- heartbeat：5 帧。
- IMU：501 帧。
- `imu_rate_hz=100.2`。
- CRC 错误：0 帧。
- 最后一帧 heartbeat `status_flags=0x0001`，表示 ICM42688 ready，IMU 帧来自真实 ICM42688 数据路径。

### 后续 TODO

- 在进入 F407 -> MP157 联调前，决定 INT1/EXTI、I2C 错误恢复和 UART 发送方式是否需要先补齐。

## 2026-06-19 - ICM42688 I2C Firmware Data Source

### 本次完成

- 阅读 ICM42688 参考 HAL 工程和模块规格书，确认 `AD0=3.3V` 时 I2C 7-bit 地址为 `0x69`。
- 明确当前板级接线需要修正为 `ICM42688 SCL -> F407 PB10`、`ICM42688 SDA -> F407 PB11`；此前描述的 `SDA -> PB10`、`SCL -> PB11` 与 CubeMX 配置相反。
- 新增 `MX_I2C2_Init()`，启用 F407 `I2C2`，配置 100 kHz 标准模式。
- 补充 STM32F4 HAL I2C 源文件并加入 F407 固件 CMake 构建。
- 新增 `board_i2c_mem_read()` / `board_i2c_mem_write()` BSP，底层使用 `HAL_I2C_Mem_Read/Write`。
- 新增 ICM42688 固件数据源：`WHO_AM_I=0x47` 检测、accel ±4g、gyro ±1000 dps、100 Hz ODR、温度/加速度/陀螺仪定点换算。
- F407 IMU 上报优先使用 ICM42688，初始化或读取失败时回退 Mock IMU，并通过 heartbeat `status_flags` 标记状态。
- 将 PB12 命名为 `ICM42688_INT1` 并新增 `board_icm42688_data_ready()` BSP。
- 增强 `scripts/verify_f407_uart.ps1`，输出最后一帧 heartbeat 的 `status_flags`，便于区分真实 ICM42688 数据和 Mock fallback。

### 修改文件

- `f407/sensor-hub/firmware/stm32cube/Core/Inc/i2c.h`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/i2c.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/main.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/stm32f4xx_hal_msp.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Inc/stm32f4xx_hal_conf.h`
- `f407/sensor-hub/firmware/bsp/`
- `f407/sensor-hub/firmware/sensors/`
- `f407/sensor-hub/firmware/protocol/mcu_frame_builder_c.h`
- `f407/sensor-hub/firmware/CMakeLists.txt`
- `README.md`
- `f407/sensor-hub/README.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `docs/stage1_bringup_plan.md`
- `docs/mcu_protocol.md`
- `docs/repo_structure.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `scripts/verify_f407_uart.ps1`

### 验证结果

- 已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_f407.ps1 -BuildType Debug
```

- F407 固件构建成功，Debug 固件使用 10176 B Flash、1768 B RAM。
- F407 PC mock CTest 1/1 通过。
- `git diff --check` 通过，仅保留 Cube 生成文件 CRLF/LF 转换 warning。
- 已修正接线为 `SCL -> PB10`、`SDA -> PB11` 后执行上板验证：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5
```

- UART Bootloader 识别成功：Bootloader version `0x31`，Chip ID `0x0413`。
- 固件写入和逐字节回读校验成功。
- 5 秒读取 1781 字节，共解析 55 帧。
- heartbeat：5 帧。
- IMU：50 帧。
- CRC 错误：0 帧。
- 最后一帧 heartbeat `status_flags=0x0001`，表示 ICM42688 ready，IMU 帧来自真实 ICM42688 数据路径。
- 第一次刷写后串口验证曾读到 0 字节，重新触发应用复位后恢复正常输出；该现象与此前一键下载电路应用态复位状态切换一致。

### 后续 TODO

- 完成文档一致性清理后，将当前 F407 + ICM42688 里程碑作为可提交节点固定下来。
- 在 MP157 Runtime 新增真实 MCU 串口输入路径，复用现有 `McuFrameParser`、`ImuPayloadParser` 和 `runtime_status.json` 输出。
- 新增串口相关配置项：`mcu_input_mode`、`mcu_serial_device`、`mcu_serial_baud`、`mcu_serial_capture_seconds`；默认继续使用 mock 文件模式。
- 完成 F407 USART1_TX/GND 到 MP157 UART_RX/GND 的板间验证。
- 根据 INT1 实际输出模式决定是否将 PB12 从普通输入升级为 EXTI 数据就绪中断。
- 根据串口稳定性评估 F407 USART 从阻塞式 `HAL_UART_Transmit()` 演进到 DMA/环形缓冲。

## 2026-06-17 - IMU Pin Reservation

### 本次完成

- 接收新的 F407 CubeMX 管脚配置：PB10/PB11/PB12 用于后续 IMU 对接。
- 舍弃此前错误模块资料假设，后续以实际模块资料为准。
- 恢复 CubeMX 重新生成后被覆盖的 USART1 初始化和 Sensor Hub app 主循环接入。
- 将 PB12 调整为 IMU INT 输入，并新增数据就绪 BSP 封装。

### 修改文件

- `f407/sensor-hub/firmware/stm32cube/`
- `f407/sensor-hub/firmware/bsp/board_icm42688_gpio.*`
- `f407/sensor-hub/firmware/bsp/stm32/board_icm42688_gpio_stm32.c`
- `f407/sensor-hub/firmware/CMakeLists.txt`
- `README.md`
- `f407/sensor-hub/README.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `docs/project_design.md`
- `docs/mcu_protocol.md`
- `docs/repo_structure.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `scripts/flash_f407_uart.ps1`

### 验证结果

- 已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_f407.ps1 -BuildType Debug
```

- F407 固件构建成功，生成 `sensor_hub.elf`、`sensor_hub.hex`、`sensor_hub.bin` 和 `sensor_hub.map`。
- Debug 固件使用 6624 B Flash、1680 B RAM。
- 已执行：

```powershell
cmake -S f407/sensor-hub -B f407/sensor-hub/build
cmake --build f407/sensor-hub/build
ctest --test-dir f407/sensor-hub/build -C Debug --output-on-failure
```

- F407 PC mock CTest 1/1 通过。
- 已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
```

- UART Bootloader 识别成功：Bootloader version `0x31`，Chip ID `0x0413`。
- 固件写入和逐字节回读校验成功。
- 已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5
```

- 5 秒读取 1809 字节，共解析 55 帧。
- heartbeat：5 帧。
- Mock IMU：50 帧。
- CRC 错误：0 帧。

### 遇到的问题与解决方案

- 问题：CubeMX 重新生成后覆盖了 `main.c` 中的 `MX_USART1_UART_Init()`、`sensor_hub_app_init()`、`sensor_hub_app_poll()` 和 LED 心跳逻辑，同时 `HAL_UART_MODULE_ENABLED` 与 USART1 MSP 初始化缺失。
  解决：恢复 USART1 初始化、Sensor Hub app 主循环入口、PF9 LED 心跳、HAL UART 模块开关和 PA9/PA10 USART1 MSP 初始化。
- 问题：早期模块资料假设错误。
  解决：舍弃错误资料实现依据，将 PB12 作为 IMU INT 输入，等待真实模块资料再实现驱动。
- 问题：普通全片擦除阶段等待 ACK 时，`scripts/flash_f407_uart.ps1` 使用 2 秒串口读超时，实测在 F407 上会偶发超时。
  解决：将 erase 阶段 ACK 等待窗口临时扩展到 30 秒，并在擦除完成后恢复原串口超时。
- 问题：刷写成功后第一次串口验证读到 0 字节。
  解决：复核一键下载电路 DTR/RTS 状态，确认 `DTR=false / RTS=true` 会停在非应用输出状态；切回应用复位状态后复测通过。

### 后续 TODO

- 等待正确模块资料后实现 I2C 初始化、寄存器读取、数据换算和真实传感器协议帧。
- 根据真实数据格式决定是否替换当前 Mock IMU 帧或新增独立传感器帧。

## 2026-06-16 - F407 UART Bootloader Flash and Frame Validation

### 本次完成

- 新增 `scripts/flash_f407_uart.ps1`，支持通过 CH340 `COM3` 进入 STM32 ROM UART Bootloader。
- 处理 F407 读保护解除流程：`Readout Unprotect`、等待完成 ACK、重新进入 Bootloader。
- 通过 UART Bootloader 烧录 `sensor_hub.bin` 到 `0x08000000`。
- 对烧录内容进行逐字节回读校验。
- 新增 `scripts/verify_f407_uart.ps1`，抓取并解析 F407 USART1 二进制 MCU 帧。

### 修改文件

- `scripts/flash_f407_uart.ps1`
- `scripts/verify_f407_uart.ps1`
- `README.md`
- `f407/sensor-hub/README.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `docs/project_design.md`
- `docs/dev_log.md`

### 验证结果

- 已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3 -ReadoutUnprotectFirst
```

- 烧录和逐字节回读校验成功。
- 已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 5
```

- 5 秒读取 1781 字节，共解析 55 帧。
- heartbeat：5 帧。
- Mock IMU：50 帧。
- CRC 错误：0 帧。

### 遇到的问题与解决方案

- 问题：本机最初无法直接从 PATH 调用 STM32CubeProgrammer CLI，且未发现 ST-LINK。
  解决：用户确认 CLI 路径为 `D:\Program Files (x86)\ST\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`；本次优先走 F407 ROM UART Bootloader，不依赖 ST-LINK。
- 问题：STM32_Programmer_CLI 通过 UART 连接时会改变 DTR/RTS，导致一键下载电路进入 Bootloader 的时序被打断。
  解决：新增 `scripts/flash_f407_uart.ps1`，在同一个串口会话内控制 DTR/RTS 并直接实现 Bootloader 写入、回读和校验流程。
- 问题：初始 Bootloader 只接受 Get、GetVersion、GetID、ReadoutUnprotect，Read/Write/Erase 返回 NACK，表现为读保护状态。
  解决：执行 `Readout Unprotect (0x92)`，等待约 15 秒完成 ACK；该操作会擦除用户 Flash，随后重新进入 Bootloader 并跳过额外 erase，直接写入固件。
- 问题：烧录后应用态串口一度没有输出。
  解决：确认一键下载电路的应用复位时序，验证脚本使用 `DTR=true`、`RTS=false -> true` 后读取 115200 8N1 二进制帧。
- 问题：PowerShell 验证脚本早期误报 CRC 错误。
  解决：在 CRC 计算和接收 CRC 拼接处显式将 byte 转为 `[int]` 后再移位/异或，复测 5 秒 55 帧、CRC 错误 0 帧。

### 后续 TODO

- 在 MP157 Runtime 新增 `/dev/tty*` 串口二进制输入源。
- 将真实串口字节流送入 `McuFrameParser`，替换或并存当前 mock 文件输入。

## 2026-06-15 - F407 Mock Data UART Reporting

### 本次完成

- 配置 USART1：PA9 TX、PA10 RX、115200 8N1、无流控。
- 补充 STM32F4 HAL UART 驱动并加入固件构建。
- 将 `board_get_tick_ms()` 对接到 `HAL_GetTick()`。
- 将 `board_uart_send_bytes()` 对接到阻塞式 `HAL_UART_Transmit()`。
- 将 Sensor Hub app、C 协议构建和 Mock IMU 数据源加入 ARM 固件。
- 在 Cube 主循环调用 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()`。
- LED 心跳改为非阻塞轮询，避免影响 100 ms IMU 上报周期。

### 修改文件

- `f407/sensor-hub/firmware/stm32cube/atk_f407_sensorhub.ioc`
- `f407/sensor-hub/firmware/stm32cube/Core/Inc/usart.h`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/usart.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/main.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/stm32f4xx_hal_msp.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Inc/stm32f4xx_hal_conf.h`
- `f407/sensor-hub/firmware/stm32cube/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_uart.h`
- `f407/sensor-hub/firmware/stm32cube/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c`
- `f407/sensor-hub/firmware/bsp/stm32/`
- `f407/sensor-hub/firmware/CMakeLists.txt`
- 相关 README、Stage 1、设计和协议文档

### 验证结果

- F407 ARM 固件构建通过。
- Debug 固件使用 6544 B Flash、1680 B RAM。
- 已生成新的 ELF、HEX、BIN 和 map。
- ELF 中确认 `sensor_hub_app_poll()`、`board_uart_send_bytes()`、`HAL_UART_Transmit()` 和两个帧构建函数均已链接。
- F407 PC mock CTest 1/1 通过。
- 尚未执行真实串口抓包和 MP157 板间联调。

### 后续 TODO

- 烧录新固件并用 USB-UART 抓取二进制帧。
- 验证 heartbeat 1 Hz、Mock IMU 10 Hz 和 CRC。
- 在 MP157 Runtime 新增真实串口输入源，将二进制字节流送入 `McuFrameParser`。

## 2026-06-15 - F407 GNU ARM/CMake Firmware Build

### 本次完成

- 安装并验证 GNU Arm Embedded Toolchain 14.2.Rel1。
- 新增 F407 固件独立 CMake 工程和 Windows 构建脚本。
- 新增 STM32F407ZG Flash、SRAM、CCMRAM 链接布局。
- 新增 GCC 启动、异常/中断向量表和最小 newlib 系统调用。
- 直接编译 Cube Core、HAL 和 CMSIS 依赖。
- 自动生成 `sensor_hub.elf`、`.hex`、`.bin`、`.map` 和 size 输出。
- 在 Cube `main.c` 的 `USER CODE` 区域加入 PF9 LED0 每 500 ms 翻转的基线逻辑。

### 修改文件

- `f407/sensor-hub/firmware/CMakeLists.txt`
- `f407/sensor-hub/firmware/cmake/arm-none-eabi-gcc.cmake`
- `f407/sensor-hub/firmware/linker/STM32F407ZGTx_FLASH.ld`
- `f407/sensor-hub/firmware/startup/startup_stm32f407xx.c`
- `f407/sensor-hub/firmware/platform/syscalls.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/main.c`
- `scripts/build_f407.ps1`
- `README.md`
- `f407/sensor-hub/README.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_plan.md`
- `docs/stage1_bringup_plan.md`
- `docs/adr/0008-f407-cube-source-and-cmake-build-boundary.md`
- `docs/changelog.md`

### 验证结果

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build_f407.ps1 -BuildType Debug
```

- 构建成功，Debug 基线使用 4572 B Flash、1584 B RAM。
- ELF 入口为 `0x0800024d`，向量表位于 `0x08000000`，栈顶为 `0x20020000`。
- 已生成 ELF、HEX、BIN 和 map 文件。
- F407 PC mock CMake 构建通过，CTest 1/1 通过。
- 项目自有纯 C 固件骨架通过本机 `gcc -fsyntax-only`。
- 当前环境未发现 STM32CubeProgrammer 或 ST-LINK CLI，因此未执行下载和板上 LED 验证。

### 后续 TODO

- 安装或配置 STM32CubeProgrammer/ST-LINK CLI。
- 下载固件并验证 PF9 LED0 每 500 ms 翻转。
- 验证后接入 Sensor Hub app 和 HAL tick。

## 2026-06-15 - STM32Cube F407 Baseline Import

### 本次完成

- 浏览并盘点 STM32CubeMX 生成的 STM32F407ZG 基础工程。
- 将 Cube 生成内容从 `f407/sensor-hub/stm32cube/` 移动到 `f407/sensor-hub/firmware/stm32cube/`。
- 明确 PC mock、项目自有固件代码和 Cube 生成代码的目录边界。
- 将自主固件编译拆分为 GNU ARM/CMake、最小上板、应用接入和 UART 联调步骤。
- 新增 ADR-0008，记录保留 Cube 源码基线并由仓库维护独立 CMake 构建的决策。

### 修改文件

- `f407/sensor-hub/firmware/stm32cube/`
- `f407/sensor-hub/README.md`
- `.gitignore`
- `README.md`
- `docs/repo_structure.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `docs/stage1_bringup_plan.md`
- `docs/adr/0008-f407-cube-source-and-cmake-build-boundary.md`
- `docs/changelog.md`

### 验证结果

- 已检查 Cube `.ioc`、`main.c`、GPIO 初始化和 MDK-ARM 工程源文件清单。
- 已确认当前 Cube 工程仅配置系统时钟和 PF9/PF10 LED GPIO，未配置 UART。
- 已确认当前环境可用 CMake 和 Ninja，但未安装 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy`。
- 已执行 F407 PC mock CMake 构建和 CTest。
- 未执行真实 F407 固件编译：当前缺少 GNU ARM 工具链、GNU 启动文件、链接脚本和固件 CMake 入口。

### 后续 TODO

- 完成 Step 2：建立最小 GNU ARM/CMake 固件构建。
- 生成 ELF、HEX、BIN、map 和 size 输出。
- 再进行 LED 上板基线和 Sensor Hub 应用接入。

## 2026-06-14 - F407 Firmware Bring-up Skeleton

### 本次完成

- 新增 `f407/sensor-hub/firmware/` 纯 C 固件骨架。
- 新增 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()`。
- 新增 `board_uart_send_bytes()` 和 `board_get_tick_ms()` BSP 占位接口。
- 新增 C 版 CRC16/MODBUS、MCU frame builder 和 Mock IMU Provider。
- `sensor_hub_app_poll()` 当前每 1000 ms 发送 heartbeat，每 100 ms 发送 Mock IMU frame。
- 新增 `docs/stage1_bringup_plan.md`，记录后续对接 STM32 HAL UART 的计划。

### 修改文件

- `f407/sensor-hub/firmware/`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/changelog.md`
- `README.md`

### 验证结果

- 已执行：

```bash
cmake -S f407/sensor-hub -B f407/sensor-hub/build
cmake --build f407/sensor-hub/build
ctest --test-dir f407/sensor-hub/build -C Debug --output-on-failure
```

- 已执行：

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -If407/sensor-hub/firmware -fsyntax-only f407/sensor-hub/firmware/app/sensor_hub_app.c f407/sensor-hub/firmware/bsp/board_uart.c f407/sensor-hub/firmware/bsp/board_tick.c f407/sensor-hub/firmware/protocol/crc16_modbus_c.c f407/sensor-hub/firmware/protocol/mcu_frame_builder_c.c f407/sensor-hub/firmware/sensors/mock_imu_provider_c.c
```

- 已执行：

```bash
cmake --build mp157/outdoor-core-service/build
ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1
```

- 验证结果：F407 PC mock 构建和测试通过，新增 firmware C 文件语法检查通过，MP157 Runtime 构建和测试通过，Runtime 验证脚本通过。

### 后续 TODO

- 在 STM32Cube HAL 工程中将 `board_get_tick_ms()` 对接到 `HAL_GetTick()`。
- 在 STM32Cube HAL 工程中将 `board_uart_send_bytes()` 对接到 `HAL_UART_Transmit()`。
- 使用真实串口链路验证 heartbeat 和 Mock IMU frame。

## 2026-06-14 - Stage 1 Mock IMU Chain

### 本次完成

- 在 `common/protocol` 中新增 MCU 公共协议常量、CRC16/MODBUS、`MSG_TYPE_SENSOR_IMU` 和 `ImuSample`。
- 在 F407 侧新增 `MockImuProvider`、IMU 协议帧打包和 `Icm42688Driver` 占位接口。
- 在 MP157 侧新增 IMU payload parser，并将 Mock IMU 数据输出到 `runtime_status.json` 的 `imu` 字段。
- 新增 CRC、帧解析、IMU payload 解析和 F407 mock 打包测试。
- 新增 `tools/frame_decoder`，用于解析十六进制 MCU 协议帧。

### 修改文件

- `common/protocol/`
- `f407/sensor-hub/`
- `mp157/outdoor-core-service/`
- `tools/frame_decoder/`
- `docs/stage1_plan.md`
- `docs/changelog.md`
- `docs/mcu_protocol.md`
- `docs/project_design.md`
- `docs/repo_structure.md`

### 验证结果

- 已执行：

```bash
cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build
cmake --build mp157/outdoor-core-service/build
ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure
```

- 已执行：

```bash
cmake -S f407/sensor-hub -B f407/sensor-hub/build
cmake --build f407/sensor-hub/build
ctest --test-dir f407/sensor-hub/build -C Debug --output-on-failure
```

- 已执行：

```bash
cmake -S tools/frame_decoder -B tools/frame_decoder/build
cmake --build tools/frame_decoder/build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1
```

- 验证结果：MP157 Runtime 构建通过，F407 mock 模块构建通过，frame_decoder 构建通过，CTest 全部通过，Runtime 验证脚本通过。

### 后续 TODO

- ICM42688 到货后，将 F407 侧 Mock IMU 数据源替换为真实驱动读取。
- 后续硬件阶段补充 STM32F407ZG 真实固件部署和串口链路验证。

## 2026-06-11 - Monorepo Structure

### 本次完成

- 将 MP157 Linux Runtime 工程移动到 `mp157/outdoor-core-service/`。
- 新增 `f407/sensor-hub/`、`common/protocol/`、`tools/` 和根级 `scripts/`。
- 根目录 `README.md` 改为 Monorepo 总项目说明。
- `mp157/outdoor-core-service/README.md` 改为模块级编译和运行说明。
- 新增 `docs/repo_structure.md` 说明仓库结构。
- 新增 `scripts/build_mp157.sh`，用于从仓库根目录构建 MP157 Runtime。

### 修改文件

- `README.md`
- `docs/repo_structure.md`
- `docs/project_design.md`
- `docs/mcu_protocol.md`
- `docs/dev_log.md`
- `scripts/build_mp157.sh`
- `mp157/outdoor-core-service/`
- `f407/sensor-hub/.gitkeep`
- `common/protocol/.gitkeep`
- `tools/.gitkeep`

### 验证结果

- 已执行：

```bash
cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build
cmake --build mp157/outdoor-core-service/build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1
```

- 已执行仓库级构建脚本验证：

```bash
scripts/build_mp157.sh
```

- 验证结果：MP157 Runtime CMake 构建通过，模块验证脚本通过，仓库级构建脚本在补齐 shell PATH 后通过。

### 后续 TODO

- 后续小 stage 再处理 `common/protocol/` 中共享协议头文件抽取。
- 暂不实现 F407 固件代码。

## 2026-06-10 - Stage 1 Linux Runtime MCU Protocol

### 本次完成

- 进入 Stage 1：STM32F407ZG Sensor Hub。
- 新增 MCU 协议文档和 Stage 1 计划。
- 新增 MCU 协议常量、`McuFrame`、`McuFrameParser` 和 `McuStatus`。
- 支持解析 heartbeat 帧和 mock sensor 帧。
- 支持 CRC16/MODBUS 校验，并在 mock 样例中验证错误 CRC 拒绝逻辑。
- 新增 `McuMockService`，只在 Linux Runtime 侧读取 mock frame 文件，不实现真实 STM32 固件。
- Runtime 状态输出升级为 `runtime/runtime_status.json`，并包含 MCU 状态。
- 新增 MCU 帧协议 ADR。

### 修改文件

- `CMakeLists.txt`
- `README.md`
- `config/runtime.conf`
- `data/mcu_mock_frames.txt`
- `docs/adr/0007-mcu-frame-protocol.md`
- `docs/dev_log.md`
- `docs/mcu_protocol.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `include/config/app_config.h`
- `include/mcu/`
- `include/runtime/runtime_status.h`
- `include/services/mcu_mock_service.h`
- `scripts/verify_runtime.ps1`
- `src/config/config_loader.cpp`
- `src/ipc/status_publisher.cpp`
- `src/main.cpp`
- `src/mcu/`
- `src/runtime/runtime_manager.cpp`
- `src/services/mcu_mock_service.cpp`

### 验证结果

- 已执行：

```bash
cmake -S . -B build
cmake --build build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

- 已执行构建产物验证：

```powershell
.\build\Debug\outdoor_core_runtime.exe --config config\runtime.conf --log-level debug
```

- 验证结果：CMake 构建通过，Runtime 验证脚本通过，`runtime_status.json` 包含 MCU heartbeat 和 mock sensor 状态，错误 CRC 帧被拒绝。

### 后续 TODO

- Stage 1.6 接入真实 IMU 前，先确认具体 IMU 硬件和 F407 固件边界。
- 后续硬件阶段再实现真实 STM32F407ZG 固件和串口输入。

## 2026-06-10 - Stage 0 Plan Closure

### 本次完成

- 闭环 `docs/stage0_plan.md` 中 Stage 0.4 和 Stage 0.5 的剩余未完成项。
- NMEA Parser 增加 checksum 校验。
- 修正 `data/nmea_sample.txt` 中不匹配的 checksum。
- 新增 `data/nmea_edge_cases.txt`，覆盖南/西半球坐标、无效定位和错误 checksum。
- Runtime 验证脚本增加边界样例和 checksum mismatch 检查。
- `StatusPublisher` 改为先写临时文件，再替换状态文件。
- 新增 Unix domain socket 状态查询接口评估 ADR。

### 修改文件

- `README.md`
- `data/nmea_sample.txt`
- `data/nmea_edge_cases.txt`
- `docs/adr/0004-minimal-nmea-parser.md`
- `docs/adr/0005-file-status-ipc-prototype.md`
- `docs/adr/0006-evaluate-unix-domain-status-query.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `scripts/verify_runtime.ps1`
- `src/gnss/nmea_parser.cpp`
- `src/ipc/status_publisher.cpp`

### 验证结果

- 已执行：

```bash
cmake -S . -B build
cmake --build build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

- 验证结果：CMake 构建通过，Runtime 验证脚本通过，边界样例和 checksum mismatch 检查通过。

### 后续 TODO

- Stage 1 接入真实 UBLOX-M10 串口数据。
- 根据真实 Linux 部署环境推进 Unix domain socket 状态查询接口。

## 2026-06-10 - Stage 0.5

### 本次完成

- 完成最小 IPC/状态发布原型。
- 新增 `runtime::RuntimeStatus` 和 Runtime 状态枚举。
- `RuntimeManager` 维护基础状态、服务数量、已启动服务数量和最近错误。
- 新增 `ipc::StatusPublisher`，将 Runtime 状态写入 `runtime/status.txt`。
- 配置文件新增 `status_output_path`。
- Runtime 验证脚本增加状态文件生成和字段检查。
- 新增文件型状态发布 ADR。

### 修改文件

- `.gitignore`
- `CMakeLists.txt`
- `README.md`
- `config/runtime.conf`
- `docs/adr/0005-file-status-ipc-prototype.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `include/config/app_config.h`
- `include/ipc/status_publisher.h`
- `include/runtime/runtime_manager.h`
- `include/runtime/runtime_status.h`
- `scripts/verify_runtime.ps1`
- `src/config/config_loader.cpp`
- `src/ipc/status_publisher.cpp`
- `src/main.cpp`
- `src/runtime/runtime_manager.cpp`
- `src/runtime/runtime_status.cpp`

### 验证结果

- 已执行：

```bash
cmake -S . -B build
cmake --build build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

- 验证结果：CMake 构建通过，Runtime 验证脚本通过，状态文件输出包含 `state=stopped` 和 `service_count=1`。

### 后续 TODO

- 评估 Unix domain socket 状态查询接口。
- 增加状态文件原子替换写入。
- 为 Runtime 状态输出补充更标准的测试。

## 2026-06-10 - Stage 0.4

### 本次完成

- 完成 GNSS 基础数据模型 `GnssFix`。
- 新增最小 `NmeaParser`，支持解析 RMC/GGA 基础字段。
- `GnssReplayService` 在文件回放时输出原始 NMEA 和基础定位状态。
- Runtime 验证脚本增加 `GNSS fix:` 输出检查。
- 新增最小 NMEA Parser ADR。

### 修改文件

- `CMakeLists.txt`
- `README.md`
- `docs/adr/0004-minimal-nmea-parser.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `include/gnss/gnss_fix.h`
- `include/gnss/nmea_parser.h`
- `include/services/gnss_replay_service.h`
- `scripts/verify_runtime.ps1`
- `src/gnss/nmea_parser.cpp`
- `src/services/gnss_replay_service.cpp`

### 验证结果

- 已执行：

```bash
cmake -S . -B build
cmake --build build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

- 验证结果：CMake 构建通过，Runtime 验证脚本通过，输出包含 `GNSS fix:`。

### 后续 TODO

- 增加 NMEA checksum 校验。
- 补充更多 NMEA 样例和边界测试。
- 推进 Stage 0.5：IPC 原型与基础运行状态输出。

## 2026-06-10 - Stage 0.3

### 本次完成

- 完成 Runtime Manager 与 Service 抽象。
- 新增 `runtime::IService`，定义 `start/run/stop` 生命周期。
- 新增 `runtime::RuntimeManager`，支持顺序启动、运行和停止服务。
- 新增 `services::GnssReplayService`，将 NMEA 文件回放封装为 Runtime 服务。
- `main.cpp` 改为负责配置加载、命令行覆盖和 Runtime 组装。
- 新增 Runtime 服务架构 ADR。

### 修改文件

- `CMakeLists.txt`
- `README.md`
- `docs/adr/0003-runtime-service-architecture.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `include/runtime/i_service.h`
- `include/runtime/runtime_manager.h`
- `include/services/gnss_replay_service.h`
- `scripts/verify_runtime.ps1`
- `src/main.cpp`
- `src/runtime/runtime_manager.cpp`
- `src/services/gnss_replay_service.cpp`

### 验证结果

- 已执行：

```bash
cmake -S . -B build
cmake --build build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

- 验证结果：CMake 构建通过，Runtime 验证脚本通过。

### 后续 TODO

- 推进 Stage 0.4：GNSS mock 服务与最小 NMEA Parser。
- 为 Runtime Manager 补充更标准的 CTest 或单元测试。

## 2026-06-10

### 本次完成

- 完成 Stage 0.2 日志与配置模块增强。
- 新增 `ConfigLoader` 和 `AppConfig`，支持加载 `config/runtime.conf`。
- 日志模块新增 `debug/info/warn/error` 最小日志级别过滤。
- 主程序支持 `--config`、`--input`、`--log-level` 参数，并兼容 Stage 0.1 的直接 NMEA 文件路径用法。
- 新增最小 PowerShell 验证脚本。
- 新增配置格式 ADR。

### 修改文件

- `.gitattributes`
- `CMakeLists.txt`
- `README.md`
- `config/runtime.conf`
- `docs/adr/0002-use-simple-key-value-config.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `include/config/app_config.h`
- `include/config/config_loader.h`
- `include/log/logger.h`
- `scripts/verify_stage0_2.ps1`
- `src/config/config_loader.cpp`
- `src/log/logger.cpp`
- `src/main.cpp`

### 验证结果

- `cmake -S . -B build` 未执行成功：当前环境中 `cmake` 不在 PATH。
- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_stage0_2.ps1
```

- 补充验证结果：脚本使用本机 `g++` 成功编译，并运行 `outdoor_core_runtime` 读取配置文件与样例 NMEA 数据。
- 脚本同时检查默认运行输出包含 NMEA 行，并检查 `--log-level warn` 会抑制 INFO 日志。

### 后续 TODO

- 安装或配置 CMake 后执行标准 CMake 构建验证。
- 推进 Stage 0.3 Runtime Manager 与 Service 抽象。

## 2026-06-09

### 本次完成

- 完成 Stage 0.1 Outdoor Core Runtime 工程骨架。
- 新增简单日志模块、输入源抽象和 NMEA 文件回放实现。
- 新增样例 NMEA 文件，并实现主循环逐行读取和日志打印。

### 修改文件

- `.gitignore`
- `CMakeLists.txt`
- `README.md`
- `docs/adr/0001-use-cmake.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `docs/dev_log.md`
- `include/input/i_input_source.h`
- `include/input/file_replay_input.h`
- `include/log/logger.h`
- `src/input/file_replay_input.cpp`
- `src/log/logger.cpp`
- `src/main.cpp`
- `data/nmea_sample.txt`

### 验证结果

- `cmake -S . -B build` 未通过：当前环境中 `cmake` 不在 PATH。
- `cmake --build build` 未执行：CMake 配置阶段无法运行。
- 已使用本机 `g++` 进行补充验证：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/input/file_replay_input.cpp src/log/logger.cpp -o outdoor_core_runtime.exe
./outdoor_core_runtime.exe
```

- 补充验证结果：程序成功读取 `data/nmea_sample.txt` 并逐行打印 NMEA 日志。

### 后续 TODO

- 补充自动化测试或验证脚本。
- 继续推进 Stage 0.2 日志与配置模块增强。
