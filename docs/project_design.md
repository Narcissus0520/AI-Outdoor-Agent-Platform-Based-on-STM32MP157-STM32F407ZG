# Project Design

## 项目目标

AI Outdoor Agent Platform 目标是围绕 STM32MP157 + STM32F407ZG 构建一个户外智能体平台，逐步体现 Embedded Linux、C/C++ 工程化、GNSS、Sensor Integration、IPC、Runtime Service 和 AI Terminal 能力。

## 项目范围

当前 Stage 1 范围：

- CMake C++17 工程骨架
- 简单日志模块与日志级别配置
- 基础 `key=value` 配置文件加载
- 输入源抽象 `IInputSource`
- NMEA 文件回放 `FileReplayInput`
- Runtime Service 生命周期接口 `IService`
- Runtime Manager 顺序启动、协作式轮询和逆序停止服务；单线程事件循环同时推进采集、状态发布与 dashboard
- GNSS 文件回放服务 `GnssReplayService`
- GNSS 基础数据模型 `GnssFix`
- NMEA Parser，支持 RMC/GGA/VTG/GSA/GSV 基础字段和 checksum 校验
- MP157 UART5 GNSS/NMEA serial 输入软件路径，默认设备 `/dev/ttySTM2`
- `outdoor-agent` 仪表盘 APP 原型 `DashboardService`，支持 `runtime/dashboard.txt` 文本输出和 Linux fbdev `/dev/fb0` framebuffer 输出；framebuffer 视觉布局参考深色科技风户外终端仪表盘，包含左侧导航、左上角程序图标、顶部状态、方向/速度/位置/环境展示和底部状态栏
- `outdoor-agent` 轻量 APP launcher：先绘制中央程序图标/磁贴，再通过 Linux evdev `/dev/input/touchscreen0` 点击进入 dashboard；提供 `launcher_auto_start_seconds` 便于无人值守上板验证
- 板端 APP 进程入口：`config/outdoor_agent_app.conf` + `scripts/run_outdoor_agent_app.sh`，用于在 7 英寸 RGB 屏上显示 launcher 并常驻刷新 `outdoor-agent`
- 主程序组装 Runtime，并由服务读取配置指定的 NMEA 文件和输出基础定位状态
- 文件型 Runtime 状态发布原型
- MP157 SD 卡文件/目录型持久化基础设施，启用后输出状态 JSON、文本 dashboard 和运行日志
- Runtime 基础状态输出
- MCU 协议常量、帧结构和 CRC16/MODBUS 校验
- `McuFrameParser`
- `McuStatus`
- Heartbeat mock 帧解析
- Mock sensor 帧解析
- Mock IMU payload 定义、F407 mock 打包和 MP157 解析
- F407 `firmware/` 固件应用骨架和 STM32CubeMX/HAL 基础工程
- F407 I2C2 读取 ICM42688 的固件数据源，由 PB12 INT1 data-ready 触发主循环读取，真实 IMU 帧已完成当前接线上板验证
- F407 I2C2 读取 MMC5603 三轴磁场，20 Hz `sensor_magnetometer` 真实帧已完成当前接线上板验证
- MP157 `CompassEstimator` 融合 F407 IMU/磁场样本，支持硬铁偏置、3×3 软铁/安装矩阵、倾斜补偿、磁偏角和质量状态；离线 `compass_calibrator` 已能从 History CSV 生成带质量报告的候选参数，真实整机系数仍待采集和独立验收
- F407 I2C2 读取 BMP390 气压和温度，10 Hz `sensor_barometer` 真实帧已完成当前接线上板验证
- `runtime_status.json` 输出 MCU 状态
- `runtime_status.json` 输出 IMU 状态
- `tools/frame_decoder` 十六进制 MCU 协议帧解析工具
- `tools/compass_calibrator` 无第三方依赖的 MMC5603 全椭球离线校准和候选配置生成工具
- 最小 PowerShell 验证脚本
- F407 UART Bootloader 烧录、回读校验和 PC 侧串口帧抓取验证脚本
- MP157 板载 ICM20608 字符设备读取服务，保留 IIO 可选读取路径
- MP157 live serial 输入路径，默认按 USART3 `/dev/ttySTM1` 读取 F407 UART4 二进制 MCU 帧，并已完成上板验证
- MP157 -> F407 最小下行命令 `command_ping -> command_ack` 软件路径
- `runtime_status.json` 输出 `board_imu` 状态
- MP157 板载 ICM20608 fake character-device / fake-IIO 自动化测试和 Runtime 冒烟验证

当前不包含：

- 完整 NMEA 协议覆盖
- UART5 GNSS 已完成通信验证；真实卫星定位仍待室外环境验证
- MP157 反向控制 F407 的完整命令集、权限/状态机和长期可靠性策略
- 更多真实外置传感器 HAL 外设绑定
- HTTP API
- 完整触摸控件体系、桌面环境或窗口系统
- 真实 AI Agent 本地推理、对话交互和任务编排

## 当前阶段

当前处于 Stage 1：Sensor Integration 与 STM32F407ZG Sensor Hub 协议解析。MP157 板载 ICM20608、F407 侧 ICM42688/MMC5603/BMP390、F407/MP157 双向 UART 链路均已完成历史真实上板验证。2026-07-19 的 FIFO/TX queue 固件整板复电后 60 秒验收为 `status_flags=0x01A9`；后续长测暴露周期性 fallback。为形成可重复隔离条件，已增加默认关闭、带 `0x8000` 标识的 ICM-only 编译模式，以及要求 `0x8181` 和所有 I2C/FIFO 累计值为零的 MP157 预检 profile。COM6 关闭后的 400 kHz `XMWvGC` 在约 58 秒累计 recovery/failure `232/45`；同窗口 100 kHz `wB2xhs` 更退化为 `0xD206`、init step 1，recovery/failure 速率分别为 400 kHz 的 `3.29/3.67` 倍。该 A/B 排除其他两颗从机访问、COM6 打开和单纯 400 kHz 过快为必要条件。原 COM6 随后未枚举，用户确认设备重枚举为 COM3；COM3 状态/占用检查通过后，已用 Bootloader `0x31`、chip ID `0x0413` 恢复并逐字节校验 400 kHz ICM-only 基线。正式三传感器和小时级结论尚未形成，下一步需要电气测量和波形证据。MP157 已实现可配置罗盘解算和离线全矩阵候选校准工具，默认明确标为未标定。UBLOX-M10 已确认在 `/dev/ttySTM2`、38400 8N1 输出有效 NMEA，但当前无卫星定位。

