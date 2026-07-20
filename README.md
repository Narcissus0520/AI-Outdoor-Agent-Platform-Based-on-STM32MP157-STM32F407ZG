# AI Outdoor Agent Platform

AI Outdoor Agent Platform 是一个围绕 STM32MP157 + STM32F407ZG 的户外智能体长期作品集项目。当前仓库采用 Monorepo 结构，将 MP157 Linux 主控侧程序、F407 Sensor Hub 固件、公共协议和工具脚本分目录管理。

## 当前状态

- `mp157/outdoor-core-service/`: Linux Outdoor Core Runtime，已完成 Stage 0，并在 Stage 1 支持 GNSS 文件回放、UART5 GNSS/NMEA 软件输入路径、`outdoor-agent` 文本和 framebuffer 仪表盘 APP、SD 卡目录持久化输出、传感器 CSV 历史记录、有限容量日志轮转、MCU heartbeat/传感器协议解析、MP157 板载 ICM20608、F407 live serial、最小 `command_ping` 下行命令，以及可同时推进采集、状态发布和常驻 dashboard 的协作式 Service 调度。Stage 2 已新增只读 Unix domain socket 状态查询、`outdoor_status_query` 客户端，以及默认只安装不启动的 systemd Runtime 托管方案。
- `f407/sensor-hub/`: STM32F407ZG Sensor Hub 软件目录，包含 PC mock C++ 工程，以及 `firmware/` 下的项目自有 C 代码和 STM32CubeMX/HAL 工程。当前固件通过 UART4 PC10/PC11 对接 MP157 USART3，发送 heartbeat、ICM42688 IMU、MMC5603 磁力计和 BMP390 气压计协议帧；UART4 RX 使用中断接收和 64 字节环形缓冲，可可靠回复 MP157 下发的 `command_ping`，并通过 diagnostics schema 1 发布累计 RX 丢字节计数。
- `common/protocol/`: MP157 与 F407 共享的协议常量、CRC16/MODBUS 和各传感器 payload 定义。
- `tools/`: PC 调试工具，当前包含 `frame_decoder` 十六进制 MCU 协议帧解析工具和从 `magnetometer.csv` 拟合候选硬铁/全 3×3 软铁参数的 `compass_calibrator`。
- `scripts/`: 放置仓库级构建、烧录、串口传输和上板验证脚本。

当前不包含 HTTP API 或真实 AI Agent 推理/交互能力。Sensor Integration 已完成 MP157 板载 ICM20608、F407 侧 ICM42688/MMC5603/BMP390 和 F407/MP157 双向串口链路的历史真实上板验证。2026-07-19 已完成 FIFO/TX 队列镜像的 60 秒真实传感器验收；随后小时任务捕获到可自恢复的 `0x022E/0x03AE` FIFO fallback，原任务已作废。完整断电重上电恢复了 ICM42688 初始化，但累计 diagnostics 仍显示持续 I2C/FIFO 恢复；三传感器 100 kHz 和两种 fixed partial-read 均已实板证伪并撤回。首轮 ICM-only `gsVcum` 因 COM6 未关闭降级为受污染证据；COM6 关闭后的 400 kHz `XMWvGC` 仍在约 58 秒累计 recovery/failure `232/45`。同窗口单器件 100 kHz `wB2xhs` 更退化为 `0xD206`、init step 1，recovery/failure 速率分别是 400 kHz 的 `3.29/3.67` 倍，因此其他两颗从机访问、COM6 打开和单纯 400 kHz 过快均已排除为异常成立的必要条件。回滚时原 COM6 未枚举，用户确认设备改为 COM3；COM3 重新枚举正常且无终端占用后，已以 Bootloader `0x31`、chip ID `0x0413` 将 400 kHz ICM-only 基线完整写入并逐字节校验。当前板上为可信的 400 kHz 隔离镜像，但问题仍未修复，30/60 秒和小时级验收继续禁止；下一步需要万用表或逻辑分析仪检查供电、共地、上拉、接触和事务波形。UBLOX-M10 已确认从 MP157 UART5 `/dev/ttySTM2` 以 38400 8N1 输出有效 NMEA；当前环境无卫星信号，因此定位仍无效。`outdoor-agent` 当前可在 7 英寸 RGB 屏 `/dev/fb0` 显示 APP launcher、仪表盘和 AI Agent 预留区。

