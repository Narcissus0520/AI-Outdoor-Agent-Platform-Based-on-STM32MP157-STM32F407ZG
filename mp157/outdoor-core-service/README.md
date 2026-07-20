# Outdoor Core Service

`outdoor-core-service` 是 AI Outdoor Agent Platform 的 STM32MP157 Linux 主控侧 Runtime 服务。该模块独立使用 CMake 编译，当前包含 Stage 0 Linux Outdoor Core Runtime 能力、Stage 1 MCU/Sensor Integration，以及 Stage 2 本地只读 IPC、systemd 进程托管、Local Agent mock 软件边界和状态恢复型板端验收入口。

本模块不包含 STM32F407ZG 固件代码，不实现 HTTP API 或真实 AI Agent；当前提供 `outdoor-agent` 文本和 fbdev framebuffer 仪表盘 APP 原型。

## 当前能力

- C++17 / CMake Linux Runtime 工程
- 日志、配置和 Runtime Service 生命周期；服务通过单线程协作式 `poll()` 调度并行推进，避免共享状态的数据竞争
- 常驻运行期间每秒更新 `runtime_status.json`，支持 SIGINT/SIGTERM 和 `--runtime-run-seconds` 受控停止
- MP157 SD 卡持久化存储适配：启用后保存状态 JSON、文本仪表盘、有限容量轮转日志和可选传感器 CSV 历史
- NMEA 文件回放与 RMC/GGA/VTG/GSA/GSV Parser
- MP157 UART5 GNSS/NMEA serial 输入路径，已验证 `/dev/ttySTM2`、38400 8N1
- GNSS 基础定位状态输出
- 文件型 Runtime 状态输出：`runtime/runtime_status.json`
- 默认关闭的 Unix domain stream socket 状态查询：复用状态文件 JSON schema，提供 `GET_STATUS`/`PING` 最小协议和 `outdoor_status_query` 客户端
- 默认只安装不启动的 `outdoor-agent-runtime.service`，包含 ICM20608 loader 依赖、SIGTERM、失败重启上限、运行目录、沙箱和设备白名单
- schema v1 的有界 Local Agent 请求/响应、可替换 backend 和 `outdoor_agent_terminal`；当前只提供明确不执行 AI inference 的确定性 mock
- 显式 `--confirm` 的 Stage 2 板端验收脚本：保存/恢复 Runtime active/enabled 状态，覆盖 socket、mock terminal、SIGTERM 和失败自动恢复
- `outdoor-agent` 仪表盘 APP 原型：`runtime/dashboard.txt` 文本输出、可选 `/dev/fb0` framebuffer 输出、主界面程序图标、轻量 fbdev/evdev APP launcher，以及板端 APP 配置/启动脚本
- MCU mock frame 解析
- `McuFrame` / `McuFrameParser` / `McuStatus`
- CRC16/MODBUS 校验
- Heartbeat、mock sensor 和 Mock IMU 帧解析
- Sensor Hub diagnostics 解析与独立状态节点，兼容 legacy 44 字节 schema 0，并解析当前 48 字节 schema 1 的 I2C/FIFO 累计计数、最近失败上下文和 UART4 RX 累计丢弃字节数
- `runtime_status.json` 输出独立 `imu` 字段
- `runtime_status.json` 输出独立 `magnetometer` 字段，解析 F407 MMC5603 三轴磁场帧
- `runtime_status.json` 输出独立 `compass` 字段，支持硬铁偏置、3×3 软铁/安装矩阵、倾斜补偿、磁偏角和质量门槛
- `runtime_status.json` 输出独立 `barometer` 字段，解析 F407 BMP390 气压和温度帧
- F407 live serial 输入路径，默认按 MP157 USART3 `/dev/ttySTM1` 读取 115200 8N1 二进制 MCU 帧；2026-07-18 已验证 heartbeat、IMU、磁力计和气压计真实帧
- 最小 MP157 -> F407 `command_ping -> command_ack` 软件路径，serial 模式可通过 `--mcu-command ping` 发送命令；当前 ARM Runtime 已完成真实双向验收
- MP157 板载 ICM20608 字符设备读取服务，输出独立 `board_imu` 字段
- ICM20608 IIO sysfs reader 保留为后续可选来源

## 目录结构