## 硬件平台

- STM32MP157 开发板：后续 Embedded Linux 部署目标
- STM32F407ZG 开发板：后续异构协作目标
- 7 英寸 RGB 屏：已通过 Linux fbdev `/dev/fb0` 显示 `outdoor-agent` 仪表盘 APP 原型
- Goodix 触摸屏输入：当前板端节点为 `/dev/input/touchscreen0 -> event0`；launcher evdev 等待路径已实现，并已通过真实手指点击启动验证
- UBLOX-M10 GNSS 开发板：已确认从 MP157 UART5 `/dev/ttySTM2` 以 38400 8N1 输出有效 NMEA；当前环境无有效卫星 fix

## 软件架构

当前软件架构保持最小可运行：

```text
Outdoor Core Runtime
├── main loop
├── config::ConfigLoader
├── runtime::RuntimeManager
│   ├── cooperative IService::poll() scheduler
│   ├── running/completed/failed service state
│   └── periodic status callback + stop predicate
├── ipc::StatusPublisher
├── runtime::IService
│   └── services::GnssReplayService
│       ├── gnss::NmeaParser
│       ├── gnss::GnssFix / GnssStatus
│       └── input::FileReplayInput
│   └── services::GnssSerialService
│       ├── gnss::NmeaParser
│       └── gnss::GnssStatus
│   └── services::McuMockService
│       ├── mcu::McuFrameParser
│       ├── mcu::ImuPayloadParser
│       └── mcu::McuStatus
│   └── services::McuSerialService
│       ├── mcu::McuCommand
│       ├── mcu::McuFrameStreamDecoder
│       ├── mcu::McuFrameParser
│       ├── mcu::ImuPayloadParser
│       └── mcu::McuStatus
│   └── services::Icm20608Service
│       ├── sensors::Icm20608CharReader
│       ├── sensors::Icm20608IioReader
│       └── sensors::BoardImuStatus
│   └── services::DashboardService
│       ├── gnss::GnssStatus
│       ├── mcu::McuStatus
│       ├── sensors::BoardImuStatus
│       └── fbdev dashboard + evdev launcher
├── log::Logger
├── storage::HistoryRecorder
└── input::IInputSource
```

后续会逐步加入 Unix domain socket 查询接口、串口长期服务化、更多硬件输入源和更完整的 MCU 控制命令集。

## 模块划分

