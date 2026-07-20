# Repository Structure

本仓库采用 Monorepo 结构，但 MP157 Linux 程序和 STM32F407ZG 固件分目录独立管理、独立编译。

## `mp157/`

`mp157/` 用于 Linux 主控侧程序。

当前模块：

```text
mp157/outdoor-core-service/
```

该目录包含 Outdoor Core Runtime 的独立 CMake 工程，内部保留：

- `include/`
- `src/`
- `config/`
- `data/`
- `tests/`
- `CMakeLists.txt`

当前 MP157 Runtime 已包含 `include/sensors` 和 `src/sensors`，用于主控侧板载传感器读取。当前实现为 ICM20608 字符设备 reader 和 IIO sysfs reader；运行时服务位于 `include/services/icm20608_service.h` 和 `src/services/icm20608_service.cpp`，默认关闭，上板时通过配置或 `--board-imu` 启用。真实 MP157 板上已通过 `/dev/icm20608` 字符设备完成 Runtime 读取验证。

当前 MP157 Runtime 包含 `GnssReplayService`、`GnssSerialService`、`NmeaParser` 和 `GnssStatus`，默认从 NMEA 样例文件回放，也可通过 `gnss_input_mode = serial` 从 MP157 UART5 `/dev/ttySTM2`、38400 8N1 读取 UBLOX-M10 NMEA。该通信路径已上板验证，当前环境无有效卫星 fix。

当前 MP157 Runtime 也包含 `DashboardService`，默认生成 `runtime/dashboard.txt` 文本仪表盘，并可通过 `dashboard_output_mode = framebuffer | both` 写入 `/dev/fb0` 显示 `outdoor-agent` 7 英寸 RGB 屏仪表盘；`config/outdoor_agent_app.conf` 和 `scripts/run_outdoor_agent_app.sh` 提供板端 APP 进程入口，`dashboard_refresh_count = 0` 表示常驻刷新。当前已加入最小 fbdev/evdev APP launcher，支持从图标/磁贴进入 dashboard，但仍不是完整 GUI、窗口系统或通用触摸控件框架。

`mp157/outdoor-core-service/scripts/run_board_health_preflight.sh` 在正式验收前执行 8 秒真实硬件健康检查并保存独立报告；`run_board_long_validation.sh` 只有通过 F407 `0x01A9`、diagnostics schema 1、UART4 RX drop 0、四类 MCU 数据、ping ACK、GNSS NMEA、板载 IMU、Logger 轮转/写入失败计数为零和核心错误门槛后，才统一启动 GNSS/MCU 双串口、板载 IMU、framebuffer、SD history 和日志轮转，并由 `monitor_board_runtime_health.sh` 逐秒保存状态位、核心 error、I2C/FIFO 累计计数、最近失败上下文、诊断版本和 UART4 RX 累计丢弃计数。长测保存 Runtime/monitor PID、Runtime SHA256、预检目录、健康时间线、SD 使用量与独立证据目录；`audit_board_long_validation.sh` 固化结束状态、诊断计数单调性/增量、diagnostics schema、UART4 RX drop、Logger 健康、时间线、CSV 和日志审计，`run_board_crash_recovery_validation.sh` 固化预检后的 SIGKILL 与同目录恢复验证，`run_board_gnss_fix_validation.sh` 固化室外有效定位门槛。这些脚本以 `/proc/<pid>/cmdline` 或 PID 元数据避免竞争进程污染证据。`mp157/outdoor-core-service/deploy/systemd/outdoor-agent-icm20608.service` 安装到板端 `/etc/systemd/system/`，在 `link-modules.service` 之后加载 ICM20608。

板端验证文件持久化安装到 `/opt/outdoor-agent/`，分别使用 `bin/`、`config/` 和 `scripts/`；Runtime 产出的状态、dashboard、日志和 CSV 证据继续保存到 SD 卡独立目录，不与 eMMC 程序文件混放。

当前 MP157 Runtime 支持 SD 卡文件/目录型 storage。默认关闭，当前板端实测启用 `--storage-root /run/media/mmcblk1p1/outdoor-agent` 后会在 SD 卡挂载目录下创建 `status/`、`dashboard/`、`logs/`、`data/` 和 `captures/`。`include/storage/history_recorder.h` 与 `src/storage/history_recorder.cpp` 在 `data/history/` 分别追加 GNSS、MCU IMU、磁力计、气压计和 Board IMU CSV；GNSS/MCU serial 在每个合法更新后触发记录，Logger 支持固定大小和有限备份数轮转。主机/ARM 构建及真实 MP157 60/20/30 秒短时 history/频率/轮转复验已通过，小时级与掉电写入仍待验证。`/mnt/sdcard` 当前不存在，如需该路径应在系统层增加挂载规则或软链接。

