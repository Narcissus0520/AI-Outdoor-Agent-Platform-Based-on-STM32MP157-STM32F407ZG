# Stage 1 Plan

Stage 1 目标：在现有 Linux Outdoor Core Runtime 基础上，增加传感器集成能力、STM32F407ZG Sensor Hub 协议解析和 Sensor Hub 状态管理。

当前已完成 F407 侧 MMC5603 I2C2 数据源、20 Hz 磁力计帧和 MP157 parser/status 接入，并通过真实磁场数据上板验证。BMP390 气压/温度软件路径、10 Hz 协议帧和 MP157 parser/status 已实现并通过构建，尚未上板。ICM42688 当前重新接线后仍处于 Mock fallback，待单独排查。UBLOX-M10 已确认 MP157 UART5 `/dev/ttySTM2`、38400 8N1 可收到有效 NMEA；当前无卫星信号。

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

- [x] 接入 MMC5603 I2C 地址 `0x30`，实现产品 ID、SET/RESET 和单次测量
- [x] 新增 `sensor_magnetometer` 协议帧和 MP157 parser/status 输出
- [x] 以 20 Hz 上报 MMC5603 三轴磁场并完成真实上板验证
- [x] 接入 Bosch BMP3 Sensor API v2.0.5，以 I2C2 自动探测 BMP390 `0x77/0x76`
- [x] 新增 10 Hz `sensor_barometer` 协议帧和 MP157 parser/status 输出
- [x] 完成 BMP390 F407、MP157 和 frame decoder 构建测试
- [ ] 烧录并上板验证 BMP390 地址探测、补偿气压/温度和 10 Hz 帧
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

## Stage 1.11: MP157 Downlink Command Prototype

- [x] 新增 `command_ping` / `command_ack` 帧类型和 payload 文档
- [x] MP157 Runtime 新增 `mcu_command = none | ping` 配置项和 `--mcu-command none|ping` 命令行参数
- [x] MP157 serial 模式在打开 `/dev/ttySTM1` 后可发送 `command_ping`
- [x] F407 UART4 RX 新增非阻塞字节接收 BSP 和命令帧 decoder
- [x] F407 收到 `command_ping` 后通过 UART4 回传 `command_ack`，并原样返回 nonce
- [x] MP157 `runtime_status.json` 新增 `command_ack_seen`、`command_ack_request_type`、`command_ack_status`、`command_ack_nonce`
- [x] PC/交叉编译验证覆盖 `command_ping` 构帧和 `command_ack` 解析
- [x] 完成真实板上 raw shell `command_ping -> command_ack` 验证，确认 F407 UART4 RX 命令解析和回包有效
- [ ] 完成新版 ARM Runtime 在板端执行 `--mcu-command ping` 验证，并记录 `command_ack_seen=true` 的证据

## Stage 1.12: MP157 UART5 UBLOX-M10 GNSS and Dashboard Prototype

- [x] 新增 `GnssStatus`，将 GNSS fix、语句统计、source 和 last error 输出到 Runtime 状态
- [x] 增强 `NmeaParser`，支持 RMC/GGA/VTG/GSA/GSV 和 checksum 校验
- [x] 新增 `gnss_input_mode = file | serial`
- [x] 新增 `gnss_serial_device = /dev/ttySTM2`
- [x] 新增 `gnss_serial_baud`，上板实测 M10 当前使用 38400
- [x] 新增 `gnss_serial_capture_seconds = 5`
- [x] 新增 `GnssSerialService`，在 Linux 目标上按 raw 8N1 读取 UART5 NMEA 行
- [x] `runtime_status.json` 新增 `gnss` 字段
- [x] 新增 `DashboardService`，默认输出 `runtime/dashboard.txt` 文本仪表盘原型
- [x] 新增 NMEA parser 单元测试，覆盖 RMC/GGA/VTG/GSA/GSV 和坏 checksum
- [x] 新增 ADR-0010，记录先使用 NMEA over UART5 和文本仪表盘的方案取舍
- [x] 上板确认 MP157 UART5 对应 Linux 设备节点为 `/dev/ttySTM2`
- [x] 上板确认 UBLOX-M10 当前波特率为 38400，并输出 GGA/GSA/GSV/GLL/RMC/VTG
- [x] 完成 UART5 真实接线和 NMEA checksum 验证
- [ ] 在室外完成卫星捕获和有效 fix 验证
- [x] 将文本仪表盘升级为 `outdoor-agent` framebuffer 仪表盘 APP 原型，支持 7 英寸 RGB 屏 `/dev/fb0`
- [x] 新增 `dashboard_output_mode = text | framebuffer | both`、`dashboard_framebuffer_device`、`dashboard_refresh_count` 和 `dashboard_refresh_interval_ms`
- [x] 在 `outdoor-agent` 屏幕中预留 `AI LOCAL AGENT: PLANNED` 区域，真实 AI Agent 本地部署和交互后续实现
- [x] 按参考示意图升级 framebuffer 视觉布局：左侧导航、顶部状态栏、方向罗盘、大速度表、位置地图、温度/光照展示区和底部状态栏