- `mp157/outdoor-core-service/src/main.cpp`: 程序入口、命令行参数处理和当前主循环
- `mp157/outdoor-core-service/include/config`, `mp157/outdoor-core-service/src/config`: 应用配置结构和配置加载器
- `mp157/outdoor-core-service/include/storage`, `mp157/outdoor-core-service/src/storage`: GNSS、MCU 传感器和 Board IMU 的追加式 CSV 历史记录
- `mp157/outdoor-core-service/include/gnss`, `mp157/outdoor-core-service/src/gnss`: GNSS 数据模型、状态模型和 NMEA Parser
- `mp157/outdoor-core-service/include/ipc`, `mp157/outdoor-core-service/src/ipc`: 当前最小 IPC/状态发布原型
- `mp157/outdoor-core-service/include/mcu`, `mp157/outdoor-core-service/src/mcu`: MCU 协议常量、传感器/命令/诊断帧解析和 Sensor Hub 状态
- `mp157/outdoor-core-service/include/sensors`, `mp157/outdoor-core-service/src/sensors`: MP157 板载传感器读取抽象，当前实现 ICM20608 character-device reader 和 IIO sysfs reader
- `common/protocol`: MP157 与 F407 共享协议常量、CRC16/MODBUS、IMU、磁力计和气压计 payload 定义
- `f407/sensor-hub`: F407 侧 Mock IMU Provider、IMU 协议帧打包和真实 ICM42688 数据源
- `f407/sensor-hub/firmware`: F407 固件工作区，包含项目自有 app、BSP、协议帧构建、ICM42688/MMC5603/BMP390 数据源和 Mock IMU fallback
- `f407/sensor-hub/firmware/stm32cube`: CubeMX 生成的 STM32F407ZG HAL/CMSIS 基础工程和 `.ioc` 配置
- `f407/sensor-hub/firmware/cmake`, `linker`, `startup`, `platform`: GNU ARM 工具链、内存布局、启动和最小 newlib 系统调用支持
- `tools/frame_decoder`: 十六进制 MCU 协议帧解析工具
- `tools/compass_calibrator`: 读取 `magnetometer.csv`，执行硬铁/全 3×3 软铁椭球拟合、离群点裁剪、覆盖/残差门槛和候选 Runtime 配置输出
- `mp157/outdoor-core-service/include/log`, `mp157/outdoor-core-service/src/log`: 日志模块和日志级别过滤
- `mp157/outdoor-core-service/include/runtime`, `mp157/outdoor-core-service/src/runtime`: 服务生命周期接口和 Runtime Manager
- `mp157/outdoor-core-service/include/services`, `mp157/outdoor-core-service/src/services`: 当前 Runtime 服务实现
- `mp157/outdoor-core-service/include/input`, `mp157/outdoor-core-service/src/input`: 输入源接口和 NMEA 文件回放实现
- `mp157/outdoor-core-service/config/runtime.conf`: Stage 0/Stage 1 默认开发和自动化验证配置
- `mp157/outdoor-core-service/config/outdoor_agent_app.conf`: MP157 7 英寸 RGB 屏 `outdoor-agent` APP 进程配置
- `mp157/outdoor-core-service/scripts/run_outdoor_agent_app.sh`: 板端启动 `outdoor-agent` APP 进程的轻量脚本
- `mp157/outdoor-core-service/scripts/run_board_health_preflight.sh`: 默认 `full` profile 是正式长测/异常恢复前的硬件健康门槛；显式 `icm42688_only` profile 只用于带 `0x8000` 标识的隔离镜像，要求 `0x8181`、其他两类传感器 absent 和全部 I2C/FIFO/init/drop 累计值为零
- `mp157/outdoor-core-service/scripts/monitor_board_runtime_health.sh`: 正式长测期间逐秒保存 Runtime state、F407 状态位、核心 error、diagnostics schema 和 UART4 RX 累计丢弃计数，用完成标记证明监控覆盖至进程退出
- `mp157/outdoor-core-service/scripts/run_board_long_validation.sh`: 通过健康预检后启动真实 MP157 双串口、板载 IMU、framebuffer、SD history 和日志轮转的有界长测，保存测试元数据、二进制 SHA256、PID、预检目录与独立证据目录
- `mp157/outdoor-core-service/scripts/audit_board_long_validation.sh`: 长测完成后检查最终状态、diagnostics 版本、UART4 RX 丢弃计数单调性/增量、Logger 健康、五类 CSV schema/单调键/频率/末行、日志上限和 SD 增长，并输出机器可复查的 PASS/FAIL 报告
- `mp157/outdoor-core-service/scripts/run_board_crash_recovery_validation.sh`: 在无其他 Runtime 时执行 SIGKILL、文件完整性检查、同目录受控重启和追加增长验证；通过 `/proc/<pid>/cmdline` 保证设备独占
- `mp157/outdoor-core-service/scripts/run_board_gnss_fix_validation.sh`: 限时等待真实 UART5 fix，并以 status、有效 RMC 坐标、有效 GGA 卫星/质量和 CSV 完整性构成正向门槛；使用独立合法 MCU fixture 避免负向测试数据污染
- `mp157/outdoor-core-service/deploy/systemd/outdoor-agent-icm20608.service`: 板端 `icm20608` 开机加载 unit，显式依赖并排在板厂 `link-modules.service` 之后；真实软重启已验证 probe ID `0xAE` 和 `/dev/icm20608`
- `mp157/outdoor-core-service/data/nmea_sample.txt`: Stage 0 样例 NMEA 数据
- `mp157/outdoor-core-service/data/nmea_edge_cases.txt`: checksum、无效定位和南/西半球坐标边界样例
- `mp157/outdoor-core-service/data/mcu_mock_frames.txt`: Stage 1 MCU mock heartbeat 和 mock sensor 帧
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`: 当前 Windows 开发环境下的最小验证脚本
- `scripts/send_xmodem.ps1`: 通过 MP157 串口 console 和板端 `rx` 上传 XMODEM-CRC 验证包，并截断协议尾部填充
- `scripts/deploy_mp157_runtime.ps1`: 通过 COM9 幂等创建 `/opt/outdoor-agent`、上传 Runtime/配置/验证脚本和 ICM20608 unit，设置权限、启用服务，并用板端 SHA256、shell 语法、设备节点和 systemd 状态闭环验证

真实 MP157 的当前持久化验证部署根目录为 `/opt/outdoor-agent`：ARM Runtime 位于 `bin/outdoor_core_runtime`，配置位于 `config/runtime.conf`，长测/审计/异常恢复脚本位于 `scripts/`。该路径位于 eMMC rootfs，不随 `/tmp` 重启清空；采集证据仍写入可移除 SD 卡 `/run/media/mmcblk1p1`。

## 数据流

```text
mp157/outdoor-core-service/config/runtime.conf
  -> ConfigLoader
  -> AppConfig
  -> StatusPublisher

mp157/outdoor-core-service/data/nmea_sample.txt
  -> FileReplayInput::readLine()
  -> GnssReplayService::poll(), one bounded step
  -> NmeaParser::parseLine()
  -> GnssStatus
  -> RuntimeManager cooperative loop
  -> Logger::info()
  -> stdout

MP157 UART5 GNSS path
  -> UBLOX-M10 TX to MP157 UART5 RX
  -> /dev/ttySTM2, validated 38400 8N1, raw mode
  -> services::GnssSerialService
  -> line buffer
  -> NmeaParser::parseLine()
  -> GnssStatus
  -> runtime_status.json gnss

Dashboard path
  -> GnssStatus / McuStatus / BoardImuStatus
  -> services::DashboardService
  -> optional launcher screen when launcher_enabled = true and output mode includes framebuffer
  -> evdev input from /dev/input/touchscreen0 to start dashboard after tapping the APP icon/tile
  -> runtime/dashboard.txt text dashboard
  -> optional /dev/fb0 framebuffer dashboard for the 7-inch RGB screen
  -> reference-style dark dashboard layout with app icon/navigation/sidebar/gauges/map/footer
  -> UI placeholder metrics for light/air/power/signal until real sensors are integrated
  -> AI Local Agent placeholder panel, real local AI deployment pending

outdoor-agent APP process path
  -> scripts/run_outdoor_agent_app.sh
  -> outdoor_core_runtime --config config/outdoor_agent_app.conf
  -> dashboard_output_mode = both
  -> launcher_enabled = true
  -> launcher_input_device = /dev/input/touchscreen0
  -> dashboard_refresh_count = 0, long-lived refresh loop
  -> central launcher icon/tile starts the dashboard
  -> runtime/dashboard.txt + /dev/fb0
  -> board validation passed on COM3 with /dev/fb0 1024x600, 16 bpp and launcher auto-start

RuntimeManager loop callback
  -> HistoryRecorder observes Board IMU and file/mock status changes each scheduler round
  -> one-second status publish gate / final state publish
  -> StatusPublisher::publish()
  -> runtime/runtime_status.json

MP157 SD card storage path
  -> storage_enabled / --storage-root
  -> create status/, dashboard/, logs/, data/, captures/
  -> status/runtime_status.json
  -> dashboard/dashboard.txt
  -> logs/outdoor_core_runtime.log + size-based .1 ... .N rotation
  -> --history / HistoryRecorder
  -> GNSS/MCU serial per-valid-update callback
  -> data/history/{gnss,mcu_imu,magnetometer,barometer,board_imu}.csv

