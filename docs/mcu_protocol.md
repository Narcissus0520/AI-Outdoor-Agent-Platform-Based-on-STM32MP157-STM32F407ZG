# STM32F407ZG Sensor Hub Protocol

本文档记录 Stage 1 当前使用的 MCU 协议原型。当前 F407 固件通过 UART4 PC10、115200 8N1 发送 heartbeat 和 IMU 二进制帧到 MP157 USART3 PD9，MP157 Linux 侧设备为 `/dev/ttySTM1`；UART4 PC11 / MP157 PD8 已接线并初始化，后续用于 MP157 下行控制。F407 侧已接入 ICM42688 I2C 读取路径，初始化或读取失败时回退到 Mock IMU。当前 IMU 帧目标频率为 100 Hz，MP157 Runtime 的 serial 输入源和板间联调已完成最小验证。

串口上传输的是连续二进制帧，不附加换行符，也不转换为十六进制文本。

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
0x11 sensor_imu
```

## Heartbeat Payload

```text
offset  size  field
0       4     uptime_ms
4       2     status_flags
```

当前 F407 固件使用的 `status_flags`：

```text
bit 0 / 0x0001: ICM42688 ready，IMU 帧来自真实 ICM42688
bit 1 / 0x0002: IMU fallback active，IMU 帧来自 Mock IMU
bit 2 / 0x0004: IMU init/read error，ICM42688 初始化或读取失败
```

PC 侧历史 `scripts/verify_f407_uart.ps1` 会打印最后一帧 heartbeat 的 `last_heartbeat_status_flags`；当前板间验证以 MP157 Runtime `runtime_status.json` 中的 `mcu.status_flags` 为准。`0x0001` 表示当前 IMU 数据来自真实 ICM42688。

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

## IMU Payload

IMU payload 由 `common/protocol/imu_payload.h` 中的 `ImuSample` 定义。协议层使用定点整数，避免在 MCU 协议中直接传输浮点。

```text
offset  size  field
0       4     uptime_ms
4       2     accel_x_mg
6       2     accel_y_mg
8       2     accel_z_mg
10      4     gyro_x_mdps
14      4     gyro_y_mdps
18      4     gyro_z_mdps
22      2     temperature_centi_c
```

换算关系：

- `accel_*_mg / 1000.0`
- `gyro_*_mdps / 1000.0`
- `temperature_centi_c / 100.0`

## Mock Frames

Linux Runtime 侧 mock 帧样例位于：

```text
mp157/outdoor-core-service/data/mcu_mock_frames.txt
```

当前样例包含：

- 合法 heartbeat 帧
- 合法 mock sensor 帧
- 合法 Mock IMU 帧
- 故意错误 CRC 帧，用于验证 CRC 拒绝逻辑
