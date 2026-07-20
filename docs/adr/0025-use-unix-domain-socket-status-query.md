# ADR-0025: 使用 Unix Domain Socket 提供 Runtime 状态查询

## 状态

Accepted

## 背景

Stage 0.5 选择原子替换的 JSON 状态文件作为最小 IPC，Stage 1 的 dashboard、板端预检、健康时间线和离线审计都已依赖该文件。ADR-0006 当时决定把交互式 Unix domain socket 推迟到 MP157 Linux 目标和 Runtime 生命周期明确之后。

当前 Runtime 已采用单线程协作式 `poll()` 调度并完成 ARMv7 Linux 交叉构建，具备在不引入线程竞争和第三方框架的前提下增加本地查询服务的条件。新接口必须保留文件兼容、限制资源占用、避免慢客户端阻塞采集，并为后续 systemd 托管和本地 AI Terminal 进程提供清晰边界。

## 可选方案

### 方案 A: 客户端继续轮询 JSON 文件

优点：

- 无新增服务端协议和 socket 生命周期。
- 现有脚本完全兼容。

缺点：

- 客户端只能观察最近一次周期发布，不能按需取得内存中的当前快照。
- 不能体现请求失败、活跃实例冲突和连接资源边界。

### 方案 B: 引入 HTTP/REST 服务

优点：

- 客户端生态成熟，后续远程访问方便。

缺点：

- 当前只需要板内本地状态查询，HTTP 扩大协议、安全和依赖范围。
- 自实现 HTTP 容易形成不完整协议，引入库又增加交叉编译和长期维护成本。

### 方案 C: Unix domain stream socket + 最小行协议

优点：

- 使用 Linux/POSIX 原生 API，无第三方依赖和网络暴露。
- 可作为协作式 Runtime Service 非阻塞轮询。
- 文件权限可限制本机调用方，并可提供独立查询客户端。

缺点：

- Windows 原生环境不能执行真实 POSIX socket 生命周期测试。
- 需要处理 stale path、活跃实例冲突、慢客户端、超长请求和退出清理。

## 最终决策

选择方案 C，同时保留方案 A。文件发布仍是脚本、dashboard 和离线审计的稳定兼容接口；可选 `UnixStatusService` 提供按需查询。

协议和资源边界：

- `AF_UNIX` + `SOCK_STREAM`，路径由 `status_socket_path` 配置。
- `GET_STATUS\n` 返回与文件发布相同的完整 JSON；`PING\n` 返回 `PONG\n`。
- 请求最多 64 字节，最多 4 个并发客户端，客户端空闲 5 秒后关闭。
- listener 和 client fd 均使用 non-blocking + close-on-exec；发送使用 `MSG_NOSIGNAL`。
- socket 权限固定为 0660；默认关闭，通过配置或 CLI 显式启用。
- 启动只删除确认无法连接的 stale socket；普通文件和活跃 socket 会导致服务启动失败。
- Runtime 停止时关闭全部连接并删除自身 socket path。
- 提供 `outdoor_status_query` 客户端，避免依赖目标镜像是否包含支持 `nc -U`/`socat` 的工具。

## 决策理由

该方案使用标准 POSIX API，资源上限固定，适合 STM32MP157 Embedded Linux。服务直接复用 `RuntimeStatus` 和 `StatusPublisher::serialize()`，避免文件与 socket schema 分叉。把它实现为协作式 `IService`，可继续维持单线程状态所有权，不引入互斥锁和后台线程。

默认关闭是为了保持现有 file/mock 自动化和有限服务运行的退出语义：启用后 socket 服务是常驻服务，Runtime 需要 SIGINT/SIGTERM 或 `runtime_run_seconds` 才会停止。

## 风险

- 单次查询会分配完整 JSON 字符串；当前状态规模较小，后续 schema 显著增长时需要测量延迟和峰值内存。
- 固定 4 客户端避免无界资源增长，但本地恶意或失控进程仍可暂时占满连接槽；5 秒空闲门槛限制影响窗口。
- Windows 本机只能覆盖协议和不支持边界；真实 fd、权限和 stale socket 行为仍需 Linux 执行证据。
- 当前没有认证、授权协商或写命令；0660 和部署时的用户/组设置是安全边界。

## 验证

- `StatusPublisher` 测试确认文件内容与内存序列化完全一致，并覆盖 `ipc` 状态字段。
- 跨平台测试覆盖 `GET_STATUS`、`PING`、未知命令、缺失 provider 和非 POSIX 失败边界。
- POSIX 测试源码覆盖 stale socket、活跃实例冲突、真实 client/server 查询和 stop unlink。
- Windows GCC Release CTest 11/11 与 Runtime verifier 通过。
- GNU ARM Linux 9.2.1 完成 `outdoor_core_runtime`、`outdoor_status_query` 和 Unix socket 测试目标的全量编译链接。
- 本轮按任务约束未执行 COM9 部署和 MP157 运行；板端权限、连续查询和清理属于 Stage 2.1 待联调项。

## 后续 TODO

- 在 MP157 上使用专用运行目录启用 socket，执行 stale/active/permission/连续查询/退出清理验收。
- Stage 2.2 已用 systemd `RuntimeDirectory` 固定 socket 目录，并以 root + device allow-list 作为待板端权限验收的过渡方案；详见 ADR-0026。
- 若后续增加写命令，必须新增独立协议版本、权限模型、幂等与审计设计，不能复用只读 `GET_STATUS` 直接扩展。