mp157/outdoor-core-service/data/mcu_mock_frames.txt
  -> McuMockService::poll()
  -> McuFrameParser::parseFrame()
  -> ImuPayloadParser::parse()
  -> McuStatus
  -> RuntimeStatus
  -> runtime/runtime_status.json

MP157 live serial path
  -> F407 PC10 UART4_TX
  -> MP157 PD9 USART3_RX
  -> /dev/ttySTM1, 115200 8N1, raw mode
  -> services::McuSerialService
  -> McuFrameStreamDecoder
  -> McuFrameParser::parseFrame()
  -> ImuPayloadParser::parse()
  -> McuStatus
  -> runtime/runtime_status.json

MP157 downlink command path
  -> --mcu-command ping / mcu_command = ping
  -> services::McuSerialService
  -> mcu::buildPingCommandFrame()
  -> /dev/ttySTM1, 115200 8N1, raw mode
  -> MP157 PD8 USART3_TX
  -> F407 PC11 UART4_RX
  -> UART4_IRQHandler() / HAL_UART_RxCpltCallback()
  -> 64-byte UART4 RX ring buffer
  -> board_uart_receive_byte()
  -> mcu_command_decoder_feed_byte()
  -> mcu_build_command_ack_frame()
  -> F407 PC10 UART4_TX
  -> MP157 PD9 USART3_RX
  -> McuFrameParser::applyFrame()
  -> runtime_status.json command_ack_*

MP157 onboard ICM20608 path
  -> modprobe icm20608
  -> /dev/icm20608
  -> sensors::Icm20608CharReader
  -> services::Icm20608Service
  -> sensors::BoardImuStatus
  -> runtime/runtime_status.json board_imu

Optional MP157 onboard ICM20608 IIO path
  -> /sys/bus/iio/devices/iio:device*/in_accel_*_raw
  -> /sys/bus/iio/devices/iio:device*/in_anglvel_*_raw
  -> /sys/bus/iio/devices/iio:device*/in_temp_*
  -> sensors::Icm20608IioReader
  -> services::Icm20608Service
  -> sensors::BoardImuStatus
  -> runtime/runtime_status.json board_imu

f407/sensor-hub MockImuProvider
  -> McuFrameBuilder::buildImuFrame()
  -> MCU sensor_imu frame
  -> mp157/outdoor-core-service/data/mcu_mock_frames.txt
  -> MP157 Runtime mock replay

f407/sensor-hub/firmware sensor_hub_app_poll()
  -> board_get_tick_ms()
  -> PB12 EXTI ISR records ICM42688 FIFO threshold/full event
  -> main loop coalesces pending events and calls icm42688_provider_read_fifo()
  -> read FIFO record count and burst-drain up to 16 complete 16-byte records from FIFO_DATA
  -> parse complete accel+gyro records; treat HEADER_MSG as FIFO-empty termination
  -> rebuild 10 ms sample timeline and publish up to 16 IMU samples per batch
  -> timeout/read failure uses mock_imu_provider_next() and periodic reinitialization
  -> sensor_sample_filter_c fixed-point IIR
  -> UART4 RX interrupt stores command bytes in a 64-byte ring buffer
  -> board_uart_receive_byte() consumes buffered command bytes
  -> mcu_command_decoder_feed_byte() handles command_ping
  -> mcu_build_heartbeat_frame() / mcu_build_imu_frame() / mcu_build_command_ack_frame()
  -> board_uart_send_bytes()
  -> atomic enqueue into UART4/USART1 fixed TX queues
  -> HAL_UART_Transmit_IT() and TX-complete callback drain
  -> PC10 TX, 115200 8N1
  -> MP157 USART3 /dev/ttySTM1 live serial validation

ICM42688 I2C sensor path
  -> PB10 I2C2_SCL
  -> PB11 I2C2_SDA
  -> PB12 ICM42688_INT1 rising-edge EXTI12
  -> sensor_hub_app_init()
  -> register bank 0
  -> DEVICE_CONFIG soft reset
  -> HAL_I2C_Mem_Read/Write(I2C2, 0x69 << 1)
  -> WHO_AM_I 0x47 check
  -> ACCEL_CONFIG0/GYRO_CONFIG0 range + 100 Hz ODR
  -> GYRO_ACCEL_CONFIG0 about 25 Hz UI LPF
  -> record-count FIFO, 4-record watermark and accel/gyro/temperature sources
  -> INT_CONFIG/INT_CONFIG1/INT_SOURCE0 FIFO threshold/full routing
  -> PWR_MGMT0 accel/gyro enable
  -> EXTI15_10_IRQHandler records event only
  -> sensor_hub_app_poll() burst drains FIFO_DATA after event
  -> alpha=1/4 fixed-point IIR for accel/gyro, alpha=1/8 for temperature
  -> MCU sensor_imu frame

MMC5603 I2C sensor path
  -> PB10 I2C2_SCL / PB11 I2C2_SDA
  -> address 0x30
  -> software reset + product ID + SET/RESET pulse
  -> 20 Hz one-shot measurement
  -> wait 7 ms, then poll STATUS1 at most 3 times with 1 ms spacing
  -> alpha=1/4 fixed-point IIR
  -> MCU sensor_magnetometer frame
  -> MP157 McuFrameParser / runtime_status.json magnetometer
  -> CompassEstimator subtracts hard-iron bias
  -> applies 3x3 soft-iron / sensor-to-body transform
  -> combines nearby ICM42688 acceleration for roll/pitch tilt compensation
  -> magnetic heading + declination -> runtime_status.json compass
  -> dashboard heading, quality and calibrated/uncalibrated state

MMC5603 calibration preparation path
  -> MP157 HistoryRecorder magnetometer.csv
  -> host tools/compass_calibrator CSV loader (nT -> uT)
  -> gross radial MAD trim
  -> normalized full 3D ellipsoid least-squares fit
  -> positive-definite eigen square root
  -> hard-iron bias + determinant-normalized 3x3 correction
  -> optional independently measured sensor-to-body rotation
  -> coverage / axis ratio / residual quality gates
  -> candidate config with compass_calibration_valid=false
  -> independent eight-direction / tilt / repeatability validation

