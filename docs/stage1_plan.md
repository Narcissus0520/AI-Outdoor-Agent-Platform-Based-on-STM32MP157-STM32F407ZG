# Stage 1 Plan

Stage 1 目标：在现有 Linux Outdoor Core Runtime 基础上，增加传感器集成能力、STM32F407ZG Sensor Hub 协议解析和 Sensor Hub 状态管理。

当前已完成 F407 侧 ICM42688、MMC5603、BMP390 I2C2 数据源和 MP157 parser/status 接入。2026-07-19 FIFO/TX 队列最终固件已完成整板复电、两次独立烧录复位、60 秒 COM6 和 MP157 `/dev/ttySTM1` 双向验收：最终 `status_flags=0x01A9`，IMU/磁力计/气压计约 `101.43/19.78/9.92 Hz`，CRC、时间戳、UART4/USART1 TX overflow 和 ping ACK 均通过。当前 diagnostics 已升级为兼容 legacy 44 字节的 schema 1 尾部扩展，并把 UART4 RX 累计丢弃计数接入 MP157 预检、时间线和结束审计。UBLOX-M10 已确认 MP157 UART5 `/dev/ttySTM2`、38400 8N1 可收到有效 NMEA；当前无卫星信号。

## Stage 1.1: 协议文档和协议常量