```text
outdoor-core-service/
├── CMakeLists.txt
├── README.md
├── config/
├── data/
├── deploy/
├── include/
├── scripts/
├── src/
└── tests/
```

`deploy/systemd/outdoor-agent-icm20608.service` 用于在板厂 `link-modules.service` 完成内核模块目录链接后自动加载 `icm20608`，真实软重启已验证 `/dev/icm20608` 自动出现。`scripts/run_board_health_preflight.sh` 默认使用 `full` profile，强制要求 F407 `status_flags=425 (0x01A9)`、diagnostics schema 1、UART4 RX 丢弃计数为零、四类 MCU 数据、ping ACK、GNSS NMEA、板载 IMU、日志轮转/写入失败计数为零和核心错误字段全部通过。第五个参数可显式设为 `icm42688_only`，仅供带 heartbeat `0x8000` 标识的非生产隔离镜像使用：要求磁力计/气压计 absent、状态 `0x8181`，并要求 I2C/FIFO/init/drop 累计值全为零。`run_board_long_validation.sh` 仍只调用默认 `full`，不会被隔离 profile 放宽。通过后还会启动 `monitor_board_runtime_health.sh`，逐秒保存状态位、核心错误、诊断版本和 UART4 RX 累计丢弃计数。执行前应校验脚本和 Runtime 二进制的 SHA256，且不得并发运行多个 Runtime。

当前真实板验证使用持久化目录 `/opt/outdoor-agent`：

```text
/opt/outdoor-agent/
├── bin/
│   ├── outdoor_core_runtime
│   ├── outdoor_status_query
│   └── outdoor_agent_terminal
├── config/
│   ├── runtime.conf
│   └── outdoor_agent_service.conf
├── scripts/
│   ├── run_board_long_validation.sh
│   ├── run_board_health_preflight.sh
│   ├── monitor_board_runtime_health.sh
│   ├── audit_board_long_validation.sh
│   ├── run_board_crash_recovery_validation.sh
│   ├── run_board_gnss_fix_validation.sh
│   └── run_stage2_board_acceptance.sh
└── tests/
    └── unix_status_service_tests
```

前六个 Stage 1 验证脚本默认使用上述 Runtime 与配置路径，采集结果写到 `/run/media/mmcblk1p1` 下的唯一目录。健康预检、逐秒时间线和正式验收相互关联但保留独立产物；`/tmp` 仅保存当前测试 PID/root/monitor 指针，不再保存可执行文件和配置。Stage 2 验收脚本独立保存 `/tmp/outdoor-agent-stage2-acceptance-*` 证据，不启动第二个传感器 Runtime。

室外 GNSS fix 验收可执行：

```bash
/opt/outdoor-agent/scripts/run_board_gnss_fix_validation.sh 600
```

脚本限时等待 `fix_valid=true`，并要求 GNSS CSV 同时出现有效 RMC 非零坐标和有效 GGA 卫星数/质量；室内零卫星环境会明确返回 1，不把“收到 NMEA”误判为“定位成功”。

在仓库根目录可通过 COM9 一次完成目录创建、XMODEM 上传、权限、systemd 安装和哈希/语法验证：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts/deploy_mp157_runtime.ps1 `
  -PortName COM9
```

部署会上传 Runtime、查询客户端、Local Agent terminal、Unix socket self-test、Stage 1/2 验收脚本、普通验证配置、headless systemd 配置和两个 unit，并运行本地主机 supervision/acceptance verifier。默认仅安装 Runtime unit，不 enable、不 start；ICM20608 loader 保持既有安装/启动行为。后续明确进入板端联调时，可附加：

```powershell
-EnableRuntimeService -StartRuntimeService
```

`-StartRuntimeService` 会隐式 enable，并在 10 秒有界窗口内用 `outdoor_status_query` 检查 socket。正式小时长测前必须停止 `outdoor-agent-runtime.service`；现有预检/长测脚本也会拒绝竞争的 `outdoor_core_runtime` 进程。

完整 Stage 2 生命周期验收必须由用户在板端联调窗口显式执行：

```bash
/opt/outdoor-agent/scripts/run_stage2_board_acceptance.sh --confirm
```

