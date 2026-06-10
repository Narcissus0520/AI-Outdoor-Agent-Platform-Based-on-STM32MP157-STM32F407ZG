# ADR-0006: 评估 Unix Domain Socket 状态查询接口

## 状态

Accepted

## 背景

Stage 0.5 已经使用文件型状态发布完成最小 IPC 原型，默认输出 `runtime/status.txt`。剩余任务需要评估 Unix domain socket 是否应在 Stage 0 内实现为 Runtime 状态查询接口。

## 可选方案

### 方案 A: Stage 0 内实现 Unix domain socket 查询接口

优点：

- 更贴近 Embedded Linux 用户态服务常见 IPC 形态。
- 可以支持外部客户端按需查询 Runtime 状态。

缺点：

- 当前 Windows/MSVC 验证环境不适合直接闭环。
- 需要定义客户端协议、连接生命周期和错误处理。
- 对当前 Stage 0 的作品集目标来说，会扩大实现范围。

### 方案 B: Stage 0 保留文件型状态发布，Stage 1 后再实现 Unix domain socket

优点：

- 保持 Stage 0 简单、可编译、可验证。
- 当前状态输出已经能被脚本和外部工具读取。
- 后续接近 STM32MP157 Linux 部署时再实现 socket，更符合真实运行环境。

缺点：

- 当前还不是交互式 IPC。
- 外部状态读取需要轮询状态文件。

## 最终决策

Stage 0 不实现 Unix domain socket 查询接口，只完成方案评估。当前继续使用文件型状态发布作为最小 IPC 原型，并将 Unix domain socket 查询接口放入 Stage 1 或后续 Linux 部署阶段。

## 决策理由

Stage 0 的主要目标是完成 Linux 用户态 Runtime 原型骨架。文件型状态发布已经满足基础运行状态输出和验证需求，而 Unix domain socket 更适合在 STM32MP157 Linux 目标环境明确后实现。

## 风险

- 如果后续状态查询需求变强，文件型状态发布会显得不够实时。
- Stage 1 需要重新设计状态查询协议，避免和当前状态文件格式割裂。

## 后续 TODO

- Stage 1 评估 Unix domain socket 路径、权限和客户端协议。
- 保持 `RuntimeStatus` 作为状态数据来源，避免 IPC 方式变化影响 Runtime Manager。
