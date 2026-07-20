# ADR-0028: 使用恢复初始状态的 Stage 2 板端验收脚本

## 状态

Accepted

## 背景

Stage 2.1 已完成 Unix domain socket 状态查询，Stage 2.2 已完成 systemd Runtime 托管，Stage 2.3 已完成明确不执行真实推理的 Local Agent mock/terminal。部署脚本能够选择性 enable/start Runtime，并在启动后做一次状态查询，但还不能独立证明以下板端生命周期：

- stale socket 清理、活跃 socket 冲突保护、真实 client/server 查询和退出 unlink；
- Runtime 目录/socket 权限与连续查询；
- Agent terminal 无 context/显式 Runtime context 两条路径；
- SIGTERM 受控退出、socket 清理、再次启动；
- 主进程异常退出后的 systemd 自动恢复。

若后续联调只依靠零散手工命令，容易遗漏步骤、丢失证据，或在失败后留下与联调前不同的 enable/active 状态。当前任务又明确要求不在本轮访问 COM3/COM9、复位、上下电或执行板端交互，因此需要先把验收逻辑开发并在主机侧验证契约，真实执行留待用户统一触发。

## 可选方案

### 方案 A: 仅保留 README 手工命令

优点：

- 不增加脚本代码。

缺点：

- 步骤顺序、超时、失败处理和结果保存依赖操作者。
- active/enabled 初始状态难以可靠恢复。
- 不易形成可重复的面试证据和问题闭环。

### 方案 B: 增加直接启停和故障注入脚本，不恢复状态

优点：

- 能自动覆盖主要生命周期。

缺点：

- 成功或失败后可能把原本 inactive/disabled 的 Runtime 留在运行或启用状态。
- 中断脚本时副作用更难判断，不适合作为默认验收入口。

### 方案 C: 显式确认、固定目标、保存并恢复初始状态

优点：

- 只有 `--confirm` 才进入会改变 systemd 进程状态的路径。
- 在第一次 mutation 前保存 active/enabled 状态，并通过 EXIT/INT/TERM 收尾恢复。
- 使用固定 unit、安装路径、socket 和有界等待，缩小误操作范围。
- 每步产生 PASS/FAIL 和原始 JSON/journal/哈希证据。

缺点：

- 脚本仍会在真实执行时短暂停止 Runtime，并注入一次 SIGKILL。
- 初始状态不是稳定的 `active|inactive` 与 `enabled|disabled` 时只能拒绝执行，不能自动猜测恢复目标。

## 最终决策

选择方案 C，新增：

- `run_stage2_board_acceptance.sh`：真实 MP157 上由 root 显式执行的验收入口；
- `verify_stage2_board_acceptance.ps1`：纯主机静态契约验证；
- `verify_stage2_board_acceptance_tests.ps1`：一个正向和六个负向 fixture；
- 部署清单中的 `unix_status_service_tests` ARM 测试二进制，安装到 `/opt/outdoor-agent/tests/`。

真实验收脚本固定操作 `outdoor-agent-runtime.service`，并遵守以下顺序：

1. 要求唯一参数 `--confirm`、root、必要工具和固定部署产物。
2. 用 `systemd-analyze verify` 检查 unit，拒绝活动长测、异常 systemd 初始状态和不属于该 unit 的 Runtime 进程。
3. 保存初始 active/enabled 状态，之后才允许修改 service 状态。
4. 运行隔离的 `unix_status_service_tests`，在 `/tmp` 唯一路径验证 stale socket、活跃实例冲突、真实查询和退出 unlink，不启动第二个传感器 Runtime。
5. 启动 Runtime，验证目录 `0750`、socket `0660`，连续查询三次并检查 running/socket 字段。
6. 分别运行无 context 与显式 socket context 的 Agent mock，校验 `mock_no_inference`、context 标志和“no AI inference was executed”。
7. `systemctl stop` 验证 SIGTERM 退出与 socket 删除，再启动并验证 PID 改变和状态可查询。
8. 对 main process 注入一次 SIGKILL，等待不同 PID 自动恢复，并再次查询状态。
9. EXIT/INT/TERM 收尾恢复原 active/enabled 状态，保存 report、JSON、hash、systemd-analyze 和最近 journal。

脚本不执行 reboot、poweroff、halt、shutdown、固件烧录、XMODEM、COM 访问或物理电源动作。

## 决策理由

该方案把“后续再联调”转化为一个有明确副作用、确认门槛、超时、证据和恢复语义的可重复流程。Unix socket 冲突/stale 测试使用独立测试进程和临时路径，避免为了验证 IPC 而并行启动第二个会占用 `/dev/ttySTM1`、`/dev/ttySTM2` 和 `/dev/icm20608` 的 Runtime。真实 service 生命周期仍必须在 MP157/systemd 上执行，主机静态测试只证明脚本契约没有被弱化。

## 依赖与部署影响

- 不新增第三方库；板端脚本使用 POSIX shell、systemd 工具和常见基础命令。
- `unix_status_service_tests` 已属于 CMake/CTest 目标并由 GNU ARM Linux 9.2.1 全目标构建覆盖，本决策只把该产物纳入验收部署。
- 部署新增 `/opt/outdoor-agent/tests`，继续默认不 enable/start Runtime；验收脚本也不会由部署流程自动执行。
- 测试二进制只使用 `/tmp/outdoor_unix_status_test_*`，不连接串口或传感器设备。

## 风险

- SIGKILL 故障注入会短暂中断 Runtime 数据采集，必须在用户安排的联调窗口执行。
- 若目标镜像缺少脚本要求的 systemd/基础命令，脚本会在 mutation 前拒绝；需根据真实镜像证据调整，不能静默跳过。
- 进程恢复以 active 状态、非零且变化的 MainPID、socket 重建和查询成功为主要证据；`NRestarts` 在旧 systemd 不提供时只作为可选证据。
- restoration 失败会使整体验收失败并在报告中同时写出期望/实际状态，不能标记验收通过。
- 本脚本不替代 Stage 1 小时长测、掉电、室外 GNSS fix、罗盘标定或 I2C 波形测量。

## 验证

- PowerShell parser 检查 deploy、Runtime verifier 和两个新 verifier 脚本。
- Git for Windows `sh -n` 检查板端脚本语法。
- 无 `--confirm` 的主机 smoke 返回 2，且在 root/systemd 检查前拒绝。
- 静态契约正向 fixture 通过；确认门槛、EXIT trap、SIGKILL、三次查询、禁止上下电命令和 self-test 部署六类弱化 fixture 均被拒绝。
- 完整 Runtime verifier 调用新契约测试并通过；MP157 C++ 14/14 CTest、F407 7/7 CTest 和 GNU ARM Linux 9.2.1 全目标构建通过，ARM socket self-test 为 64336 B。
- 本轮未执行板端脚本、部署、COM3/COM9、systemd 状态变更、复位或上下电。

## 后续 TODO

- 用户进入统一联调窗口后，在确认 F407/MP157/GNSS 供电和接线状态的前提下执行部署，再显式运行 `/opt/outdoor-agent/scripts/run_stage2_board_acceptance.sh --confirm`。
- 将输出的 `/tmp/outdoor-agent-stage2-acceptance-*` 完整目录回传并关联 `docs/troubleshooting_log.md`；只有报告通过且初始状态恢复成功，Stage 2.1/2.2/2.3 板端项才能勾选。