它会运行临时 socket self-test，检查 Runtime 目录/socket 权限、连续三次查询、Agent mock 无/有 context、SIGTERM 停止与 socket 删除、再次启动和单次 SIGKILL 自动恢复；退出或中断时恢复运行前 active/enabled 状态。没有 `--confirm` 不会进入 systemd 操作。该脚本不执行烧录、整板重启或物理上下电。2026-07-21 真实 MP157 验收全部通过，最终恢复 inactive/disabled 且 socket 不存在；报告保存在 `/tmp/outdoor-agent-stage2-acceptance-IIppv5/`。

### systemd Runtime 托管

`deploy/systemd/outdoor-agent-runtime.service` 使用：

- `Type=simple`，直接监管前台 Runtime，不使用 PID 文件或二次 fork
- `Requires/After=outdoor-agent-icm20608.service`
- `RuntimeDirectory=outdoor-agent`，0750 的 `/run/outdoor-agent`
- `Restart=on-failure`、2 秒间隔、60 秒内最多 5 次启动
- SIGTERM 与 15 秒停止超时
- `ProtectSystem=strict`、`NoNewPrivileges`、`AF_UNIX` 限制和 closed device allow-list

配套 `config/outdoor_agent_service.conf` 持续读取 `/dev/ttySTM2`、`/dev/ttySTM1` 和 `/dev/icm20608`，将 JSON/socket 写入 `/run/outdoor-agent`，默认关闭 framebuffer、launcher、SD storage 与 history。当前明确使用 root；真实回读的两路 tty 为 root:dialout 0660，而 ICM20608 为 root:root 0600，尚不能直接切换统一非 root 用户。unit 沙箱只允许两路串口和 ICM20608。主机静态验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_runtime_supervision_tests.ps1
```

静态/无硬件 smoke 与真实 MP157 的 `systemd-analyze verify`、enable/disable、start/stop/restart、SIGTERM 和单次 SIGKILL 自动恢复均已通过；板端验收最终恢复 inactive/disabled。

## 编译

在本目录执行：

```bash
cmake -S . -B build
cmake --build build
```

构建会生成 `outdoor_core_runtime`、`outdoor_status_query` 和 `outdoor_agent_terminal`；后两者分别是无第三方依赖的状态查询客户端和 Local Agent mock 终端。

也可以在仓库根目录执行：

```bash
./scripts/build_mp157.sh
```

面向 MP157 板端部署时，需要使用 ARMv7 hard-float Linux 交叉编译器。2026-06-19 上板验证使用 Windows host `arm-none-linux-gnueabihf-g++ 9.2.1` 编译 `outdoor_core_runtime`；正点原子资料目录中的 `gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz` 和 OpenSTLinux SDK 脚本面向 Linux host，不能直接在当前 Windows PowerShell 环境原生运行。

## 运行

Linux / macOS:

```bash
./build/outdoor_core_runtime --config config/runtime.conf
```

Windows CMake 默认 Visual Studio 生成器:

```powershell
.\build\Debug\outdoor_core_runtime.exe --config config\runtime.conf
```

默认 file GNSS 模式会解析 `data/nmea_sample.txt`，并输出：

```text
runtime/runtime_status.json
runtime/dashboard.txt
```

协作式调度支持持续采集和常驻 dashboard 同时运行：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --gnss-input-mode serial --gnss-serial-capture-seconds 0 \
  --mcu-input-mode serial --mcu-serial-capture-seconds 0 \
  --dashboard-refresh-count 0 --runtime-run-seconds 3600
```

其中 `gnss_serial_capture_seconds = 0`、`mcu_serial_capture_seconds = 0`、`board_imu_sample_count = 0` 和 `dashboard_refresh_count = 0` 分别表示对应服务持续运行；`runtime_run_seconds = 0` 表示不设置 Runtime 总时长上限。运行中的状态文件增加 `active_service_count` 和 `completed_service_count`，可区分仍在调度的服务与已经完成的一次性服务。

罗盘解算由 `navigation/compass_estimator` 完成，机身坐标约定为 `+X` 向前、`+Y` 向右、`+Z` 向上。默认配置只输出标记为 `quality=uncalibrated` 的估计：

```text
compass_enabled = true
compass_calibration_valid = false
compass_declination_degrees = 0.0
compass_hard_iron_x_ut = 0.0
compass_hard_iron_y_ut = 0.0
compass_hard_iron_z_ut = 0.0
compass_soft_iron_00 = 1.0
compass_soft_iron_11 = 1.0
compass_soft_iron_22 = 1.0
```

