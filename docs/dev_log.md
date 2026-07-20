# Dev Log

## 2026-07-21 - Stage 2.4 Repeatable Board Acceptance Harness

### 本次完成

- 新增 `run_stage2_board_acceptance.sh`；只有 root 显式传入 `--confirm` 才进入 systemd 状态变更。
- 在第一次 mutation 前保存 Runtime active/enabled 状态，EXIT/INT/TERM 收尾恢复并验证初始状态；拒绝活动长测、异常初始状态和非 unit 管理的 Runtime。
- 板端步骤覆盖 `systemd-analyze verify`、隔离 Unix socket self-test、0750/0660 权限、三次查询、Agent mock 无/有 context、SIGTERM/socket 清理、重新启动和单次 SIGKILL 自动恢复。
- socket stale/active collision 通过既有 `unix_status_service_tests` 的 ARM 产物在唯一 `/tmp` 路径验证，不并行启动第二个会访问传感器的 Runtime。
- 部署清单新增 `/opt/outdoor-agent/tests/unix_status_service_tests` 与 Stage 2 验收脚本，继续默认不 enable/start Runtime，也不会自动执行验收。
- 新增主机静态 contract verifier 和六个负向 fixture，分别证明确认门槛、EXIT trap、SIGKILL、三次查询、禁止整板电源命令和 self-test 部署不能被弱化。
- ADR-0028 记录手工命令、无恢复脚本与显式确认/状态恢复方案比较，以及副作用和证据边界。

### 修改文件

- `mp157/outdoor-core-service/scripts/run_stage2_board_acceptance.sh`
- `mp157/outdoor-core-service/scripts/verify_stage2_board_acceptance.ps1`
- `mp157/outdoor-core-service/scripts/verify_stage2_board_acceptance_tests.ps1`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `scripts/deploy_mp157_runtime.ps1`
- `README.md`、`mp157/outdoor-core-service/README.md`
- `docs/project_design.md`、`docs/repo_structure.md`、`docs/stage2_plan.md`
- `docs/adr/0028-use-state-restoring-stage2-board-acceptance.md`
- `docs/changelog.md`、`docs/dev_log.md`、`docs/troubleshooting_log.md`

### 验证结果

- MP157 Windows GCC Release CTest 14/14 通过。
- GNU ARM Linux 9.2.1 全目标交叉构建通过：Runtime 273944 B、status query 23168 B、Agent terminal 40124 B、Unix socket self-test 64336 B。
- F407 GCC Release CTest 7/7 通过。
- 完整 `verify_runtime.ps1` 通过，包含 supervision verifier tests、Stage 2 acceptance 正向/六个负向 contract tests 和 Runtime smoke。
- PowerShell parser、Git for Windows `sh -n`、无 `--confirm` 返回 2 的 guard 和 `git diff --check` 通过。
- 未执行部署、板端验收脚本、COM3/COM9、systemd 状态变更、复位、烧录或物理上下电。

### 后续 TODO

- 用户进入统一联调窗口后部署 ARM 产物，并显式执行 `/opt/outdoor-agent/scripts/run_stage2_board_acceptance.sh --confirm`。
- 回传完整 `/tmp/outdoor-agent-stage2-acceptance-*` 目录；核对 PASS 报告和初始状态恢复后再关闭 Stage 2 板端项。
- Stage 1 的 ICM42688 电气/波形、小时长测、掉电、室外 GNSS fix 和罗盘实采标定继续保持未完成。

## 2026-07-21 - Stage 2.3 Local Agent Mock Boundary

### 本次完成

- 新增 schema v1 `AgentRequest`/`AgentResponse`、completed/rejected/failed 状态和稳定 error code。
- 新增可替换 `IAgentBackend` 与同步单请求 `LocalAgentService`，限制 ID、prompt、Runtime context、backend 名、成功正文和错误长度。
- 服务拒绝非法 schema/ID、空/多行/超长 prompt、超长 context，并隔离 backend 缺失、失败、异常、空响应和超长响应。
- 新增 `MockAgentBackend`，名称固定为 `mock_no_inference`，所有成功正文明确声明没有执行 AI inference。
- 新增 `outdoor_agent_terminal` JSON/text CLI；默认不依赖 Runtime，只有显式 `--status-socket` 才读取现有只读状态 context。
- 部署脚本增加 Agent terminal，不改变 Runtime unit 默认只安装、不 enable/start 的边界。
- ADR-0027 比较立即本地模型、远程 HTTP 与接口优先方案，并记录第三方依赖、ARM、RAM/CPU/存储、温升、离线、许可证、deadline/cancellation 和隔离门槛。

### 修改文件

- `mp157/outdoor-core-service/include/agent/`
- `mp157/outdoor-core-service/src/agent/`
- `mp157/outdoor-core-service/src/agent_terminal_main.cpp`
- `mp157/outdoor-core-service/include/ipc/unix_status_client.h`、`src/ipc/unix_status_client.cpp`
- `mp157/outdoor-core-service/tests/local_agent_service_tests.cpp`
- `mp157/outdoor-core-service/CMakeLists.txt`
- `scripts/deploy_mp157_runtime.ps1`
- `README.md`、`mp157/outdoor-core-service/README.md`
- `docs/project_design.md`、`docs/repo_structure.md`、`docs/stage2_plan.md`
- `docs/adr/0027-define-local-agent-backend-boundary.md`
- `docs/changelog.md`、`docs/dev_log.md`、`docs/troubleshooting_log.md`

### 验证结果

- MP157 Windows GCC Release CTest 14/14 通过，包含 Local Agent 边界、terminal help 和 mock smoke。
- GNU ARM Linux 9.2.1 完成 Runtime、status query、Agent terminal 和全部测试目标交叉链接；三项产物分别为 273944 B、23168 B、40124 B。
- 完整 `verify_runtime.ps1`、F407 GCC Release CTest 7/7、PowerShell parser 和 `git diff --check` 通过。
- 未引入第三方依赖，未实现或宣称真实 AI 推理。
- 未执行 deploy script、COM3/COM9、板端 terminal、systemd 状态变更、复位或物理上下电。

### 后续 TODO

- 后续在 MP157 验证 mock terminal 无 context 和显式 Runtime context 两条路径。
- 采集 MP157 资源/温升/交互时延门槛后，再单独选择真实本地或远程 backend 并新增 ADR。
- 真实 backend 前补充可终止 deadline、cancellation、进程隔离和 crash recovery。

## 2026-07-21 - Stage 2.2 Runtime Process Supervision

### 本次完成

- 新增 `outdoor-agent-runtime.service`，以 systemd `Type=simple` 托管前台 Runtime，并依赖已验证的 ICM20608 loader unit。
- 新增 headless live-source service config：GNSS/MCU 双串口和板载 ICM20608 常驻，JSON/socket 写入 `/run/outdoor-agent`，默认关闭 framebuffer、launcher、SD storage 与 history。
- unit 使用 SIGTERM/15 秒停止超时、`Restart=on-failure`、2 秒间隔和 60 秒 5 次启动上限；加入只读系统、kernel/control-group 保护、`AF_UNIX` 和 closed device allow-list。
- 部署脚本增加 Runtime unit/config，默认只安装不 enable/start；显式 `-EnableRuntimeService`、`-StartRuntimeService` 才改变板端 Runtime 状态。
- 新增 supervision contract verifier 和负向自测，证明禁用重启、无限重启、开放设备策略和关闭 service socket 会被拒绝。
- Runtime verifier 实际加载 service config，并用 file/mock/无硬件 CLI 覆盖完成 stopped 状态 smoke。
- ADR-0026 记录 systemd 选型、root 过渡边界、长测独占和专用用户前置证据；TRB-20260721-037 闭环负向测试的 PowerShell stderr/退出码误判。

### 修改文件

- `mp157/outdoor-core-service/deploy/systemd/outdoor-agent-runtime.service`
- `mp157/outdoor-core-service/config/outdoor_agent_service.conf`
- `mp157/outdoor-core-service/scripts/verify_runtime_supervision.ps1`
- `mp157/outdoor-core-service/scripts/verify_runtime_supervision_tests.ps1`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `scripts/deploy_mp157_runtime.ps1`
- `README.md`、`mp157/outdoor-core-service/README.md`
- `docs/project_design.md`、`docs/repo_structure.md`、`docs/stage2_plan.md`
- `docs/adr/0026-use-systemd-runtime-supervision.md`
- `docs/troubleshooting_log.md`、`docs/changelog.md`、`docs/dev_log.md`

### 验证结果

- Runtime supervision verifier 正向 contract 与四个负向 fixture 全部符合预期。
- `verify_runtime.ps1` 通过，包含 service config 实际加载与无硬件 smoke。
- MP157 Windows GCC Release CTest 11/11、GNU ARM Linux 9.2.1 全目标交叉构建通过；Runtime 273944 B、查询客户端 18984 B。
- F407 GCC Release CTest 7/7 通过；四份相关 PowerShell 脚本 parser errors 为 0，`git diff --check` 通过。
- 未执行 deploy script、COM3/COM9、systemd enable/start/stop/restart、复位或物理上下电。

### 后续 TODO

- 后续在 MP157 执行 `systemd-analyze verify` 和 enable/start/query/stop/restart/失败上限验收。
- 用真实 device ownership/group/udev 证据评估专用用户，当前不猜测板端组名。
- 进入 Stage 2.3，定义不夸大真实 AI 能力的 Local Agent mock 软件边界。

## 2026-07-21 - Stage 2.1 Unix Runtime Status Query

### 本次完成

- 保留原子 `runtime_status.json` 文件发布，并从 `StatusPublisher` 提取公共序列化入口，保证文件与查询响应使用同一 JSON schema。
- 新增默认关闭的 `UnixStatusService`：以非阻塞 `AF_UNIX/SOCK_STREAM` 接入协作式 Runtime，支持 `GET_STATUS`/`PING`，限制 4 个客户端、64 字节请求和 5 秒空闲连接。
- socket 固定权限为 0660；启动拒绝普通文件和活跃 socket，只清理确认失活的 stale socket，退出时关闭连接并删除自身路径。
- 新增 `outdoor_status_query` 配套客户端、配置/CLI 开关和部署文件清单，不引入第三方依赖。
- 新增跨平台协议/错误边界测试和 POSIX 集成测试源码；使用 `Threads::Threads` 精确声明测试专用线程依赖。
- 新增 Stage 2 计划和 ADR-0025；TRB-20260721-035/036 记录了 ARM 配置参数与 pthread 链接问题及修复证据。

### 修改文件

