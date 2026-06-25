# STM32F407ZG Sensor Hub Protocol

本文档记录 Stage 1 当前使用的 MCU 协议原型。当前 F407 固件通过 UART4 PC10、115200 8N1 发送 heartbeat、IMU、MMC5603 磁力计和 BMP390 气压计二进制帧到 MP157 USART3 PD9，MP157 Linux 侧设备为 `/dev/ttySTM1`。

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
0x12 sensor_magnetometer
0x13 sensor_barometer
0x80 command_ping
0x81 command_ack
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
bit 3 / 0x0008: MMC5603 ready，磁力计帧来自真实 MMC5603
bit 4 / 0x0010: MMC5603 init/read error
bit 5 / 0x0020: BMP390 ready，气压计帧来自真实 BMP390
bit 6 / 0x0040: BMP390 init/read error
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

## Magnetometer Payload

磁力计 payload 由 `common/protocol/magnetometer_payload.h` 定义，F407 当前使用 20 Hz 单次测量上报。

```text
offset  size  field
0       4     uptime_ms
4       4     magnetic_x_nt
8       4     magnetic_y_nt
12      4     magnetic_z_nt
```

换算关系：`magnetic_*_nt / 1000.0 = μT`。
- `humidity_permille / 10.0`
- `accel_*_mg / 1000.0`

## Barometer Payload

气压计 payload 由 `common/protocol/barometer_payload.h` 定义。F407 当前以 10 Hz 发布 BMP390 补偿后的气压和温度。

```text
offset  size  field
0       4     uptime_ms
4       4     pressure_pa
8       2     temperature_centi_c
```

换算关系：

- `pressure_pa` 为 Pa
- `temperature_centi_c / 100.0` 为 °C

协议暂不直接传输海拔。海拔换算依赖当地海平面参考气压，后续应由 MP157 配置和计算。

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

## Command Ping Payload

`command_ping` 是 MP157 发往 F407 的最小下行命令，用于验证反向串口链路和固件命令解析。当前 MP157 侧可通过 `--mcu-command ping` 发送，默认 nonce 为 `0x50494E47`。

```text
offset  size  field
0       4     nonce
```

## Command Ack Payload

`command_ack` 是 F407 对 `command_ping` 的回应。当前 `status = 0` 表示命令已识别并处理成功；`nonce` 原样返回，用于 MP157 侧确认回应属于本次请求。

```text
offset  size  field
0       1     request_type
1       1     status
2       2     reserved = 0
4       4     nonce
```

MP157 Runtime 解析到 `command_ack` 后，会在 `runtime_status.json` 的 `mcu` 字段中输出：

```text
command_ack_seen
command_ack_request_type
command_ack_status
command_ack_nonce
```

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