完整配置还包含其余矩阵项、磁场/加速度范围和最大样本时差。只有真实整机完成多姿态采集、拟合并验证后，才应设置 `compass_calibration_valid=true`。仓库级 `tools/compass_calibrator` 可直接读取 History Recorder 的 `magnetometer.csv`，输出硬铁偏置、完整 3×3 软铁矩阵、覆盖度和残差报告；可选安装旋转必须来自独立测量，候选文件强制保持 calibration false。`data/mcu_mock_frames_compass.txt` 仅用于确定性软件集成验证，不是现场标定数据。罗盘 fixture 当时使用的 254152 B ARM Runtime SHA256 为 `8f4c1423...10d5`；板端 JSON/dashboard 均得到 heading `30.303°`、field `57.711 uT` 和 `quality=uncalibrated`。当前部署版本已继续包含 Logger 健康和 diagnostics schema 1 扩展，不能用 fixture 结果替代真实整机标定。

MP157 插入并挂载 SD 卡后，可以把 Runtime 输出切换到 SD 卡目录。默认配置不强制启用 SD 卡，避免 PC 开发和自动化测试依赖板端挂载状态：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --storage-root /run/media/mmcblk1p1/outdoor-agent --history
```

启用后会自动创建：

```text
/run/media/mmcblk1p1/outdoor-agent/status/runtime_status.json
/run/media/mmcblk1p1/outdoor-agent/dashboard/dashboard.txt
/run/media/mmcblk1p1/outdoor-agent/logs/outdoor_core_runtime.log
/run/media/mmcblk1p1/outdoor-agent/data/history/gnss.csv
/run/media/mmcblk1p1/outdoor-agent/data/history/mcu_imu.csv
/run/media/mmcblk1p1/outdoor-agent/data/history/magnetometer.csv
/run/media/mmcblk1p1/outdoor-agent/data/history/barometer.csv
/run/media/mmcblk1p1/outdoor-agent/data/history/board_imu.csv
/run/media/mmcblk1p1/outdoor-agent/captures/
```

本项目当前 MP157 实测 SD 卡自动挂载为 `/run/media/mmcblk1p1`；`/mnt/sdcard` 当前不存在。若后续系统镜像改为固定挂载 `/mnt/sdcard`，只需要替换 `--storage-root` 或配置中的 `storage_root_path`。`runtime_status.json` 的 `storage` 节点会记录 history 路径、日志轮转配置、`log_rotation_failure_count`、`log_write_failure_count`、`log_last_error` 和聚合初始化/运行错误。

默认 `storage_log_max_bytes = 1048576`、`storage_log_backup_count = 3`，轮转文件后缀为 `.1` 到 `.3`；max bytes 为 0 时关闭轮转。Logger 在一个文件输出会话内累计轮转失败和写/flush 失败；轮转失败时尝试重新打开活动文件并继续追加，但失败事实保持到最终状态，不会因后续成功写入被清除。History Recorder 按 GNSS 句子计数、各 MCU 传感器 uptime 和 Board IMU sample count 去重，每秒 flush。GNSS/MCU serial service 会在每个合法句子/帧应用后触发记录，避免一次 read 内多帧覆盖；CSV 仍不是包含噪声、CRC 错误和解析失败载荷的 raw capture。

2026-07-19 真实 MP157 短时验证：60 秒双串口/Board IMU/framebuffer/SD history 联合运行成功受控退出；发现旧快照观察方式只记录约 57 Hz MCU IMU 后，按 `TRB-20260719-011` 增加逐帧 callback，修复版 20 秒 framebuffer 复验为 MCU IMU 约 98.1 Hz、磁力计约 20.1 Hz、气压计 10.0 Hz，四类 MCU 状态和 ping ACK 正常。另一次 30 秒 info 日志测试在 VFAT SD 卡生成 718 KiB 活动日志和 1.0 MiB `.1` 备份，验证按大小轮转生效。

当前部署的 ARM Runtime 为 258560 B，SHA256 `47c4bcc9f34f1d45add17e27734f2e8acda7b2e77364ba87e7d151b7092bb739`。短时 monitor 探针取得 4 行、每行 23 列、schema 1/drop 0 的时间线；随后 40960 字节受控下行压力使预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-jYMnDJ` 解码到 `uart4_rx_drop_count=2231` 并按设计拒绝，F407 应用复位后的 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-XOXgzn` 恢复 schema 1/drop 0。复位后预检仍只因 F407 `status_flags=0x102E` 失败，因此不能宣称真实 ICM42688 或 FIFO/I2C 修复已通过板端验收。

7 英寸 RGB 屏 framebuffer 仪表盘运行方式：

```bash
./outdoor_core_runtime --config config/runtime.conf --board-imu --dashboard-output-mode both --dashboard-framebuffer-device /dev/fb0 --dashboard-refresh-count 3 --dashboard-refresh-interval-ms 500
```

当前屏幕 APP 名称为 `outdoor-agent`，framebuffer 版本按深色科技风参考图组织为左侧导航、左上角程序图标、顶部状态栏、方向罗盘、大速度表、位置地图、温度/光照展示区和底部状态栏。真实数据来源仍以 GNSS、F407 Sensor Hub 和 MP157 Board IMU 为主；光照、空气质量、电池、信号等是 UI 占位/演示指标，真实 AI Agent 本地部署和交互后续实现。

板端真正作为 APP 进程启动时，使用独立配置和脚本：

```bash
./scripts/run_outdoor_agent_app.sh
```

该脚本默认执行 `./outdoor_core_runtime --config config/outdoor_agent_app.conf`。APP 配置将 `dashboard_output_mode` 设为 `both`，`dashboard_framebuffer_device` 设为 `/dev/fb0`，并使用 `dashboard_refresh_count = 0` 表示常驻刷新；同时默认开启 `launcher_enabled = true`，先显示中央 `OUTDOOR AGENT` 图标/磁贴，点击后进入 dashboard。普通 PC 开发和自动化验证仍默认使用 `config/runtime.conf` 的文本/单次刷新路径。

2026-07-05 已通过 COM3 完成 MP157 UI 上板验证：`/dev/fb0` 报告 1024x600、16 bpp；Goodix 触摸设备为 `/dev/input/touchscreen0 -> event0`；当前 ARM Runtime SHA256 为 `0fa01c5199a903b43bf60c09c4cefb11c122015233f4c72a059d3b2cd5b600c5`；`outdoor-agent` APP launcher 可显示到 `/dev/fb0`，并已验证 `--launcher-auto-start-seconds 2` 自动进入 dashboard 和真实手指点击图标启动。真实点击日志为 `Launcher icon tapped at x=530, y=292`，随后成功输出 `Dashboard rendered to framebuffer: /dev/fb0`。

UBLOX-M10 UART5 软件路径运行方式：

```bash
./outdoor_core_runtime --config config/runtime.conf --gnss-input-mode serial --gnss-serial-device /dev/ttySTM2 --gnss-serial-baud 38400 --gnss-serial-capture-seconds 5
```

该路径已完成上板通信验证：M10 持续输出 checksum 正确的 NMEA；当前环境无卫星信号，因此 `GGA fix=0`、`RMC=V`。

MP157 板载 ICM20608 上板验证:

```bash
modprobe icm20608
./outdoor_core_runtime --config config/runtime.conf --board-imu --board-imu-source char_device --board-imu-device-path /dev/icm20608 --board-imu-samples 5
```

已验证结果：`runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`、`source=icm20608_char`、`sample_count=5`、`last_error=""`，静置样例中 Z 轴约 `0.983g`。

F407 -> MP157 live serial 软件路径已实现，默认目标是 MP157 `USART3_RX` 对应的 `/dev/ttySTM1`。当前已按如下接线完成上板验证：

```text
F407 PC10 (UART4_TX) -> MP157 PD9 (USART3_RX, /dev/ttySTM1)
F407 PC11 (UART4_RX) <- MP157 PD8 (USART3_TX)
F407 GND              - MP157 GND
```

运行方式：

```bash
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5
```

2026-06-25 MMC5603 验证结果：F407 以约 20 Hz 输出 `sensor_magnetometer`，平均磁场强度约 `47.24 μT`。MP157 parser 会输出 `mcu.magnetometer_seen` 和独立 `magnetometer` 节点。该次验证中 ICM42688 仍为 Mock fallback；2026-07-18 当前接线已确认 ICM42688 和 MMC5603 同时 ready。

BMP390 软件路径会解析 `sensor_barometer`，并输出 `mcu.barometer_seen` 以及独立 `barometer.pressure_pa`、`barometer.temperature_celsius`。2026-07-18 真实 F407/BMP390 上板验证已确认 `barometer_seen=true`。

MP157 -> F407 最小下行 ping 命令：

```bash
./outdoor_core_runtime --config config/runtime.conf --mcu-input-mode serial --mcu-serial-device /dev/ttySTM1 --mcu-serial-baud 115200 --mcu-serial-capture-seconds 5 --mcu-command ping
```

F407 通过 UART4 RX 中断缓冲解析 `command_ping`，再通过 UART4 TX 返回 `command_ack`。2026-07-18 已在 MP157 COM9 上运行当前 ARM Runtime 完成验证：`mcu.command_ack_seen=true`、`mcu.command_ack_request_type=128`、`mcu.command_ack_status=0`、nonce `0x50494E47`；同次状态文件还确认 heartbeat、IMU、磁力计和气压计均已收到。

### Unix 状态查询

socket 默认关闭，避免改变现有 file/mock 有限服务运行的退出语义。Linux/MP157 上可按需启用：

```bash
./outdoor_core_runtime --config config/runtime.conf \
  --status-socket --status-socket-path runtime/outdoor_core.sock \
  --runtime-run-seconds 60