## 快速编译 MP157 Runtime

在仓库根目录执行：

```bash
./scripts/build_mp157.sh
```

也可以单独进入模块目录编译：

```bash
cd mp157/outdoor-core-service
cmake -S . -B build
cmake --build build
```

## MP157 协作式 Runtime

MP157 Runtime 已将阻塞式顺序 `IService::run()` 演进为单线程协作式 `poll()` 调度。GNSS、MCU、板载 IMU、状态发布、launcher 和 dashboard 在同一事件循环中按短步骤推进，不引入线程数据竞争或新的第三方依赖。`runtime_status.json` 在常驻运行期间每秒更新，并增加 `active_service_count` 与 `completed_service_count`。

持续采集时，GNSS/MCU `capture_seconds = 0`、Board IMU `sample_count = 0` 和 dashboard `refresh_count = 0` 分别表示常驻；`runtime_run_seconds` 可限制整次验证时长，0 表示只由服务完成或 SIGINT/SIGTERM 结束。小时级板端验证可使用：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --gnss-input-mode serial --gnss-serial-capture-seconds 0 \
  --mcu-input-mode serial --mcu-serial-capture-seconds 0 \
  --dashboard-output-mode both --dashboard-refresh-count 0 \
  --runtime-run-seconds 3600
```

当前本机 11/11 CTest、连续 dashboard 一秒受控运行、storage/history/compass 组合验证和 ARMv7 hard-float 全目标交叉构建已通过。真实 MP157 也已完成 60 秒多设备/SD history 冒烟、修复后 20 秒 framebuffer history 频率复验和 30 秒 1 MiB 日志轮转复验；Stage 2.1 socket 板端运行、小时级与掉电验证仍待执行。

正式小时长测和 SIGKILL 恢复验收启动前会自动执行 8 秒板端健康预检；只有 F407 四类数据、ping ACK、`status_flags=0x01A9`、GNSS NMEA、板载 ICM20608、日志轮转/写入失败计数为零和核心错误字段全部通过，才会创建并启动正式测试，避免 Mock fallback、外设失联或存储日志故障污染耐久结论。

小时长测期间还会逐秒保存健康时间线；结束审计除最终状态外，还检查样本覆盖、致命状态位、核心错误、健康占比、最长连续降级，以及 Sensor Hub I2C/FIFO 累计诊断的单调性和运行期增量，避免 ICM42688 FIFO 自恢复后由最终 `0x01A9` 掩盖运行期 fallback。

## MP157 本地状态查询

Stage 2.1 在原子更新的 `runtime_status.json` 之外增加可选 Unix domain stream socket。文件和 socket 复用同一个 JSON 序列化入口；socket 仅接受 `GET_STATUS`、`PING` 最小行协议，默认权限 0660，并限制连接数、请求长度和空闲时间。该接口没有远程网络暴露、写命令或第三方依赖。

默认配置保持关闭。Linux/MP157 上可显式启用并使用配套客户端查询：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --status-socket --status-socket-path runtime/outdoor_core.sock \
  --runtime-run-seconds 60
./outdoor_status_query runtime/outdoor_core.sock
```

启用 socket 后 Runtime 包含常驻服务，需要 SIGINT/SIGTERM 或 `runtime_run_seconds` 结束。本机协议与非 POSIX 边界测试、Runtime verifier 以及 GNU ARM Linux 9.2.1 全目标交叉链接已通过；真实 MP157 上的权限、连续查询、stale/active 冲突和退出清理按当前约束留待后续联调验收。

## MP157 Runtime 进程托管

Stage 2.2 提供 `outdoor-agent-runtime.service` 和专用 `config/outdoor_agent_service.conf`。unit 以 systemd `Type=simple` 托管前台 Runtime，依赖既有 ICM20608 loader，使用 `/run/outdoor-agent` 保存状态与 socket，以 SIGTERM 受控停止，并对失败重启频率、可写路径、地址族和设备节点做有界限制。

