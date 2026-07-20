# ADR-0003: 使用 Runtime Manager 管理服务生命周期

## 状态

Accepted

## 背景

Stage 0.3 需要把 Outdoor Core Runtime 从单一主循环推进为可扩展的服务运行框架。后续 GNSS、Sensor、IPC、MCU 协作和 AI Terminal 能力都会以独立模块逐步加入，因此当前需要先建立清晰的服务生命周期边界。

## 可选方案

### 方案 A: 继续在 `main.cpp` 中直接调用所有模块

优点：

- 当前实现最简单。
- 代码路径短，调试直接。

缺点：

- 后续模块增多后 `main.cpp` 会快速膨胀。
- 服务启动、运行、停止逻辑分散，不利于展示 Runtime Service 设计能力。

### 方案 B: 引入轻量 `IService` 和 `RuntimeManager`

优点：

- 使用统一的 `start/run/stop` 生命周期。
- `main.cpp` 只负责配置和组装服务。
- 便于后续加入 GNSS Parser、Sensor 服务、IPC 服务和 MCU 通信服务。
- 只依赖 C++17 标准库，不增加交叉编译负担。

缺点：

- 相比单一主循环多一层抽象。
- 当前仍是顺序运行模型，还不是完整的多进程或多线程 Runtime。

## 最终决策

Stage 0.3 引入 `runtime::IService` 和 `runtime::RuntimeManager`，并将现有 NMEA 文件回放封装为 `services::GnssReplayService`。

## 决策理由

该方案能在不引入复杂依赖的前提下，建立后续 Runtime Service 演进所需的最小生命周期模型。当前服务仍按顺序启动和运行，保持实现简单，后续再根据真实需求扩展状态管理、错误恢复或并发模型。

## 风险

- 当前 `RuntimeManager` 只支持顺序服务运行，不适合长时间阻塞服务并发运行。
- 当前没有服务状态机和健康检查。

## 后续 TODO

- 在 Stage 0.4 后评估 GNSS 服务是否需要持续运行模型。
- 在 Stage 0.5 IPC 原型中补充 Runtime 状态输出。

## 2026-07-19 后续修订

Stage 1 已采用单线程协作式 `poll()` 调度替代本 ADR 初始的顺序阻塞式 `run()`。服务并发演进的方案比较、接口变化和风险见 [ADR-0017](0017-use-cooperative-runtime-service-scheduler.md)。本 ADR 保留 Stage 0 初始决策和演进背景，不覆盖历史记录。