./outdoor_status_query runtime/outdoor_core.sock
```

`GET_STATUS\n` 返回与状态文件完全相同的当前 JSON，`PING\n` 返回 `PONG\n`。服务使用非阻塞协作式轮询、0660 权限、最多 4 个客户端、64 字节请求和 5 秒空闲超时；启动不会覆盖普通文件或活跃 socket，只会清理确认失活的 stale socket。启用后 Runtime 包含常驻服务，需要 SIGINT/SIGTERM 或 `runtime_run_seconds` 停止。

Windows 原生构建会明确报告 Unix socket 不受支持，便于覆盖错误边界。当前全模块 Windows CTest 为 14/14，Runtime verifier 和 GNU ARM Linux 9.2.1 全目标交叉链接通过；真实 MP157 的 socket 权限、连续查询、stale/active 冲突、SIGTERM 清理和失败恢复后查询也已通过。

### Local Agent mock 终端

当前 Agent 层只固定软件边界，不包含真实模型：

- `AgentRequest` schema v1：request ID、单行 prompt、可选 Runtime JSON context
- `AgentResponse` schema v1：completed/rejected/failed、backend、context 标记、message、稳定 error
- `IAgentBackend`：未来本地或远程实现的替换点
- `MockAgentBackend`：名为 `mock_no_inference`，固定声明没有执行 AI inference
- `LocalAgentService`：同步单请求校验、backend exception containment 和响应上限

无 Runtime context 的确定性 smoke：

```bash
./outdoor_agent_terminal --prompt "status" --request-id demo-1 --format json
```

显式读取 Stage 2.1 只读状态 socket：

```bash
./outdoor_agent_terminal --prompt "status" --request-id demo-2 \
  --status-socket /run/outdoor-agent/outdoor_core.sock --format text
