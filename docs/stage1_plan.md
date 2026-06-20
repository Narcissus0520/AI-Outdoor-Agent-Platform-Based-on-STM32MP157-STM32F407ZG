# Stage 1 Plan

Stage 1 目标：在现有 Linux Outdoor Core Runtime 基础上，增加传感器集成能力、STM32F407ZG Sensor Hub 协议解析和 Sensor Hub 状态管理。

当前在 STM32Cube 固件基线和 MP157 Linux Runtime 协议解析基础上，已经完成 F407 侧 ICM42688 I2C 数据源接入。F407 固件已实现真实 ICM42688 优先、Mock IMU 兜底的数据源策略；真实传感器读数已在修正 SCL/SDA 接线后完成 F407 上板串口抓包验证。MP157 开发板自带 ICM20608 的主控侧传感器读取和上板验证已完成；F407 -> MP157 已按 `F407 UART4 PC10/PC11 <-> MP157 USART3 PD9/PD8` 完成最小板间联调。

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

- [x] 确认真实 IMU 资料尚未定版，先使用 Mock IMU 继续推进软件链路
- [x] 在 `common/protocol` 定义 `ImuSample` 和 IMU payload 布局
- [x] 新增 `MSG_TYPE_SENSOR_IMU`
- [x] F407 侧新增 `MockImuProvider`
- [x] F407 侧将 Mock IMU 数据打包为 IMU 协议帧
- [x] MP157 侧新增 IMU payload parser
- [x] `runtime_status.json` 增加 `imu` 字段
- [x] 添加 CRC、帧解析、IMU payload 解析测试
- [x] 新增 `tools/frame_decoder` 十六进制 MCU 协议帧解析工具
- [x] 使用 ICM42688 资料替换此前错误模块假设
- [x] STM32F407ZG 侧接入 I2C2 + ICM42688 读取代码路径
- [x] 修正板级 SCL/SDA 接线后验证真实 ICM42688 IMU 数据路径
- [x] 在真实串口链路上验证 MP157 Runtime 解析真实 IMU 数据帧

## Stage 1.7: F407 Board Bring-up 与真实 IMU

- [x] 新增 `f407/sensor-hub/firmware/` 固件工作区
- [x] 新增 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()`
- [x] 新增 `board_uart_send_bytes()` BSP 占位接口
- [x] 新增 `board_get_tick_ms()` BSP 占位接口
- [x] 新增 C 版 CRC16/MODBUS 和 MCU frame builder
- [x] 新增 C 版 Mock IMU Provider
- [x] `sensor_hub_app_poll()` 每 1000 ms 发送 heartbeat，每 10 ms 发送 IMU frame
- [x] 导入 STM32CubeMX 生成的 STM32F407ZG HAL 基础工程
- [x] 将 Cube 生成代码隔离到 `firmware/stm32cube/`
- [x] 补充 GNU ARM 启动文件、链接脚本和 CMake toolchain
- [x] 使用 `arm-none-eabi-gcc` 生成最小 ELF/HEX/BIN/map
- [x] 加入 PF9 LED 心跳基线并通过 ARM 编译
- [x] 在 STM32Cube HAL 工程中将 `board_uart_send_bytes()` 对接到 `HAL_UART_Transmit()`
- [x] 在 STM32Cube HAL 工程中将 `board_get_tick_ms()` 对接到 `HAL_GetTick()`
- [x] 将 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()` 接入 Cube 主循环
- [x] 配置 USART1 PA9/PA10、115200 8N1，作为 UART Bootloader 下载口
- [x] 上板验证 LED 基线、heartbeat 和 Mock IMU 串口帧
- [x] 配置 ICM42688 管脚 PB10/PB11/PB12
- [x] 新增 PB12 INT1 输入读取 BSP 封装
- [x] 新增 I2C2 BSP 和 ICM42688 固件数据源
- [x] 修正 SCL/SDA 接线后完成真实 ICM42688 上板串口抓包验证
- [x] 将 F407 侧 ICM42688 读取和 IMU 帧上报周期调整为 100 Hz
- [x] 上板复测 100 Hz IMU 输出，5 秒抓取 heartbeat 5 帧、IMU 501 帧、`imu_rate_hz=100.2`、CRC 错误 0 帧
- [x] 新增 UART4 PC10/PC11 作为 F407 与 MP157 的专用板间通信口，USART1 保留为 UART Bootloader 下载口
- [x] 将 `board_uart_send_bytes()` 从 USART1 切换到 UART4

## Stage 1.8: F407 Sensor Hub 完成项

- [ ] 明确是否启用 ICM42688 INT1 数据就绪中断；当前为 10 ms 轮询读取
- [ ] 增加 I2C 读失败后的重新初始化/恢复策略；当前失败时回退 Mock IMU 并设置 heartbeat `status_flags`
- [ ] 评估是否需要 ICM42688 FIFO；当前直接 burst 读取最新 14 字节样本
- [ ] 评估是否需要 UART DMA/环形缓冲；当前 100 Hz IMU 帧带宽低于 115200 8N1 能力，但发送仍为阻塞式 `HAL_UART_Transmit()`
- [ ] 清理 PC mock C++ 层 `Icm42688Driver` 占位接口或将其重命名为 host-side placeholder，避免与真实固件数据源混淆

