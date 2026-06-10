# Stage 0 Plan

Stage 0 目标：完成 Linux 用户态 Runtime 原型，为后续 GNSS、Sensor、MCU 协作和 AI Agent 能力打基础。

## Stage 0.1: Outdoor Core Runtime 工程骨架与 NMEA 文件回放

- [x] 建立最小 CMake C++17 工程
- [x] 建立基础目录结构
- [x] 实现简单日志模块
- [x] 定义输入源抽象 `IInputSource`
- [x] 实现 NMEA 文件回放 `FileReplayInput`
- [x] 添加 `data/nmea_sample.txt`
- [x] 实现 `main.cpp` 主循环，逐行读取 NMEA 并打印日志

## Stage 0.2: 日志与配置模块增强

- [x] 增加日志级别配置
- [x] 增加基础配置文件加载
- [x] 支持配置默认 NMEA 输入路径
- [x] 支持命令行覆盖 NMEA 输入路径
- [x] 支持命令行覆盖日志级别
- [x] 添加最小 PowerShell 验证脚本

## Stage 0.3: Runtime Manager 与 Service 抽象

- [x] 定义 Service 生命周期接口
- [x] 实现 Runtime Manager
- [x] 将 GNSS mock/file replay 封装为服务
- [x] 更新 Runtime 验证脚本

## Stage 0.4: GNSS Mock 服务与 NMEA Parser

- [x] 设计 GNSS 数据模型
- [x] 实现最小 NMEA Parser
- [x] 输出基础定位状态
- [x] 增加 NMEA checksum 校验
- [x] 补充更多 NMEA 样例和边界测试

## Stage 0.5: IPC 原型与运行状态输出

- [x] 评估 IPC 方案
- [x] 实现最小 IPC 原型
- [x] 输出 Runtime 基础状态
- [x] 评估 Unix domain socket 状态查询接口
- [x] 增加状态文件原子替换写入

## 当前限制

- 暂不实现串口、HTTP API、UI 和 AI 功能
- 暂不假设 UBLOX-M10 已完成真实接入
- 暂不假设 STM32F407ZG 已完成通信联调