```

ID 最多 64 B、prompt 512 B、Runtime context 64 KiB、成功 message 2 KiB、error 256 B。当前不提供网络监听、写控制命令、对话历史、并发 daemon、deadline/cancellation、语音/触摸交互或真实模型推理。引入第三方 backend 前必须单独验证 ARM 交叉编译、许可证、RAM/CPU/存储、温升、离线更新与故障隔离。

## 验证

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_runtime.ps1
```

验证脚本会检查：

- NMEA 回放和 GNSS fix 输出
- RMC/GGA/VTG/GSA/GSV 解析
- `runtime_status.json` 中的 `gnss` 字段
- `outdoor-agent` 文本仪表盘 `runtime/dashboard.txt`
- 文本 dashboard 输出 `app_icon_visible: true`，用于覆盖主界面程序图标显示检查
- `config/outdoor_agent_app.conf` APP 配置可在覆盖为文本/单次刷新后正常运行
- APP launcher 配置项可加载，PC 文本验证路径不会访问 `/dev/fb0` 或 evdev
- Runtime Manager 协作式交错调度、服务完成/失败、停止条件和循环回调
- 无限 dashboard 在 `--runtime-run-seconds 1` 下与有限输入服务共同运行并正常停止
- NMEA checksum 错误拒绝
- MCU heartbeat mock 帧
- MCU mock sensor 帧
- MCU Mock IMU 帧和 `imu` 状态字段
- MCU byte-stream decoder 对前导噪声、分片帧和连续帧的处理
- 默认 `board_imu` 状态字段
- fake character-device / fake-IIO 的 ICM20608 换算测试
- MCU CRC16 错误拒绝
- `runtime_status.json` 状态输出
- 文件状态与内存序列化内容一致，且包含 `ipc` 节点
- `GET_STATUS`、`PING`、未知命令、缺失 provider 和非 POSIX 失败边界
- POSIX 测试源码覆盖 stale socket、活跃实例冲突、真实 client/server 查询和退出 unlink
- systemd unit/config 正向静态检查，以及 restart disabled、unbounded restart、开放设备策略、socket disabled 四个负向 fixture
- `outdoor_agent_service.conf` 经 Runtime 实际加载，并在 CLI 覆盖为 file/mock、关闭硬件后完成 stopped 状态 smoke
- Local Agent 合法/拒绝/backend 失败与异常/输出超限/JSON 转义测试，以及 terminal help/mock smoke
- Stage 2 验收脚本静态契约正向测试，以及确认门槛、恢复 trap、SIGKILL、重复查询、禁止上下电和 self-test 部署六个负向 fixture
- storage 模式下的状态 JSON、文本 dashboard 和日志文件生成
- history 模式下五类 CSV 的创建、数据追加和状态字段
- History Recorder 去重、CSV 转义，以及日志轮转顺序和备份数量上限
- 日志轮转失败故障注入、恢复后继续写入、累计健康状态，以及非零状态 JSON 转义

