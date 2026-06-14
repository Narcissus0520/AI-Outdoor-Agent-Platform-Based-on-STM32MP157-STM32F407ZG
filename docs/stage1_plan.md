# Stage 1 Plan

Stage 1 目标：在现有 Linux Outdoor Core Runtime 基础上，增加 STM32F407ZG Sensor Hub 协议解析和 Sensor Hub 状态管理。

当前只修改 Linux Runtime 侧代码，不实现真实 STM32F407ZG 固件，不接入真实 IMU。

## Stage 1.1: 协议文档和协议常量

- [x] 新增 MCU 协议文档
- [x] 定义 MCU 帧头、版本、帧类型和 payload 长度常量
- [x] 定义 CRC16/MODBUS 校验方式

## Stage 1.2: F407 发送 heartbeat

- [x] 定义 heartbeat 帧格式
- [x] 提供 Linux Runtime 侧 mock heartbeat 帧样例
- [ ] 实现真实 STM32F407ZG 固件发送 heartbeat

## Stage 1.3: Linux Runtime 解析 heartbeat

- [x] 新增 `McuFrame`
- [x] 新增 `McuFrameParser`
- [x] 支持解析 heartbeat 帧
- [x] 支持 heartbeat CRC16 校验
- [x] 更新 `McuStatus`

## Stage 1.4: F407 发送 Mock Sensor 数据

- [x] 定义 mock sensor 帧格式
- [x] 提供 Linux Runtime 侧 mock sensor 帧样例
- [x] 支持解析 mock sensor 帧
- [ ] 实现真实 STM32F407ZG 固件发送 mock sensor 数据

## Stage 1.5: Runtime 状态输出增强

- [x] 新增 `McuStatus`
- [x] Runtime 状态输出升级为 `runtime_status.json`
- [x] 将 MCU heartbeat 状态加入 Runtime 状态输出
- [x] 将 mock sensor 状态加入 Runtime 状态输出
- [x] 验证脚本检查 MCU 状态字段

## Stage 1.6: 接入真实 IMU

- [ ] 选择和确认真实 IMU 硬件
- [ ] STM32F407ZG 侧采集真实 IMU
- [ ] Linux Runtime 侧解析真实 IMU 数据帧
- [ ] 将真实 IMU 状态加入 Runtime 状态输出

## 当前限制

- 当前 Stage 1 只实现 Linux Runtime 侧协议解析和 mock 数据验证。
- 暂不实现真实 STM32F407ZG 固件。
- 暂不接入真实 IMU。
- 暂不引入 UI、HTTP API 或 AI Agent。
