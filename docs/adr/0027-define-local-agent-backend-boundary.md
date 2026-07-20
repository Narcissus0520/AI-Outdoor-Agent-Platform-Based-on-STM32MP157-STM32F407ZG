# ADR-0027: 定义可替换的 Local Agent Backend 边界

## 状态

Accepted

## 背景

项目长期目标包含 AI Terminal，但当前仓库没有经过选型、交叉编译、资源测量和板端验收的模型运行时。直接加入某个推理框架会把模型格式、硬件加速、内存、存储、许可证和更新机制同时带入 Stage 2，也容易把 mock 交互误写成真实 AI 能力。

Stage 2.1 已提供只读 Runtime status socket，Stage 2.2 已定义 headless Runtime 进程托管。下一步需要先固定本地终端对 Agent 核心的请求/响应边界、错误语义和资源上限，让未来 backend 能替换，而不要求当前就选择实际模型。

## 可选方案

### 方案 A: 立即集成一个本地模型运行时

优点：

- 可以尽早展示真实生成结果。

缺点：

- 尚未确认 MP157 CPU 指令集、可用 RAM/存储、推理时延和散热门槛。
- 新增大型第三方依赖会增加 ARM 交叉编译、许可证和长期维护成本。
- 过早绑定模型格式和 API，可能反向污染 Runtime/IPC 边界。

### 方案 B: 使用远程 HTTP 模型 API

优点：

- 板端算力与模型存储压力较小。

缺点：

- 引入网络、认证、隐私、离线不可用、重试和费用边界。
- 当前项目明确没有远程服务依赖与安全模型，且户外终端需要考虑断网。

### 方案 C: 标准库接口 + 确定性 mock backend

优点：

- 先稳定 schema、校验、错误和大小上限，不增加第三方依赖。
- mock 能端到端验证 CLI、Runtime context 获取和 backend 替换点，同时明确不代表推理。
- 后续本地/远程 backend 都必须遵守同一服务边界，可独立记录依赖取舍。

缺点：

- 当前只能证明软件边界，不能展示模型质量、真实时延或资源占用。
- 同步单请求接口不能中止一个内部永久阻塞的未来 backend；接入真实 backend 前还需要 deadline/cancellation 设计。

## 最终决策

选择方案 C。

请求 `AgentRequest` schema version 1：

- `request_id`：1..64 字节，只允许 ASCII 字母、数字、`.`、`_`、`-`。
- `prompt`：单行、非空、最多 512 字节；拒绝 ASCII control character。
- `runtime_context_json`：可选、不解析的可信 Runtime JSON 快照，最多 64 KiB。

响应 `AgentResponse` schema version 1：

- `state`：`completed`、`rejected` 或 `failed`。
- `backend`、`runtime_context_available`、`message` 和稳定的 error code/message。
- backend 名最多 32 字节，成功 message 最多 2048 字节，error message 最多 256 字节。
- `LocalAgentService` 同步处理单个请求，捕获 backend exception，并拒绝空响应或超限响应。

`IAgentBackend` 是替换点；当前 `MockAgentBackend` 名为 `mock_no_inference`，所有成功响应都明确写出“no AI inference was executed”。`outdoor_agent_terminal` 只在显式 `--status-socket` 时读取 Runtime context，并把 status client 接收上限收紧到 64 KiB；默认不依赖正在运行的 Runtime。终端支持 JSON 和文本输出，但不提供网络监听、写控制命令或后台 daemon。

## 决策理由

该方案让 C++ 接口、CLI、错误码和资源上限先成为可编译、可测试的工程资产，同时避免把尚未选型的 AI 依赖伪装成已完成能力。Runtime context 通过现有只读 socket 获取，Agent backend 不能修改传感器或 Runtime 状态，降低 Stage 2 的权限范围。

## 第三方依赖评估门槛

当前实现仅使用 C++17 标准库和项目既有 Unix status client。引入真实 backend 前必须单独记录并验证：

- 为什么需要该运行时，标准库/现有接口为何不能替代。
- ARMv7 hard-float 交叉编译流程、工具链补丁和目标镜像运行库。
- 模型文件格式、许可证、量化方式、RAM 峰值、存储占用、冷启动和单请求时延。
- CPU 指令优化、是否依赖 GPU/NPU、温升与持续运行功耗。
- 离线可用、模型更新/回滚、损坏检测和存储磨损。
- 如果使用远程服务：TLS、凭据、隐私、费用、断网、重试、幂等和审计。
- deadline、cancellation、进程隔离、并发上限和 backend crash 恢复。

在以上证据形成前，不选择或引入具体模型/runtime。

## 风险

- mock 输出只证明边界与工具链，不证明任何语义理解或推理能力。
- prompt/context 上限按字节计算；当前不执行 Unicode 规范化或 Runtime JSON 语义校验。
- backend 是同步单请求接口。未来可能耗时的实现应放入独立进程，并在 Agent service 外层实现可终止 deadline，而不是简单套线程后假称有超时。
- Runtime context 当前作为不透明快照传给 backend；真实 backend 必须定义最小字段选择，避免不必要的数据复制和 prompt 膨胀。
- 当前 CLI 是本地开发/集成入口，不是完整语音、触摸 UI 或对话历史系统。

## 验证

- 单元测试覆盖合法无 context/有 context 请求、schema、ID、空/超长/control prompt、超长 context、缺失/失败/异常/空/超长 backend 和 JSON 转义。
- CTest 覆盖终端 help 与确定性 mock JSON smoke；MP157 主机 CTest 14/14。
- GNU ARM Linux 9.2.1 完成 Runtime、status query、Agent terminal 和全部测试目标交叉链接；三项产物分别为 273944 B、23168 B、40124 B。
- 部署清单包含 `/opt/outdoor-agent/bin/outdoor_agent_terminal`，本轮未通过 COM9 上传或运行。

## 后续 TODO

- 在真实 MP157 上运行 mock terminal，并分别验证无 context 与显式 status socket context。
- 真实 backend 选型前先采集 MP157 可用 RAM、CPU、存储、温升和可接受交互时延门槛。
- 若进入真实推理阶段，新增独立 ADR 和故障隔离设计；README 在板端正向证据形成前必须继续标为 planned/not integrated。