- `mp157/outdoor-core-service/include/ipc/`、`src/ipc/`、`src/status_query_main.cpp`
- `mp157/outdoor-core-service/CMakeLists.txt`、`src/main.cpp`、配置、状态模型和测试
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`、`scripts/deploy_mp157_runtime.ps1`
- `README.md`、`mp157/outdoor-core-service/README.md`
- `docs/project_design.md`、`docs/repo_structure.md`、`docs/stage1_plan.md`、`docs/stage2_plan.md`
- `docs/adr/0006-evaluate-unix-domain-status-query.md`、`docs/adr/0025-use-unix-domain-socket-status-query.md`
- `docs/troubleshooting_log.md`、`docs/changelog.md`、`docs/dev_log.md`

### 验证结果

- MP157 Windows GCC Release 构建与 CTest 11/11 通过；`verify_runtime.ps1` 通过。
- GNU ARM Linux 9.2.1 全目标交叉构建通过：`outdoor_core_runtime` 273944 B、`outdoor_status_query` 18984 B，包含 POSIX Unix socket 源码和测试目标链接。
- F407 GCC Release 与 MSVC Debug CTest 均为 7/7；F407 ARM 固件构建通过。
- Compass Calibrator CTest 3/3 通过；Frame Decoder 构建通过；两份修改后的 PowerShell 脚本 parser errors 为 0。
- 当前 Windows 环境没有 WSL 发行版、Docker/Podman 或 ARM 用户态模拟器，因此 POSIX 集成测试源码未在本机 Linux 用户态执行。
- 未访问 COM3/COM9，未执行部署、烧录、复位、物理上下电或真实板端运行。

### 后续 TODO

- 继续 Stage 2.2 Runtime systemd 进程托管的软件设计、unit 和静态验证。
- 后续统一在 MP157 验证 socket 权限、活动实例冲突、stale 恢复、连续查询与受控退出清理。
- Stage 1 真实硬件、电气、室外、小时级和掉电验收维持未完成。

## 2026-07-21 - F407 Host Test Checks Stay Active in Release

### 本次完成

- 在全新 GCC Release 构建树中复现 F407 主机测试的 `NDEBUG` 假绿：标准 `assert` 删除了其中的初始化和被测调用，`uart_tx_queue_tests` 最终触发数值异常。
- 新增测试专用 `test_check.h`，提供 Debug/Release 都会执行、失败时打印表达式与源码位置并返回非零的轻量检查。
- 将 7 个 F407 主机测试统一切换到 always-on 检查语义；生产固件源码和 ARM 编译选项不变。
- 完整排查过程、失败基线和验证证据记录到 TRB-20260721-033/034。

### 修改文件

- `f407/sensor-hub/tests/test_check.h`
- `f407/sensor-hub/tests/*_test.cpp`
- `f407/sensor-hub/README.md`
- `docs/troubleshooting_log.md`
- `docs/dev_log.md`

### 验证结果

- MP157 GCC Release CTest 10/10 通过；Compass Calibrator CTest 3/3 通过；Frame Decoder 构建通过。
- F407 GCC Release CTest 7/7 通过，原先由断言删除造成的未使用告警消失。
- F407 MSVC Debug CTest 7/7 通过。
- `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_f407.ps1` 通过。
- 未执行烧录、串口抓取、板端运行、复位或物理上下电；本次变更只涉及主机测试基础设施。

### 后续 TODO

- Stage 2.1 实现 Unix domain socket Runtime 状态查询，保留现有 JSON 状态文件兼容。
- 真实硬件与小时级验收继续按 Stage 1 未完成清单统一留待后续。

## 2026-07-21 - ICM42688-Only 100 kHz A/B and 400 kHz Rollback

### 本次完成

- 用户当前没有万用表，无法继续 VCC/AD0/SCL/SDA、连续性和有效上拉测量，因此把下一步收敛为“保持 ICM-only 隔离条件，仅把 I2C2 从 400 kHz 降为 100 kHz”的可回滚 A/B。
- F407 固件新增 `SENSOR_HUB_I2C2_CLOCK_HZ` CMake cache 参数，只接受 `100000` 或 `400000`；默认 400 kHz 时不注入额外编译宏，100 kHz 构建打印非默认诊断警告。
- I2C2 初始化改用编译期频率宏，默认仍回退到 `400000U`；F407 README 增加单器件 100 kHz 构建、非生产边界和完整复电要求。
- 首个代码/README 三文件补丁因 README 现有 PowerShell 构建段与预期上下文不一致被原子拒绝，所有文件未写入；定位准确段落后按代码与文档拆分补丁成功。

### 修改文件

- `f407/sensor-hub/firmware/CMakeLists.txt`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/i2c.c`
- `f407/sensor-hub/README.md`
- README、项目设计、Stage 1、ADR、Changelog、开发日志和问题排查复盘文档

### 验证结果

- F407 主机 Debug 构建和 CTest 7/7 通过。
- 默认正式 400 kHz ARM clean build 仍为 19384 B、text 19364 B、data 20 B、BSS 4408 B、SHA256 `f0addc29eec272be28aee46a063e95fd878b5681be83df0dc6e430f7687b9d1e`。
- ICM-only 400 kHz clean build 仍为 15072 B、text 15052 B、data 20 B、BSS 4252 B、SHA256 `c07e235a5890b5a1fb3c1ffc8f053848be73d0f6223fe1d66abf68344142f80a`；build.ninja 中没有频率 override。
- ICM-only 100 kHz build 为 15072 B、text 15052 B、data 20 B、BSS 4252 B、SHA256 `56b31d760f4a550e0837eaa3a7173c3f10a159b9c8c0339d63ed253a649c3e56`，build.ninja 明确包含 `SENSOR_HUB_I2C2_CLOCK_HZ=100000U`；非法值 `123456` 在 CMake 配置阶段按设计失败。
- 100 kHz 镜像首次 COM6 烧录会话读到 Bootloader `0x31`、chip ID `0x0413`，但首个写地址响应为异常 `0xF9`；该次未计为成功。第二个全新会话读到异常 chip ID `0x0493`，写入后逐字节回读在 `0x0800013D` 不一致；同样未计为成功。Windows 仍枚举 CH340/COM6 为 OK 且未发现终端占用，但连续传输异常使当前 F407 Flash 不可信，已停止盲目重试并重新打开 TRB-002。
- 用户物理拔插 COM6 USB/USB-UART 并保持终端关闭后，Windows 重新枚举正常；新的烧录会话取得 Bootloader `0x31`、chip ID `0x0413`，100 kHz 镜像全擦、写入、逐字节回读和 GO 全部通过。当前 Flash 已恢复可信，但这只是热复位部署，尚未形成 ICM A/B 结论。
- 用户保持 COM6 关闭并让 100 kHz 镜像与 ICM42688 完整断电重上电；COM9 确认无残留 Runtime 后，唯一一次 8 秒 `icm42688_only` 预检目录为 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-wB2xhs`。预检以 9 项失败拒绝：heartbeat `53766 (0xD206)`、init step 1、recovery/failure `759/164`、FIFO overflow/malformed/empty/drain/skipped `58/22/12/0/309`，最后失败为备用地址 `0x68/REG_BANK_SEL(0x76)` 单字节 write、HAL error `0x04 (ACK failure)`；30/60 秒未启动。
- 与窗口接近的 400 kHz `XMWvGC` 比较，100 kHz recovery 从 `3.980/s` 增至 `13.104/s`（`3.29×`），最终 failure 从 `0.772/s` 增至 `2.831/s`（`3.67×`），状态还从 `0x8181/init step 0` 退化为 `0xD206/init step 1`。较低的 FIFO overflow/skipped 来自真实 FIFO 工作中断和 fallback，不能解释为改善；“单纯 400 kHz 过快”假设被实板 A/B 证伪。
- 准备回滚 400 kHz 时，首次命令在打开串口前报告 `The port 'COM6' does not exist`；没有进入 Bootloader、擦除或写入，因此该失败没有改变板上完整的 100 kHz 镜像。只读枚举当时仅见 COM9，历史 COM6 为 `Present=False/Status=Unknown`。
- 用户确认 F407 USB-UART 实际已从 COM6 变为 COM3。只读检查确认 `USB-SERIAL CH340 (COM3)` 和 COM9 均为 OK，未发现实际串口终端占用。随后通过 COM3 将 15072 B、SHA256 `c07e235a...2f80a` 的 400 kHz ICM-only 基线重新全擦、写入并逐字节回读；Bootloader `0x31`、chip ID `0x0413`、GO `0x08000000` 全部成功。当前板上 Flash 已恢复为可信 400 kHz 隔离基线。

### 后续 TODO

- 保持 MMC5603/BMP390 断开和串口终端关闭；获取万用表后测量 ICM VCC/AD0、SCL/SDA 空闲电平、有效上拉、短线连续性和共地，或使用逻辑分析仪捕获 `REG_BANK_SEL/FIFO_COUNTH/FIFO_DATA/SIGNAL_PATH_RESET` 的 ACK/边沿。
- 电气条件或器件问题修正前不再启动 30/60 秒和小时级验证；修正后先在 400 kHz ICM-only 基线上完整复电并通过 8 秒零累计门槛，再进入 30/60 秒，最后才恢复正式三传感器镜像并逐颗接回。

## 2026-07-20 - Reproducible ICM42688-Only Diagnostic Profile

### 本次完成

- 用户报告只保留 ICM42688 并完整复电后，先通过 COM9 确认没有残留 Runtime，再执行 8 秒正式预检；目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-9D6RBq` 按设计失败。
- 该状态并非干净的 ICM-only 证据：heartbeat 为 `21046 (0x5236)`、ICM init step 1；diagnostics 已累计 recovery 3377、最终 failure 3264、overflow/malformed/empty/skipped 84/14/14/458，最近事务仍为 MMC5603 `0x30/STATUS1(0x18)`；同时 `barometer_seen=true`。因此不把失败归因 ICM 本体，也不假设人工接线动作已经形成可验证的电气隔离。
- 为消除正式三传感器固件对缺失从机的访问，新增默认关闭的 `SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC` CMake 选项；启用后编译期移除 MMC5603/BMP390 初始化与周期轮询，并在 heartbeat 设置 `0x8000` 诊断镜像位。
- 扩展 `run_board_health_preflight.sh` 第五个参数 `validation_profile`：默认 `full` 保持既有正式门槛；`icm42688_only` 要求磁力计/气压计 absent、状态精确为 `0x8181`，且 I2C recovery/failure、FIFO overflow/malformed/empty/drain/skipped、init step 和 UART4 RX drop 全部为零。
- ICM-only profile 脚本经 Git Bash 和板端 `/bin/sh -n` 验证，通过 XMODEM 上传、SHA256 核对后部署到 `/opt/outdoor-agent/scripts`。
- 新增协议 bit 15 和 F407 构建说明；隔离构建目录加入 `.gitignore`，避免诊断 BIN/ELF/map/CMake cache 进入提交范围。
- 将串口嵌套 here-string 复现、无效的正式固件隔离预检、新建 ARM 构建参数失败和所有回退步骤追加到 TRB-024/026/031/032。
- 用户确认诊断镜像与 ICM42688 完整断电重上电后，通过 COM9 确认没有残留 Runtime，并执行唯一一次 8 秒 `icm42688_only` profile；证据目录为 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-gsVcum`。
- 本轮 heartbeat 精确为 `33153 (0x8181)`、真实 IMU seen、init step 0、磁力计/气压计 absent，证明完整复电恢复初始化且隔离模式有效；但累计 recovery/failure 为 `660/105`，FIFO overflow/malformed/empty/drain/skipped 为 `707/126/39/0/2933`，profile 以 7 项失败拒绝，30/60 秒按门槛未启动。
- 最后事务为 ICM `0x69/SIGNAL_PATH_RESET(0x4B)` 单字节写，HAL status `3 (TIMEOUT)`、error `32 (0x20)`；其他从机软件访问已被编译期排除，因此根因边界收敛到 ICM 单器件供电/布线/上拉/模块本体和 FIFO 恢复交互，具体根因仍待电气测量或波形确认。
- 用户随后补充该轮完整复电与预检期间 COM6 实际未关闭，现已关闭。原目录和全部计数继续作为真实异常证据保留，但由于 COM6 控制线存在应用热复位风险，本轮不再算作干净完整复电验收；必须在 COM6 全程关闭下再次完整复电并重跑 8 秒门槛。
- COM6 全程关闭后再次完整复电，COM9 确认无残留 Runtime；唯一一次干净 8 秒 profile 目录为 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-XMWvGC`，仍以 7 项累计门槛失败。
- 干净轮次为 `0x8181`、init step 0、真实 IMU、其他两类传感器 absent，但 diagnostics uptime `58298 ms` 已累计 recovery/failure `232/45` 和 FIFO overflow/malformed/empty/drain/skipped `186/21/20/0/671`。最后失败为 `0x69/FIFO_DATA(0x30)` 112 B read、HAL status 1、error `0x04 (ACK failure)`；30/60 秒未启动。
- 干净与受污染轮次的 recovery/overflow/skipped 均为相同数量级，否定“COM6 未关闭是异常必要条件”；根因仍未最终确认，下一步转入 ICM 单器件电压、连续性、上拉和波形测量。

### 修改文件

- `f407/sensor-hub/firmware/CMakeLists.txt`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `mp157/outdoor-core-service/scripts/run_board_health_preflight.sh`
- `.gitignore`
- README、MCU 协议、Stage 1、设计、开发日志、Changelog 和问题排查复盘文档

### 验证结果

- F407 主机 CTest 7/7 通过。
- 默认正式 ARM clean build 保持 BIN 19384 B、text 19364 B、data 20 B、BSS 4408 B、SHA256 `f0addc29...b9d1e`，证明默认 `OFF` 没有改变正式产物。
- ICM-only ARM Release build 通过：BIN 15072 B、text 15052 B、data 20 B、BSS 4252 B、SHA256 `c07e235a5890b5a1fb3c1ffc8f053848be73d0f6223fe1d66abf68344142f80a`。
- ICM-only 镜像通过 COM6 ROM Bootloader 烧录和逐字节回读；Bootloader version `0x31`、chip ID `0x0413`。该动作只完成部署，属于热复位，尚未形成新的传感器隔离结论。
- 热复位后的 `icm42688_only` profile 烟测目录 `outdoor-agent-health-preflight-eGaj29` 正确识别 `0x8000` 诊断位和磁力计/气压计 absent；FIFO 五项为零，但 ICM init step 1、recovery/failure 898、最后 `0x68/REG_BANK_SEL` NACK，最终 `0xD006`，按设计以 5 项失败拒绝。该结果只验证模式和脚本，不替代完整复电。
- 预检脚本本机/板端语法通过，部署前后 SHA256 均为 `7363227686be09955bbc67b79a126deeeb789e03f3ef49ca2430625d3e312dbf`，权限为 0755。
- 首次新目录 CMake 命令因 toolchain/Ninja 参数边界失败；改用绝对路径参数数组和新目录后通过。首次 COM9 helper 也因嵌套 here-string 在主机解析阶段失败；改用固定标记命令后通过。两次失败均未修改板端目标，已记录而非隐藏。
- 首次读取热复位烟测又把完整结束 token 放进被回显的命令，主机提前返回；未重复启动 Runtime，随后用执行期拼接 token 读取同一目录完成闭环，复现已追加到 TRB-012。
- 完整复电后的 8 秒预检首次启动命令在主机 PowerShell 解析阶段因嵌套 `"$root/..."` 报 `Unexpected token`；串口未打开、板端未执行。改用 PowerShell 单引号原样保存远端命令后成功，复现已追加到 TRB-010。
- 首次多文件文档补丁因预期上下文漏了“Git Bash 和板端”之间的空格而原子校验失败，所有目标文件均未写入；改为按文件拆分并使用精确上下文后成功，随后 `git diff --check` 通过。
- COM6 前提补正文档的首轮 `rg` 校验把含 Markdown 反引号的模式放进 PowerShell 双引号字符串，主机报 `The string is missing the terminator`；未打开串口或执行板端命令，改用单引号搜索模式后完成校验，复发追加到 TRB-010。

### 后续 TODO

- 保持 ICM-only 诊断镜像、MMC5603/BMP390 断开和 COM6 关闭；上电测量 ICM VCC、AD0、PB10/SCL、PB11/SDA 对 GND 电压，断电测量 PB10↔SCL、PB11↔SDA、GND↔GND 连续性。
- 电气静态条件确认后，优先抓取 `FIFO_COUNTH/FIFO_DATA/SIGNAL_PATH_RESET` 事务波形；没有示波器/逻辑分析仪时，再设计单器件 100 kHz 可回滚 A/B。
- 隔离门槛最终通过后刷回默认 19384 B 正式镜像；按 MMC5603、BMP390 顺序逐颗接回，每一步完整复电并重复同条件增量审计。

## 2026-07-19 - Full-Power Runtime I2C/FIFO Failure Reproduction and Isolation Plan

### 本次完成

- 在用户确认 F407、ICM42688 和相关传感器已完整断电重上电后，执行正式 8 秒预检：`outdoor-agent-health-preflight-rjgpa6` 以 `0x01A9`、四类 F407 数据、ping ACK、diagnostics schema 1/drop 0、GNSS NMEA、Board IMU 和 Logger 门槛全部通过，确认上电初始化已恢复。
- 没有仅以最终 heartbeat 判定稳定。首个预检在 F407 uptime 104078 ms 已累计 I2C recovery 234、FIFO overflow/malformed/skipped 56/19/713；约 73 秒后，长测入口的第二次预检 `outdoor-agent-health-preflight-Hq1vCJ` 以 `0x03A9` 拒绝启动，累计增量 recovery/failure/overflow/malformed/empty/skipped 为 `+190/+2/+58/+15/+16/+531`，最近失败为 ICM42688 `0x69/FIFO_COUNTH(0x2E)` 读取。
- 对照 TDK ICM42688-P 官方 FIFO 定义核对 `FIFO_COUNT_REC`、`FIFO_WM_GT_TH`、16 字节 accel+gyro packet 和 header 位；当前 `INTF_CONFIG0=0x70`、正式 `FIFO_CONFIG1=0x27` 的 record-count 配置语义正确。
- 完成三个可回滚 A/B：I2C2 100 kHz、固定 partial-read 8 records/128 B、固定 partial-read 4 records/64 B。100 kHz 在约 22 秒已失败；两个 fixed-window 版本分别持续产生高频 recovery/malformed/empty，均未解决问题并全部撤回。
- 用一次性诊断镜像在 parse 失败时抓取 64 B FIFO 的四个 16 B 边界首字节，实板得到 `06 FF FF FF`，不符合正常 `68` 或 empty `80` 包头；证据支持“成功返回的数据流已错位/损坏”，但尚不能在没有波形和单器件隔离的情况下断言具体物理根因。
- 删除临时诊断复用，恢复 400 kHz、record-count 和精确完整记录读取；相应 provider/parser 测试恢复到正式行为。当前正式 BIN 已重新刷入并逐字节回读，板端不再运行实验镜像。
- 将完整复电结果、每个失败假设、固件哈希、预检目录、增量和后续单器件隔离步骤追加到 TRB-004/TRB-024；ADR-0021 明确更新为“软件决策保留，但板端验收未通过”。

### 修改文件

- `f407/sensor-hub/firmware/sensors/icm42688_provider_c.c`
- `f407/sensor-hub/firmware/sensors/icm42688_fifo_parser_c.c`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/i2c.c`
- `f407/sensor-hub/firmware/stm32cube/atk_f407_sensorhub.ioc`
- `f407/sensor-hub/tests/{icm42688_provider_test.cpp,icm42688_fifo_parser_test.cpp}`
- README、Stage 1、设计、ADR、变更和问题复盘文档

### 验证结果

- 恢复后的 F407 主机 CMake/MSVC Debug 构建通过，`ctest --test-dir build -C Debug --output-on-failure` 为 7/7；首次遗漏 `-C Debug` 得到 7 个 `Not Run`，补正后全部实际执行通过，该命令用法继续遵循 TRB-022。
- F407 ARM clean build 通过：BIN 19384 B，text 19364 B、data 20 B、BSS 4408 B，SHA256 `f0addc29eec272be28aee46a063e95fd878b5681be83df0dc6e430f7687b9d1e`。
- COM6 ROM Bootloader 烧录和逐字节回读通过，Bootloader version `0x31`、chip ID `0x0413`。这是热复位，不计为本轮物理故障修复验证。
- 30/60 秒正式测试和 3600 秒长测均未启动：第二次预检已在创建正式测试前拒绝，避免把持续 I2C/FIFO 恢复当作健康结论。

### 后续 TODO

- 完全断电后先断开 MMC5603 与 BMP390，只保留 ICM42688 及其 3.3 V/GND、PB10 SCL、PB11 SDA、PB12 INT1、AD0 3.3 V；复电后执行同一 diagnostics 增量审计。
- 若 ICM-only 稳定，依次接回 MMC5603、BMP390 定位拖累源；若仍失败，测量 SCL/SDA 空闲电平、确认 4.7k–10k 上拉和短线共地，优先取得逻辑分析仪/示波器波形。
- 只有短测 recovery/failure/overflow/malformed/skipped 增量满足零门槛后，才重新启动 30/60 秒和 3600 秒正式测试。

## 2026-07-19 - Offline Full-Matrix Compass Calibration Tool

### 本次完成

- 新增独立 C++17 `tools/compass_calibrator`，直接读取 History Recorder 的 `magnetometer.csv`，把 nT 转换为 μT，并输出 Runtime 已支持的硬铁偏置和完整 3×3 候选矩阵。
- 对数据做平移/尺度归一化，使用 9 参数完整三维二次曲面最小二乘；自行实现 3×3 Jacobi 特征分解和正定平方根，不引入 Eigen、Python、NumPy 或其他第三方依赖。
- 以矩阵行列式归一保留三轴几何平均场强；质量报告包含输入/使用/离群样本数、八分体覆盖、方向方差比、椭球轴比、原始场强范围、校正场强、RMS 和最大残差。
- 增加两阶段稳健处理：拟合前用分量中位数与径向 MAD 去除极端强磁点，拟合后按残差 MAD 裁剪并重新拟合。
- 支持可选、独立测量的 proper sensor-to-body rotation，最终配置矩阵为 `R * soft_iron`；磁力计椭球本身不用于猜测安装方向。
- 只要拟合得到候选参数，无论质量门禁是否通过，配置都固定输出 `compass_calibration_valid=false`；拟合失败不写候选，后续八方向/倾角/动态/磁偏角验收通过后才能人工启用。
- 新增 ADR-0024 和工具 README，并将构建、数据流、验收边界同步到项目 README、设计、Stage 1 和仓库结构文档。

### 修改文件

- `tools/compass_calibrator/CMakeLists.txt`
- `tools/compass_calibrator/include/calibration/compass_calibrator.h`
- `tools/compass_calibrator/src/{compass_calibrator.cpp,main.cpp}`
- `tools/compass_calibrator/tests/{compass_calibrator_test.cpp,data/magnetometer_sphere.csv}`
- `tools/compass_calibrator/README.md`
- `.gitignore`
- `docs/adr/0024-fit-compass-calibration-offline.md`
- README、Stage 1、设计、仓库结构、变更和问题复盘文档

### 验证结果

- MSVC Debug CMake 构建通过，CTest 3/3；GCC 16.1 Release + Ninja 构建通过，`-Wall -Wextra -Wpedantic` 无警告，CTest 3/3。新增固定球面 CSV 端到端用例，验证 CSV 读取、命令行拟合和候选配置写出。
- 全仓回归再次遇到 TRB-023：当前 PowerShell `PATH` 无 `bash`，首轮 `bash -n` 未执行；改用已确认存在的 `C:\Program Files\Git\bin\bash.exe` 后，仓库 8 份 shell 脚本语法检查全部通过。失败命令和替代路径均保留在排查复盘，未把首次失败误报为通过。
- 新工具首次 GCC Release 构建后，`git status --untracked-files=all` 暴露 `build-gcc/` 未被现有 `build/` 规则覆盖；新增工具级忽略规则并记录为 TRB-031，避免缓存、静态库和 smoke 输出混入提交。
- 全仓回归通过：F407 主机 CTest 7/7，F407 ARM Release 为 19376 B、SHA256 `365fb9c3...129c`；MP157 主机 CTest 10/10、Runtime verifier 通过，ARM Runtime 为 258560 B、SHA256 `47c4bcc9...b739`；PowerShell parser errors 为 0，`git diff --check` 通过。
- 1600 点 Fibonacci 全姿态合成数据包含已知硬铁偏置、非对角全矩阵畸变和确定性噪声；拟合偏置误差小于 0.05 μT、每个矩阵元素误差小于 0.01、RMS 残差小于 0.2%、最大残差小于 0.5%，8/8 八分体通过。
- 近平面数据被拒绝；5 个约 250 μT 三轴极端干扰点使初版普通最小二乘失败，加入粗筛后全部被剔除且最终 RMS 小于 0.5%。
- 90° Z 轴 sensor-to-body 旋转与软铁矩阵组合结果逐元素通过；非正交矩阵被拒绝。
- CSV loader 覆盖真实 History header、带引号时间字段和 nT/μT 换算；候选文本明确包含 `compass_calibration_valid = false` 和完整 `00..22` 配置键。
- 首轮 CTest 因裸 `assert` 打开 MSVC modal 并留下测试/CTest 进程；精确核对路径后终止 PID 21828/24720。随后发现 Release 下 `assert` 被编译掉导致假绿和未使用变量警告，最终改为所有构建类型始终生效的 `CHECK`。完整过程记录为 TRB-030。
- 尚未使用真实室外 MMC5603 CSV；当前证据只证明工具在合成畸变、失败数据和跨编译器下的行为，不能把罗盘标定项标记完成。

### 后续 TODO

- 完整复电恢复 ICM42688 后采集多个独立全姿态磁场数据集，用该工具比较偏置、矩阵、场强和残差稳定性。
- 独立测量传感器到机身的轴向旋转，完成八方向静态、不同倾角、重复性和动态航向验收，再人工启用 calibration valid。

## 2026-07-19 - Versioned UART4 RX Drop Diagnostics

### 本次完成

- 将 F407 已有的 UART4 RX 环形缓冲丢弃计数纳入 `0x02` Sensor Hub diagnostics：保留 legacy 44 字节载荷的语义，在尾部追加 schema version 1 和 32 位累计丢弃字节数，当前载荷长度为 48 字节。
- MP157 parser 同时接受 legacy 44 字节/schema 0 和当前 48 字节/schema 1；未知扩展版本直接拒绝，避免把未来字段静默误解为当前格式。
- `runtime_status.json` 和文本 MCU 状态新增 `diagnostics.schema_version`、`diagnostics.uart4_rx_drop_count`。
- 8 秒预检、逐秒健康时间线、结束审计和 COM6 verifier 新增 schema 1、drop 0 门槛；时间线审计还检查 RX drop 单调性、运行期增量和最终值。
- 新增 F407 diagnostics frame-builder 测试，验证 48 字节长度、schema 1、32 位小端计数和 CRC；扩展 MP157 parser/status publisher 测试，覆盖 schema 1、legacy 兼容和未知版本拒绝。
- 新增 ADR-0023 和 TRB-028，记录为什么选择版本化尾部扩展、部署顺序、验收边界和真实溢出注入结果；新增 TRB-029，保留首轮手工预检参数顺序错误及纠正过程。

### 修改文件

- `common/protocol/mcu_protocol.h`
- `f407/sensor-hub/firmware/protocol/mcu_frame_builder_c.{h,c}`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/tests/mcu_frame_builder_test.cpp`
- `f407/sensor-hub/CMakeLists.txt`
- `mp157/outdoor-core-service/include/mcu/{mcu_protocol.h,mcu_status.h}`
- `mp157/outdoor-core-service/src/mcu/{mcu_frame_parser.cpp,mcu_status.cpp}`
- `mp157/outdoor-core-service/src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/tests/{mcu_protocol_tests.cpp,status_publisher_tests.cpp}`
- `mp157/outdoor-core-service/scripts/{run_board_health_preflight.sh,monitor_board_runtime_health.sh,audit_board_long_validation.sh}`
- `scripts/verify_f407_uart.ps1`
- README、协议、Stage 1、设计、仓库结构、ADR、变更和问题复盘文档

### 验证结果

- F407 主机 CMake/MSVC Debug 构建和 CTest 7/7 通过；MP157 主机 CMake/MSVC Debug 构建和 CTest 10/10 通过；完整 `verify_runtime.ps1` 通过。
- F407 ARM Release 构建通过：BIN 19376 B，text 19356 B、data 20 B、BSS 4408 B，SHA256 `365fb9c32998bcb232329f6b200a9cd9a6d83635b56974471a9085c579af129c`。
- MP157 ARMv7 hard-float 构建通过：Runtime 258560 B，SHA256 `47c4bcc9f34f1d45add17e27734f2e8acda7b2e77364ba87e7d151b7092bb739`。
- 三份修改后的 POSIX shell 脚本均通过 `sh -n`；通过 COM9 部署后，Runtime/脚本哈希、权限、systemd 和设备门槛全部通过。
- 通过 COM6 ROM Bootloader 烧录 F407 schema 1 镜像并逐字节回读通过。该动作会热复位 F407，不能视为传感器复电验证；启动后 ICM42688 仍不 ACK。
- 实板预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-ZVXjOR` 已看到 diagnostics schema 1、`uart4_rx_drop_count=0`、GNSS、四类 MCU 数据、ping ACK、Board IMU、Logger 三项健康和空核心错误；唯一失败项仍为真实 F407 sensor status `4142 (0x102E)`。累计 I2C recovery/failure 均为 56，最近失败为设备 `0x68`、寄存器 `0x76` 写事务、HAL status 1/error 4、init step 1。
- 在 MP157 上用当前最终状态执行 3 秒 monitor 脚本探针，得到表头 23 列、4 行数据、所有行字段数 23、schema 1、drop 0；临时目录已清理。
- 确认 `/dev/ttySTM1` 无进程占用后，以 115200 8N1 连续写入 40960 字节；F407 heartbeat 变为 `12334 (0x302E)`，预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-jYMnDJ` 解码 `schema_version=1`、`uart4_rx_drop_count=2231`，并同时拒绝非零 drop 与原有 ICM 状态两项。
- 首次手工调用把 Runtime 路径误放在第一个位置，脚本返回 `duration_seconds must be a positive integer`、rc 2 且没有启动 Runtime；按实际 `duration runtime config storage_mount` 顺序重试后才取得上述有效证据。该自动化错误记录为 TRB-029。
- 使用与现有 verifier 相同的 COM6 DTR/RTS 应用复位序列清零 F407 启动周期状态；复位后预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-XOXgzn` 通过 schema 1/drop 0 和全部软件门槛，只剩原有 ICM42688 `0x102E` 一项失败。

### 后续 TODO

- F407 与 ICM42688 3.3 V 完整掉电再上电后，先通过 8 秒预检，再执行 30/60 秒 diagnostics 增量审计；不要用 COM6 热复位替代完整复电。
- 在扩展完整下行命令集前测量主循环最坏消费延迟和不同持续流量下的容量边界，再决定是否扩大 RX ring 或引入 DMA/idle-line 接收。

## 2026-07-19 - Structured Logger Failure Health

### 本次完成

- 新增 `LoggerStatus`，在每个文件输出会话内累计日志轮转失败、写/flush 失败和最后错误；关闭输出不会清除已经发生的故障证据。
- 轮转文件操作失败后尝试重新以 append 模式打开活动日志；恢复成功时继续写入触发日志，但失败计数和错误保持可审计。
- `runtime_status.json` 的 `storage` 节点新增 `log_rotation_failure_count`、`log_write_failure_count` 和 `log_last_error`；Logger 错误同时进入聚合 `storage.last_error`。
- 健康预检与长测结束审计新增 Logger 两类计数为零、错误为空的门槛，避免仅通过日志文件存在和备份数量判断存储健康。
- 新增 `status_publisher_tests`，验证非零计数和带引号错误文本的 JSON 序列化；Logger 测试新增可移植的轮转失败故障注入和恢复后继续写入验证。
- 新增 TRB-027，记录“日志错误只写 stderr、无法进入无人值守验收证据”的观测性缺口、方案和验证边界。

### 修改文件

- `mp157/outdoor-core-service/include/log/logger.h`
- `mp157/outdoor-core-service/src/log/logger.cpp`
- `mp157/outdoor-core-service/include/runtime/runtime_status.h`
- `mp157/outdoor-core-service/src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/tests/logger_tests.cpp`
- `mp157/outdoor-core-service/tests/status_publisher_tests.cpp`
- `mp157/outdoor-core-service/CMakeLists.txt`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `mp157/outdoor-core-service/scripts/run_board_health_preflight.sh`
- `mp157/outdoor-core-service/scripts/audit_board_long_validation.sh`
- README、Stage 1、设计、ADR、变更和问题复盘文档

### 验证结果

- MP157 CMake/MSVC Debug 构建通过，CTest 10/10；`logger_tests` 用非空 `.1` 目录确定性触发轮转失败，确认计数为 1、写入失败计数为 0、错误保留且活动日志恢复后仍包含触发消息。
- `status_publisher_tests` 验证轮转失败计数 3、写失败计数 2 和含引号错误文本均正确写入 JSON；完整 `verify_runtime.ps1` 通过正常路径零错误断言。
- ARMv7 hard-float 交叉构建通过。最新 Runtime 为 258560 B，SHA256 `330b7bc4ee95d58e83ce7590f122e67cffc8e112a8b741801102eb95cb52a645`。
- 通过 COM9 完成 `/opt/outdoor-agent` 幂等部署，Runtime、配置、七份 shell 脚本、三份 MCU fixture、NMEA fixture 和 systemd unit 的 SHA256/权限/语法/服务/设备门槛全部通过。
- 真实 MP157 确定性运行发布 `log_rotation_failure_count=0`、`log_write_failure_count=0`、`log_last_error=""` 并以 rc 0 结束；临时目录 `/tmp/outdoor-agent-log-health-ZXwTJb` 经绝对路径与前缀校验后已删除。
- 最新真实预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-FImCIn` 中三个 Logger 门槛均通过；预检仍仅因 F407 `status_flags=4142 (0x102E)` 返回 1。累计 I2C recovery `6636`、最终 failure `6632`，最后失败仍为 ICM42688 `0x68/REG_BANK_SEL(0x76)` 写事务、init step 1，因此未启动正式 30 秒测试。
- 尚未在真实 SD 卡上注入写满、拔卡或只读重挂载导致的 write/flush 失败，不能把正常路径计数为零描述为介质故障注入已经通过。

### 后续 TODO

- 在独立、可恢复的测试目录和明确停机边界下设计 SD 写满/只读故障注入，验证 `log_write_failure_count`、status 持久化和长测审计拒绝路径。
- F407 与 ICM42688 完整掉电复电后继续 8 秒预检和 30/60 秒 diagnostics 增量审计。

## 2026-07-19 - Configurable Tilt-Compensated Compass Estimation

### 本次完成

- 在 MP157 Runtime 新增 `CompassEstimator`，组合 F407 ICM42688 加速度和 MMC5603 磁场样本，输出磁航向、磁偏角修正航向、roll/pitch、磁场强度和质量状态。
- 支持三轴硬铁偏置与完整 3×3 软铁/传感器安装变换矩阵；配置校验拒绝非有限数、奇异矩阵、非法范围和零样本时差。
- 增加样本时差、静态加速度模长、磁场模长和 F407 heartbeat 来源健康门槛；动态、受干扰、Mock fallback 或 FIFO/I2C 当前错误状态返回明确错误，不沿用旧航向。
- `runtime_status.json` 增加独立 `compass` 节点；text/framebuffer dashboard 使用有效罗盘航向，并删除固定 `62.3°` 假航向。罗盘无效时只允许有效且速度不低于 1 km/h 的 GNSS course 回退。
- 默认单位矩阵和零偏置明确设置 `compass_calibration_valid=false`，输出 `quality=uncalibrated`；没有虚构真实整机标定已经完成。
- MCU frame 成功应用后立即更新派生罗盘状态，再触发 history callback，保证同轮 dashboard 看到一致状态。
- 新增 ADR-0022；三次集成验证失败及根因记录为 TRB-025。

### 修改文件

- `mp157/outdoor-core-service/include/navigation/compass_estimator.h`
- `mp157/outdoor-core-service/src/navigation/compass_estimator.cpp`
- `mp157/outdoor-core-service/tests/compass_estimator_tests.cpp`
- `mp157/outdoor-core-service/data/mcu_mock_frames_compass.txt`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`
- `mp157/outdoor-core-service/src/services/mcu_mock_service.cpp`
- `mp157/outdoor-core-service/config/*.conf`
- `docs/adr/0022-compute-calibrated-compass-heading-on-mp157.md`
- README、Stage 1、设计、变更和问题复盘文档

### 验证结果

- MP157 CMake/MSVC Debug 构建通过，CTest 9/9；新增测试覆盖四方位、磁偏角、硬铁修正、30° 倾斜、缺输入、样本过期、磁场越界和奇异矩阵。
- `verify_runtime.ps1` 最终通过；聚焦 fixture 的 JSON/dashboard 同时报告 `valid=true`、`tilt_compensated=true`、`quality=uncalibrated`，航向在 `[0,360)`。
- ARMv7 hard-float 交叉构建通过。
- 通过 COM9 将 254152 B ARM Runtime、配置、验证脚本和聚焦 fixture 部署到 `/opt/outdoor-agent`，全部 SHA256/权限/语法/systemd/设备门槛通过；板端 Runtime SHA256 为 `8f4c142301188a617bad7d49a60f7aeeaf9d70986998146ce0c030dcbf0210d5`。
- MP157 板端使用聚焦 fixture 得到 JSON/dashboard 一致的 `valid=true`、`tilt_compensated=true`、heading `30.303°`、field `57.711 uT`、`quality=uncalibrated`；该结果验证 ARM 软件链路，不代表真实传感器标定。
- 首次验证因默认 fixture 没有磁力计失败；第二、三次因有限 dashboard 刷新早于 mock 注释/传感器帧消费失败。过程、无效尝试和最终方案均记录在 TRB-025。
- 板端 `/tmp` 清理的一次性嵌套引号命令在主机解析阶段失败，随后复用参数化 helper 精确清理并验证不存在；记录为 TRB-026。
- 使用同一最新 ARM Runtime 执行真实 8 秒健康预检：目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-C8NBPu` 中 GNSS、MCU 四类数据、ping ACK、diagnostics、Board IMU 和核心 error 门槛均通过，但 F407 仍为 `status_flags=4142 (0x102E)`；累计 I2C recovery `5211`、最终 failure `5208`，最后失败仍是 ICM42688 `0x68/REG_BANK_SEL(0x76)` 写事务、init step 1。预检返回 1，未启动 30 秒正式测试；重复证据已追加到 TRB-004/TRB-024。
- 尚未执行真实板罗盘标定和精度验收，不能将软件通过描述为真实航向已校准。

### 后续 TODO

- 在室外远离磁干扰源完成多姿态采集，拟合硬铁偏置和 3×3 软铁/安装矩阵。
- 结合当地磁偏角执行八方向、倾斜、重复性和动态稳定性验收。
- 等待 F407/ICM42688 完整复电后继续 diagnostics 短测与小时级验证。

## 2026-07-19 - Diagnostics Short-Test Root Cause and Bounded Shared-I2C Polling

### 本次完成

- 将 UART4 Sensor Hub diagnostics 镜像部署到真实 F407/MP157 链路，执行两轮同条件 30 秒审计并保留失败证据；没有用最终健康 heartbeat 覆盖累计错误。
- 第一轮确认 ICM42688 `HEADER_MSG` 空 FIFO 标记被旧 parser 放大为 skipped，并将 FIFO 改为大端 record-count、4 条记录 watermark、完整 16 字节记录读取；同时移除运行期 `INT_STATUS` 事务、将 I2C2 提升到 400 kHz，并把单次 FIFO 失败与立即 Mock fallback 解耦。
- 第二轮 skipped 从 9281 降至 290，但 recovery/failure/overflow 仍失败；最近事务横跨 ICM42688、MMC5603 和 BMP390，据此定位 MMC5603 单次测量后的无间隔 `STATUS1` 紧轮询。
- 按 MMC5603 默认 `BW=00` 的 6.6 ms 测量时序，触发后等待 7 ms，再最多检查 3 次状态且每次间隔 1 ms；新增 provider 主机测试和 ADR-0021。
- 最新 19356 B 固件已烧录并逐字节回读。热刷写后 ICM42688 再次不 ACK，真实预检以 `0x102E` 拒绝正式短测；该负向结果记录在 TRB-004、TRB-020 和 TRB-024。

### 修改文件

- `f407/sensor-hub/firmware/sensors/icm42688_fifo_parser_c.c`
- `f407/sensor-hub/firmware/sensors/icm42688_provider_c.c`
- `f407/sensor-hub/firmware/sensors/mmc5603_provider_c.c`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/i2c.c`
- `f407/sensor-hub/firmware/stm32cube/atk_f407_sensorhub.ioc`
- `f407/sensor-hub/tests/icm42688_fifo_parser_test.cpp`
- `f407/sensor-hub/tests/mmc5603_provider_test.cpp`
- `docs/adr/0021-bound-mmc5603-polling-and-use-fifo-record-count.md`
- `docs/troubleshooting_log.md` 及项目状态文档

### 验证结果

- F407 主机 Debug 构建和 CTest 6/6 通过。
- GNU ARM 固件构建通过：BIN 19356 B、text 19336 B、data 20 B、BSS 4408 B，SHA256 `d220cbc7b9dcf2751be55eba930199a745aaf4296e7a9a4e0f5a69684e32ad56`。
- COM6 UART Bootloader 烧录和逐字节回读通过。
- 真实预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-HuwN9j` 保存 `status_flags=4142 (0x102E)`、52 次最终 I2C failure 和 ICM42688 `REG_BANK_SEL(0x76)` 写 NACK；预检返回非零且没有启动正式测试，符合门槛设计。
- 最新修复尚未取得完整复电后的 30/60 秒正向板端结果，不能宣称修复已通过真实板验收。

### 后续 TODO

- F407 与 ICM42688 3.3 V 完整掉电几秒再上电，先执行 8 秒预检，再执行同条件 30/60 秒 diagnostics 增量审计。
- 短测通过后从新目录启动 3600 秒耐久；随后执行 SIGKILL 恢复和室外 GNSS fix 验收。

## 2026-07-19 - Board Health Preflight, Timeline and Sensor Hub Diagnostics

### 本次完成

- 新增 `run_board_health_preflight.sh`，在正式板端验收前独占执行 8 秒真实 GNSS/MCU/Board IMU 采集并保存独立状态、日志、元数据和报告。
- 预检强制要求 Runtime 受控停止、GNSS seen、F407 四类数据 seen、ping ACK status 0、`status_flags=425 (0x01A9)`、Board IMU seen，以及 Runtime/storage/MCU/Board IMU error 为空。
- 小时长测与 SIGKILL 恢复脚本只有预检通过才创建正式测试目录；元数据关联预检目录，失败报告不会混入正式耐久证据。
- 部署脚本增加预检脚本的上传、权限、SHA256 和 `sh -n` 门槛；README、设计、目录、Stage 1 和问题复盘同步更新。
- 新增 `TRB-20260719-020`，记录“结束时才检查传感器健康”造成无效耐久任务的风险、根因和方案取舍。
- 新增 `monitor_board_runtime_health.sh`，长测逐秒记录状态位和核心 error；审计新增样本覆盖、致命位、99% 健康占比、最长 5 秒降级和最终 `0x01A9` 门槛。
- 新增 ADR-0019 和 `TRB-20260719-021`，记录最终快照漏洞、瞬态与永久故障的方案取舍，以及当时无法从 MP157 获取 USART1-only 累计 diagnostics 的证据边界。
- 将 44 字节 `0x02 sensor_hub_diagnostics` 同时送入 UART4/MP157 和 USART1/COM6；MP157 新增解析、状态发布和主机测试，健康时间线同步保存 I2C/FIFO 累计计数和最近失败上下文。
- ICM42688 provider 区分首次 reset 与运行期 reinit；运行期重初始化不再清零 FIFO 累计计数，保证同一 F407 启动周期内诊断单调。
- 预检增加 diagnostics seen 版本配对门槛；长测审计增加累计计数单调性，以及 I2C final failure、FIFO overflow/malformed/stall 的零增量门槛。
- 新增 ADR-0020；新增 `TRB-20260719-022`，记录 Visual Studio 多配置 CTest 未执行、44 字节 fixture 断言和 MSVC modal 窗口造成的自动化阻塞。

### 修改文件

- `mp157/outdoor-core-service/scripts/run_board_health_preflight.sh`
- `mp157/outdoor-core-service/scripts/monitor_board_runtime_health.sh`
- `mp157/outdoor-core-service/scripts/run_board_long_validation.sh`
- `mp157/outdoor-core-service/scripts/run_board_crash_recovery_validation.sh`
- `mp157/outdoor-core-service/scripts/audit_board_long_validation.sh`
- `common/protocol/mcu_protocol.h`
- `mp157/outdoor-core-service/include/mcu/mcu_status.h`
- `mp157/outdoor-core-service/src/mcu/mcu_frame_parser.cpp`
- `mp157/outdoor-core-service/src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/tests/mcu_protocol_tests.cpp`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/sensors/icm42688_provider_c.h/.c`
- `scripts/deploy_mp157_runtime.ps1`
- `README.md`、`mp157/outdoor-core-service/README.md`
- `docs/project_design.md`、`docs/repo_structure.md`
- `docs/stage1_plan.md`、`docs/stage1_bringup_plan.md`
- `docs/troubleshooting_log.md`、`docs/dev_log.md`
- `docs/adr/0019-use-sampled-health-timeline-for-board-endurance.md`
- `docs/adr/0020-expose-sensor-hub-diagnostics-on-uart4.md`

### 验证结果

- 本地 Git Bash `bash -n` 和板端五份 `sh -n` 全部通过；部署脚本真实执行 68.7 秒，11 个文件 SHA256、权限、ICM20608 systemd 服务和设备节点全部通过，输出 `deployment=PASSED`。
- 真实 F407 状态已恢复：连续两次 8 秒预检均得到 `425 (0x01A9)`，四类 MCU 数据、ping ACK、GNSS NMEA、Board IMU 和核心 error 门槛全部通过。
- 第一次预检自动放行 60 秒正式冒烟，完整审计通过；GNSS/MCU IMU/磁力计/气压计/Board IMU 分别约 `9.0/100.63/18.47/9.53/8.0 Hz`，日志有 3 个备份且单文件不超过 1 MiB。
- 第二次预检自动放行 PID `4868` 的 3600 秒正式长测，证据目录为 `/run/media/mmcblk1p1/outdoor-agent-hour-final-o7GBsX`。
- PID `4868` 运行早期出现 `558 (0x022E)` 和 `942 (0x03AE)` FIFO error/fallback，随后恢复 `425`；因为旧审计只看最终快照，已将目录标记 `ABORTED_MISSING_HEALTH_TIMELINE` 并用 SIGTERM 在 1 秒内停止，不计入小时结论。
- Monitor POSIX 语法通过，并使用本机真实 `runtime_status.json` 验证逐秒解析 Runtime state、F407 status flags 和四个 error 字段。
- 20 秒真实时间线冒烟保存 21 个样本，其中 20 个可评估样本只有 15 个为 `0x01A9`；审计以 `healthy_permille=750`、5 个降级样本和最终 `0x03AE` 正确返回失败，证明周期性 FIFO fallback 不再被最终快照掩盖。
- MP157 Debug CTest 8/8、Runtime 验证脚本、ARMv7 hard-float 构建和 F407 PC CTest 5/5 通过。
- 当前 UART4 diagnostics F407 Release 镜像构建通过：BIN 19396 B，text 19376 B、data 20 B、BSS 4408 B；该镜像尚未刷入，板端 diagnostics seen 和计数增量门槛尚未验证。
- 首次 F407 CTest 因未指定 `-C Debug` 显示 5 个 `Not Run`；首次 MP157 diagnostics fixture 少 1 字节并触发 MSVC modal 断言。两次失败尝试、精确清理的孤儿进程、修正方式和最终返回码均记录在 `TRB-20260719-022`。
- 当前硬件在首次运行新预检前已经恢复，未取得新脚本对真实 `0x102E` 的负向退出证据；不能将正向放行替代负向分支验证。

### 后续 TODO

- 部署新 ARM Runtime 并刷入 UART4 diagnostics F407 固件；短时板测读取累计计数，定位周期性 FIFO fallback 的具体来源并完成最小修复。
- 短时诊断审计通过后，从零启动 3600 秒长测。
- 小时级通过后执行 SIGKILL/同目录追加恢复验收。
- 使用可控 fixture 或下次自然 `0x102E` 状态补齐预检负向分支的直接证据。

## 2026-07-19 - MP157 Long Validation Harness and ICM20608 Auto-load

### 本次完成

- 初版 `deploy/modules-load.d/outdoor-agent.conf` 在手工重启 `systemd-modules-load` 时可恢复 ICM20608，但真实 boot 暴露其早于板厂 `link-modules.service` 执行，已撤回该配置。
- 新增 `deploy/systemd/outdoor-agent-icm20608.service`，通过 `Requires/After=link-modules.service` 建立正确顺序；真实软重启确认服务 active/enabled、标准 modules-load Result=success、ICM20608 probe ID `0xAE` 且 `/dev/icm20608` 自动出现。
- 新增 POSIX `scripts/run_board_long_validation.sh`，统一检查设备节点，创建独立 SD 证据目录，记录 Runtime SHA256、内核、板端时间、SD 使用量和 PID，并启动有界联合长测。
- 使用 XMODEM-CRC 上传长测脚本，本地/板端 SHA256 均为 `cb1656fe6157e97d4f3e04b6ac3a5925a1ecea46feacc477075c9816068a067e`。
- 新增 `audit_board_long_validation.sh` 和 `run_board_crash_recovery_validation.sh`，把 CSV schema/单调键/频率/末行、状态、ACK、健康位、日志轮转和异常退出后追加恢复固化为板端验收。
- 初次启动 PID `2005` 的 3600 秒长测后，异常恢复脚本因 BusyBox `ps` 截断进程名而误启动第二 Runtime；该数据已明确作废。修复为 `/proc/<pid>/cmdline` 独占检测并用真实 PID 验证拒绝分支后，PID `2005` 已受控停止。
- 使用同一 ARM Runtime 从全新目录重新启动 PID `3127` 后，实验性 COM6 `SkipReset` 打开仍触发 F407 复位，ICM42688 初始化失败并回退 `0x102E`；该第二次数据也已作废，PID `3127` 受控停止。
- 撤回误导性的 `-SkipReset` 参数；当前等待 F407 与 3.3V 传感器完整断电上电恢复 `0x01A9`，复电前没有有效小时长测在运行。
- MP157 软重启后将 ARM Runtime、配置和三份验证脚本部署到持久化 `/opt/outdoor-agent/{bin,config,scripts}`；本地/板端 SHA256 全部一致，脚本 `sh -n` 通过，权限收敛为可执行文件 0755、配置 0644。
- 新增仓库级 `scripts/deploy_mp157_runtime.ps1`，封装目录创建、部署清单 XMODEM 上传、权限、旧配置清理、systemd enable/restart、SHA256 和最终状态检查；初版真实 COM9 完整执行 58.5 秒，最终十文件版本执行 66.1 秒，所有远端 SHA256、四份 shell 语法、服务和设备节点门槛通过并输出 `deployment=PASSED`。
- 新增 `run_board_gnss_fix_validation.sh` 和合法 `mcu_mock_frames_valid.txt`，以 status/RMC/GGA/CSV 四层证据验收室外 fix；两轮真实室内负向测试帮助修正 section error 误判和负向 fixture 复用问题。
- 按问题排查约束新增 `TRB-20260719-012` 至 `TRB-20260719-015`，并对 `TRB-20260719-010` 的 PowerShell 引号问题复发追加证据，保留所有无效启动、测试污染和恢复步骤。

### 修改文件

- `mp157/outdoor-core-service/deploy/systemd/outdoor-agent-icm20608.service`
- `scripts/deploy_mp157_runtime.ps1`
- `mp157/outdoor-core-service/scripts/run_board_gnss_fix_validation.sh`
- `mp157/outdoor-core-service/data/mcu_mock_frames_valid.txt`
- `mp157/outdoor-core-service/scripts/run_board_long_validation.sh`
- `mp157/outdoor-core-service/scripts/audit_board_long_validation.sh`
- `mp157/outdoor-core-service/scripts/run_board_crash_recovery_validation.sh`
- `mp157/outdoor-core-service/deploy/systemd/outdoor-agent-icm20608.service`
- `README.md`、`mp157/outdoor-core-service/README.md`
- `docs/project_design.md`、`docs/repo_structure.md`
- `docs/stage1_plan.md`、`docs/stage1_bringup_plan.md`
- `docs/troubleshooting_log.md`、`docs/dev_log.md`

### 验证结果

- 长测启动前确认 SD 卡、`/dev/ttySTM1`、`/dev/ttySTM2`、`/dev/icm20608` 和 `/dev/fb0` 均存在，且无遗留 Runtime 进程。
- 长测审计脚本通过板端 `sh -n`；用既有 20 秒数据集验证五类 CSV 字段数、严格递增键、约 `9.0/98.05/20.05/10.0/8.0 Hz` 和末行换行，旧数据集缺少长测元数据/日志时按预期返回失败。
- 异常恢复脚本初次负向 guard 测试暴露 BusyBox `ps` 截断问题；修复后在 PID `2005` 存活时，长测/异常恢复脚本分别以 3/2 拒绝并未创建目录。
- PID `3127` 独占启动通过，但被 COM6 控制线复位污染后作废；当前无有效 3600 秒测试，不能标记小时级验证通过。
- 第二次 MP157 软重启后，持久化 `/opt/outdoor-agent` 部署完成；Runtime SHA256 为 `8b499705...b7f94`，三份脚本和配置均完成板端哈希/语法校验。
- 新部署脚本完成 PowerShell 语法和非法端口保护测试，并在真实 MP157 上逐一确认六个本地/远端 SHA256、三份 `sh -n`、ICM20608 服务 active/enabled 和设备节点。
- 本轮重新执行 MP157 Debug 构建与 CTest 8/8、`verify_runtime.ps1`、ARMv7 Release 交叉构建；F407 主机 Debug 构建与 CTest 5/5、ARM Release 固件构建，全部通过。
- GNSS 最终负向复验：5 秒采集 45 行，Runtime 受控停止，三个核心 section 无错误，脚本仅因无 fix、无有效 RMC 坐标和无有效 GGA 质量返回 1；报告目录 `/run/media/mmcblk1p1/outdoor-agent-gnss-fix-brYOEH`。
- 在没有打开 COM6 的前提下执行 8 秒 F407/MP157 健康探针：PID `2791` 正常停止，四类 MCU 状态、ping ACK、Board IMU 和 error 字段正常，但 `status_flags=4142 (0x102E)` 未恢复，确认仍需人工完整断电上电。
- 本次未修改 C/C++ 代码，未重新执行 CMake/CTest；最新代码构建与 CTest 8/8 结果沿用同日前序记录。

### 后续 TODO

- 3600 秒结束后核验受控停止、五类 CSV 全时段频率、状态错误字段、日志轮转、SD 容量增长和 F407 累计诊断。
- 执行进程强制终止/重启恢复测试，明确其与真实断电测试的证据边界。
- 真实断电一致性仍需人工配合；MP157 软重启自动加载已经闭环。

## 2026-07-19 - MP157 CSV History Recording and Log Rotation

### 本次完成

- 新增 `HistoryRecorder`，分别追加 GNSS、MCU IMU、磁力计、气压计和 Board IMU CSV。
- 使用 GNSS valid sentence count、各 MCU sample uptime 和 Board IMU sample count 去重，并保留 host UTC、MCU uptime 和协议原始整数单位。
- History Recorder 不注册为永不完成的 Service，保持有限文件回放可以自然退出；GNSS/MCU serial 每个合法更新后触发记录，Runtime 循环观察器覆盖 Board IMU 和 file/mock 路径。
- 新增 `history_enabled`、`history_output_path`、`history_flush_interval_ms`、`--history` 和 `--history-output`；storage 模式下相对路径解析到 storage root。
- Logger 新增按大小轮转，默认单文件 1 MiB、保留 3 个 `.1` 到 `.3` 备份；状态 JSON 输出 rotation/history 实际配置。
- 新增 History Recorder 和 Logger 单元测试，并扩展 Runtime 脚本验证 storage/history 组合路径。
- 新增 ADR-0018，记录 CSV/JSONL/SQLite 与按日期/按大小日志策略的比较、选择和风险。
- 按排查记录约束登记测试字段误写、Board IMU 模块未加载、PowerShell 多层引号和 history 降频问题 `TRB-20260719-008` 至 `TRB-20260719-011`。

### 修改文件

- `mp157/outdoor-core-service/include/storage/history_recorder.h`
- `mp157/outdoor-core-service/src/storage/history_recorder.cpp`
- `mp157/outdoor-core-service/include/log/logger.h`、`src/log/logger.cpp`
- `mp157/outdoor-core-service/include/config/app_config.h`、`src/config/config_loader.cpp`、`config/`
- `mp157/outdoor-core-service/include/runtime/runtime_status.h`、`src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/src/main.cpp`、`CMakeLists.txt`
- `mp157/outdoor-core-service/tests/history_recorder_tests.cpp`、`tests/logger_tests.cpp`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `README.md`、`mp157/outdoor-core-service/README.md`、`docs/project_design.md`、`docs/stage1_plan.md`、`docs/stage1_bringup_plan.md`、`docs/repo_structure.md`
- `docs/adr/0018-use-csv-history-and-size-based-log-rotation.md`、`docs/troubleshooting_log.md`

### 验证结果

- 首次 MSVC 构建因测试夹具误写 `ImuSample::accelZG` 失败；生产库和 Runtime 已完成编译。根因和修复记录在 `TRB-20260719-008`。
- 修正为协议原始字段 `accelZMg` 后，MP157 Debug 全量构建成功，CTest 8/8 通过。
- `powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1` 通过，确认五类 CSV 创建、GNSS/MCU IMU 数据行、history 状态和 rotation 配置字段。
- ARMv7 hard-float `cmake --build build-arm` 成功，Runtime、History Recorder、Logger 和 8 个测试目标全部完成编译链接。
- 初版 ARM Runtime 本地/板端 SHA256 均为 `26184ec8c3755a7bd97dcc989e64b21bed9b90ed83ced23e289e6ef3000cab39`；真实 MP157 60 秒双串口/Board IMU/framebuffer/SD history 联合运行正常停止，五类 CSV、status 和 dashboard 均生成，四类 MCU seen 且 ping ACK status 0。
- 60 秒联合运行发现 MCU IMU history 约 57 Hz；`--no-dashboard` 20 秒约 65 Hz，而 COM6 原始镜像为 104.2 Hz，定位到一次 serial read 内多帧覆盖最新状态快照。
- 增加 GNSS/MCU 合法更新逐帧回调后，修复版本地/板端 SHA256 均为 `8b4997053d32ccc364f7b5c06d1bb5a0e591fa22e1bef3bee0a6f36feb0b7f94`；真实 framebuffer 联合运行 20 秒得到 MCU IMU 1961 条约 98.1 Hz、磁力计约 20.1 Hz、气压计 10.0 Hz。
- 真实 VFAT SD 卡 30 秒 info 日志运行生成 718 KiB 活动日志和 1.0 MiB `.1` 备份，最终 state stopped、ACK 正常，证明默认 1 MiB 大小轮转生效。
- `/dev/icm20608` 首次预检缺失；确认是模块未加载后，`modprobe icm20608` 返回 0、probe ID 为 `0xAE` 并恢复字符设备。尚未固化开机自动加载。
- 未在真实 MP157 SD 卡上执行小时级记录或掉电测试。

### 后续 TODO

- 在真实 MP157 上同时启用双串口、Board IMU、framebuffer、history 和 storage，执行至少一小时受控运行。
- 保存 CSV 行数/频率、日志文件大小与备份、CPU/内存、状态更新时间和异常统计。
- 执行进程强制终止与整板断电测试，评估 CSV 最后一行、flush 周期和日志轮转一致性。
- 若后续要求无损保存 100 Hz IMU，新增有界事件队列或直接记录解码帧，并定义 overflow 策略。
- 将 `icm20608` 加入板端自动加载配置，并在整板重启后验证 `/dev/icm20608` 自动出现。

## 2026-07-19 - MP157 Cooperative Runtime Scheduler

### 本次完成

- 将阻塞式 `IService::run()` 改为短步骤 `poll()`，由 Runtime Manager 单线程交错推进全部活动服务。
- 将 GNSS replay/serial、MCU mock/serial、板载 ICM20608、dashboard 和 evdev launcher 改为非阻塞或有界轮询。
- 增加运行期状态发布回调、active/completed service 计数、SIGINT/SIGTERM 停止和 `runtime_run_seconds` 总时长限制。
- 为持续采集统一 0 值语义：GNSS/MCU capture seconds、Board IMU sample count 和 dashboard refresh count 为 0 时常驻。
- 新增 Runtime Manager 测试，覆盖交错调度、服务失败、停止条件、循环回调和启动失败回滚。
- 新增 ADR-0017，并将问题原因、方案比较、验证结果和剩余板端风险回填到 `TRB-20260719-007`。

### 修改文件

- `mp157/outdoor-core-service/include/runtime/`、`src/runtime/`
- `mp157/outdoor-core-service/include/services/`、`src/services/`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/include/config/app_config.h`、`src/config/config_loader.cpp`、`config/`
- `mp157/outdoor-core-service/tests/runtime_manager_tests.cpp`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `README.md`、`docs/project_design.md`、`docs/stage0_plan.md`、`docs/stage1_plan.md`、`docs/stage1_bringup_plan.md`、`docs/repo_structure.md`
- `docs/adr/0003-runtime-service-architecture.md`、`docs/adr/0017-use-cooperative-runtime-service-scheduler.md`、`docs/troubleshooting_log.md`

### 验证结果

- 修改前基线：MP157 CTest 5/5、F407 CTest 5/5 通过。
- 修改后：MP157 Debug 构建成功，CTest 6/6 通过。
- `scripts/verify_runtime.ps1` 通过，包含 `dashboard_refresh_count = 0` 与 `runtime_run_seconds = 1` 的持续模式受控退出验证。
- 既有 ARMv7 hard-float `build-arm` 交叉构建成功，Linux fbdev/evdev 和串口分支均完成编译。
- 未执行真实 MP157 双串口、板载 IMU、触摸和 framebuffer 小时级联合运行，不能据此声明整机长期稳定。

### 后续 TODO

- 在真实 MP157 上执行小时级多设备协作运行，并保存状态文件、日志与串口统计作为 `TRB-20260719-007` 关闭证据。
- 继续实现 SD 卡历史采样记录服务与日志轮转，控制长期运行的存储占用。

## 2026-07-19 - Troubleshooting Retrospective and Recording Policy

### 本次完成

- 新增 `docs/troubleshooting_log.md`，建立问题索引、证据等级、状态流转、详细复盘和新问题记录模板。
- 首批回填 2026-07-18 至 2026-07-19 F407/MP157 联调的关键问题，包括 UART4 RX 丢包、共享 I2C2 锁死、Bootloader `0xF9`、IMU 频率不足、ICM42688 不 ACK、FIFO 恢复硬化和时间戳连续性。
- 补录 50 kHz/80 ms、1 ms 延迟和 16 字节分段 FIFO 读取等已回退实验；由于缺少独立临时固件和原始抓包，将其明确标记为 C 级证据，避免把过程回忆当作已验证结论。
- 更新 `AGENTS.md`，要求后续问题在分析开始时建档，并完整记录无效尝试、根因证据、修复和验证，作为面试输入资料。

### 修改文件

- `AGENTS.md`
- `README.md`
- `docs/troubleshooting_log.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/changelog.md`
- `docs/dev_log.md`

### 验证结果

- 本次仅修改 Markdown 文档，没有修改代码，因此未执行 CMake 构建和 CTest。
- 已检查新增文档链接和问题 ID 的唯一性。

### 后续 TODO

- 后续遇到问题时，在分析阶段即时更新复盘表，避免仅在问题关闭后回忆补录。
- 为小时级 I2C 耐久和故障注入测试保存原始抓包或摘要文件，并关联到对应问题 ID。

## 2026-07-19 - ICM42688 FIFO Final Board Validation and Recovery Hardening

### 本次完成

- 整板断电上电后恢复真实 ICM42688，并完成 FIFO 配置/数据路径的板端诊断和最终 60 秒验收。
- 启用 `FIFO_WM_GT_TH`，把 ICM42688 调整为三颗传感器中最后初始化，并将主循环 FIFO 最短读取周期设为 30 ms，避免启动积压和高频伪事件造成 I2C 事务风暴。
- 删除不可靠的“读取后 FIFO count 未下降即 drain stall”判断；FIFO 写入可与主机读取并发，该判断会误报。
- 增加 USART1-only `0x02` Sensor Hub diagnostics，记录 I2C 最近失败上下文、累计恢复/失败、FIFO overflow/malformed/empty/skipped 和初始化步骤；UART4/MP157 业务链路不发送该帧。
- 将有副作用的 FIFO_DATA 读取改为不可重放事务：失败后恢复 I2C2 并 flush FIFO，普通寄存器事务仍允许单次恢复重试。
- 修正 I2C 恢复成功判定和 heartbeat bit 14 语义：累计失败保留在诊断帧，heartbeat 只报告当前仍未恢复的事务状态。
- 新增真实/Mock 共用的 IMU 时间轴归一化模块，批次按上一已发布样本连续锚定并保留批内间隔，消除批次回推、fallback 切换和 I2C 恢复造成的时间戳回退/大空洞。
- `scripts/verify_f407_uart.ps1` 增加诊断帧、heartbeat 历史、时间戳异常详情和最大间隔位置输出。

### 修改文件

- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/sensors/icm42688_provider_c.*`
- `f407/sensor-hub/firmware/sensors/imu_timestamp_normalizer_c.*`
- `f407/sensor-hub/firmware/bsp/board_i2c.*`
- `f407/sensor-hub/firmware/bsp/stm32/board_i2c_stm32.c`
- `f407/sensor-hub/firmware/protocol/mcu_frame_builder_c.*`
- `scripts/verify_f407_uart.ps1`
- `common/protocol/mcu_protocol.h`
- `README.md`、`docs/` 和 `f407/sensor-hub/README.md`

### 验证结果

- 主机 CMake/Ninja 构建和 `ctest --test-dir build-gcc --output-on-failure` 通过，5/5 tests passed；新增覆盖时间轴批次衔接和 FIFO_DATA 失败不重放/flush。
- GNU ARM Release 构建通过：BIN 19368 B，text 19348 B、data 20 B、BSS 4424 B。
- 最终镜像经 COM6 ROM Bootloader 多次烧录并逐字节回读通过；一次下载阶段 `0xF9` 失败未计入成功，重新进入 Bootloader 后通过。
- 最终镜像独立烧录复位后的 10 秒回归通过：IMU `101.5 Hz`、磁力计 `19.9 Hz`、气压计 `10.0 Hz`，最大 IMU 时间戳间隔 10 ms，最终 `0x01A9`。
- 最终独立烧录复位后的 60 秒抓取通过：7988 帧，heartbeat 60、IMU 6086、磁力计 1187、气压计 595；频率约 `101.43/19.78/9.92 Hz`，最终 `0x01A9`，CRC/协议/载荷错误、时间戳回退和 UART4/USART1 TX overflow 均为 0。
- 长测诊断累计记录 I2C recovery 100、最终失败 1、FIFO overflow/malformed 各 10；这些事件均已自愈，最终 heartbeat 的 FIFO/I2C 当前错误位均为 0。该计数提示共享 I2C2 仍需小时级和物理层故障注入验证，不能解读为总线零瞬态错误。
- MP157 `/dev/ttySTM1` 最终复验：heartbeat/IMU/magnetometer/barometer 均 seen，`status_flags=425 (0x01A9)`；`command_ack_seen=true`、status 0、nonce `0x50494E47`。

### 后续 TODO

- 执行小时级持续运行、SDA 卡住和传感器断线/复接故障注入，重点观察共享 I2C2 累计恢复率。
- 只有出现真实 TX 队列溢出或更高持续带宽需求时，再评估 UART DMA。

## 2026-07-19 - ICM42688 FIFO and Interrupt-Driven UART TX Queues

### 本次完成

- 将 ICM42688 从读取最新 14 字节寄存器改为 byte-count FIFO：启用 accel/gyro/temperature、64 字节 watermark、stream-to-FIFO，以及 threshold/full INT1。
- 新增可变 FIFO 包解析：发布 16 字节 accel+gyro 包，跳过并在批次时间轴中保留 8 字节单传感器包，忽略消息头；最多一次读取 256 字节、发布 16 个样本。
- 对 overflow、畸形/截断包和输出容量不足执行 flush，并增加错误计数和 heartbeat 诊断位；后续实测删除了并发增长下会误报的 drain-stall 启发式判断。
- UART4 与 USART1 各新增 1024 字节固定 TX 队列，使用 `HAL_UART_Transmit_IT()` 和 TX complete callback 非阻塞 drain；队列空间不足时整帧拒绝。
- 保留 UART4 64 字节 RX 中断环形缓冲，并增加 TX/RX 丢弃、ICM 初始化和 I2C 运行期失败诊断。
- I2C 超时从 20 ms 调整为 30 ms；传感器初始化完成后清零诊断，避免 BMP390 备用地址探测的预期 NACK 污染运行期故障位。
- 修正 Mock IMU、MMC5603 和 BMP390 在长事务/重初始化后按旧 deadline 追赶补发的问题；Mock 输出统一到一个 10 ms 调度点，FIFO 错误/INT 噪声只更新状态而不再额外发送，避免同一 uptime 重复发布。
- 删除未被调用的 PC mock C++ `Icm42688Driver` 占位接口，新增 ADR-0016。

### 修改文件

- `docs/adr/0016-use-icm42688-fifo-and-interrupt-uart-tx-queues.md`
- `f407/sensor-hub/firmware/sensors/icm42688_fifo_parser_c.*`
- `f407/sensor-hub/firmware/sensors/icm42688_provider_c.*`
- `f407/sensor-hub/firmware/bsp/uart_tx_queue_c.*`
- `f407/sensor-hub/firmware/bsp/stm32/board_uart_stm32.c`
- `f407/sensor-hub/firmware/bsp/board_i2c.*`
- `f407/sensor-hub/firmware/bsp/stm32/board_i2c_stm32.c`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/tests/icm42688_fifo_parser_test.cpp`
- `f407/sensor-hub/tests/icm42688_provider_test.cpp`
- `f407/sensor-hub/tests/uart_tx_queue_test.cpp`
- `scripts/verify_f407_uart.ps1`
- `README.md`、`docs/` 和 `f407/sensor-hub/README.md` 的相关状态说明

### 验证结果

- `cmake -S f407/sensor-hub -B f407/sensor-hub/build` 和主机构建通过。
- `ctest --test-dir f407/sensor-hub/build -C Debug --output-on-failure --timeout 10` 通过，5/5 tests passed；新增 provider 测试覆盖 FIFO/INT 寄存器配置、批量读取时间轴、full/超量、畸形包、并发增长和伪中断路径。
- 当时复电前诊断固件 text 22108 B、data 20 B、BSS 4384 B，BIN 22128 B；最终板测镜像尺寸见上方“Final Board Validation”条目。
- 初步板端探索曾得到约 99 Hz、无时间戳回退的输出，但 heartbeat 为 `0x102E` 且 IMU 数值来自 Mock fallback；初始化阶段诊断表明 ICM42688 已不再 ACK，因此该结果不计为 FIFO 真实传感器验收。
- 复电前 22128 B 诊断版本烧录和逐字节回读通过；10 秒 COM6 抓取收到 1290 帧，IMU `97.8 Hz`、磁力计 `20.1 Hz`、气压计 `10.1 Hz`，CRC/协议/长度错误均为 0，IMU 时间戳无回退且最大间隔 51 ms，UART4/USART1 TX overflow 均为 false。heartbeat 为 `0x502E`，确认未经历整板断电的 ICM42688 仍不 ACK，因此 Mock fallback 不能计为 FIFO 验收。
- MP157 `/dev/ttySTM1` 原始抓取收到 4096 字节连续 MCU 帧；ARM Runtime 同时看到 heartbeat、IMU、磁力计、气压计，并收到 `command_ping -> command_ack`，ack status 为 0、nonce 为 `0x50494E47`。该结果验证了 UART4 TX 队列、RX 环形缓冲和 ack TX 路径。
- USART1/COM6 镜像抓取和 MP157 UART4 主链路均已验证；真实 FIFO 验收已在上方最终条目完成。

### 后续 TODO

- 整板复电、真实 FIFO 60 秒和独立复位验收已在上方最终条目完成；后续执行小时级运行和故障注入。

## 2026-07-19 - ICM42688 INT1 Trigger and Sensor Filtering

### 本次完成

- 将 PB12 从普通输入改为上升沿 EXTI12，ICM42688 配置为 INT1 脉冲、推挽、高有效，并将 100 Hz UI data-ready 路由到 INT1。
- EXTI ISR 只累计事件；主循环合并积压事件后执行 I2C burst read，避免在中断上下文调用阻塞 HAL I2C。
- 增加 heartbeat bit 7 `0x0080`；三颗传感器 ready 且近期 INT1 活动时为 `0x00A9`。
- 增加 250 ms INT1 超时、Mock IMU fallback、连续读取失败计数和 1 秒 ICM42688 重新初始化路径。
- ICM42688 内部 UI LPF 配置为约 25 Hz；新增定点一阶 IIR，IMU accel/gyro 与磁力计 `alpha=1/4`，IMU 温度和 BMP390 压力/温度 `alpha=1/8`。
- 新增滤波单元测试和 ADR-0015；MMC5603 继续 20 Hz 轮询，BMP390 继续 10 Hz 轮询。

### 修改文件

- `README.md`
- `docs/adr/0015-use-icm42688-int1-and-fixed-point-filtering.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/mcu_protocol.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `f407/sensor-hub/CMakeLists.txt`
- `f407/sensor-hub/README.md`
- `f407/sensor-hub/tests/sensor_sample_filter_test.cpp`
- `f407/sensor-hub/firmware/CMakeLists.txt`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/bsp/board_icm42688_gpio.c`
- `f407/sensor-hub/firmware/bsp/board_icm42688_gpio.h`
- `f407/sensor-hub/firmware/bsp/stm32/board_icm42688_gpio_stm32.c`
- `f407/sensor-hub/firmware/sensors/icm42688_provider_c.c`
- `f407/sensor-hub/firmware/sensors/sensor_sample_filter_c.c`
- `f407/sensor-hub/firmware/sensors/sensor_sample_filter_c.h`
- `f407/sensor-hub/firmware/stm32cube/atk_f407_sensorhub.ioc`
- `f407/sensor-hub/firmware/stm32cube/Core/Inc/stm32f4xx_it.h`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/gpio.c`
- `scripts/verify_f407_uart.ps1`

### 验证结果

- F407 ARM 交叉构建通过：20356 B Flash、2264 B RAM。
- 首次 COM6 下载在写入阶段收到异常响应 `0xF9`，该次未计为成功；重新进入 ROM Bootloader、重新擦写后，20356 字节固件逐字节回读校验通过，Bootloader version `0x31`、chip ID `0x0413`。
- 60 秒 COM6 抓取 7412 帧：heartbeat 60、IMU 5547、磁力计 1203、气压计 602。
- 实测频率：IMU `92.45 Hz`、磁力计 `20.05 Hz`、气压计 `10.03 Hz`；最终 heartbeat `0x00A9`，`icm42688_int1_active=true`。
- CRC、协议版本、载荷长度、时间戳和物理范围检查均通过。
- F407 PC CTest 2/2 通过；滤波测试覆盖首样本直通、正负阶跃和重置行为。
- 随后三次独立复位的 10 秒 INT1 回归均通过，最终 heartbeat 均为 `0x00A9`；三路滤波输出均持续变化，CRC 和协议错误均为 0。

### 后续 TODO

- ICM42688 FIFO 和 UART4/USART1 非阻塞 TX 队列已在后续同日条目实现；真实板端回归仍待完成。
- 根据姿态融合场景增加可配置滤波参数或独立 raw 数据通道。

## 2026-07-19 - F407 I2C2 Sensor Interaction Recovery

### 本次完成

- 扩展 COM6 验收脚本，完整解码 ICM42688 IMU、MMC5603 磁力计和 BMP390 气压计帧，并检查频率、CRC、协议字段、时间戳、ready/error 位和宽范围物理合理性。
- 30 秒持续抓包复现共享 I2C2 运行期锁死：最终 heartbeat 从 `0x0029` 变为 `0x0056`，三颗真实传感器同时停止上报。
- 在 I2C2 BSP 增加 HAL 去初始化、外设硬复位、最多 18 个 SCL 释放脉冲、STOP 和单次原事务重试。
- 新增 ADR-0014，记录恢复方案比较、边界和后续诊断 TODO。

### 修改文件

- `README.md`
- `docs/adr/0014-recover-i2c2-after-runtime-errors.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `f407/sensor-hub/README.md`
- `f407/sensor-hub/firmware/bsp/stm32/board_i2c_stm32.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Inc/i2c.h`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/i2c.c`
- `scripts/verify_f407_uart.ps1`

### 验证结果

- F407 ARM 交叉构建通过，当前固件使用 18808 B Flash、2096 B RAM。
- COM6 ROM Bootloader 烧录 18808 字节并逐字节回读校验通过，Bootloader version `0x31`、chip ID `0x0413`。
- 修复后 60 秒持续抓取 7532 帧：heartbeat 60、IMU 5666、磁力计 1204、气压计 602；最终 heartbeat 为 `0x0029`。
- 随后两次独立复位的 30 秒回归均通过，最终 heartbeat 均为 `0x0029`，三路数据未再次停发。
- 实测频率：IMU `94.43 Hz`、磁力计 `20.07 Hz`、气压计 `10.03 Hz`；CRC、协议版本、载荷长度、时间戳和数据合理性检查均通过。
- F407 PC mock CTest 使用 `-C Debug` 运行，1/1 通过。首次未带 `-C Debug` 的 CTest 调用因 Visual Studio 多配置生成器未选择配置而未运行测试，不属于用例失败。

### 后续 TODO

- 增加 I2C 自动恢复计数和最近 HAL error code，供 heartbeat 或独立诊断状态读取。
- 补充多次冷启动、小时级耐久和 SDA 卡住故障注入测试。

## 2026-07-18 - F407/MP157 Bidirectional Board Validation

### 本次完成

- 使用 F407 COM6 和 MP157 COM9 完成 UART4 PC10/PC11 与 USART3 PD9/PD8 的真实双向联调。
- 定位到 F407 UART4 RX 的零超时主循环轮询会在阻塞式传感器帧发送期间丢失连续 `command_ping` 字节。
- 将 UART4 RX 改为 HAL 单字节中断接收和固定 64 字节环形缓冲，保留主循环中的命令 decoder。
- 新增 `scripts/send_xmodem.ps1`，通过 MP157 console 调用板端 `rx` 完成 XMODEM-CRC 上传，并自动去除 128 字节分组产生的尾部填充。
- 新增 ADR-0013，记录 UART4 轮询、IRQ 环形缓冲和 DMA 三种方案的取舍。

### 修改文件

- `README.md`
- `docs/adr/0013-use-uart4-interrupt-rx-ring-buffer.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `f407/sensor-hub/README.md`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/bsp/board_uart.c`
- `f407/sensor-hub/firmware/bsp/board_uart.h`
- `f407/sensor-hub/firmware/bsp/stm32/board_uart_stm32.c`
- `mp157/outdoor-core-service/README.md`
- `scripts/send_xmodem.ps1`

### 验证结果

- F407 固件交叉构建通过，当前 Debug 固件使用 18156 B Flash、2096 B RAM。
- COM6 UART Bootloader 烧录通过：version `0x31`、chip ID `0x0413`、18156 字节写入和逐字节回读校验成功。
- COM6 诊断镜像 3 秒抓取 378 帧：heartbeat 3、IMU 285、CRC 错误 0；最终传感器 ready heartbeat 为 `status_flags=0x0029`。
- 修复前，MP157 连续发送三个合法 ping 均没有 ACK；修复后连续三个 ping 均返回 `command_ack`，`status=0`、nonce `0x50494E47`。
- 当前 ARM Runtime 通过 XMODEM-CRC 上传到 MP157；可执行文件 SHA256 为 `0fa01c5199a903b43bf60c09c4cefb11c122015233f4c72a059d3b2cd5b600c5`，与本地一致。
- MP157 Runtime 返回 0，`runtime_status.json` 确认 `heartbeat_seen=true`、`imu_seen=true`、`magnetometer_seen=true`、`barometer_seen=true`、`status_flags=41`、`command_ack_seen=true`、`command_ack_request_type=128`、`command_ack_status=0`、`command_ack_nonce=1346981447`。
- XMODEM 脚本使用 341 字节 NMEA 文件复测，板端文件大小和 SHA256 `ec8c232aacb1b829cb0cc1d514713e3e61f92fe2ecc5bb3d744cbca724bb6a6d` 与本地一致。
- MP157 本机构建通过，CTest 5/5 通过，`verify_runtime.ps1` 通过，ARM 交叉构建通过。
- F407 PC mock 构建通过，CTest 1/1 通过。

### 后续 TODO

- 增加 UART4 RX overflow/error 计数，并做长时间双向压力测试。
- 验证多次冷启动和 I2C 瞬时错误恢复稳定性。
- MP157 板端系统时间当前仍为 2020 年，需要后续配置 RTC/NTP 或启动校时。

## 2026-07-05 - MP157 APP Launcher Touch Start

### 本次完成

- `DashboardService` 新增轻量 fbdev/evdev APP launcher：启动时先绘制中央 `OUTDOOR AGENT` 图标/磁贴，点击后进入原有 framebuffer dashboard。
- 新增 `launcher_enabled`、`launcher_input_device` 和 `launcher_auto_start_seconds` 配置项，并提供 `--launcher`、`--no-launcher`、`--launcher-input-device`、`--launcher-auto-start-seconds` 命令行覆盖项。
- `config/outdoor_agent_app.conf` 默认开启 launcher，`config/runtime.conf` 默认关闭 launcher，避免影响 PC 文本验证路径。
- launcher 等待逻辑支持 `BTN_TOUCH`、`ABS_MT_TRACKING_ID` 和仅坐标上报的触摸帧；可点击区域扩大到 APP 图标/磁贴区域，便于 7 英寸屏手指操作。

### 修改文件

- `README.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/config/runtime.conf`
- `mp157/outdoor-core-service/config/outdoor_agent_app.conf`
- `mp157/outdoor-core-service/include/config/app_config.h`
- `mp157/outdoor-core-service/include/services/dashboard_service.h`
- `mp157/outdoor-core-service/src/config/config_loader.cpp`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`

### 验证结果

- `cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build` 通过。
- `cmake --build mp157/outdoor-core-service/build` 通过。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过，5/5 tests passed。
- `powershell -NoProfile -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过。
- `git diff --check` 通过。
- `cmake --build mp157/outdoor-core-service/build-arm` 通过。
- 通过 COM3 + XMODEM checksum 模式上传最终验证包到 MP157；板端解包后 `outdoor_core_runtime` SHA256 为 `0fa01c5199a903b43bf60c09c4cefb11c122015233f4c72a059d3b2cd5b600c5`，与本地 ARM 构建一致。
- 板端确认 Goodix 触摸设备节点为 `/dev/input/touchscreen0 -> event0`。
- 板端运行 `./scripts/run_outdoor_agent_app.sh --launcher-auto-start-seconds 2 --dashboard-refresh-count 1 --log-level info` 返回 `FINAL_AUTOSTART_DONE:0`；日志确认 `Launcher waiting for icon tap on /dev/input/touchscreen0`、`Launcher auto-start timeout reached` 和 `Dashboard rendered to framebuffer: /dev/fb0`。
- 板端重新执行无自动超时真实点击复测通过：串口日志出现 `Launcher icon tapped at x=530, y=292`，随后 `Dashboard rendered: runtime/dashboard.txt`、`Dashboard rendered to framebuffer: /dev/fb0`，最终 `TOUCH_RETEST_DONE:0`。

### 后续 TODO

- 后续若需要多 APP 或返回主页，再将 launcher 从 `DashboardService` 中拆分为独立 UI 状态机。
- 后续若扩展多个可点击控件，需要补充触摸坐标校准、屏幕旋转适配和 UI 状态机测试。

## 2026-07-05 - outdoor-agent APP Icon Display

### 本次完成

- 在 `outdoor-agent` framebuffer 主界面左上角新增明确的程序图标显示，图标采用轻量 fbdev primitive 绘制，不引入第三方 GUI 或图片依赖。
- 调整左侧标题区布局，使程序图标、`OUTDOOR / AGENT / APP` 文本都位于侧栏标题框内。
- 文本 dashboard 新增 `app_icon: outdoor-agent compass mark` 和 `app_icon_visible: true`，用于 PC 自动化验证主界面图标能力。
- `verify_runtime.ps1` 新增默认 dashboard 与 APP profile dashboard 的图标标记检查。
- 通过 COM3 将当前 ARM Runtime 和 APP 配置打包部署到 MP157，并完成 `/dev/fb0` UI 显示验证。

### 修改文件

- `README.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`

### 验证结果

- `cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build` 通过。
- `cmake --build mp157/outdoor-core-service/build` 通过。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过，5/5 tests passed。
- `powershell -NoProfile -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过。
- `git diff --check` 通过。
- `cmake --build mp157/outdoor-core-service/build-arm` 通过，当前 ARM Runtime SHA256 为 `f3fee04a5d86cdea7de7e24b81e4874f981d7ee3ab967438d1c16c88593ec812`。
- MP157 COM3 上板确认：`/dev/fb0` 存在，`virtual_size=1024,600`，`bits_per_pixel=16`。
- 通过 XMODEM checksum 模式上传验证包到 `/tmp/ai_outdoor_ui_pkg.tar.gz`；解压后的 `outdoor_core_runtime` 与本地 ARM Runtime SHA256 一致。
- 板端运行 `./outdoor_core_runtime --config config/outdoor_agent_app.conf --dashboard-refresh-count 2 --dashboard-refresh-interval-ms 300 --log-level info` 返回 `RUNTIME_EXIT:0`，日志确认 `Dashboard rendered to framebuffer: /dev/fb0` 输出 2 次。
- 板端运行 `./scripts/run_outdoor_agent_app.sh --dashboard-refresh-count 1 --dashboard-refresh-interval-ms 100 --log-level info` 返回 `APP_SCRIPT_EXIT:0`，确认脚本入口可写入 `/dev/fb0`。
- 板端 `runtime/dashboard.txt` 可见 `outdoor-agent`、`app_icon_visible: true`、`ai_agent_state: planned`、`source: file` 和 `source: icm20608_char`。
- `/dev/fb0` 前 64 KiB 样本 SHA256 为 `ee8558205ec95c0b10fd8ef06e9b756e70d78a4a5740ca80f7ea259293b9ba9d`，非零字节数 `58232`。

### 后续 TODO

- 后续如接入触摸或桌面环境，再评估是否需要外部 bitmap 图标资源。

## 2026-06-29 - outdoor-agent APP Process Entry

### 本次完成

- 新增 MP157 屏幕侧 `outdoor-agent` APP 配置 `config/outdoor_agent_app.conf`，默认以 `dashboard_output_mode = both` 同时输出文本 dashboard 和 `/dev/fb0` framebuffer。
- 新增 `scripts/run_outdoor_agent_app.sh`，板端可直接启动 `./outdoor_core_runtime --config config/outdoor_agent_app.conf`。
- 将 `dashboard_refresh_count = 0` 定义为 dashboard 常驻刷新模式，使 framebuffer 仪表盘可以作为进程持续显示。
- 保留 `config/runtime.conf` 的文本/单次刷新默认行为，避免影响 PC 开发和自动化验证。
- `verify_runtime.ps1` 新增 APP 配置烟测：使用 APP 配置，但覆盖为文本输出和单次刷新，验证配置可加载且不会访问 PC 环境不存在的 `/dev/fb0`。

### 修改文件

- `README.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/config/outdoor_agent_app.conf`
- `mp157/outdoor-core-service/scripts/run_outdoor_agent_app.sh`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`

### 验证结果

- `cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build`
- `cmake --build mp157/outdoor-core-service/build`
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure`
- `powershell -NoProfile -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- 本轮按任务要求未进行 MP157 `/dev/fb0` 上板验证。

### 后续 TODO

- 上板运行 `./scripts/run_outdoor_agent_app.sh`，确认 7 英寸 RGB 屏持续显示 `outdoor-agent`。
- 将 Runtime Manager 从顺序模型演进为可并发运行的服务模型，使 GNSS/MCU/Board IMU 长期采集和 dashboard 常驻刷新同时工作。

## 2026-06-29 - F407 I2C Sensor Revalidation Snapshot

### 本次完成

- F407 I2C2 初始化前新增 GPIO open-drain 总线恢复，释放 SDA/SCL 并发送 9 个 SCL 脉冲和 STOP 条件。
- F407 初始化顺序调整为 GPIO、USART1、UART4、I2C2，I2C 异常时仍保留 UART 诊断能力。
- F407 上行协议帧同步镜像到 USART1，便于通过 COM6 抓取 heartbeat、IMU、磁力计和气压计帧；UART4 仍作为 MP157 应用链路。
- `flash_f407_uart.ps1` 在写入和回读校验后使用 STM32 ROM Bootloader `GO 0x08000000` 启动应用，并释放到运行态 DTR/RTS 电平。
- ICM42688 provider 支持 `0x69` 优先、`0x68` fallback 的地址探测。
- BMP390 provider 优先探测 `0x76`，再探测 `0x77`，兼容 Bosch BMP3/BMP390 chip-id，并在轮询路径直接读取最新补偿值。

### 验证结果

- `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_f407.ps1` 通过，当前固件约 Flash `16760 B`、RAM `2024 B`。
- `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM6` 通过，Bootloader version `0x31`，chip ID `0x0413`，逐字节回读校验成功，并通过 `GO 0x08000000` 启动应用。
- COM6 USART1 协议镜像抓包确认应用态运行和协议 CRC 正常。
- 在 MMC5603 断开、ICM42688 按 PB10/PB11/AD0=3.3V 接线的状态下，10 秒抓包结果为 `bytes_read=34297`、`frames=1014`、`heartbeat=10`、`imu=1004`、`magnetometer=0`、`barometer=0`、`crc_bad=0`。
- 最后 heartbeat `status_flags=0x0056`，表示当前 ICM42688 仍为 Mock fallback/error，MMC5603 因断开为 error，BMP390 仍未 ACK/未 ready。
- 本次未把 ICM42688 或 BMP390 记录为通过；当前提交只固定诊断能力、容错改动和真实故障状态。

### 后续 TODO

- 只保留 ICM42688 一个 I2C2 设备时，继续排查 VCC/GND、SCL/SDA、AD0、电平上拉和模块本体，目标是恢复 heartbeat `0x0001`。
- BMP390 需要确认 VDD/VDDIO、CSB、SDO 和 I2C 上拉后再复测，目标是出现 `sensor_barometer` 帧且 heartbeat `0x0020` 置位、`0x0040` 清除。
- MMC5603 当前已按排障需要断开，恢复接线后需复测 `sensor_magnetometer` 20 Hz 帧和 ready 位。

## 2026-06-28 - MP157 SD Card Runtime Storage

### 本次完成

- 新增 MP157 Runtime storage 配置，默认关闭，板端可通过 `--storage-root /run/media/mmcblk1p1/outdoor-agent` 启用。
- 启用 storage 后自动创建 `status/`、`dashboard/`、`logs/`、`data/` 和 `captures/` 目录。
- 将 `runtime_status.json`、文本 dashboard 和 Runtime 日志重定向到 storage root。
- `runtime_status.json` 新增 `storage` 节点，记录 storage 是否启用、根目录、输出路径和初始化错误。
- 新增 ADR-0012，记录当前采用 SD 卡文件/目录型持久化的原因。

### 修改文件

- `mp157/outdoor-core-service/include/config/app_config.h`
- `mp157/outdoor-core-service/src/config/config_loader.cpp`
- `mp157/outdoor-core-service/include/log/logger.h`
- `mp157/outdoor-core-service/src/log/logger.cpp`
- `mp157/outdoor-core-service/include/runtime/runtime_status.h`
- `mp157/outdoor-core-service/src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/config/runtime.conf`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `docs/adr/0012-use-sd-card-file-storage.md`
- README、设计文档、Stage 1 计划和 changelog

### 验证结果

- `cmake --build mp157/outdoor-core-service/build` 通过。
- `powershell -NoProfile -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过，并覆盖 storage 模式目录和文件生成。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过，5/5 tests passed。
- MP157 COM3 上板检查确认：`/mnt/sdcard` 当前不存在，SD 卡实际挂载点为 `/run/media/mmcblk1p1`，vfat、rw。
- 已通过串口部署当前 ARM Runtime 包，包 SHA256 为 `254499abc4d5fa961cd31c04b4ec05fcc99974c3785fa3a96939fa02d2895bb4`。
- 板端执行 `--storage-root /run/media/mmcblk1p1/outdoor-agent` 返回 `EXIT:0`，并确认 `STATUS_OK`、`DASHBOARD_OK`、`LOG_OK`。
- 板端 `runtime_status.json` 中 `storage.enabled=true`、`storage.root_path=/run/media/mmcblk1p1/outdoor-agent`、`storage.last_error=""`。

### 后续 TODO

- 增加 GNSS、IMU、磁力计、气压计历史数据记录服务。
- 增加日志轮转或按日期分文件，避免长期运行占满 SD 卡。

## 2026-06-26 - F407 BMP390 Software Integration

### 本次完成

- 参考本地 BMP390 模块资料和 STM32 示例工程，将 Bosch BMP3 Sensor API v2.0.5 集成到 F407 固件。
- 新增 BMP390 I2C provider，复用 PB10/PB11 I2C2，自动探测 `0x77` 和 `0x76`。
- 配置温度/气压 2x 过采样、IIR 系数 3、25 Hz ODR，并以 10 Hz 发布 `sensor_barometer`。
- 扩展 MP157 Runtime、状态 JSON、文本仪表盘和 frame decoder 的气压计解析。
- 新增 ADR-0011，记录第三方驱动选型。

### 修改文件

- `common/protocol/barometer_payload.h`
- `f407/sensor-hub/firmware/sensors/bmp390_provider_c.*`
- `f407/sensor-hub/firmware/third_party/bosch_bmp3/`
- F407 frame builder、Sensor Hub app 和固件 CMake
- MP157 MCU parser/status、状态发布、仪表盘和测试
- 协议、设计、阶段计划和模块 README

### 验证结果

- F407 ARM 构建通过：Flash 16740 B，RAM 2024 B。
- MP157 本机构建通过，CTest 5/5 通过，ARM 交叉构建通过。
- `tools/frame_decoder` 构建通过，`git diff --check` 通过。
- 按任务要求未烧录固件，也未进行 BMP390 上板验证。

### 后续 TODO

- 按接线方案上板，确认实际地址、heartbeat ready/error 位和 `barometer_seen=true`。
- 检查室内环境下气压和温度补偿值，并验证 10 Hz 长时间稳定性。
- 后续增加可配置海平面参考气压，由 MP157 计算相对海拔。

## 2026-06-25 - F407 MMC5603 Integration and Board Validation

### 本次完成

- 在 F407 I2C2 总线接入 MMC5603，默认地址 `0x30`。
- 实现软件复位、产品 ID 检查、SET/RESET 线圈脉冲、单次测量和 20-bit XYZ 解码。
- 新增 `sensor_magnetometer` 协议帧，MP157 parser、状态 JSON 和仪表盘 MAG 指标同步适配。

### 验证结果

- MP157 Runtime 本机构建通过，CTest 5/5 通过。
- F407 PC mock 构建和测试通过。
- F407 ARM 构建通过，Flash 11892 B、RAM 1936 B。
- COM6 UART Bootloader 烧录和逐字节回读校验通过。
- MP157 `/dev/ttySTM1` 五秒抓取 100 个磁力计帧，频率 `19.8 Hz`。
- 平均磁场约 `(-35.60, -25.18, 18.16) μT`，合成强度约 `47.24 μT`。
- heartbeat `0x000E` 表示 MMC5603 ready；ICM42688 当前仍为 Mock fallback/error。

### 后续 TODO

- 完成 MMC5603 硬铁、软铁和倾斜补偿后再输出真实罗盘航向。
- 单独排查当前 ICM42688 I2C 初始化失败。

## 2026-06-22 - Reference-style outdoor-agent Dashboard Layout

### 本次完成

- 按用户提供的仪表盘示意图升级 `outdoor-agent` framebuffer 视觉布局。
- 新增轻量 fbdev 绘制 primitive：矩形描边、直线/粗线、圆、圆弧、仪表盘刻度、地图网格、条形图等。
- 7 英寸 RGB 屏布局调整为：左侧导航栏、顶部状态栏、方向罗盘、大速度表、位置地图、温度面板、光照展示区和底部设备状态栏。
- 继续保持无第三方 GUI 依赖，沿用 Linux fbdev `/dev/fb0` 直接绘制路径。

### 修改文件

- `README.md`
- `docs/adr/0010-use-nmea-uart5-text-dashboard.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`

### 验证结果

- `cmake --build mp157/outdoor-core-service/build` 通过。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过，5/5 tests passed。
- `powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过。
- `cmake --build mp157/outdoor-core-service/build-arm` 通过。
- `git diff --check` 通过。
- MP157 COM3 上板验证通过：通过 XMODEM 传输当前 ARM runtime 包，包大小 `91737` bytes，SHA256 校验为 `f32a2aa049a4be72535894ef1922eb7e81a79aae3f8f00319ddb10a62d546086`。
- 板端运行 `./outdoor_core_runtime --config config/runtime.conf --board-imu --dashboard-output-mode both --dashboard-framebuffer-device /dev/fb0 --dashboard-refresh-count 2 --dashboard-refresh-interval-ms 300`，日志确认 `Dashboard rendered to framebuffer: /dev/fb0` 输出 2 次。
- 板端 `runtime/dashboard.txt` 验证可见 `outdoor-agent`、`ai_agent_state: planned`、`[AI Local Agent]`、`source: file` 和 `source: icm20608_char`。

### 后续 TODO

- 光照、空气质量、电池、信号等展示项当前仍是 UI 占位/演示指标，后续需要接入真实传感器或系统状态来源。
- 当前仍是轻量 fbdev 原型，不包含触摸交互、窗口系统或完整 GUI 控件。

## 2026-06-21 - outdoor-agent 7-inch RGB Framebuffer Dashboard

### 本次完成

- 将 `DashboardService` 从文本仪表盘原型扩展为 `outdoor-agent` APP 渲染入口。
- 新增 `dashboard_output_mode = text | framebuffer | both`，默认仍为文本模式，便于 PC 自动化验证。
- 新增 `dashboard_framebuffer_device = /dev/fb0`，支持直接写入 MP157 7 英寸 RGB 屏 framebuffer。
- 新增 `dashboard_refresh_count` 和 `dashboard_refresh_interval_ms`，形成可控的屏幕刷新循环。
- 屏幕界面显示 `outdoor-agent` 标题、GNSS、F407 Sensor Hub、MP157 Board IMU 和 `AI LOCAL AGENT: PLANNED` 预留区。
- 同步更新 README、设计文档、Stage 1 计划、ADR-0010、changelog 和模块 README。

### 修改文件

- `README.md`
- `docs/adr/0010-use-nmea-uart5-text-dashboard.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/config/runtime.conf`
- `mp157/outdoor-core-service/include/config/app_config.h`
- `mp157/outdoor-core-service/include/services/dashboard_service.h`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `mp157/outdoor-core-service/src/config/config_loader.cpp`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`

### 验证结果

- `cmake --build mp157/outdoor-core-service/build` 通过。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过，5/5 tests passed。
- `powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过。
- `cmake --build mp157/outdoor-core-service/build-arm` 通过。
- MP157 COM3 上板验证通过：`/dev/fb0` 存在，`/sys/class/graphics/fb0/virtual_size=1024,600`，`bits_per_pixel=16`。
- 通过 XMODEM 向 MP157 传输当前 ARM runtime 包，包大小 `80025` bytes，SHA256 校验为 `c27907d312d95e1278537e8d8bff20555d81a0d05be8f9c8848d70d6859079c1`。
- 板端运行 `./outdoor_core_runtime --config config/runtime.conf --board-imu --dashboard-output-mode both --dashboard-framebuffer-device /dev/fb0 --dashboard-refresh-count 3 --dashboard-refresh-interval-ms 500`，日志确认 `Dashboard rendered: runtime/dashboard.txt` 和 `Dashboard rendered to framebuffer: /dev/fb0` 各输出 3 次。
- 板端 `runtime/dashboard.txt` 验证可见 `outdoor-agent`、`ai_agent_state: planned`、`[AI Local Agent]` 和 `source: icm20608_char`。

### 后续 TODO

- UBLOX-M10 UART5 真实接线和 NMEA 采集仍待上板验证。
- `outdoor-agent` 当前是轻量 fbdev 仪表盘，不包含触摸交互、窗口系统或真实 AI Agent 本地推理。
- 后续可把 Runtime 从一次性服务编排演进为常驻进程，并让 dashboard 从实时状态模型持续刷新。

## 2026-06-21 - MP157 UART5 UBLOX-M10 Software Path and Dashboard Prototype

### 本次完成

- 新增 `GnssStatus`，将 GNSS source、fix、语句统计和 last error 纳入 Runtime 状态。
- 将 `NmeaParser` 从 RMC/GGA 扩展到 RMC/GGA/VTG/GSA/GSV，支持位置、海拔、速度、航向、卫星数、fix type 和 DOP。
- 新增 `GnssSerialService`，用于在 MP157 Linux 目标上从 UART5 NMEA 串口读取 UBLOX-M10 数据；默认设备 `/dev/ttySTM2`，默认波特率 9600。
- 新增配置项：`gnss_input_mode`、`gnss_serial_device`、`gnss_serial_baud`、`gnss_serial_capture_seconds`。
- `runtime_status.json` 新增 `gnss` 字段。
- 新增 `DashboardService`，默认生成 `runtime/dashboard.txt` 文本仪表盘原型。
- 新增配置项：`dashboard_enabled`、`dashboard_output_path`。
- 新增 NMEA parser 单元测试，覆盖 RMC/GGA/VTG/GSA/GSV 和错误 checksum。
- 新增 ADR-0010，记录先做 UART5 NMEA 输入和文本仪表盘原型的方案取舍。

### 修改文件

- `README.md`
- `docs/adr/0010-use-nmea-uart5-text-dashboard.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/CMakeLists.txt`
- `mp157/outdoor-core-service/config/runtime.conf`
- `mp157/outdoor-core-service/include/gnss/*`
- `mp157/outdoor-core-service/include/services/gnss_serial_service.h`
- `mp157/outdoor-core-service/include/services/dashboard_service.h`
- `mp157/outdoor-core-service/src/gnss/*`
- `mp157/outdoor-core-service/src/services/gnss_serial_service.cpp`
- `mp157/outdoor-core-service/src/services/dashboard_service.cpp`
- `mp157/outdoor-core-service/tests/nmea_parser_tests.cpp`

### 验证结果

- `cmake --build mp157/outdoor-core-service/build` 通过。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过，5/5 tests passed。
- `powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过。
- 本机默认 file GNSS 模式生成 `runtime/runtime_status_gnss_verify.json` 和 `runtime/dashboard_gnss_verify.txt`，dashboard 中可见 GNSS 经纬度、速度、卫星数、DOP、F407 IMU 和 MP157 board IMU 区块。

### 后续 TODO

- 上板确认 MP157 UART5 对应 Linux 设备节点是否为 `/dev/ttySTM2`。
- 上板确认 UBLOX-M10 当前输出波特率和 NMEA 语句集合。
- 完成 UART5 TX/RX/GND 接线和真实 NMEA 采集验证。
- framebuffer 屏幕显示已在后续 `outdoor-agent` 条目完成；TUI/完整 GUI 可后续评估。

## 2026-06-20 - MP157 to F407 Ping Ack Prototype

### 本次完成

- 在 MCU 协议中新增 `command_ping` 和 `command_ack`，复用现有二进制帧头、payload length 和 CRC16/MODBUS。
- MP157 Runtime 新增 `mcu_command = none | ping` 配置项和 `--mcu-command none|ping` 命令行参数。
- MP157 serial 模式将 `/dev/ttySTM1` 以读写方式打开，启动采集前发送 `command_ping`，随后继续读取 F407 heartbeat、IMU 和 ack 帧。
- F407 固件新增 UART4 RX 非阻塞字节读取 BSP、命令帧 decoder 和 ping ack 回包逻辑。
- `runtime_status.json` 的 `mcu` 字段新增 `command_ack_seen`、`command_ack_request_type`、`command_ack_status` 和 `command_ack_nonce`。

### 修改文件

- `common/protocol/mcu_protocol.h`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/mcu_protocol.md`
- `docs/project_design.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `f407/sensor-hub/README.md`
- `f407/sensor-hub/firmware/app/sensor_hub_app.c`
- `f407/sensor-hub/firmware/bsp/board_uart.c`
- `f407/sensor-hub/firmware/bsp/board_uart.h`
- `f407/sensor-hub/firmware/bsp/stm32/board_uart_stm32.c`
- `f407/sensor-hub/firmware/protocol/mcu_command_decoder_c.c`
- `f407/sensor-hub/firmware/protocol/mcu_command_decoder_c.h`
- `f407/sensor-hub/firmware/protocol/mcu_frame_builder_c.c`
- `f407/sensor-hub/firmware/protocol/mcu_frame_builder_c.h`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/include/mcu/mcu_command.h`
- `mp157/outdoor-core-service/src/mcu/mcu_command.cpp`
- `mp157/outdoor-core-service/src/services/mcu_serial_service.cpp`

### 验证结果

- `powershell -ExecutionPolicy Bypass -File scripts\build_f407.ps1` 通过。
- `powershell -ExecutionPolicy Bypass -File scripts\flash_f407_uart.ps1 -PortName COM6` 通过，刷写 `sensor_hub.bin` 11100 B，Bootloader version `0x31`，chip ID `0x0413`，逐字节回读校验成功。
- `cmake --build mp157/outdoor-core-service/build` 通过。
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure` 通过。
- `powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1` 通过。
- `cmake --build mp157/outdoor-core-service/build-arm` 通过。
- 在 MP157 shell 中执行 raw ping 验证：向 `/dev/ttySTM1` 写入 `a5 5a 01 80 01 00 04 00 47 4e 49 50 c3 43`。
- F407 回包首帧为 `a5 5a 01 81 f1 99 08 00 80 00 00 00 47 4e 49 50 f1 59`，确认 `command_ack`、`request_type=0x80`、`status=0`、nonce `0x50494E47`。

### 后续 TODO

- 使用更可靠的部署方式在 MP157 上运行新版 ARM Runtime `--mcu-command ping`，确认 `command_ack_seen=true`、`command_ack_status=0`。
- 将当前 ping/ack 原型扩展为版本化命令集前，需要先定义命令权限、错误码、重试和超时策略。
- 后续根据长期运行结果评估 UART4 DMA/环形缓冲。

### 问题与解决

- 刷写后第一次从 MP157 `/dev/ttySTM1` 抓包为 0 字节，重新通过 COM6 DTR/RTS 组合触发 F407 应用态复位后恢复，随后可连续抓到 `a5 5a` IMU 帧。
- 通过 COM3 串口 console 上传新版 ARM Runtime 压缩包时，多次出现 `ttySTM0 input overrun`，板端 SHA256 校验失败；降速后仍有字符丢失。当前先用 MP157 shell raw ping 验证 F407 固件命令解析和物理双向链路，Runtime 板端复测留到更可靠的部署路径。

## 2026-06-20 - F407 UART4 to MP157 USART3 Board Validation

### 本次完成

- 将 F407 与 MP157 的对通方案从临时 USART1 旁听方案调整为专用 UART4 方案。
- F407 USART1 PA9/PA10 继续保留为 USB-UART / STM32 ROM Bootloader 下载口；当前 F407 板卡通过 `COM6` 烧录。
- 新增 F407 UART4 PC10/PC11 初始化，`board_uart_send_bytes()` 改为通过 `huart4` 发送 Sensor Hub heartbeat 和 IMU 二进制帧。
- 当前板间接线为：`F407 PC10 UART4_TX -> MP157 PD9 USART3_RX`，`F407 PC11 UART4_RX <- MP157 PD8 USART3_TX`，两板 GND 共地。
- MP157 console 使用 `COM3`，确认 `/dev/ttySTM1` 存在，对应 USART3。
- 在 MP157 上通过 `stty` + `dd | hexdump` 抓取 `/dev/ttySTM1`，确认可见连续 `A5 5A` MCU 帧头。
- 部署当前 ARM 版 `outdoor_core_runtime` 到 `/tmp/ai_outdoor_runtime_serial`，运行 serial 模式完成 F407 -> MP157 最小闭环验证。

### 修改文件

- `README.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/mcu_protocol.md`
- `docs/project_design.md`
- `docs/stage1_bringup_plan.md`
- `docs/stage1_plan.md`
- `f407/sensor-hub/README.md`
- `f407/sensor-hub/firmware/bsp/stm32/board_uart_stm32.c`
- `f407/sensor-hub/firmware/stm32cube/atk_f407_sensorhub.ioc`
- `f407/sensor-hub/firmware/stm32cube/Core/Inc/usart.h`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/main.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/stm32f4xx_hal_msp.c`
- `f407/sensor-hub/firmware/stm32cube/Core/Src/usart.c`
- `mp157/outdoor-core-service/README.md`

### 验证结果

- `powershell -ExecutionPolicy Bypass -File scripts/build_f407.ps1` 通过，生成 `sensor_hub.bin`，大小 10320 B。
- `powershell -ExecutionPolicy Bypass -File scripts/flash_f407_uart.ps1 -PortName COM6` 通过，Bootloader version `0x31`，chip ID `0x0413`，逐字节回读校验成功。
- MP157 `/dev/ttySTM1` 5 秒内读取 512 字节样例，`hexdump` 中可见连续 `a5 5a` MCU 帧头。
- MP157 Runtime serial 模式运行 5 秒后，`runtime/runtime_status.json` 中 `mcu.heartbeat_seen=true`、`mcu.imu_seen=true`、`mcu.status_flags=1`、`imu.seen=true`。
- 样例末帧：`last_sequence=16835`，`imu.accel=(-0.577, -0.043, 0.817) g`，`imu.gyro=(-0.610, -0.274, 0.000) dps`，`imu.temperature_celsius=24.970`。

### 问题与解决

- 第一次 MP157 Runtime 部署包只包含可执行文件和配置，缺少 `data/nmea_sample.txt`，导致 `gnss_replay_service` 启动失败，MCU serial 服务未运行。
- 解决：重新打包部署 `outdoor_core_runtime`、`config/runtime.conf`、`data/nmea_sample.txt` 和 `data/mcu_mock_frames.txt`，板端 SHA256 校验通过后 Runtime serial 验证成功。
- 第一次 `/dev/ttySTM1` 抓包未读到数据；通过 `COM6` 触发 F407 应用态复位后再次抓包成功。

### 后续 TODO

- 实现 MP157 -> F407 下行命令协议，当前 UART4 RX 已初始化并接线，但固件尚未解析命令。
- 评估 UART4 发送从阻塞式 `HAL_UART_Transmit()` 演进到 DMA/环形缓冲。
- 补充长期运行稳定性验证，观察 `/dev/ttySTM1` 连续读取、CRC 错误率和 Runtime 状态刷新。

## 2026-06-20 - MP157 USART3 MCU Serial Input Software Path

### 本次完成

- 按 MP157 USART3 方案新增 F407 -> MP157 live serial 软件路径，默认设备为 `/dev/ttySTM1`。
- 新增 `mcu_input_mode = mock_file | serial`，默认保持 `mock_file`，避免未接线时影响 PC 开发和自动化测试。
- 新增 `mcu_serial_device`、`mcu_serial_baud`、`mcu_serial_capture_seconds` 配置项，默认 `/dev/ttySTM1`、`115200`、`5`。
- 新增 `McuFrameStreamDecoder`，负责从连续串口字节流中重组 MCU 二进制帧，再复用现有 `McuFrameParser` 和 `ImuPayloadParser`。
- 新增 `McuSerialService`，Linux 目标上使用 termios 配置 raw、115200 8N1、无硬件流控读取 `/dev/ttySTM1`。
- 新增 decoder 单元测试，覆盖前导噪声、分片帧和连续帧。
- 文档最初记录临时单向连线：`F407 PA9 USART1_TX -> MP157 PD9 USART3_RX`，以及 `F407 GND -> MP157 GND`；该方案已被后续 UART4 专用板间通信方案替代。

### 修改文件

- `README.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/CMakeLists.txt`
- `mp157/outdoor-core-service/README.md`
- `mp157/outdoor-core-service/config/runtime.conf`
- `mp157/outdoor-core-service/include/config/app_config.h`
- `mp157/outdoor-core-service/include/mcu/mcu_frame_stream_decoder.h`
- `mp157/outdoor-core-service/include/services/mcu_serial_service.h`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `mp157/outdoor-core-service/src/config/config_loader.cpp`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/src/mcu/mcu_frame_stream_decoder.cpp`
- `mp157/outdoor-core-service/src/services/mcu_serial_service.cpp`
- `mp157/outdoor-core-service/tests/mcu_frame_stream_decoder_tests.cpp`

### 验证结果

- 本条记录创建时暂未执行真实 F407 -> MP157 上板验证；后续已在 “F407 UART4 to MP157 USART3 Board Validation” 中完成。

### 后续 TODO

- 临时 PA9 方案已废弃；后续使用 `F407 UART4 PC10/PC11 <-> MP157 USART3 PD9/PD8`。
- `stty` + `hexdump` 和 Runtime serial 模式验证已完成。

## 2026-06-19 - MP157 Onboard ICM20608 Service

### 本次完成

- 参考 正点原子 STM32MP157 ICM20608 字符设备和 IIO 示例，新增 MP157 Runtime 板载 ICM20608 读取路径。
- 通过 MP157 串口 console 确认当前板上系统为 Linux `5.4.31-g886e225be` / OpenSTLinux，设备树包含 `/soc/spi@44004000/icm20608@0`，SPI 设备枚举为 `spi0.0`。
- 通过 `modprobe icm20608` 确认当前镜像加载 `/lib/modules/5.4.31-g886e225be/kernel/drivers/char/icm20608.ko`，内核打印 `ICM20608 ID = 0XAE`。
- 确认当前板上默认可用入口为 `/dev/icm20608` 字符设备，IIO 目录当前只有 ADC/DAC。
- 新增 `Icm20608CharReader`，读取 `/dev/icm20608` 返回的 7 个 `signed int`，并兼容正点原子字符设备驱动 `read()` 返回 0 的行为。
- 新增 `Icm20608IioReader`，从 IIO sysfs 读取 `in_accel_*_raw`、`in_anglvel_*_raw` 和 `in_temp_*`，并按示例公式换算 g、dps 和摄氏度。
- 新增 `Icm20608Service`，通过 Runtime Service 生命周期接入主程序。
- 新增 `BoardImuStatus`，并在 `runtime_status.json` 中输出独立 `board_imu` 字段，避免与 F407 MCU 协议帧的 `imu` 字段混淆。
- 新增配置项：`board_imu_enabled`、`board_imu_source`、`board_imu_device_path`、`board_imu_iio_path`、`board_imu_sample_count`、`board_imu_sample_interval_ms`。
- 已完成近期计划调整后的 MP157 板载 ICM20608 上板验证，下一步继续 F407 -> MP157 串口对通。
- 新增 ADR-0009，记录“优先使用 MP157 板载 ICM20608 做主控侧验证”的方案取舍。

### 修改文件

- `README.md`
- `docs/adr/0009-use-mp157-icm20608-iio-first.md`
- `docs/changelog.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/repo_structure.md`
- `docs/stage1_plan.md`
- `mp157/outdoor-core-service/CMakeLists.txt`
- `mp157/outdoor-core-service/config/runtime.conf`
- `mp157/outdoor-core-service/include/config/app_config.h`
- `mp157/outdoor-core-service/include/runtime/runtime_manager.h`
- `mp157/outdoor-core-service/include/runtime/runtime_status.h`
- `mp157/outdoor-core-service/include/sensors/`
- `mp157/outdoor-core-service/include/services/icm20608_service.h`
- `mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- `mp157/outdoor-core-service/src/config/config_loader.cpp`
- `mp157/outdoor-core-service/src/ipc/status_publisher.cpp`
- `mp157/outdoor-core-service/src/main.cpp`
- `mp157/outdoor-core-service/src/runtime/runtime_manager.cpp`
- `mp157/outdoor-core-service/src/sensors/`
- `mp157/outdoor-core-service/src/services/icm20608_service.cpp`
- `mp157/outdoor-core-service/tests/icm20608_char_reader_tests.cpp`
- `mp157/outdoor-core-service/tests/icm20608_iio_reader_tests.cpp`

### 验证结果

- 已执行：

```powershell
cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build
cmake --build mp157/outdoor-core-service/build
ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure
powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1
```

- CMake 配置和构建通过。
- CTest 通过：`mcu_protocol_tests`、`icm20608_iio_reader_tests`、`icm20608_char_reader_tests`。
- Runtime 验证脚本通过，默认配置下 `board_imu.enabled=false`，默认 source 为 `icm20608_char`。
- 已执行 fake-IIO Runtime 冒烟验证：使用临时 IIO sysfs 目录运行 `--board-imu`，状态文件中 `board_imu.enabled=true`、`board_imu.seen=true`、`sample_count=2`。
- 首次 Windows 构建因残留并发编译进程触发 MSVC Debug PDB `C1041`，已通过清理残留进程并为 MSVC 增加 `/FS` 解决。
- 已执行 MP157 串口 console 检查：`modprobe icm20608` 成功，内核打印 `ICM20608 ID = 0XAE`。
- 已通过串口临时传输并运行 正点原子 `22_spi/icm20608App`，确认 `/dev/icm20608` 可连续读取真实数据；静置样例中 Z 轴约 `0.97g`，温度约 `39.5°C`。
- 临时传输文件 `/tmp/icm20608App` 和 `/tmp/icm20608App.b64` 已清理。
- 已确认 `F:\BaiduNetdiskDownload\【正点原子】STM32MP157开发板（A盘）-基础资料\05、开发工具\01、交叉编译器` 中的工具链包面向 Linux host，当前 Windows PowerShell 环境不能直接原生使用；本次改用 Windows host `arm-none-linux-gnueabihf-g++ 9.2.1`。
- 已交叉编译项目自有 `outdoor_core_runtime` 为 ARMv7 hard-float ELF，并通过 CH340 串口 base64/tar 包部署到 `/tmp/ai_outdoor_runtime`；上传包 SHA256 校验一致。
- 已在 MP157 板上运行：

```bash
cd /tmp/ai_outdoor_runtime
modprobe icm20608
./outdoor_core_runtime --config config/runtime.conf --board-imu --board-imu-source char_device --board-imu-device-path /dev/icm20608 --board-imu-samples 5 --log-level info
```

- 验证结果：程序 `EXIT:0`，`runtime/runtime_status.json` 中 `board_imu.enabled=true`、`board_imu.seen=true`、`source=icm20608_char`、`sample_count=5`、`last_error=""`；静置样例 `accel_z_g=0.983`、`gyro_x_dps=-0.671`、`gyro_y_dps=0.549`、`temperature_celsius=36.521`。

### 后续 TODO

- 将当前手动 ARMv7 交叉编译和串口部署流程沉淀为脚本或独立部署文档。
- 完成 MP157 板载 ICM20608 验证后，再推进 F407 -> MP157 真实串口输入源。

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
- 后续实际改为 UART4 专用通信口，并已完成 `F407 PC10 UART4_TX -> MP157 PD9 USART3_RX` 板间验证。
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
- 当时尚未执行真实串口抓包和 MP157 板间联调；后续已通过 UART4/USART3 完成。

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