当前 MP157 Runtime 也包含 `McuSerialService`、`McuFrameStreamDecoder` 和 `McuCommand`，用于从 `/dev/ttySTM1` 读取 F407 UART4 输出的 MCU 二进制帧，并通过同一串口发送最小 `command_ping` 下行命令；2026-07-19 已使用最终 FIFO/TX 队列固件复验 heartbeat/IMU/magnetometer/barometer 均 seen、`status_flags=0x01A9` 和 command ACK status 0。

Runtime 服务生命周期已从阻塞式 `run()` 演进为单线程协作式 `poll()`。`RuntimeManager` 交错推进 GNSS、MCU、板载 IMU、dashboard 和 launcher，周期发布 active/completed service 计数，并支持 SIGINT/SIGTERM 与 `runtime_run_seconds` 受控退出；`tests/runtime_manager_tests.cpp` 覆盖交错、完成、失败、停止和启动回滚。本机与 ARM 交叉构建已验证，真实 MP157 多设备小时级协作运行仍待执行。

Stage 2.1 在保留原子 JSON 文件发布的基础上新增 `UnixStatusService` 和 `UnixStatusClient`。默认关闭的 `AF_UNIX/SOCK_STREAM` 服务以非阻塞方式接入同一协作式调度，`GET_STATUS` 返回与文件完全相同的 JSON，`PING` 用于存活探测；配套 `outdoor_status_query` 避免依赖板端 `nc -U` 或 `socat`。Windows 11/11 CTest、Runtime verifier 和 GNU ARM Linux 9.2.1 全目标交叉构建已通过，真实 MP157 socket 生命周期验收仍待执行。

Stage 2.2 新增 `deploy/systemd/outdoor-agent-runtime.service` 和 `config/outdoor_agent_service.conf`。unit 以 `Type=simple` 托管 headless live-source Runtime，依赖 ICM20608 loader，在 `/run/outdoor-agent` 发布 JSON/socket，并限制失败重启、可写目录、地址族和可访问设备。`verify_runtime_supervision*.ps1` 覆盖当前契约及四个不安全变体；部署默认只安装、不 enable/start，真实 systemd 生命周期待 MP157 后续验收。

Stage 2.3 新增 `include/agent/`、`src/agent/` 和 `outdoor_agent_terminal`。`LocalAgentService` 校验 schema v1 与固定字节上限，通过 `IAgentBackend` 调用当前唯一的 `mock_no_inference`，并序列化 completed/rejected/failed 响应；terminal 默认独立运行，只有显式 `--status-socket` 才读取只读 Runtime context。该目录没有真实模型 runtime、网络 daemon、对话历史或任务执行。

F407 当前还通过 `sensor_magnetometer` 帧上报 MMC5603 三轴磁场，MP157 Runtime 将其解析为独立 `magnetometer` 状态。`include/navigation/compass_estimator.h` 与 `src/navigation/compass_estimator.cpp` 再组合时间接近的 ICM42688 加速度，执行硬铁、3×3 软铁/安装矩阵、倾斜补偿和磁偏角修正，输出独立 `compass` 状态；默认系数只标记为 `uncalibrated`，真实板标定待完成。

F407 BMP390 通过 `sensor_barometer` 帧上报补偿后的气压和温度，MP157 Runtime 将其解析为独立 `barometer` 状态；2026-07-18 已完成真实上板验证。F407 I2C2 BSP 还会在运行期事务失败后硬复位外设、释放共享总线并重试一次，2026-07-19 已通过 60 秒三传感器持续采集。

## `docs/`

`docs/` 保存项目设计、阶段计划、协议、ADR、开发日志和问题排查复盘。其中 `troubleshooting_log.md` 为问题级证据入口，记录问题现象、复现条件、排查假设、无效/回退尝试、根因、最终方案、验证结果和面试讲述要点；`dev_log.md` 记录阶段性开发结果，`adr/` 记录重要技术方案比较和决策。

## `f407/`

`f407/` 用于 STM32F407ZG Sensor Hub 固件。

当前目录：

```text
f407/sensor-hub/
```

当前提供两类代码：

- PC mock/host-test 工程：可在本机验证 Mock IMU Provider、MCU 协议帧、定点滤波、ICM42688 FIFO parser/provider、MMC5603 有界状态检查和 UART TX 队列；未使用的 `Icm42688Driver` 占位接口已删除。
- `firmware/`：真实 F407 固件工作区，包含项目自有的 app、BSP、协议、ICM42688 record-count FIFO、MMC5603/BMP390 数据源、Mock IMU fallback 和最小 command decoder。
- `firmware/stm32cube/`：STM32CubeMX 6.17.0 基于 STM32Cube FW_F4 V1.28.3 生成的 `STM32F407ZGTx` HAL 基础工程。