## Stage 1.13: MP157 SD Card Runtime Storage

- [x] 新增 `storage_enabled`、`storage_root_path`、`storage_status_output_path`、`storage_dashboard_output_path` 和 `storage_log_file_path` 配置项
- [x] 新增 `--storage-root`、`--storage`、`--no-storage` 等命令行覆盖项
- [x] 启用 storage 后自动创建 `status/`、`dashboard/`、`logs/`、`data/` 和 `captures/` 目录
- [x] 启用 storage 后将 `runtime_status.json`、文本 dashboard 和 Runtime 日志写入 storage root
- [x] `runtime_status.json` 新增 `storage` 节点，记录实际输出路径和初始化错误
- [x] 本机验证脚本覆盖 storage 模式的目录和文件生成
- [x] 在 MP157 上确认 SD 卡实际挂载路径为 `/run/media/mmcblk1p1`，`/mnt/sdcard` 当前不存在
- [x] 执行 `--storage-root /run/media/mmcblk1p1/outdoor-agent` 板端写入验证，确认 status、dashboard 和 log 文件均生成
- [ ] 基于 `data/` 或 `captures/` 增加 GNSS / IMU / 磁力计 / 气压计历史采样记录服务
- [ ] 增加日志轮转或按日期分文件，避免长期运行占满 SD 卡

## 当前限制

- 当前 Stage 1 保留 ICM42688 优先、Mock IMU 兜底策略；当前板上实际处于 Mock fallback。
- STM32F407ZG CubeMX 基础工程、仓库自主管理的 GNU ARM 构建和 UART Bootloader 上板验证已经完成。
- ICM42688 I2C 固件路径已实现并通过构建；真实传感器数据已在修正接线后通过 F407 串口抓包确认，当前 100 Hz 版本实测 `imu_rate_hz=100.2`。
- 当前 F407 固件已在板上通过 UART4 输出 heartbeat、Mock IMU 和真实 MMC5603 磁力计帧；MP157 Runtime serial 输入模式默认使用 `/dev/ttySTM1`。
- MP157 Runtime 已新增板载 ICM20608 字符设备读取服务，并保留 IIO 可选路径；PC 侧已通过 fake character-device 和 fake-IIO 自动化验证，真实 Runtime ARM 可执行文件已部署到 MP157 并通过 `/dev/icm20608` 读取验证。
- F407 -> MP157 最小上行数据链路已打通；MP157 -> F407 当前已完成最小 raw `ping -> ack` 上板验证，尚未扩展为完整控制命令集，新版 ARM Runtime `--mcu-command ping` 板端复测仍待完成。
- UBLOX-M10 UART5 已完成真实 NMEA 通信验证，当前无卫星信号；有效定位需移到室外复测。
- MMC5603 已完成真实磁场读取；航向输出仍需校准和倾斜补偿。
- 当前 `outdoor-agent` 已支持 7 英寸 RGB 屏 framebuffer 仪表盘显示，但仍是轻量 fbdev 原型，不包含触摸/窗口系统/复杂 GUI 控件；光照、空气质量、电池和信号等展示项暂为 UI 占位/演示指标。
- MP157 SD storage 已通过本机验证和真实板端写入验证；当前 SD 卡实际挂载路径为 `/run/media/mmcblk1p1`，后续若需要固定 `/mnt/sdcard`，应在系统层增加挂载规则或软链接。
- 暂不引入 HTTP API 或真实 AI Agent 本地推理与交互。
