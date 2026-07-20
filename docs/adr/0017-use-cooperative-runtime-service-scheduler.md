# ADR-0017: Use a Cooperative Runtime Service Scheduler

## 状态

Accepted

## 背景

MP157 Runtime 原有 `IService::run()` 表示“运行到完成”，`RuntimeManager::run()` 按注册顺序逐个调用服务。文件回放和有限串口抓取可以工作，但 `DashboardService` 在 `dashboard_refresh_count = 0` 时不会返回，导致其他需要持续执行的 GNSS、MCU 和板载 IMU 服务无法与常驻仪表盘同时运行。运行中的 `runtime_status.json` 也不会周期更新。

Stage 1 需要在不引入大型框架的前提下，让多个 I/O 服务长期推进，并为后续传感器历史记录和日志轮转提供稳定调度基础。

## 可选方案

### 方案 A：继续顺序运行，通过调整服务顺序规避阻塞

优点：

- 不修改现有生命周期接口。
- 文件回放等一次性流程行为简单。

缺点：

- 任意常驻服务都会阻塞其后的服务。
- 把常驻服务放到最后也只能让前面的有限采集先结束，不能实现持续采集与持续显示。
- 无法满足长期 Runtime 的目标。

### 方案 B：每个服务使用独立线程

优点：

- 服务可以真正并行运行。
- 阻塞式服务改动相对集中。

缺点：

- GNSS、MCU 和 Board IMU 状态当前由采集服务、dashboard 和状态发布器共享，直接加线程会产生数据竞争。
- 需要引入锁、快照、线程退出、异常传播和 join 超时等完整并发模型。
- 当前串口带宽、刷新频率和计算负载没有证明需要多核并行，线程复杂度与收益不匹配。

### 方案 C：单线程协作式 `poll()` 调度

优点：

- 每个服务一次只执行一个有界、非阻塞步骤，Runtime Manager 可轮询全部活动服务。
- 所有共享状态仍在同一线程访问，不需要锁和动态线程资源。
- 适合串口 non-blocking fd、定时采样、文件回放、evdev 和 framebuffer 等当前工作负载。
- 服务失败、完成、停止和周期状态发布可以由 Runtime Manager 统一管理。

缺点：

- 每个 `poll()` 都必须保持有界；一个阻塞调用仍会拖慢所有服务。
- 不提供真正的多核并行，高计算量 AI 推理未来应使用独立进程或专用工作线程。
- framebuffer 整帧渲染仍会占用一个调度周期。

## 最终决策

选择方案 C。

- `IService::run()` 替换为 `poll()`，返回 `Running`、`Completed` 或 `Failed`。
- `RuntimeManager` 以 5 ms 让步周期轮询所有未完成服务，记录 active/completed service 数量。
- 文件回放和 mock 服务每次处理一行；串口服务每次执行一次 non-blocking read；MCU 下行命令支持分段 non-blocking write。
- GNSS/MCU `capture_seconds = 0` 表示持续采集；Board IMU `sample_count = 0` 表示持续采样；dashboard 原有 `refresh_count = 0` 继续表示常驻刷新。
- launcher evdev 从阻塞式 `poll/read` 循环改为 non-blocking 状态机，等待触摸期间不阻塞其他服务。
- Runtime 每秒执行状态发布回调，使常驻进程的 `runtime_status.json` 持续更新。
- 支持 SIGINT/SIGTERM 停止；新增 `runtime_run_seconds` / `--runtime-run-seconds`，便于有限时长回归和小时级耐久验证，0 表示不额外限制运行时长。

## 决策理由

当前 Runtime 的工作负载以低带宽 I/O 和低频 UI 刷新为主，单线程协作式调度可以解决服务互相阻塞的问题，同时保持共享状态访问确定、内存使用可控和交叉编译依赖简单。相比直接增加线程，它更符合当前 Stage 1 的风险和复杂度边界。

该实现只使用 C++17 标准库和现有 POSIX API，不增加第三方依赖，对 Windows 主机测试和 ARMv7 hard-float 交叉编译路径均保持兼容。

## 风险

- 新增服务如果在 `poll()` 中执行长时间阻塞 I/O，会破坏所有服务的调度延迟，代码评审必须检查该约束。
- 固定 5 ms 让步周期目前不可配置；后续应根据 MP157 CPU 占用和真实串口积压评估。
- framebuffer 渲染期间其他服务暂时不能执行；若实测出现串口积压，应先测量最坏渲染时间，再评估分块渲染或独立 UI 进程。
- Runtime 当前仍是单进程模型，不适合直接承载未来高计算量 AI 推理。

## 后续 TODO

- 在 MP157 上使用真实 `/dev/ttySTM1`、`/dev/ttySTM2` 和 `/dev/fb0` 完成小时级协作调度验证。
- 记录调度循环 CPU 占用、串口帧率、状态文件更新时间和 dashboard 刷新耗时。
- 基于该调度模型增加传感器历史采样记录服务和日志轮转。
- 只有在测量证明单线程调度无法满足延迟或吞吐需求时，再设计线程安全快照或独立进程边界。