部署脚本默认只上传并静态验证 unit/config，不 enable、不启动 Runtime；需要板端状态变更时必须显式选择：

```powershell
# 仅安装，保持 Runtime 当前 enable/active 状态不变
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts/deploy_mp157_runtime.ps1 -PortName COM9

# 后续联调阶段才使用；Start 会隐式 Enable
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts/deploy_mp157_runtime.ps1 -PortName COM9 `
  -EnableRuntimeService -StartRuntimeService
```

当前纯软件 verifier 已覆盖 unit 依赖、运行目录、SIGTERM、重启上限、沙箱、设备白名单和 live-source 配置，并以四个不安全变体证明检查会失败。由于真实设备 ownership/group 尚未形成证据，首版显式以 root 运行但收窄能力；切换专用用户需后续板端权限验收。本轮没有执行上述部署、enable 或 start 命令。

## MP157 历史记录与日志轮转

启用 storage 与 history 后，Runtime 会在 storage root 下追加 GNSS、MCU IMU、磁力计、气压计和 Board IMU CSV，并按大小轮转 Runtime 日志：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --storage-root /run/media/mmcblk1p1/outdoor-agent --history
```

默认历史目录为 `data/history/`，日志单文件上限为 1 MiB，保留 3 个备份。`runtime_status.json` 的 `storage` 节点发布 `log_rotation_failure_count`、`log_write_failure_count` 和 `log_last_error`；任何日志失败也会进入聚合 `storage.last_error`，供预检与长测审计拒绝。serial service 会在每个合法 GNSS/MCU 更新后追加历史；修复后的真实 framebuffer 联合测试记录 MCU IMU 约 98.1 Hz。CSV 适合 Excel/Python 分析，但不包含噪声、CRC 错误等原始串口字节；真实 SD 卡小时级增长、掉电一致性和写入磨损仍待验证。

## MP157 罗盘解算

Runtime 已将 F407 ICM42688 加速度与 MMC5603 三轴磁场组合为独立 `compass` 状态，支持三轴硬铁偏置、完整 3×3 软铁/安装变换矩阵、roll/pitch 倾斜补偿、磁偏角和输入质量门槛。只有 heartbeat 确认真实 IMU/磁力计 ready 且无 fallback/FIFO/I2C 当前错误时才接受输入，`0x102E` 等 Mock fallback 不会生成有效航向。text/framebuffer dashboard 优先使用有效罗盘航向；罗盘无效时，只在 GNSS fix 有效且速度不低于 1 km/h 时使用 GNSS course，不再显示固定假航向。

默认配置使用零偏置和单位矩阵，并明确设置 `compass_calibration_valid = false`，所以输出质量为 `uncalibrated`，不代表真实整机已经标定。现场完成多姿态采集和系数拟合后，才能写入 `compass_hard_iron_*_ut`、`compass_soft_iron_00..22`、当地 `compass_declination_degrees` 并把标定状态设为 true。算法、Runtime verifier、ARM 交叉构建和 MP157 板端确定性 fixture 已通过；板端示例为 `heading=30.303°`、`field=57.711 uT`、JSON/dashboard 一致。真实八方向精度和动态稳定性仍待室外验收。

仓库现提供无第三方依赖的离线全椭球拟合工具：

```powershell
cmake -S tools/compass_calibrator -B tools/compass_calibrator/build
cmake --build tools/compass_calibrator/build --config Debug
tools\compass_calibrator\build\Debug\compass_calibrator.exe `
  --input path\to\magnetometer.csv `
  --output path\to\compass_candidate.conf
```

工具会检查三维姿态覆盖、轴比和拟合残差，支持极端磁干扰点稳健裁剪及可选 sensor-to-body proper rotation。输出候选配置始终保持 `compass_calibration_valid=false`；磁力计数据不能唯一推导安装方向，且拟合不能替代八方向、倾角、动态和磁偏角验收。详细说明见 [Compass Calibrator](tools/compass_calibrator/README.md)。