## 配置

默认配置文件：

```text
config/runtime.conf
```

当前字段：

```text
nmea_input_path = data/nmea_sample.txt
gnss_input_mode = file
gnss_serial_device = /dev/ttySTM2
gnss_serial_baud = 38400
gnss_serial_capture_seconds = 5
mcu_input_mode = mock_file
mcu_mock_input_path = data/mcu_mock_frames.txt
mcu_serial_device = /dev/ttySTM1
mcu_serial_baud = 115200
mcu_serial_capture_seconds = 5
mcu_command = none
runtime_run_seconds = 0
status_output_path = runtime/runtime_status.json
status_socket_enabled = false
status_socket_path = runtime/outdoor_core.sock
storage_enabled = false
storage_root_path = /run/media/mmcblk1p1/outdoor-agent
storage_status_output_path = status/runtime_status.json
storage_dashboard_output_path = dashboard/dashboard.txt
storage_log_file_path = logs/outdoor_core_runtime.log
storage_log_max_bytes = 1048576
storage_log_backup_count = 3
history_enabled = false
history_output_path = data/history
history_flush_interval_ms = 1000
board_imu_enabled = false
board_imu_source = char_device
board_imu_device_path = /dev/icm20608
board_imu_iio_path = /sys/bus/iio/devices/iio:device0
board_imu_sample_count = 5
board_imu_sample_interval_ms = 100
dashboard_enabled = true
dashboard_output_path = runtime/dashboard.txt
dashboard_output_mode = text
dashboard_framebuffer_device = /dev/fb0
dashboard_refresh_count = 1
dashboard_refresh_interval_ms = 1000
launcher_enabled = false
launcher_input_device = /dev/input/touchscreen0
launcher_auto_start_seconds = 0
log_level = info
```

板端 APP 配置文件：
```text
config/outdoor_agent_app.conf
```

其中 `dashboard_refresh_count = 0` 表示 dashboard 服务常驻刷新，适合屏幕 APP 进程；`launcher_enabled = true` 表示先进入 APP launcher，点击图标/磁贴后启动 dashboard；`launcher_auto_start_seconds` 只用于无人值守板端验证。自动化验证会通过命令行覆盖为 `--dashboard-output-mode text --dashboard-refresh-count 1` 或设置短自动启动超时。
