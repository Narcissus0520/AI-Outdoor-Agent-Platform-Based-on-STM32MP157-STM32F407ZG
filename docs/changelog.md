# Changelog

## 2026-06-14

### Added

- Added shared `common/protocol` definitions for MCU frame constants, CRC16/MODBUS, `MSG_TYPE_SENSOR_IMU`, `ImuSample`, and IMU payload scaling.
- Added F407-side `MockImuProvider` and IMU frame packing code for software-only Sensor Hub validation.
- Added MP157-side IMU payload parsing and `runtime_status.json` `imu` output.
- Added CRC, MCU frame parsing, IMU payload parsing, and F407 mock frame packing tests.
- Added `tools/frame_decoder` for decoding hexadecimal MCU protocol frames.
- Added an `Icm42688Driver` placeholder that returns not supported until the real ICM42688 hardware is available.

### Changed

- Stage 1 now continues with Mock IMU data while the real ICM42688 is not available.
- The default MCU mock frame file now includes a valid `sensor_imu` frame.

### Not Implemented

- Real ICM42688 register access is intentionally not implemented.
- Real STM32F407ZG firmware deployment and serial hardware integration are still future work.
