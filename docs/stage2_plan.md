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
- [ ] 在真实 MP157 Linux 上启用 socket，验证文件权限、活动实例冲突、stale socket 恢复、连续查询和受控退出清理

## Stage 2.2: Runtime Process Supervision

- [ ] 设计 Outdoor Core Runtime 的 systemd unit、依赖顺序、运行目录和退出语义
- [ ] 将 Runtime 进程托管与现有 ICM20608 驱动加载 unit、长测独占保护和部署脚本协调
- [ ] 增加纯软件 unit 静态检查和失败重启策略测试
- [ ] 在真实 MP157 上验证 enable/start/stop/restart、SIGTERM 受控退出和异常退出恢复

## Stage 2.3: Local Agent Boundary

- [ ] 定义 AI Terminal 本地请求/响应边界，不把未集成的推理能力写成已完成
- [ ] 先实现可替换的 mock agent service 和资源上限，再评估具体模型/runtime 依赖
- [ ] 记录第三方依赖、交叉编译、内存/CPU/存储成本和离线部署取舍

## 当前验收边界

- Stage 2.1 纯软件实现、主机测试和 ARM 交叉链接已完成。
- 当前 Windows 环境没有可用 WSL 发行版、Docker/Podman 或 ARM 用户态模拟器，因此 POSIX socket 集成测试源码未在本机 Linux 用户态执行。
- 按当前任务约束，本轮没有通过 COM3/COM9 触发板端命令，也没有执行烧录、复位、物理上下电或真实硬件交互。
- Stage 1 中 ICM42688 电气/波形、室外 GNSS fix、罗盘实采标定、SD/双串口/framebuffer 小时级和掉电验收继续保持未完成，不因 Stage 2 软件推进而改变。