## Stage 1.9: MP157 板载 ICM20608 验证

- [x] 参考 正点原子 STM32MP157 ICM20608 字符设备与 IIO 示例，确认当前板上系统实际加载的是 `drivers/char/icm20608.ko`
- [x] 通过串口 console 执行 `modprobe icm20608`，确认内核打印 `ICM20608 ID = 0XAE`
- [x] 确认当前板上设备树存在 `/soc/spi@44004000/icm20608@0`，SPI 设备枚举为 `spi0.0`
- [x] 使用正点原子 `22_spi/icm20608App` 通过 `/dev/icm20608` 读取真实数据，静置时 Z 轴约 `0.97g`、温度约 `39.5°C`
- [x] 新增 `Icm20608IioReader`，读取 `in_accel_*_raw`、`in_anglvel_*_raw`、`in_temp_*` 并按示例公式换算实际值
- [x] 新增 `Icm20608CharReader`，读取 `/dev/icm20608` 返回的 7 个 `signed int`，并按示例公式换算实际值
- [x] 新增 `Icm20608Service`，通过 Runtime Service 生命周期接入 MP157 Runtime
- [x] 新增配置项：`board_imu_enabled`
- [x] 新增配置项：`board_imu_source = char_device | iio | auto`
- [x] 新增配置项：`board_imu_device_path`
- [x] 新增配置项：`board_imu_iio_path`
- [x] 新增配置项：`board_imu_sample_count`
- [x] 新增配置项：`board_imu_sample_interval_ms`
- [x] `runtime_status.json` 新增 `board_imu` 字段，独立于 F407 MCU 协议帧的 `imu` 字段
- [x] 新增 fake-IIO reader 单元测试
- [x] 新增 fake character-device reader 单元测试
- [x] 完成 PC 侧 fake-IIO Runtime 冒烟验证
- [x] 将 MP157 Runtime 交叉编译为 ARMv7 hard-float 可执行文件并部署到板上
- [x] 在真实 MP157 板上运行 `--board-imu --board-imu-source char_device`，确认 `runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`
- [x] 记录 MP157 板载 ICM20608 上板验证日志、驱动加载方式和设备树/内核模块前置条件

## Stage 1.10: MP157 Live Serial Integration

- [x] 新增 MP157 Runtime 真实 MCU 串口输入路径，复用现有 `McuFrameParser`、`ImuPayloadParser` 和 `runtime_status.json` 输出
- [x] 新增配置项：`mcu_input_mode = mock_file | serial`
- [x] 新增配置项：`mcu_serial_device = /dev/ttySTM1`
- [x] 新增配置项：`mcu_serial_baud = 115200`
- [x] 新增配置项：`mcu_serial_capture_seconds = 5`
- [x] 新增 MCU byte-stream decoder，支持前导噪声、分片帧和连续帧重组
- [x] 完成 `F407 PC10 UART4_TX -> MP157 PD9 USART3_RX`、`F407 PC11 UART4_RX <- MP157 PD8 USART3_TX` 和公共 GND 接线验证
- [x] 在 MP157 上确认 `/dev/ttySTM1` 可抓到 `A5 5A` MCU 帧头
- [x] 在 MP157 上确认 `runtime_status.json` 中 `heartbeat_seen=true`、`imu_seen=true`，且 IMU 字段来自真实 F407 串口帧

## 当前限制

- 当前 Stage 1 使用 ICM42688 真实数据源优先、Mock IMU 兜底的方式跑通 F407 Sensor Hub 软件模块到 MP157 Runtime 的协议链路。
- STM32F407ZG CubeMX 基础工程、仓库自主管理的 GNU ARM 构建和 UART Bootloader 上板验证已经完成。
- ICM42688 I2C 固件路径已实现并通过构建；真实传感器数据已在修正接线后通过 F407 串口抓包确认，当前 100 Hz 版本实测 `imu_rate_hz=100.2`。
- 当前 F407 固件已在板上通过 UART4 输出 heartbeat 和真实 ICM42688 IMU 帧；MP157 Runtime serial 输入模式默认使用 `/dev/ttySTM1`，并已完成真实接线和上板验证。
- MP157 Runtime 已新增板载 ICM20608 字符设备读取服务，并保留 IIO 可选路径；PC 侧已通过 fake character-device 和 fake-IIO 自动化验证，真实 Runtime ARM 可执行文件已部署到 MP157 并通过 `/dev/icm20608` 读取验证。
- F407 -> MP157 最小单向数据链路已打通；MP157 -> F407 下行控制协议和固件命令解析仍待实现。
- 暂不引入 UI、HTTP API 或 AI Agent。
