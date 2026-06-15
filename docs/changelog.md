# Changelog

## 2026-06-16

### Added

- Added UART Bootloader flashing and UART frame validation scripts for F407 bring-up.
- Validated F407 USART1 heartbeat and Mock IMU binary frames on hardware with zero CRC errors in a 5-second capture.
- Documented F407 board bring-up issues and fixes, including one-key-download DTR/RTS timing, readout-unprotect flow, app reset timing, and PowerShell CRC parsing.

### Changed

- Updated Stage 1 and project design documentation to mark F407 board-side heartbeat and Mock IMU UART capture as validated.
- Clarified that MP157 still needs a real `/dev/tty*` input source before board-to-board runtime validation.

## 2026-06-14

### Added

- Added USART1 PA9/PA10 at 115200 8N1 for F407-to-MP157 communication.
- Connected the Sensor Hub app, HAL tick, blocking HAL UART transport, heartbeat, and Mock IMU reporting to the Cube main loop.
- Added the independent GNU Arm/CMake build for the STM32F407ZG firmware.
- Added the F407 linker script, startup/vector implementation, newlib syscall stubs, and Windows build script.
- Added ELF, HEX, BIN, map, and size generation.
- Added a PF9 LED heartbeat baseline in the Cube `main.c` user section.
- Imported the STM32CubeMX/HAL baseline for `STM32F407ZGTx` under `f407/sensor-hub/firmware/stm32cube`.
- Added the staged GNU ARM/CMake firmware build plan and ADR-0008.
- Added F407 Sensor Hub firmware bring-up skeleton under `f407/sensor-hub/firmware`.
- Added C firmware app entry points: `sensor_hub_app_init()` and `sensor_hub_app_poll()`.
- Added BSP placeholders for `board_uart_send_bytes()` and `board_get_tick_ms()`.
- Added C protocol frame builder, C CRC16/MODBUS, and C Mock IMU Provider for STM32Cube HAL migration.
- Added `docs/stage1_bringup_plan.md` to track the next HAL UART integration steps.
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
- MP157-side real serial input is still future work.
