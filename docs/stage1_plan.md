# Stage 1 Plan

Stage 1 目标：在现有 Linux Outdoor Core Runtime 基础上，增加 STM32F407ZG Sensor Hub 协议解析和 Sensor Hub 状态管理。

当前在不依赖真实 ICM42688 硬件的前提下，同时推进 F407 侧 Mock Sensor Hub 软件、STM32Cube 固件基线和 MP157 Linux Runtime 侧协议解析。当前不接入真实 IMU 寄存器驱动。

## Stage 1.1: 协议文档和协议常量

- [x] 新增 MCU 协议文档
- [x] 定义 MCU 帧头、版本、帧类型和 payload 长度常量
- [x] 定义 CRC16/MODBUS 校验方式

## Stage 1.2: F407 发送 heartbeat

- [x] 定义 heartbeat 帧格式
- [x] 提供 Linux Runtime 侧 mock heartbeat 帧样例
- [x] 实现真实 STM32F407ZG 固件发送 heartbeat

## Stage 1.3: Linux Runtime 解析 heartbeat

- [x] 新增 `McuFrame`
- [x] 新增 `McuFrameParser`
- [x] 支持解析 heartbeat 帧
- [x] 支持 heartbeat CRC16 校验
- [x] 更新 `McuStatus`

## Stage 1.4: F407 发送 Mock Sensor 数据

- [x] 定义 mock sensor 帧格式
- [x] 提供 Linux Runtime 侧 mock sensor 帧样例
- [x] 支持解析 mock sensor 帧
- [ ] 实现真实 STM32F407ZG 固件发送 mock sensor 数据

## Stage 1.5: Runtime 状态输出增强

- [x] 新增 `McuStatus`
- [x] Runtime 状态输出升级为 `runtime_status.json`
- [x] 将 MCU heartbeat 状态加入 Runtime 状态输出
- [x] 将 mock sensor 状态加入 Runtime 状态输出
- [x] 验证脚本检查 MCU 状态字段

## Stage 1.6: 接入真实 IMU

- [x] 确认当前真实 ICM42688 尚未到货，先使用 Mock IMU 继续推进软件链路
- [x] 在 `common/protocol` 定义 `ImuSample` 和 IMU payload 布局
- [x] 新增 `MSG_TYPE_SENSOR_IMU`
- [x] F407 侧新增 `MockImuProvider`
- [x] F407 侧将 Mock IMU 数据打包为 IMU 协议帧
- [x] MP157 侧新增 IMU payload parser
- [x] `runtime_status.json` 增加 `imu` 字段
- [x] 添加 CRC、帧解析、IMU payload 解析测试
- [x] 新增 `tools/frame_decoder` 十六进制 MCU 协议帧解析工具
- [ ] ICM42688 到货后替换 F407 数据源
- [ ] STM32F407ZG 侧采集真实 IMU
- [ ] 在真实串口链路上验证 MP157 Runtime 解析真实 IMU 数据帧

## Stage 1.7: F407 Board Bring-up 骨架

- [x] 新增 `f407/sensor-hub/firmware/` 固件工作区
- [x] 新增 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()`
- [x] 新增 `board_uart_send_bytes()` BSP 占位接口
- [x] 新增 `board_get_tick_ms()` BSP 占位接口
- [x] 新增 C 版 CRC16/MODBUS 和 MCU frame builder
- [x] 新增 C 版 Mock IMU Provider
- [x] `sensor_hub_app_poll()` 每 1000 ms 发送 heartbeat，每 100 ms 发送 Mock IMU frame
- [x] 导入 STM32CubeMX 生成的 STM32F407ZG HAL 基础工程
- [x] 将 Cube 生成代码隔离到 `firmware/stm32cube/`
- [x] 补充 GNU ARM 启动文件、链接脚本和 CMake toolchain
- [x] 使用 `arm-none-eabi-gcc` 生成最小 ELF/HEX/BIN/map
- [x] 加入 PF9 LED 心跳基线并通过 ARM 编译
- [x] 在 STM32Cube HAL 工程中将 `board_uart_send_bytes()` 对接到 `HAL_UART_Transmit()`
- [x] 在 STM32Cube HAL 工程中将 `board_get_tick_ms()` 对接到 `HAL_GetTick()`
- [x] 将 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()` 接入 Cube 主循环
- [x] 配置 USART1 PA9/PA10、115200 8N1
- [x] 上板验证 LED 基线、heartbeat 和 Mock IMU 串口帧

## 当前限制

- 当前 Stage 1 使用 Mock IMU 数据跑通 F407 Sensor Hub 软件模块到 MP157 Runtime 的协议链路。
- STM32F407ZG CubeMX 基础工程、仓库自主管理的 GNU ARM 构建和 UART Bootloader 上板验证已经完成。
- 暂不实现真实 ICM42688 寄存器驱动，`Icm42688Driver` 仅保留占位接口并返回 not supported。
- 当前 F407 固件已在板上通过 USART1 输出 heartbeat 和 Mock IMU 帧；MP157 Runtime 仍使用 mock 文件输入，尚未实现真实 Linux 串口输入源。
- 暂不引入 UI、HTTP API 或 AI Agent。