BMP390 I2C sensor path
  -> PB10 I2C2_SCL / PB11 I2C2_SDA
  -> probe address 0x76, then 0x77
  -> Bosch BMP3 Sensor API v2.0.5, 64-bit compensation
  -> pressure/temperature 2x oversampling, IIR coefficient 3, 25 Hz ODR
  -> 10 Hz MCU sensor_barometer frame
  -> alpha=1/8 fixed-point IIR
  -> MP157 McuFrameParser / runtime_status.json barometer
  -> real board frame and MP157 barometer_seen=true validation complete

Validated MP157 live serial board link
  -> F407 PC10 UART4_TX to MP157 PD9 USART3_RX
  -> F407 PC11 UART4_RX from MP157 PD8 USART3_TX
  -> common GND
  -> /dev/ttySTM1 raw capture shows A5 5A MCU frame headers
  -> runtime_status.json: heartbeat_seen=true, imu_seen=true
  -> magnetometer_seen=true, barometer_seen=true, status_flags=425 (0x01A9)
  -> command_ack_seen=true, command_ack_status=0, nonce=0x50494E47
```

当前输出原始 NMEA 文本，并对通过 checksum 校验的 RMC/GGA/VTG/GSA/GSV 输出基础 GNSS 定位、速度、航向、卫星和 DOP 状态。

## IPC 方案

Stage 0.5 使用文件型状态发布作为最小 IPC 原型，Stage 1 后默认输出 `runtime/runtime_status.json`。Unix domain socket 状态查询接口已完成方案评估，暂不在 Stage 0 实现，后续在 STM32MP157 Linux 部署阶段继续推进。

## 问题排查与工程复盘

项目使用 `docs/troubleshooting_log.md` 维护问题级复盘，作为开发日志和 ADR 之外的工程证据入口。凡是需要分析的构建失败、测试失败、运行异常、硬件联调问题、性能退化或数据异常，都应创建唯一问题 ID，并持续记录现象、环境、复现、假设、失败/回退尝试、根因、修复、验证和剩余风险。

问题记录必须区分“已确认事实”和“待验证推测”，并为证据标记等级。问题未达到约定验证门槛时不能标为已关闭；当前功能恢复但缺少根因证据时，应标记为“已恢复，根因未完全确认”。涉及技术方案取舍时关联 `docs/adr/`，涉及阶段性实施结果时关联 `docs/dev_log.md`，从而形成“问题分析 -> 设计决策 -> 实施验证”的可追溯链路。

## 日志与配置方案

日志：

- 当前使用 `Logger` 输出时间戳、级别和消息
- 支持 `debug`、`info`、`warn`、`error` 最小日志级别过滤
- 默认输出到 stdout/stderr；启用 SD storage 后同时追加写入 `logs/outdoor_core_runtime.log`
- 支持按 `storage_log_max_bytes` 轮转并保留 `storage_log_backup_count` 个备份；默认 1 MiB 和 3 个备份，max bytes 为 0 时关闭轮转
- Logger 在文件输出会话内累计轮转失败和写/flush 失败，并将计数与最后错误发布到 `runtime_status.json`；日志失败同时进入 `storage.last_error`，预检与长测审计要求计数为零、错误为空
- 可选 `HistoryRecorder` 分别追加 GNSS、MCU IMU、磁力计、气压计和 Board IMU CSV；serial service 每个合法更新后触发记录，Runtime 循环观察器覆盖 Board IMU 和 file/mock 路径，默认每秒 flush

配置：

- 当前使用 `config/runtime.conf`
- 配置格式为简单 `key=value`
- 当前支持 `gnss_input_mode`、`nmea_input_path`、`gnss_serial_device`、`gnss_serial_baud`、`gnss_serial_capture_seconds`、`mcu_input_mode`、`mcu_mock_input_path`、`mcu_serial_device`、`mcu_serial_baud`、`mcu_serial_capture_seconds`、`mcu_command`、`runtime_run_seconds`、`status_output_path`、`storage_enabled`、`storage_root_path`、`storage_status_output_path`、`storage_dashboard_output_path`、`storage_log_file_path`、`storage_log_max_bytes`、`storage_log_backup_count`、`history_enabled`、`history_output_path`、`history_flush_interval_ms`、`dashboard_enabled`、`dashboard_output_path`、`dashboard_output_mode`、`dashboard_framebuffer_device`、`dashboard_refresh_count`、`dashboard_refresh_interval_ms` 和 `log_level`
- `gnss_serial_capture_seconds = 0`、`mcu_serial_capture_seconds = 0` 和 `dashboard_refresh_count = 0` 表示对应服务常驻；`runtime_run_seconds = 0` 表示 Runtime 不设置额外总时长限制
- 当前支持 `board_imu_enabled`、`board_imu_source`、`board_imu_device_path`、`board_imu_iio_path`、`board_imu_sample_count` 和 `board_imu_sample_interval_ms`，默认关闭板载 IMU，MP157 上板时显式开启；`board_imu_sample_count = 0` 表示持续采样
- 命令行参数可覆盖配置文件中的输入路径、日志级别和 storage root；`--storage-root` 会启用 SD storage，`--history` 或 `--history-output` 会启用历史记录

## 构建与部署方案

构建使用 CMake：

```bash
cmake -S . -B build
cmake --build build
```

当前本机开发构建使用 CMake；MP157 上板验证已使用 Windows host 的 `arm-none-linux-gnueabihf-g++ 9.2.1` 交叉编译 ARMv7 hard-float 可执行文件，并通过串口传输到板端运行。正点原子资料目录中的 `gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz` 与 OpenSTLinux SDK 脚本面向 Linux host，当前 Windows PowerShell 环境不能直接原生使用。

F407 固件采用双边界管理：

- CubeMX 负责生成芯片启动、HAL/CMSIS 和外设初始化基线。
- 项目自有业务代码保留在 `firmware/app`、`bsp`、`protocol` 和 `sensors`，避免散落到 Cube 生成目录。
- 仓库使用独立 GNU ARM CMake 构建直接引用 Cube 源文件，不把 Keil 工程作为唯一构建入口。
- 当前可生成 `sensor_hub.elf`、`.hex`、`.bin`、`.map` 并输出 Flash/RAM 使用量。
- 当前可通过 `scripts/flash_f407_uart.ps1` 使用 STM32 ROM UART Bootloader 烧录并回读校验 F407 固件；校验成功后脚本使用 `GO 0x08000000` 启动应用。应用态 Sensor Hub 帧通过 UART4 输出到 MP157 USART3，同时镜像到 USART1 便于 COM6 诊断抓包。

F407 上电后工作流程：

- CubeMX 生成代码负责 HAL、时钟、GPIO、I2C2、USART1 和 UART4 初始化。
- 项目 USER CODE 区域只调用 `sensor_hub_app_init()` 和 `sensor_hub_app_poll()`，业务逻辑保留在 `firmware/app`、`bsp`、`protocol` 和 `sensors`。
- `sensor_hub_app_init()` 先初始化 MMC5603 和 BMP390，最后完成 ICM42688 软复位、`WHO_AM_I=0x47` 检查、accel/gyro 100 Hz ODR、约 25 Hz UI LPF、record-count FIFO、4 条记录 watermark、`FIFO_WM_GT_TH`、FIFO threshold/full INT1 路由和工作模式使能，避免启动阶段 FIFO 积压。
- PB12 EXTI ISR 只记录 ICM42688 FIFO 事件；`sensor_hub_app_poll()` 在主循环以最短 30 ms 周期读取 FIFO record count，并批量读取最多 16 条完整记录，不在 ISR 中执行 I2C 事务。FIFO_DATA 失败时恢复 I2C2、flush FIFO 且不重放有副作用的读取。
- UART4 IRQ 持续把接收字节写入 64 字节 RX 环形缓冲；环满时拒绝新字节并累计丢弃计数。UART4/USART1 TX 使用各 1024 字节固定队列和 HAL 中断完成回调，不再阻塞主循环。
- 主循环发送 1 Hz heartbeat、1 Hz Sensor Hub diagnostics、FIFO 批量 IMU、20 Hz 磁力计和 10 Hz 气压计帧；MMC5603 单次测量触发后先等待 7 ms，再最多检查 3 次状态且每次间隔 1 ms，避免无间隔轮询占满共享 I2C2。当前 48 字节 diagnostics schema 1 在 legacy 44 字节载荷尾部追加扩展版本与 UART4 RX 累计丢弃字节数，同时进入 UART4 与 USART1；队列满时按整帧拒绝并设置 sticky 诊断位。
- ICM42688 初始化、读取或 INT1 超时失败时回退到 Mock IMU，并按 1 秒周期尝试重新初始化；heartbeat bit 7 表示近期 INT1 路径活动。
- IMU、磁力计和气压计帧在协议打包前经过无动态内存的定点一阶 IIR；ICM42688 还使用芯片内部 UI LPF。

构建系统选型记录见 `docs/adr/0001-use-cmake.md`。
配置格式选型记录见 `docs/adr/0002-use-simple-key-value-config.md`。
Runtime 服务架构记录见 `docs/adr/0003-runtime-service-architecture.md`。
最小 NMEA Parser 记录见 `docs/adr/0004-minimal-nmea-parser.md`。
文件型 IPC 原型记录见 `docs/adr/0005-file-status-ipc-prototype.md`。
Unix domain socket 状态查询接口评估见 `docs/adr/0006-evaluate-unix-domain-status-query.md`。
MCU 帧协议记录见 `docs/adr/0007-mcu-frame-protocol.md`。
F407 Cube 源码与 CMake 构建边界见 `docs/adr/0008-f407-cube-source-and-cmake-build-boundary.md`。
UART5 NMEA 输入与文本仪表盘原型见 `docs/adr/0010-use-nmea-uart5-text-dashboard.md`。BMP390 驱动选型见 `docs/adr/0011-use-bosch-bmp3-driver.md`。MP157 SD 卡文件持久化见 `docs/adr/0012-use-sd-card-file-storage.md`。UART4 中断接收环形缓冲见 `docs/adr/0013-use-uart4-interrupt-rx-ring-buffer.md`。I2C2 运行期总线恢复见 `docs/adr/0014-recover-i2c2-after-runtime-errors.md`。ICM42688 INT1 与定点滤波见 `docs/adr/0015-use-icm42688-int1-and-fixed-point-filtering.md`。ICM42688 FIFO 与 UART TX 队列见 `docs/adr/0016-use-icm42688-fifo-and-interrupt-uart-tx-queues.md`。MP157 协作式 Service 调度见 `docs/adr/0017-use-cooperative-runtime-service-scheduler.md`。CSV 历史记录与按大小日志轮转见 `docs/adr/0018-use-csv-history-and-size-based-log-rotation.md`。板端逐秒健康时间线见 `docs/adr/0019-use-sampled-health-timeline-for-board-endurance.md`。UART4 累计诊断链路见 `docs/adr/0020-expose-sensor-hub-diagnostics-on-uart4.md`。MMC5603 有界轮询与 ICM42688 record-count 修订见 `docs/adr/0021-bound-mmc5603-polling-and-use-fifo-record-count.md`。MP157 可标定倾斜补偿罗盘见 `docs/adr/0022-compute-calibrated-compass-heading-on-mp157.md`。Sensor Hub diagnostics 兼容扩展与版本门槛见 `docs/adr/0023-version-sensor-hub-diagnostics-extension.md`。离线完整矩阵罗盘校准见 `docs/adr/0024-fit-compass-calibration-offline.md`。

## 验收用例

- 能完成 CMake 配置
- 能编译生成 `outdoor_core_runtime`
- 不传参数时能读取 `mp157/outdoor-core-service/config/runtime.conf`
- 配置文件能指定默认 NMEA 输入路径
- 命令行能覆盖 NMEA 输入路径
- 命令行能覆盖日志级别
- Runtime Manager 能启动、运行并停止 GNSS 文件回放服务
- Runtime Manager 能交错轮询多个服务，区分 running/completed/failed，并在启动或运行失败时保留具体服务名
- 常驻 dashboard 不阻塞有限 GNSS/MCU 输入服务；运行期状态每秒发布，SIGINT/SIGTERM 或 `runtime_run_seconds` 可触发受控停止
- 能解析样例 RMC/GGA 并输出基础定位状态
- 能解析 VTG/GSA/GSV 并输出速度、航向、DOP 和可见卫星数
- 默认 file GNSS 模式能输出 `runtime_status.json` 的 `gnss` 字段
- 默认能生成 `runtime/dashboard.txt` 文本仪表盘原型
- 文本 dashboard 能输出 `app_icon_visible: true`，用于覆盖 framebuffer 主界面程序图标显示的自动化检查
- `config/outdoor_agent_app.conf` 能作为 APP 配置加载，并可在 PC 验证中覆盖为文本/单次刷新
- APP launcher 能读取 `launcher_enabled`、`launcher_input_device` 和 `launcher_auto_start_seconds` 配置；在 MP157 上可显示启动页，并通过 `--launcher-auto-start-seconds 2` 无人值守进入 dashboard
- APP launcher 真实手指点击启动已通过 COM3 验证：串口日志出现 `Launcher icon tapped at x=530, y=292`，随后 `Dashboard rendered to framebuffer: /dev/fb0`
- 启用 `--storage-root` 后能在 storage 目录下生成 `status/runtime_status.json`、`dashboard/dashboard.txt` 和 `logs/outdoor_core_runtime.log`
- 启用 `--history` 后能生成五类 CSV，重复状态不重复追加，CSV 字符串字段正确转义
- 日志达到大小上限后按 `.1` 到 `.N` 轮转，活动文件保留最新消息且备份数量不超过配置值
- 能拒绝 checksum 错误的 NMEA 语句
- 能生成 Runtime 状态文件并输出基础状态字段
- 能解析 MCU heartbeat 帧并更新 `McuStatus`
- 能解析 Sensor Hub diagnostics 并输出 I2C/FIFO 累计计数和最近失败上下文
- 能解析 MCU mock sensor 帧并输出到 `runtime_status.json`
- 能解析 Mock IMU 帧并输出 `runtime_status.json` 的 `imu` 字段
- 能对时间接近且 heartbeat 确认来源健康的真实 IMU/磁力计样本执行硬铁/3×3 矩阵/倾斜补偿，输出 `[0,360)` 航向、质量和标定状态；缺输入、Mock fallback、过期、动态加速度、异常磁场与奇异矩阵有负向测试
- 能从 MCU 字节流中重组前导噪声、分片帧和连续帧，再复用现有 MCU parser
- 默认 PC 配置下能输出 `runtime_status.json` 的 `board_imu` 字段，且 `enabled=false`
- 能通过 fake character-device 文件验证 ICM20608 character reader 的 accel/gyro/temp 换算
- 能通过 fake-IIO 目录验证 ICM20608 IIO reader 的 accel/gyro/temp 换算
- 在启用 `--board-imu` 且提供 `/dev/icm20608` 字符设备路径时，能读取 ICM20608 并输出 `board_imu.seen=true`；当前已在真实 MP157 板上验证 `sample_count=5`、`last_error=""`、静置 Z 轴约 `0.983g`
- 能拒绝 CRC16 错误的 MCU 帧
- 能使用 `tools/frame_decoder` 解码十六进制 IMU 协议帧
- 能使用 `tools/compass_calibrator` 从 History CSV 恢复合成硬铁/全矩阵畸变，拒绝近平面数据、强磁离群点和非法安装矩阵，并强制输出 `compass_calibration_valid=false`
- F407 firmware C 骨架能通过本机 C 语法检查
- CubeMX 基础工程的芯片、时钟、GPIO、HAL/CMSIS 文件清单有明确记录
- 能使用 GNU Arm Embedded Toolchain 编译并链接 F407 最小固件
- 能生成 ELF、HEX、BIN、map 和 size 输出
- F407 固件能通过 UART4 构建 heartbeat 和 IMU 上报链路，并通过 MP157 USART3 `/dev/ttySTM1` 读取
- MP157 Runtime serial 模式能构建并发送 `command_ping`，F407 固件通过 UART4 中断接收缓冲解析命令并返回 `command_ack`；2026-07-18 当前 ARM Runtime 板端状态 JSON 已确认 `command_ack_seen=true`、`command_ack_status=0` 和 nonce `0x50494E47`
- F407 固件能通过 USART1 UART Bootloader 烧录和回读校验；USART1 当前保留为下载链路
- MP157 Runtime serial 模式能读取真实 F407 UART4 帧，并输出 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true` 和真实 IMU 字段
- F407 固件通过 PB12 INT1 data-ready 触发 ICM42688 读取并保留 Mock fallback；2026-07-19 的 60 秒抓取为 `92.45 Hz`，最终 heartbeat `0x00A9`
- F407 固件能读取 MMC5603 并以约 20 Hz 输出 `sensor_magnetometer`，MP157 状态已确认 `magnetometer_seen=true`
- F407 固件 BMP390 路径能生成 10 Hz `sensor_barometer`，MP157 状态已确认 `barometer_seen=true`
- F407 I2C2 运行期事务错误可触发外设硬复位、总线释放和单次重试；2026-07-19 的 INT1 版本 60 秒采集最终保持 `status_flags=0x00A9`
- ICM42688 FIFO parser/批次时间轴、provider 寄存器配置与异常 flush、MMC5603 有界状态检查、定点 IIR、UART TX 队列和 diagnostics schema 1 frame builder 有独立主机测试；F407 PC CTest 当前 7/7 通过
- 已完成历史板端验收的 FIFO/TX 队列 Release BIN 为 19368 B、健康 heartbeat 为 `0x01A9`。恢复后的正式 schema 1 镜像为 19384 B、SHA256 `f0addc29...b9d1e`；400 kHz ICM-only 基线为 15072 B、SHA256 `c07e235a...2f80a`，100 kHz A/B 镜像为 15072 B、SHA256 `56b31d76...c3e56`。100 kHz 完整复电 A/B 已失败；F407 USB-UART 从 COM6 重枚举为 COM3 后，400 kHz 隔离基线已重新烧录并逐字节回读通过
- 文件不存在时返回非 0 并输出错误日志

