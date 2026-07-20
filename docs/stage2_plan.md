# Stage 2 Plan

Stage 2 目标：在 Stage 1 传感器数据链路和协作式 Runtime 基础上，继续完善 Embedded Linux 服务化、交互式 IPC、进程托管与本地 AI Terminal 软件边界。真实硬件接线、电气测量、物理上下电、户外采集和长时板端验收不计入纯软件完成项，统一保留为后续联调门槛。

## Stage 2.1: Unix Domain Socket Runtime Status Query

- [x] 保留原子替换的 `runtime_status.json` 文件发布接口，兼容现有 dashboard、脚本和离线审计
- [x] 从 `StatusPublisher` 提取公共内存序列化入口，文件与 socket 返回完全相同的 JSON schema
- [x] 实现可选的 `UnixStatusService`，接入单线程协作式 `IService::poll()` 调度
- [x] 使用非阻塞 `AF_UNIX/SOCK_STREAM`，限制 4 个客户端、64 字节请求和 5 秒空闲时间
- [x] 定义 `GET_STATUS\n` 与 `PING\n` 请求，未知命令和超长请求返回有界错误
- [x] socket 默认权限为 0660；启动时只清理确认无活动服务的 stale socket，不覆盖普通文件或活跃 socket
- [x] 增加 `status_socket_enabled`、`status_socket_path` 及对应 CLI 覆盖参数
- [x] 增加 `outdoor_status_query` 本地查询客户端，并纳入 MP157 部署文件清单
- [x] 增加跨平台协议测试、非 POSIX 失败边界测试和 POSIX socket 集成测试源码
- [x] Windows GCC Release CTest 11/11、Runtime verifier 和 GNU ARM Linux 9.2.1 全目标交叉构建通过
- [x] 在真实 MP157 Linux 上启用 socket，验证 0750/0660 权限、活动实例冲突、stale socket 恢复、连续三次查询和受控退出清理

## Stage 2.2: Runtime Process Supervision

- [x] 设计 Outdoor Core Runtime 的 systemd `Type=simple` unit、ICM20608 loader 依赖、`RuntimeDirectory`、SIGTERM 和退出语义
- [x] 增加 headless live-source service config：状态文件/socket 写入 `/run/outdoor-agent`，默认不启用 framebuffer、SD storage 或 history
- [x] 使用 60 秒 5 次启动上限、`Restart=on-failure` 和 2 秒间隔约束失败重启风暴
- [x] 增加只读系统、kernel/control-group 保护、`AF_UNIX` 限制和两路串口/ICM20608 closed device allow-list
- [x] 将 Runtime 进程托管与现有 ICM20608 驱动加载 unit、长测独占保护和部署脚本协调
- [x] 部署默认只安装不 enable/start；增加显式 `-EnableRuntimeService`、`-StartRuntimeService` 状态变更开关
- [x] 增加纯软件 unit/config 静态检查、四个不安全变体负向测试和 service config 无硬件 smoke
- [x] 在真实 MP157 上验证 enable/disable、start/stop/restart、SIGTERM 受控退出和单次 SIGKILL 异常退出恢复

## Stage 2.3: Local Agent Boundary

- [x] 定义 schema version 1 的 `AgentRequest`/`AgentResponse`、稳定 state/error code 和 JSON 输出
- [x] 实现可替换 `IAgentBackend`、同步单请求 `LocalAgentService` 和明确不执行推理的 `MockAgentBackend`
- [x] 限制 request ID 64 B、prompt 512 B、Runtime context 64 KiB、backend message 2 KiB 和 error 256 B
- [x] 增加 `outdoor_agent_terminal`，支持 JSON/text；仅在显式 `--status-socket` 时读取只读 Runtime context
- [x] 覆盖合法/拒绝/backend 失败与异常/输出超限/JSON 转义，以及 CLI help/mock smoke；MP157 CTest 14/14
- [x] GNU ARM Linux 9.2.1 全目标交叉构建通过，并将 Agent terminal 纳入部署清单
- [x] ADR-0027 记录本地/远程/接口优先方案比较，以及第三方依赖、交叉编译、内存/CPU/存储、离线与隔离门槛
- [x] 在真实 MP157 上分别验证无 Runtime context 与经 Unix socket 读取只读 context 的 `mock_no_inference` terminal 路径

真实模型 backend、语音/触摸对话、历史记忆和任务执行不属于已完成 Stage 2.3；没有形成依赖选型与板端资源证据前继续标为 planned。

## Stage 2.4: Repeatable Board Acceptance Harness

- [x] 增加只接受 `--confirm` 的 root 板端入口，不由部署流程自动执行
- [x] 在第一次 service mutation 前保存 active/enabled 状态，并由 EXIT/INT/TERM 收尾恢复
- [x] 拒绝活动长测、异常 systemd 初始状态和不属于该 unit 的 Runtime 进程
- [x] 部署并运行隔离的 Unix socket self-test，覆盖 stale path、active collision、真实查询和退出 unlink
- [x] 固化 Runtime 目录/socket 权限、连续三次查询和 Agent mock 无/有 context 验收
- [x] 固化 SIGTERM 停止/socket 清理、再次启动和单次 SIGKILL 自动恢复验收
- [x] 输出独立 report、JSON、hash、systemd-analyze 和 journal 证据目录
- [x] 增加纯主机契约 verifier 与六个安全边界负向 fixture，并纳入完整 Runtime verifier
- [x] ADR-0028 记录显式确认、固定目标、状态恢复和不触碰整板电源/烧录的取舍
- [x] 在真实 MP157 上由用户确认联调窗口后显式执行，生成并回读完整 PASS report、产物清单、哈希和最终状态

## 当前验收边界

- Stage 2.1、Stage 2.2、Stage 2.3 mock 边界和 Stage 2.4 已完成真实 MP157 集成验收。2026-07-21 通过报告位于板端 `/tmp/outdoor-agent-stage2-acceptance-IIppv5/`，report SHA256 为 `d972d97d...792786`。
- 当前 Windows 环境没有可用 WSL 发行版、Docker/Podman 或 ARM 用户态模拟器，因此 POSIX socket 集成测试源码未在本机 Linux 用户态执行。
- 本轮通过 COM9 部署并运行了 Stage 2 验收，但没有操作 COM3、烧录、复位或物理上下电；脚本最终恢复 Runtime `inactive/disabled`，socket 已删除。
- 首轮真实验收暴露 disabled/inactive unit 的 benign `reset-failed` 被误判为启动失败；TRB-20260721-046 完成根因、修复、主机回归、增量部署和第二轮板端复测闭环。
- Stage 1 中 ICM42688 电气/波形、室外 GNSS fix、罗盘实采标定、SD/双串口/framebuffer 小时级和掉电验收继续保持未完成，不因 Stage 2 软件推进而改变。