## MP157 板载 ICM20608

MP157 侧新增 `icm20608_service`，参考 正点原子 STM32MP157 ICM20608 示例读取板载 ICM20608。当前实测板上系统通过 `modprobe icm20608` 加载 `/lib/modules/.../drivers/char/icm20608.ko`，并生成 `/dev/icm20608` 字符设备。板厂镜像会在标准 modules-load 之后才运行 `link-modules.service`，因此仓库使用 `deploy/systemd/outdoor-agent-icm20608.service` 明确排在链接服务之后加载驱动；真实软重启已确认服务 active/enabled、probe ID `0xAE` 且设备节点自动出现。IIO sysfs 读取路径保留为后续可选来源。默认 PC 开发配置关闭该服务，避免没有真实设备时影响自动化验证。

2026-06-19 已完成真实 MP157 上板验证：使用 Windows host 的 `arm-none-linux-gnueabihf-g++ 9.2.1` 交叉编译 ARMv7 hard-float 可执行文件，经 CH340 串口传输到 `/tmp/ai_outdoor_runtime` 后运行通过。板端 `runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`、`source=icm20608_char`、`sample_count=5`、`last_error=""`；静置样例中 Z 轴约 `0.983g`，温度约 `36.5°C`。

本地资料目录 `F:\BaiduNetdiskDownload\【正点原子】STM32MP157开发板（A盘）-基础资料\05、开发工具\01、交叉编译器` 中的 `gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz` 和 OpenSTLinux SDK 脚本是 Linux host 工具链，不能直接在当前 Windows PowerShell 环境原生使用；本次验证改用同版本 Windows host 工具链完成交叉编译。

默认配置项位于 `mp157/outdoor-core-service/config/runtime.conf`：

```text
board_imu_enabled = false
board_imu_source = char_device
board_imu_device_path = /dev/icm20608
board_imu_iio_path = /sys/bus/iio/devices/iio:device0
board_imu_sample_count = 5
board_imu_sample_interval_ms = 100
```

在 MP157 板上启用验证：

```bash
cd mp157/outdoor-core-service
modprobe icm20608
./build/outdoor_core_runtime --board-imu --board-imu-source char_device --board-imu-device-path /dev/icm20608 --board-imu-samples 5
```

预期 `runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`，并输出 `accel_*_g`、`gyro_*_dps` 和 `temperature_celsius`。如果后续切换到 IIO 驱动，可使用 `--board-imu-source iio` 并以板上 `/sys/bus/iio/devices/` 枚举结果为准。

## UBLOX-M10 GNSS 与仪表盘

MP157 Runtime 当前新增 GNSS 输入模式：

```text
gnss_input_mode = file | serial
gnss_serial_device = /dev/ttySTM2
gnss_serial_baud = 38400
gnss_serial_capture_seconds = 5
dashboard_enabled = true
dashboard_output_path = runtime/dashboard.txt
dashboard_output_mode = text | framebuffer | both
dashboard_framebuffer_device = /dev/fb0
dashboard_refresh_count = 1      # 0 表示常驻刷新
dashboard_refresh_interval_ms = 1000
launcher_enabled = false
launcher_input_device = /dev/input/touchscreen0
launcher_auto_start_seconds = 0
```

默认仍使用 `data/nmea_sample.txt` 文件回放，方便 PC 自动化测试；后续上板时可切换到 UART5：

```bash
./outdoor_core_runtime --config config/runtime.conf --gnss-input-mode serial --gnss-serial-device /dev/ttySTM2 --gnss-serial-baud 38400
```

当前 NMEA parser 支持 RMC/GGA/VTG/GSA/GSV，可输出经纬度、海拔、速度、航向、卫星数、fix quality/type 和 DOP。`outdoor-agent` 仪表盘默认输出到 `runtime/dashboard.txt`，也可通过 `--dashboard-output-mode framebuffer|both --dashboard-framebuffer-device /dev/fb0` 直接渲染到 7 英寸 RGB 屏 framebuffer。当前 framebuffer 界面已按深色科技风参考图实现左侧导航、左上角程序图标、顶部状态栏、方向罗盘、大速度表、位置地图、温度/光照展示区和底部状态栏；其中光照、空气质量、电池、信号等暂为 UI 占位/演示指标，尚未接入真实传感器或电源管理数据。真实 AI Agent 本地部署或交互仍未实现。

