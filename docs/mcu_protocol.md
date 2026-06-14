# STM32F407ZG Sensor Hub Protocol

本文档记录 Stage 1 当前使用的 Linux Runtime 侧 MCU 协议原型。当前协议用于解析 STM32F407ZG Sensor Hub 的 heartbeat 和 mock sensor 数据；本阶段不实现真实 STM32 固件。

## 帧格式

所有多字节整数使用 little-endian。

```text
offset  size  field
0       1     sof0 = 0xA5
1       1     sof1 = 0x5A
2       1     version = 0x01
3       1     frame_type
4       2     sequence
6       2     payload_length
8       N     payload
8+N     2     crc16_modbus
```

CRC16/MODBUS 覆盖范围：

```text
version, frame_type, sequence, payload_length, payload
```

不包含 `sof0` 和 `sof1`。

## 帧类型

```text
0x01 heartbeat
0x10 mock_sensor
```

## Heartbeat Payload

```text
offset  size  field
0       4     uptime_ms
4       2     status_flags
```

## Mock Sensor Payload

```text
offset  size  field
0       4     uptime_ms
4       2     temperature_centi_c
6       2     humidity_permille
8       2     accel_x_mg
10      2     accel_y_mg
12      2     accel_z_mg
```

换算关系：

- `temperature_centi_c / 100.0`
- `humidity_permille / 10.0`
- `accel_*_mg / 1000.0`

## Mock Frames

Linux Runtime 侧 mock 帧样例位于：

```text
mp157/outdoor-core-service/data/mcu_mock_frames.txt
```

当前样例包含：

- 合法 heartbeat 帧
- 合法 mock sensor 帧
- 故意错误 CRC 帧，用于验证 CRC 拒绝逻辑
