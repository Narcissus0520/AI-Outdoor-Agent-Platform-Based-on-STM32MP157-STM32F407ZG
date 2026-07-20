# ADR-0026: 使用 systemd 托管 Outdoor Core Runtime

## 状态

Accepted

## 背景

Stage 1 已把 Runtime 演进为可响应 SIGINT/SIGTERM 的前台协作式进程，Stage 2.1 又增加了常驻 Unix socket 状态查询。手工从串口 shell 启动适合 bring-up 和证据采集，但不能表达开机依赖、受控停止、失败重启、运行目录和最小权限边界。

目标镜像已经使用 systemd，并通过 `outdoor-agent-icm20608.service` 解决板厂 `link-modules.service` 晚于标准 modules-load 的启动顺序。Runtime 托管必须复用该已验证依赖，同时不能在普通软件部署时自动开始真实传感器、串口或 SD 卡操作。

## 可选方案

### 方案 A: 继续手工前台启动

优点：

- 行为直接，适合单次调试和长测脚本。
- 不增加 init 配置。

缺点：

- 无开机托管、失败重启和标准日志入口。
- 依赖顺序、运行目录和退出超时依赖操作者记忆。

### 方案 B: BusyBox init 脚本或自定义守护进程

优点：

- 可在不含 systemd 的极简镜像上工作。

缺点：

- 当前镜像已经使用 systemd，自定义方案会重复实现依赖、重启、PID 和日志管理。
- 与既有 ICM20608 unit 形成两套生命周期模型。

### 方案 C: systemd `Type=simple` unit

优点：

- Runtime 保持前台进程，PID、SIGTERM、退出码和 journal 语义清晰。
- 能直接表达 ICM20608 loader 依赖、`RuntimeDirectory`、失败重启与启动速率限制。
- 可使用文件系统、设备和地址族限制收窄 root 进程权限。

缺点：

- unit 与 `/opt/outdoor-agent`、目标设备节点和 systemd 版本绑定。
- 当前板端设备组/udev 权限未形成可验证基线，暂时不能安全切换专用非 root 用户。

## 最终决策

选择方案 C，并保留方案 A 作为受控 bring-up/长测入口。

- `outdoor-agent-runtime.service` 使用 `Type=simple`，直接执行前台 Runtime。
- `Requires/After=outdoor-agent-icm20608.service`，保证板载 IMU driver loader 先完成。
- `RuntimeDirectory=outdoor-agent` 创建 0750 的 `/run/outdoor-agent`；专用 headless 配置把 JSON 与 socket 写入该易失目录。
- `Restart=on-failure`、`RestartSec=2s`，60 秒内最多启动 5 次；正常退出不重启。
- `TimeoutStopSec=15s`、`KillSignal=SIGTERM`，匹配 Runtime 已实现的受控停止路径。
- service 配置持续读取 GNSS `/dev/ttySTM2`、MCU `/dev/ttySTM1` 和 `/dev/icm20608`，关闭 framebuffer、launcher、SD storage 和 history；UI、耐久与掉电验收仍使用独立显式流程。
- 当前显式 `User=root`/`Group=root`，同时启用 `NoNewPrivileges`、只读系统文件、kernel/control-group 保护、`AF_UNIX` 限制和 closed device policy，仅允许两路串口与 ICM20608。
- 部署脚本默认只上传 unit/config 并执行静态检查，不 enable、不启动 Runtime。`-EnableRuntimeService` 和 `-StartRuntimeService` 是显式板端状态变更开关，后者蕴含前者。
- 长测脚本继续拒绝任何已运行的 `outdoor_core_runtime`；正式长测前必须停止 supervised service，避免竞争串口和污染证据。

## 决策理由

systemd 已是目标镜像的事实标准，并且现有 driver loader unit 已通过真实 boot 证明依赖模型有效。`Type=simple` 不需要 PID 文件或 fork 协议，最符合当前 C++ Runtime 的前台生命周期。默认只安装不启动把“软件交付完成”和“硬件交互授权”分开，满足当前先完成软件、后统一联调的约束。

## 风险

- root 是过渡方案。切换专用用户前必须在真实镜像确认 tty、ICM20608、journal 和运行目录的 udev/group 权限，不能只在 unit 中猜测组名。
- `ProtectSystem=strict`、`DevicePolicy=closed` 等依赖目标 systemd/cgroup 能力；本机静态 verifier 只能检查项目契约，板端仍需 `systemd-analyze verify` 和启动证据。
- restart limit 会在持续硬件缺失或配置错误时停止重试；这是防止无限重启风暴的设计，需要通过 `systemctl reset-failed` 在修复后恢复。
- supervised Runtime 与长测脚本不能并行占用设备。当前由默认 disabled 和进程独占检查双重防护，后续可增加显式维护模式。
- unit 固定使用 `/opt/outdoor-agent`。部署脚本暂时拒绝其他 `InstallRoot`，避免二进制、配置和 WorkingDirectory 分叉。

## 验证

- `verify_runtime_supervision.ps1` 解析 unit/config 并检查依赖、运行目录、SIGTERM、重启上限、沙箱、设备白名单和连续 live-source 配置。
- 负向自测确认 `Restart=no`、`StartLimitBurst=0`、`DevicePolicy=auto` 和 service socket disabled 均会被拒绝。
- Runtime verifier 使用 service config 加载后，通过 CLI 覆盖为 file/mock、关闭板载设备/socket/dashboard，完成无硬件 smoke 并正常发布 stopped 状态。
- 部署脚本和 verifier 通过 PowerShell parser；本轮未执行部署、enable、start 或任何 COM3/COM9 操作。

## 后续 TODO

- 在 MP157 上运行 `systemd-analyze verify`，再依次验证 enable/start/status/query/stop/restart、SIGTERM 清理和失败重启上限。
- 采集实际设备 ownership/group/udev 证据后，提出从 root 切换到专用用户的独立 ADR。
- 为长测增加经过验证的 systemd maintenance/冲突处理流程后，才允许 supervised service 与正式验收脚本切换。