## 风险与 TODO

- 当前日志已支持大小限制、有限备份轮转、轮转/写入失败累计计数和结构化最后错误；尚未实现压缩与总磁盘配额
- 当前 CSV history 保存成功解析并应用的状态更新，不包含噪声、CRC 错误和解析失败载荷；需要完整原始证据时仍应独立抓取串口字节流
- CSV 多文件写入不具备事务性；真实 SD 卡小时级增长、掉电一致性、flush 延迟和磨损仍待上板验证
- 当前配置格式只适合少量简单配置项
- 当前 MCU 输入源支持 mock 文件和 Linux 串口两种模式；F407 UART4 与 MP157 USART3 双向 Runtime 验证已完成，环形缓冲溢出已具备累计计数、schema 版本和预检/时间线/审计门槛，并已完成真实 RX 环满压力注入、拒绝与复位归零验证；后续仍需补充长期稳定性、容量边界测量和完整命令集设计
- 当前 NMEA Parser 覆盖 RMC/GGA/VTG/GSA/GSV，未覆盖完整 NMEA 协议或 UBX 二进制协议
- 当前 UBLOX-M10 UART5 已确认 `/dev/ttySTM2`、38400 和 NMEA 输出集合；室外有效 fix 尚待验证
- MMC5603 原始磁场读取已验证；硬铁、3×3 软铁/安装矩阵和倾斜补偿软件链路已实现，但当前单位矩阵只标记为 `uncalibrated`，仍需室外多姿态采集、拟合、八方向精度和动态稳定性验证
- BMP390 I2C ACK 和真实协议帧已完成上板验证；补偿值已通过宽范围合理性检查和 60 秒持续采集，仍需后续室外对照测量与小时级耐久测试
- 当前 `outdoor-agent` 是轻量 fbdev framebuffer 仪表盘和最小 evdev launcher 原型，不代表完整触摸控件体系、窗口系统或桌面 GUI 已完成；光照、空气质量、电池和信号等展示项暂为 UI 占位/演示指标
- 当前 `outdoor-agent` APP 进程入口已完成代码开发、本机配置烟测和 MP157 COM3 上板验证；验证中 `/dev/fb0` 为 1024x600、16 bpp，`run_outdoor_agent_app.sh` 可显示 launcher 并通过自动启动进入 framebuffer dashboard
- Goodix 触摸节点 `/dev/input/touchscreen0 -> event0` 已确认存在，并已通过真实手指点击启动验证；后续若扩展多控件交互，还需要补充更完整的触摸坐标校准、方向适配和 UI 状态机
- 当前 IPC 原型是状态文件发布，不是完整交互式 IPC 服务
- 当前 MP157 板载 ICM20608 读取服务已实现并通过 fake character-device / fake-IIO 验证；真实 MP157 板上已确认设备树、SPI 设备和 `modprobe icm20608` 可识别 `ICM20608 ID = 0XAE`，并用正点原子 `icm20608App` 以及项目自有 ARM Runtime 通过 `/dev/icm20608` 读到真实 accel/gyro/temp 数据
- 当前 MP157 Runtime 默认仍从 mock frame 文件读取 MCU 数据；serial 模式已按 `/dev/ttySTM1` 完成 F407 UART4 真实接线验证
- ICM42688 当前接线曾恢复真实数据，INT1/EXTI 和 I2C2 瞬时错误恢复均完成过 60 秒上板验证；本次完整复电恢复初始化却未消除运行期 NACK/FIFO 损坏。软件单器件隔离和 100/400 kHz A/B 已完成且降速失败，下一步必须检查上拉/供电/共地/布线并取得波形，再执行小时级运行和断线故障注入
- F407 USART1 保留为 UART Bootloader 下载口并镜像诊断帧；UART4/USART1 当前均使用固定内存中断 TX 队列
- ICM42688 FIFO 和非阻塞 TX 队列的历史版本已完成真实 FIFO 验收；后续 diagnostics 暴露的异常已进入 TRB-024/ADR-0021。完整复电、编译期 ICM-only 隔离和 100/400 kHz A/B 已证明现有软件收敛仍不足；当前板上已恢复 400 kHz 隔离基线，待电气根因收敛后才能执行 30/60 秒、小时级、逐颗接回、SDA 卡住和传感器断线故障注入验证
- FIFO 单次软件批次上限为 256 字节；超过上限或出现畸形包时会 flush，并通过 UART4/MP157 与 USART1/COM6 的同一诊断帧保留累计信息
- 当前滤波参数为编译期常量，后续姿态融合若需要原始高动态数据，应增加独立 raw 通道或可配置参数组
- STM32CubeProgrammer 已安装；当前烧录流程采用仓库自带 UART Bootloader 脚本来控制一键下载电路时序，ST-LINK 未接入
- 当前 Runtime Manager 已使用单线程协作式调度解决常驻 dashboard 与采集服务互相阻塞的问题，并输出 active/completed service 计数；真实 MP157 多串口、framebuffer 小时级验证和更细的服务健康指标仍待完成
- 真实 MP157 已完成 60 秒 history 联合冒烟、修复后的 20 秒 framebuffer history 频率复验和 30 秒日志轮转复验；当前仅证明短时功能与频率，不能替代小时级、掉电和 SD 磨损验证
- 小时长测和 SIGKILL 恢复脚本已增加独立健康预检；历史真实板两次以 `0x01A9` 通过，第一次放行的 60 秒联合冒烟和完整审计通过，第二次放行的 3600 秒正式长测因运行期 fallback 作废。当前预检已直接以 `0x102E` 多次拒绝 Mock fallback，并在最新 ARM Runtime 上同时验证日志健康门槛通过
- 第二次 3600 秒任务在早期发现 `0x022E/0x03AE` FIFO fallback 后自恢复；由于原审计只有最终快照，任务已主动停止并作废。新增逐秒健康时间线已正确拒绝周期性降级；完整复电后的短测再次以 diagnostics 增量证明问题仍在，三传感器与单器件 100 kHz、fixed partial-read 均已证伪。当前需先取得 ICM-only 电气与波形证据，再从零开始 30/60 秒和小时级验证