- [x] 新增 MCU 协议文档
- [x] 定义 MCU 帧头、版本、帧类型和 payload 长度常量
- [x] 定义 CRC16/MODBUS 校验方式
- [x] 为 Sensor Hub diagnostics 定义兼容尾部扩展：legacy 44 字节按 schema 0 解码，当前 48 字节 schema 1 发布 UART4 RX 累计丢弃字节数，未知版本拒绝解析

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
- [x] 实现真实 STM32F407ZG 固件发送 Mock IMU 数据；当前作为 ICM42688 初始化/运行失败时的 fallback

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
- [x] `sensor_hub_app_poll()` 每 1000 ms 发送 heartbeat；IMU 历史轮询链路已由当前 INT1 data-ready 事件链路替代
- [x] 导入 STM32CubeMX 生成的 STM32F407ZG HAL 基础工程
- [x] 将 Cube 生成代码隔离到 `firmware/stm32cube/`
- [x] 补充 GNU ARM 启动文件、链接脚本和 CMake toolchain
- [x] 使用 `arm-none-eabi-gcc` 生成最小 ELF/HEX/BIN/map
- [x] 加入 PF9 LED 心跳基线并通过 ARM 编译
- [x] 初始基线将 `board_uart_send_bytes()` 对接到 `HAL_UART_Transmit()`；Stage 1.8 已替换为中断 TX 队列
- [x] 在 STM32Cube HAL 工程中将 `board_get_tick_ms()` 对接到 `HAL_GetTick()`
- [x] 将 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()` 接入 Cube 主循环
- [x] 配置 USART1 PA9/PA10、115200 8N1，作为 UART Bootloader 下载口
- [x] 上板验证 LED 基线、heartbeat 和 Mock IMU 串口帧
- [x] 配置 ICM42688 管脚 PB10/PB11/PB12
- [x] 新增 PB12 INT1 EXTI 事件计数/消费 BSP 封装
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
- [x] 烧录 BMP390 软件路径并确认 F407 应用态协议链路正常
- [x] 完成 BMP390 真实 I2C ACK、补偿气压/温度和 10 Hz `sensor_barometer` 帧上板验证
- [x] 启用 ICM42688 INT1 data-ready 中断；ISR 只记录事件，主循环合并事件并读取最新样本
- [x] 配置 ICM42688 约 25 Hz UI LPF，并为 IMU、磁力计、气压计增加定点一阶 IIR 和独立单元测试
- [x] 增加 I2C 读写失败后的 I2C2 外设硬复位、总线释放和单次事务重试策略，并完成 60 秒三传感器上板验证
- [x] 启用 ICM42688 FIFO；历史 byte-count/可变包方案经 diagnostics 复盘后已改为 record-count、4 条记录 watermark、最多 16 条完整记录批量读取和异常 flush
- [x] 将 UART4/USART1 阻塞发送替换为各 1024 字节固定 TX 队列和 `HAL_UART_Transmit_IT()` 完成回调；保留 UART4 64 字节 RX 环形缓冲
- [x] 清理未被调用的 PC mock C++ `Icm42688Driver` 占位接口，真实实现统一位于 F407 固件数据源
- [x] 增加 FIFO、UART4/USART1 TX、UART4 RX、ICM 初始化和 I2C 运行期失败 heartbeat 诊断位，并扩展验收脚本
- [x] 将 Sensor Hub diagnostics 同时发布到 UART4/MP157 与 USART1/COM6；在旧 44 字节载荷尾部扩展为 48 字节 schema 1，MP157 parser/status、累计计数单调性和长测增量审计已完成代码与主机验证
- [x] 刷入 UART4 diagnostics 镜像并完成两轮 30 秒负向审计；已定位 empty-marker 解析放大和 MMC5603 无间隔状态轮询，证据见 TRB-024/ADR-0021
- [x] 增加 FIFO parser/时间轴、provider 配置/flush/恢复、MMC5603 有界轮询、UART TX 队列 wrap/原子拒绝和 diagnostics schema 1 frame-builder 主机测试；当前 F407 PC CTest 7/7 通过
- [x] 将共享 I2C2 提升至 400 kHz；MMC5603 测量触发后等待 7 ms，再最多执行 3 次有间隔状态检查
- [x] 将 UART4 RX 累计丢弃计数接入 heartbeat sticky 位、diagnostics schema 1、MP157 JSON、8 秒预检、逐秒健康时间线和结束审计；实板预检与 4 点 monitor 探针均为 schema 1/drop 0
- [x] 执行真实 UART4 RX 环满压力注入：40960 字节连续下行产生 drop 2231、heartbeat `0x302E`，预检按设计拒绝；F407 应用复位后 drop 归零。更长持续过载和容量边界测量保留为长期可靠性项目
- [ ] ICM-only 诊断镜像和零累计预检 profile 已实现；COM6 关闭后的 400 kHz `XMWvGC` 在约 58 秒累计 recovery/failure `232/45`。完整复电的单器件 100 kHz `wB2xhs` 退化为 `0xD206`、init step 1，recovery/failure 速率为 400 kHz 的 `3.29/3.67` 倍，降速方案已证伪。F407 USB-UART 从 COM6 重枚举为 COM3 后，400 kHz ICM-only 基线已烧录并逐字节校验恢复；待取得电气/波形证据并通过零累计门槛后，再刷回正式镜像逐颗接回 MMC5603/BMP390
- [x] 烧录 FIFO/TX 队列固件并完成逐字节回读、COM6/USART1 镜像和 MP157 UART4 双向链路验证
- [x] 整板断电恢复后完成真实 ICM42688 FIFO COM6 60 秒和多次独立复位回归；最终状态 `0x01A9`

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
- [x] 真实 boot 发现标准 `systemd-modules-load` 早于板厂 `link-modules.service`；改用 `deploy/systemd/outdoor-agent-icm20608.service` 显式建立正确依赖
- [x] 整板软重启后确认自定义服务 active/enabled、`icm20608` probe ID `0xAE` 且 `/dev/icm20608` 自动出现

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
- [x] F407 UART4 RX 新增中断接收、64 字节环形缓冲和命令帧 decoder
- [x] F407 收到 `command_ping` 后通过 UART4 回传 `command_ack`，并原样返回 nonce
- [x] MP157 `runtime_status.json` 新增 `command_ack_seen`、`command_ack_request_type`、`command_ack_status`、`command_ack_nonce`
- [x] PC/交叉编译验证覆盖 `command_ping` 构帧和 `command_ack` 解析
- [x] 完成真实板上 raw shell `command_ping -> command_ack` 验证，确认 F407 UART4 RX 命令解析和回包有效
- [x] 完成当前 ARM Runtime 在板端执行 `--mcu-command ping` 验证，记录 `command_ack_seen=true`、`command_ack_status=0` 和 nonce `0x50494E47`

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
- [x] 新增 `run_board_gnss_fix_validation.sh`，真实室内负向测试确认有 NMEA 但无 fix 时只因 status/RMC/GGA 定位门槛返回失败并保存证据
- [x] 将文本仪表盘升级为 `outdoor-agent` framebuffer 仪表盘 APP 原型，支持 7 英寸 RGB 屏 `/dev/fb0`
- [x] 新增 `dashboard_output_mode = text | framebuffer | both`、`dashboard_framebuffer_device`、`dashboard_refresh_count` 和 `dashboard_refresh_interval_ms`
- [x] 在 `outdoor-agent` 屏幕中预留 `AI LOCAL AGENT: PLANNED` 区域，真实 AI Agent 本地部署和交互后续实现
- [x] 按参考示意图升级 framebuffer 视觉布局：左侧导航、顶部状态栏、方向罗盘、大速度表、位置地图、温度/光照展示区和底部状态栏
- [x] 在 `outdoor-agent` 主界面左上角绘制程序图标，并在文本 dashboard 中输出 `app_icon_visible: true` 供自动化验证
- [x] 新增 `config/outdoor_agent_app.conf` 和 `scripts/run_outdoor_agent_app.sh`，将 framebuffer 仪表盘落实为可启动的 `outdoor-agent` APP 进程入口
- [x] 支持 `dashboard_refresh_count = 0` 作为 APP 常驻刷新模式；2026-07-05 已通过 COM3 完成 MP157 `/dev/fb0` 有限刷新上板验证
- [x] 新增最小 fbdev/evdev APP launcher：启动后先显示中央 `OUTDOOR AGENT` 图标/磁贴，支持点击进入 dashboard，并提供 `launcher_auto_start_seconds` 便于无人值守验证
- [x] 2026-07-05 通过 COM3 确认 Goodix 触摸节点为 `/dev/input/touchscreen0 -> event0`，并验证 `--launcher-auto-start-seconds 2` 可从 launcher 进入 `/dev/fb0` dashboard
- [x] 复测真实手指点击：串口日志出现 `Launcher icon tapped at x=530, y=292`，随后 `Dashboard rendered to framebuffer: /dev/fb0`

## Stage 1.13: MP157 SD Card Runtime Storage

- [x] 新增 `storage_enabled`、`storage_root_path`、`storage_status_output_path`、`storage_dashboard_output_path` 和 `storage_log_file_path` 配置项
- [x] 新增 `--storage-root`、`--storage`、`--no-storage` 等命令行覆盖项
- [x] 启用 storage 后自动创建 `status/`、`dashboard/`、`logs/`、`data/` 和 `captures/` 目录
- [x] 启用 storage 后将 `runtime_status.json`、文本 dashboard 和 Runtime 日志写入 storage root
- [x] `runtime_status.json` 新增 `storage` 节点，记录实际输出路径和初始化错误
- [x] 本机验证脚本覆盖 storage 模式的目录和文件生成
- [x] 在 MP157 上确认 SD 卡实际挂载路径为 `/run/media/mmcblk1p1`，`/mnt/sdcard` 当前不存在
- [x] 执行 `--storage-root /run/media/mmcblk1p1/outdoor-agent` 板端写入验证，确认 status、dashboard 和 log 文件均生成
- [x] 新增基于状态变更的 CSV History Recorder，分别记录 GNSS、MCU IMU、磁力计、气压计和 Board IMU
- [x] 新增 `--history` / `--history-output` 和 history 配置项；storage 模式下相对路径解析到 `data/history/`
- [x] 增加按文件大小轮转的 Runtime 日志，默认单文件 1 MiB、保留 3 个备份
- [x] 新增 History Recorder 与 Logger 单元测试，并将 storage 验证脚本扩展到 CSV 和 rotation 状态字段
- [x] 真实 MP157 60 秒联合 history 冒烟、修复后 20 秒 framebuffer history 频率和 30 秒 1 MiB 日志轮转复验通过
- [x] 新增 Logger 轮转/写入失败累计计数与最后错误；发布到 `runtime_status.json` 并合并到 `storage.last_error`
- [x] 新增可移植的轮转失败故障注入和非零状态 JSON 测试；预检/长测审计要求两类计数为零且错误为空，最新 MP157 预检已验证三个日志健康门槛
- [ ] 在真实 MP157 SD 卡上完成小时级历史记录、日志轮转、掉电一致性和容量增长验证

## Stage 1.14: MP157 Cooperative Runtime Scheduler

- [x] 将阻塞式 `IService::run()` 演进为返回 `running / completed / failed` 的短步骤 `poll()` 接口
- [x] Runtime Manager 使用单线程协作式循环交错推进全部活动服务，保留顺序启动、逆序停止和失败服务名
- [x] GNSS、MCU 串口、板载 ICM20608、dashboard 和 evdev launcher 改为非阻塞或有界步骤
- [x] 支持 GNSS/MCU `capture_seconds = 0`、Board IMU `sample_count = 0` 和 dashboard `refresh_count = 0` 常驻运行
- [x] 增加运行期状态发布回调、active/completed service 计数、SIGINT/SIGTERM 受控停止和 `runtime_run_seconds`
- [x] 新增 Runtime Manager 单元测试；当前 MP157 CTest 10/10、Runtime 持续/storage/history/compass 组合验证和 ARMv7 hard-float 交叉构建均通过
- [x] 新增长测启动、结果审计和异常恢复脚本，并修复 BusyBox `ps` 截断导致的独占保护失效；两次受污染长测均已作废，复电恢复后已完成两次健康预检和一次 60 秒冒烟
- [x] 为小时长测和 SIGKILL 恢复增加独立 8 秒健康预检；F407 非 `0x01A9`、四类数据/ACK/Board IMU 缺失或核心错误非空时，在创建正式测试前失败并保留报告
- [x] 真实 MP157 连续两次通过 `0x01A9` 健康预检；首个 60 秒正式冒烟的状态、五类 CSV 频率/完整性和日志轮转审计全部通过
- [x] 发现最终状态会掩盖运行期 `0x022E/0x03AE` FIFO fallback；作废 PID `4868`，增加逐秒健康时间线及覆盖率、致命位、99% 健康占比、最长连续降级和最终状态审计
- [x] 20 秒真实板时间线审计正确拒绝 5 个降级样本和最终 `0x03AE`；该结果是负向门槛证据，不计为耐久通过
- [x] 新增 `scripts/deploy_mp157_runtime.ps1`，真实 COM9 幂等部署验证通过，持久化文件、权限、SHA256、shell 语法、systemd 和设备节点全部闭环
- [ ] 在真实 MP157 上使用双串口、板载 IMU 和 `/dev/fb0` 完成小时级协作运行验证

## Stage 1.15: MP157 Compass Estimation

- [x] 新增无第三方依赖的 `CompassEstimator`，组合 ICM42688 加速度与 MMC5603 三轴磁场
- [x] 支持三轴硬铁偏置和完整 3×3 软铁/传感器安装变换矩阵
- [x] 使用加速度估计 roll/pitch 并执行倾斜补偿，输出磁航向和磁偏角修正后的 `[0,360)` 航向
- [x] 增加 IMU/磁力计最大时差、加速度模长、磁场模长和奇异矩阵质量门槛
- [x] `runtime_status.json` 增加 `compass` 节点，区分 valid、calibration applied、tilt compensated、quality 和 last error
- [x] text/framebuffer dashboard 优先使用有效罗盘航向；无效时仅在 GNSS fix/速度门槛通过后回退 course，不再使用固定 `62.3°` 假航向
- [x] 新增聚焦 MCU fixture、算法边界测试和 Runtime/dashboard 集成验证；MP157 CTest 当前 10/10，ARMv7 hard-float 构建通过，并在 MP157 上取得 JSON/dashboard 一致的确定性 fixture 结果
- [x] 新增无第三方依赖的 `tools/compass_calibrator`，从 History `magnetometer.csv` 拟合硬铁偏置和全 3×3 软铁矩阵，支持稳健离群点裁剪、覆盖/残差门槛和可选独立 sensor-to-body proper rotation
- [x] 用合成全矩阵畸变验证偏置/交叉轴恢复、近平面拒绝、强磁离群点、非法安装矩阵和配置输出，并用固定 CSV 跑通命令行端到端链路；MSVC Debug 与 GCC Release CTest 均为 3/3，候选始终保持 `compass_calibration_valid=false`
- [ ] 室外多姿态采集 MMC5603 原始数据，拟合并验证真实硬铁偏置、3×3 软铁/安装矩阵和当地磁偏角
- [ ] 完成八方向静态、不同倾角、重复性和动态航向稳定性板端验收后，再设置 `compass_calibration_valid=true`

## 当前限制

- 当前 Stage 1 保留 ICM42688 优先、Mock IMU 兜底策略；2026-07-19 历史 INT1 直接读取版本最终 heartbeat 为 `0x00A9`，最终 FIFO 版本为 `0x01A9`。
- FIFO/TX 队列历史版本完成过 60 秒真实板验收，但后续 diagnostics 证明周期性 fallback 仍在。三传感器 100 kHz 与 fixed partial-read A/B 均未解决。COM6 全程关闭后的 400 kHz ICM-only `XMWvGC` 仍以 7 项累计门槛失败；单器件 100 kHz `wB2xhs` 完整复电 A/B 又退化为 `0xD206/init step 1`，恢复与最终失败速率显著上升。400 kHz ICM-only 基线已通过当前 COM3 重新烧录和逐字节回读恢复；在电气测量/波形收敛前，30/60 秒和小时级耐久继续禁止。
- STM32F407ZG CubeMX 基础工程、仓库自主管理的 GNU ARM 构建和 UART Bootloader 上板验证已经完成。
- ICM42688 I2C 固件路径已实现，并通过 F407 串口和 MP157 Runtime 确认当前真实 IMU 数据。
- 当前 F407 固件已在板上通过 UART4 输出真实 heartbeat、IMU、MMC5603 磁力计和 BMP390 气压计帧。
- MP157 Runtime 已新增板载 ICM20608 字符设备读取服务，并保留 IIO 可选路径；PC 侧已通过 fake character-device 和 fake-IIO 自动化验证，真实 Runtime ARM 可执行文件已部署到 MP157 并通过 `/dev/icm20608` 读取验证。
- F407 -> MP157 上行传感器数据链路和 MP157 -> F407 Runtime `ping -> ack` 双向验证均已完成；尚未扩展为完整控制命令集和长期可靠性机制。
- UBLOX-M10 UART5 已完成真实 NMEA 通信验证，当前无卫星信号；有效定位需移到室外复测。
- MMC5603 已完成真实磁场读取；航向算法、配置和倾斜补偿已完成软件验证，但真实整机标定参数与精度验收仍待室外采集，当前输出明确标记为 `uncalibrated`。
- 当前 `outdoor-agent` 已支持 7 英寸 RGB 屏 framebuffer 仪表盘显示，并新增独立 APP 配置/启动脚本和最小 evdev launcher；2026-07-05 已通过 COM3 验证脚本入口、`/dev/fb0` 渲染链路、Goodix 触摸节点、launcher 自动进入 dashboard 和真实手指点击图标启动。该实现仍是轻量 fbdev 原型，不包含窗口系统或复杂 GUI 控件，光照、空气质量、电池和信号等展示项暂为 UI 占位/演示指标。
- 当前 Runtime Manager 已使用单线程协作式调度解决常驻 dashboard 阻塞其他服务的问题；本机持续模式和 ARM 交叉构建已验证，真实 MP157 双串口、板载 IMU、framebuffer 小时级协作运行仍待执行。
- MP157 SD storage 已通过本机验证和真实板端写入验证；当前 SD 卡实际挂载路径为 `/run/media/mmcblk1p1`，后续若需要固定 `/mnt/sdcard`，应在系统层增加挂载规则或软链接。
- CSV History Recorder 和按大小日志轮转已通过主机、ARM 构建和真实 MP157 短时复验；CSV 保存合法解析后的更新而非原始字节流，真实 SD 卡小时级写入、掉电一致性和磨损仍待验证。
- 日志轮转/写入失败已具备单调累计计数、最后错误、聚合 storage error 和验收门槛；可移植测试已注入轮转失败，真实 SD 写满/拔卡/只读导致的 write/flush 失败仍待受控故障注入。
- 暂不引入 HTTP API 或真实 AI Agent 本地推理与交互。

## Stage 2 演进

Stage 1 未完成的真实硬件、室外、小时级和掉电验收保持原状态。后续纯软件开发进入 [Stage 2 Plan](stage2_plan.md)：Stage 2.1 已完成 Unix domain socket 状态查询的软件实现、主机验证和 ARM 全目标交叉链接，真实 MP157 生命周期验收留待统一联调。