MP157 屏幕侧 APP 进程入口已固定为 `config/outdoor_agent_app.conf` 和启动脚本：

```bash
cd mp157/outdoor-core-service
./scripts/run_outdoor_agent_app.sh
```

该配置默认使用 file/mock 数据源，输出模式为 `both`，会先显示一个轻量 fbdev APP launcher，再由 `/dev/input/touchscreen0` 的 evdev 事件点击中央 `OUTDOOR AGENT` 图标/磁贴进入仪表盘；进入后会同时写入 `runtime/dashboard.txt` 并持续刷新 `/dev/fb0`。`dashboard_refresh_count = 0` 表示常驻刷新。2026-07-05 已通过 COM3 在 MP157 上板验证：`/dev/fb0` 为 1024x600、16 bpp，Goodix 触摸设备为 `/dev/input/touchscreen0 -> event0`，当前 ARM Runtime 可显示 launcher，并已验证 `--launcher-auto-start-seconds 2` 自动进入仪表盘和真实手指点击图标启动；点击日志为 `Launcher icon tapped at x=530, y=292`，随后成功渲染 `/dev/fb0`。文本 dashboard 可见 `app_icon_visible: true`。

## MP157 SD 卡持久化输出

MP157 插入并挂载 SD 卡后，Runtime 可将状态、文本仪表盘和日志写入 SD 卡目录：

```bash
./outdoor_core_runtime --config config/runtime.conf --storage-root /run/media/mmcblk1p1/outdoor-agent
```

当前 MP157 实测 SD 卡自动挂载为 `/run/media/mmcblk1p1`；`/mnt/sdcard` 当前不存在。启用后默认生成 `status/runtime_status.json`、`dashboard/dashboard.txt`、`logs/outdoor_core_runtime.log` 和五类 `data/history/*.csv`，并预建 `captures/` 目录。当前已包含按大小日志轮转、结构化日志失败计数和 CSV 历史记录，但不包含数据库、压缩归档或总磁盘配额。

## F407 固件状态

STM32CubeMX 生成的基础工程位于：

```text
f407/sensor-hub/firmware/stm32cube/
```

仓库已使用 `arm-none-eabi-gcc` 和 CMake 独立生成 F407 的 ELF/HEX/BIN/map，不依赖 Keil 工程完成编译。当前固件包含 PF9 LED 心跳；USART1 PA9/PA10 保留给 COM6 UART Bootloader 烧录，应用态 Sensor Hub 协议帧由 UART4 PC10/PC11、115200 8N1 输出到 MP157 USART3，同时镜像到 USART1。UART4 和 USART1 已分别使用 1024 字节固定 TX 队列与 `HAL_UART_Transmit_IT()` 非阻塞发送，UART4 RX 继续使用中断和 64 字节环形缓冲，并将累计丢字节数放入 48 字节 diagnostics schema 1。F407 已配置 PB10 (`I2C2_SCL`)、PB11 (`I2C2_SDA`) 和 PB12 (`ICM42688_INT1/EXTI12`)；共享 I2C2 当前为 400 kHz。ICM42688 已启用 record-count FIFO、4 条记录 watermark、`FIFO_WM_GT_TH`、最多 16 样本批量解析和 threshold/full INT1。FIFO 数据流读取失败时恢复 I2C2 但不重放有副作用的 burst，并 flush 重新对齐；普通寄存器事务仍允许恢复后重试。MMC5603 单次测量按 6.6 ms 器件时序先等待 7 ms，再执行最多 3 次有间隔的状态检查，避免紧轮询占满共享总线。传感器输出经过 ICM 内部约 25 Hz LPF 和轻量定点一阶 IIR。

