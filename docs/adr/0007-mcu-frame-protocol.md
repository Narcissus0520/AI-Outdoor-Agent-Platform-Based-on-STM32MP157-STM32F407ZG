# ADR-0007: 定义 STM32F407ZG Sensor Hub 帧协议

## 状态

Accepted

## 背景

Stage 1 需要在 Linux Runtime 侧解析 STM32F407ZG Sensor Hub 数据。当前阶段只实现 Linux Runtime 侧协议解析和 mock 数据验证，不实现真实 STM32 固件，也不接入真实 IMU。

## 可选方案

### 方案 A: 文本协议

优点：

- 易读，调试方便。
- 可以直接用日志或串口终端观察。

缺点：

- 帧边界、字段类型和校验需要额外约定。
- 后续传感器字段增多后解析成本上升。

### 方案 B: 固定头二进制帧

优点：

- 帧边界明确。
- 字段紧凑，适合 MCU 与 Linux Runtime 通信。
- 易于添加 CRC16 校验。

缺点：

- 人眼不可直接阅读。
- 需要维护协议文档和 mock 帧生成方式。

## 最终决策

Stage 1 使用固定头二进制帧协议：

- SOF: `0xA5 0x5A`
- version: `0x01`
- frame type: heartbeat 或 mock sensor
- sequence
- payload length
- payload
- CRC16/MODBUS

详细格式见 `docs/mcu_protocol.md`。

## 决策理由

固定头二进制帧适合后续 STM32F407ZG 与 STM32MP157 Linux Runtime 的串口通信方向，同时当前可以通过 mock frame 文件在 Linux Runtime 侧完成解析、CRC 和状态输出验证。

## 风险

- 当前协议仍是 Stage 1 原型，真实固件接入后可能需要版本协商、错误码和更多 sensor payload。
- 当前只实现 heartbeat 和 mock sensor 两类帧。

## 后续 TODO

- Stage 1.6 或后续硬件阶段接入真实 IMU 后，补充真实 sensor frame。
- 串口接入前补充更完整的协议兼容性测试。
