# ADR-0005: 使用文件型状态发布作为最小 IPC 原型

## 状态

Accepted

## 背景

Stage 0.5 需要实现最小 IPC 原型和 Runtime 基础运行状态输出。项目目标平台是 STM32MP157 Embedded Linux，但当前开发和验证环境同时需要在 Windows 上通过 CMake/MSVC 和 PowerShell 验证脚本运行。此阶段不适合过早引入复杂 IPC 框架。

## 可选方案

### 方案 A: Unix domain socket

优点：

- 符合 Linux 用户态服务常见 IPC 方式。
- 适合后续实时查询 Runtime 状态。

缺点：

- 在当前 Windows 验证环境中维护成本更高。
- 需要补充客户端协议和生命周期管理。
- 对 Stage 0.5 的状态输出需求偏重。

### 方案 B: POSIX FIFO

优点：

- Linux 下实现较轻量。
- 适合单向状态流。

缺点：

- Windows/MSVC 验证不方便。
- 读端生命周期和阻塞行为需要额外处理。

### 方案 C: 文件型状态发布

优点：

- 使用 C++17 标准库即可实现。
- 能被外部进程、脚本或调试工具读取。
- 当前 Windows 和后续 Linux 环境都可验证。
- 对 Stage 0.5 的基础状态输出足够。

缺点：

- 不是实时交互式 IPC。
- 状态读取方需要轮询或按需读取。
- 不适合高频数据流。

## 最终决策

Stage 0.5 使用文件型状态发布作为最小 IPC 原型，Stage 1 后默认输出到 `runtime/runtime_status.json`。状态发布采用先写临时文件再替换目标文件的方式，降低外部读取到半写入内容的概率。

## 决策理由

当前 Runtime 只需要输出基础状态，例如运行阶段、服务数量、已启动服务数量和最近错误。文件型状态发布能以最低依赖成本完成生产者到外部消费者的状态暴露，并为后续 Unix domain socket 或 FIFO 方案保留接口空间。

## 风险

- 当前不是完整 IPC 服务，不能处理客户端请求。
- 当前原子替换策略是 C++17 跨平台 best-effort 实现，目标 Linux 环境中仍需要结合文件系统语义复核。
- 后续如果状态更新频率提高，需要迁移到更合适的 IPC 方式。

## 后续 TODO

- Stage 1 前实现或重新评估 Unix domain socket 状态查询接口。
- 增加 Runtime 状态字段，例如运行时间、GNSS fix 时间和服务健康状态。