已完成历史板端验收的 FIFO/TX 队列 Release 固件 BIN 为 19368 B，链接统计为 text 19348 B、data 20 B、BSS 4424 B；整板复电后的 60 秒验收共解析 7988 帧，最终 heartbeat `0x01A9`，时间戳、CRC、协议和 TX overflow 门槛通过。UART4 diagnostics 镜像已经刷板，并用多轮预检/短测取得累计 I2C/FIFO 失败证据。完整复电后的 `rjgpa6` 首次预检通过，但 uptime 104078 ms 已累计 recovery 234、overflow 56；约 73 秒后的 `Hq1vCJ` 预检变为 `0x03A9` 并拒绝正式测试。三传感器 100 kHz、固定 128 B 和固定 64 B FIFO 读取均未改善并已撤回；临时抓到的四个候选包头为 `06 FF FF FF`。恢复后的正式 schema 1 镜像仍为 19384 B、SHA256 `f0addc29...b9d1e`；400 kHz ICM-only 基线仍为 15072 B、SHA256 `c07e235a...2f80a`。COM6 关闭后的 `XMWvGC` 在 diagnostics uptime 58298 ms 已累计 recovery/failure `232/45` 和多项 FIFO 异常。单器件 100 kHz `wB2xhs` 在相近窗口反而得到 `0xD206`、init step 1、recovery/failure `759/164`，对应速率为 400 kHz 的 `3.29/3.67` 倍；降速假设已实板证伪。回滚时 Windows 未枚举 COM6，用户确认同一 F407 USB-UART 当前变为 COM3；经 PnP/占用检查后，400 kHz ICM-only 基线已通过 COM3 重新全擦、写入、逐字节回读和 GO。当前 Flash 可信且处于 400 kHz 隔离基线，但隔离诊断尚未通过，不能写成 I2C/FIFO 修复或小时级验收通过。

MMC5603 与 ICM42688 共用 I2C2：`SCL -> PB10`、`SDA -> PB11`，MMC5603 地址为 `0x30`。固件使用 20 Hz 单次测量并发送 `sensor_magnetometer` 帧；每次触发后等待 7 ms，再进行有界状态检查。IMU 不再使用固定 10 ms 轮询。2026-07-18 当前接线已恢复并通过真实 `sensor_magnetometer` 帧验证。

BMP390 同样复用 I2C2。固件启动时优先探测 `0x76`，再探测 `0x77`，使用 Bosch BMP3 Sensor API v2.0.5 的 64-bit 定点补偿，配置温度/气压 2x 过采样、IIR 系数 3 和 25 Hz ODR，并以 10 Hz 发送 `sensor_barometer` 帧。2026-07-18 当前接线已通过真实 ACK、`sensor_barometer` 帧和 MP157 `barometer_seen=true` 验证。

详细步骤见 [F407 固件构建计划](docs/stage1_bringup_plan.md)。

Windows 开发环境可在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build_f407.ps1
```

构建产物位于 `f407/sensor-hub/firmware/build/`。

通过 UART Bootloader 烧录 F407：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM3
```

烧录后可对三类传感器帧、CRC、频率、时间戳和数据合理性做持续验收：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_f407_uart.ps1 -PortName COM3 -Seconds 60
```

当前应用态协议帧主链路走 UART4 到 MP157，同时镜像到当前 F407 USB-UART/USART1 便于本机诊断；板间验证在 MP157 上通过 `/dev/ttySTM1` 执行。Windows COM 号会随 USB 重新枚举变化，示例中的 COM3 是 2026-07-21 当前值，执行前应先用设备管理器或 `Get-PnpDevice -Class Ports -PresentOnly` 核对。

MP157 发送最小下行 ping 并等待 F407 ack：

```bash
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5 --mcu-command ping
```

2026-07-18 已在 MP157 COM9 上执行当前 ARM Runtime 完成验证：`runtime/runtime_status.json` 中 `mcu.command_ack_seen=true`、`mcu.command_ack_request_type=128`、`mcu.command_ack_status=0`、`mcu.command_ack_nonce=1346981447`；同时 heartbeat、IMU、磁力计和气压计状态均为 seen。

需要通过 MP157 串口 console 可靠上传验证包时，可使用仓库的 XMODEM-CRC 脚本；远端父目录需先创建，板端需提供 `rx`：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/send_xmodem.ps1 `
  -PortName COM9 `
  -LocalPath path/to/local-file `
  -RemotePath /tmp/target-directory/remote-file
