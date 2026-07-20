# ADR-0012: MP157 SD Card File Storage

## 状态

Accepted

## 背景

MP157 现在已插入 SD 卡，项目需要把 Runtime 日志、状态文件和后续采集数据保存到可持久化介质。当前 Runtime 仍处于 Stage 1，主要目标是稳定验证 GNSS、F407 Sensor Hub、板载 ICM20608 和仪表盘链路，不适合过早引入数据库或大型日志框架。

## 可选方案

### 方案 A: 继续只写本地工作目录

优点：

- PC 自动化测试最简单。
- 不需要处理挂载路径和权限。

缺点：

- 板端重启或清理 `/tmp` 后容易丢失运行证据。
- 无法支撑后续传感器数据长期采集。

### 方案 B: 使用 SD 卡文件/目录持久化

优点：

- 只依赖 C++17 filesystem 和普通文件 IO。
- 适合保存 `runtime_status.json`、文本 dashboard、日志和后续采集文件。
- 挂载路径可配置，适配不同 MP157 镜像和 SD 卡挂载策略。

缺点：

- 仍需要外部确认 SD 卡已挂载且可写。
- 当前不提供索引、压缩、轮转或数据查询能力。

### 方案 C: 引入 SQLite 或专用日志框架

优点：

- 更适合结构化历史数据查询。
- 后续可支持复杂数据分析。

缺点：

- 增加交叉编译、部署和维护成本。
- 对当前 Stage 1 的最小上板验证来说偏重。

## 最终决策

采用方案 B。新增可配置的 `storage_enabled` 和 `storage_root_path`，启用后 Runtime 在 SD 根目录下创建 `status/`、`dashboard/`、`logs/`、`data/` 和 `captures/`，并将状态 JSON、文本仪表盘和运行日志写入该目录树。

## 决策理由

- 当前最需要的是板端运行证据和后续采集数据落盘，不是复杂查询。
- 文件型方案与现有 `runtime_status.json` 和 dashboard 输出模型一致。
- 默认仍关闭 storage，保证 PC 开发和自动化测试不依赖 SD 卡。

## 风险

- SD 卡挂载点可能因系统镜像不同而变化；当前板端已确认 SD 卡自动挂载为 `/run/media/mmcblk1p1`，`/mnt/sdcard` 当前不存在。
- 当前日志文件只追加写入，尚未实现大小限制和轮转。
- 若 SD 卡掉线或只读，Runtime 会在 storage 初始化阶段失败，后续需要更细的降级策略。

## 后续 TODO

- 如需固定 `/mnt/sdcard`，在系统层增加挂载规则或软链接，并同步更新 `storage_root_path`。
- 增加日志轮转或按日期分文件。
- 为 GNSS、IMU、磁力计、气压计等原始采样设计 CSV/JSONL 数据记录服务。

## 后续演进（2026-07-19）

上述日志轮转和历史记录 TODO 已在 Stage 1 后续实现：采用每种传感器独立 CSV、GNSS/MCU 合法更新回调、Board IMU/file/mock 循环观察，以及按大小有限备份日志轮转。方案比较、默认配置、真实 MP157 短时验证和剩余小时级/掉电风险见 [ADR-0018](0018-use-csv-history-and-size-based-log-rotation.md)。本 ADR 保留初始 storage 目录决策和当时风险，不覆盖历史记录。