```text
f407/sensor-hub/
├── CMakeLists.txt              # PC mock C++ 构建
├── include/                    # PC mock C++ 头文件
├── src/                        # PC mock C++ 实现
├── tests/                      # PC mock CTest
└── firmware/
    ├── app/                    # 项目自有固件应用层
    ├── bsp/                    # 板级适配接口
    ├── cmake/                  # GNU Arm Embedded Toolchain 配置
    ├── linker/                 # STM32F407ZG 内存布局
    ├── platform/               # 最小 newlib 系统调用
    ├── protocol/               # C 版协议实现
    ├── sensors/                # 固件传感器数据源
    ├── startup/                # Cortex-M4 启动与中断向量表
    ├── third_party/            # Bosch BMP3 Sensor API 等受控第三方源码
    └── stm32cube/              # CubeMX 生成代码、HAL/CMSIS 和 .ioc
```

`firmware/stm32cube/` 当前配置系统时钟、PF9/PF10 LED GPIO、USART1 PA9/PA10、UART4 PC10/PC11、400 kHz I2C2 PB10/PB11，以及 ICM42688 INT1 PB12 上升沿 EXTI12。`firmware/sensors/icm42688_fifo_parser_c.*` 解析 record-count 模式的完整 16 字节记录并在 FIFO-empty message 处停止，`imu_timestamp_normalizer_c.*` 维护真实/Mock 共用的 100 Hz 发布时间轴，`firmware/bsp/uart_tx_queue_c.*` 提供固定内存 TX 队列，`firmware/sensors/sensor_sample_filter_c.*` 提供定点一阶 IIR。`tests/icm42688_provider_test.cpp` 使用 fake I2C/tick 覆盖 FIFO/INT 寄存器配置、不可重放数据流失败、异常 flush 和重初始化累计诊断保留；`tests/mmc5603_provider_test.cpp` 覆盖测量后 7 ms 等待和有界状态读取；`tests/mcu_frame_builder_test.cpp` 覆盖 48 字节 diagnostics schema 1、UART4 RX 丢弃计数的小端编码和 CRC。USART1 保留为 UART Bootloader 下载口并镜像业务帧；每秒 `0x02` 诊断帧同时进入 UART4/MP157 与 USART1/COM6。UART4 RX 环满时累计丢弃字节数，两路 TX 均通过 HAL 中断队列发送。目录中保留 MDK-ARM 工程作为 CubeMX 生成基线；仓库使用 GNU Arm Embedded Toolchain 和 CMake 生成 ELF/HEX/BIN/map，不依赖 Keil 工程完成编译。

## `common/`

`common/` 用于 MP157 与 F407 共享内容。

当前目录：

```text
common/protocol/
```

当前放置 MP157 与 F407 共享的协议常量、CRC16/MODBUS，以及 IMU、磁力计和气压计 payload 定义。

## `tools/`

`tools/` 用于 PC 调试工具，例如协议帧生成器、串口调试工具或数据回放辅助工具。

当前工具：

```text
tools/frame_decoder/
tools/compass_calibrator/
```

`frame_decoder` 用于解析十六进制 MCU 协议帧，并展开 IMU、磁力计和气压计 payload 字段。

`compass_calibrator` 使用独立 CMake 工程读取 `magnetometer.csv`，无第三方依赖地拟合硬铁偏置和完整 3×3 软铁候选矩阵。它包含极端离群点/残差两阶段裁剪、八分体/方向方差/轴比/残差门槛、可选 proper sensor-to-body rotation 和始终保持 calibration false 的候选配置输出；合成测试覆盖矩阵/偏置恢复、近平面拒绝、强磁离群点和安装旋转组合。

## `scripts/`

`scripts/` 用于仓库级统一构建脚本。

当前脚本：

```text
scripts/build_mp157.sh
scripts/build_f407.ps1
scripts/flash_f407_uart.ps1
scripts/verify_f407_uart.ps1
scripts/send_xmodem.ps1
scripts/deploy_mp157_runtime.ps1
```

`build_mp157.sh` 构建 MP157 Runtime、status query 和 Agent terminal；`build_f407.ps1` 使用 GNU Arm Embedded Toolchain 和 Ninja 构建 F407 固件；`flash_f407_uart.ps1` 通过 STM32 ROM UART Bootloader 烧录并回读校验 F407 固件；`verify_f407_uart.ps1` 通过 COM6/USART1 诊断镜像验证协议帧；`send_xmodem.ps1` 通过 MP157 串口 console 和板端 `rx` 可靠上传 XMODEM-CRC 文件，并移除协议分组填充；`deploy_mp157_runtime.ps1` 在前者基础上完成 `/opt/outdoor-agent`、三个可执行文件、验证脚本、ICM20608 loader unit 和 Runtime unit/config 的幂等部署与哈希/状态验收。Runtime unit 默认不 enable/start，状态变更需显式开关。