```

完整部署当前 ARM Runtime、配置、长测脚本和 ICM20608 systemd unit 时，使用仓库级幂等部署脚本：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts/deploy_mp157_runtime.ps1 `
  -PortName COM9
```

脚本默认安装到 `/opt/outdoor-agent`，删除已废弃的早期 modules-load.d 配置，enable/restart `outdoor-agent-icm20608.service`，并以板端 SHA256、shell 语法、服务 active/enabled 和 `/dev/icm20608` 作为成功门槛。2026-07-19 已在当前真实 MP157 上完成一次完整幂等部署，最终输出 `deployment=PASSED`。

如果 Bootloader 因读保护返回 NACK，可在确认会擦除用户 Flash 后追加 `-ReadoutUnprotectFirst`。

## 目录结构

```text
ai-outdoor-agent-platform/
├── README.md
├── AGENTS.md
├── common/
│   └── protocol/
├── docs/
├── f407/
│   └── sensor-hub/
│       └── firmware/
│           └── stm32cube/
├── mp157/
│   └── outdoor-core-service/
├── scripts/
└── tools/
```

详细说明见 [docs/repo_structure.md](docs/repo_structure.md)。

## 设计文档

- [仓库结构说明](docs/repo_structure.md)
- [项目设计文档](docs/project_design.md)
- [问题排查复盘表](docs/troubleshooting_log.md)
- [Stage 0 计划](docs/stage0_plan.md)
- [Stage 1 计划](docs/stage1_plan.md)
- [Stage 2 计划](docs/stage2_plan.md)
- [MCU 协议文档](docs/mcu_protocol.md)
- [Stage 1 Bring-up 计划](docs/stage1_bringup_plan.md)
- [ADR-0009: MP157 板载 ICM20608 优先验证](docs/adr/0009-use-mp157-icm20608-iio-first.md)
- [ADR-0010: UART5 NMEA 输入与文本仪表盘原型](docs/adr/0010-use-nmea-uart5-text-dashboard.md)
- [ADR-0012: MP157 SD 卡文件持久化](docs/adr/0012-use-sd-card-file-storage.md)
- [ADR-0016: ICM42688 FIFO 与中断 UART TX 队列](docs/adr/0016-use-icm42688-fifo-and-interrupt-uart-tx-queues.md)
- [ADR-0017: MP157 协作式 Runtime Service 调度](docs/adr/0017-use-cooperative-runtime-service-scheduler.md)
- [ADR-0018: CSV 历史记录与按大小日志轮转](docs/adr/0018-use-csv-history-and-size-based-log-rotation.md)
- [ADR-0021: 限制 MMC5603 轮询并使用 ICM42688 FIFO record-count](docs/adr/0021-bound-mmc5603-polling-and-use-fifo-record-count.md)
- [ADR-0022: 在 MP157 计算可标定的倾斜补偿罗盘航向](docs/adr/0022-compute-calibrated-compass-heading-on-mp157.md)
- [ADR-0023: 为 Sensor Hub Diagnostics 扩展增加版本](docs/adr/0023-version-sensor-hub-diagnostics-extension.md)
- [ADR-0024: 离线拟合完整矩阵罗盘校准参数](docs/adr/0024-fit-compass-calibration-offline.md)
- [ADR-0025: 使用 Unix Domain Socket 提供 Runtime 状态查询](docs/adr/0025-use-unix-domain-socket-status-query.md)
- [ADR-0026: 使用 systemd 托管 Outdoor Core Runtime](docs/adr/0026-use-systemd-runtime-supervision.md)

## 开源协议

当前尚未添加 LICENSE 文件。后续计划补充开源协议。
