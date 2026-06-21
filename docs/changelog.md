# Changelog

## 2026-06-20

### Added

- Added minimal MP157 -> F407 downlink command prototype with `command_ping` and `command_ack` MCU frame types.
- Added MP157 `mcu_command = none | ping` config and `--mcu-command none|ping` CLI option for serial mode.
- Added F407 UART4 RX byte polling, command frame decoder, and ping ack response.
- Added `command_ack_*` fields to MP157 `runtime_status.json`.
- Added MP157 MCU live serial input mode for F407 UART4 frames, defaulting to USART3 `/dev/ttySTM1` at 115200 8N1.
- Added `McuFrameStreamDecoder` to reconstruct binary MCU frames from noisy, fragmented, continuous serial byte streams.
- Added serial configuration keys: `mcu_input_mode`, `mcu_serial_device`, `mcu_serial_baud`, and `mcu_serial_capture_seconds`.
- Added byte-stream decoder tests for leading noise, split frames, and back-to-back frames.
- Added F407 UART4 PC10/PC11 initialization and routed Sensor Hub protocol output through UART4 while keeping USART1 for Bootloader flashing.

### Verified

- F407 firmware build passed after adding UART4 RX command handling.
- F407 UART Bootloader flashing over `COM6` passed for the ping/ack firmware, writing `sensor_hub.bin` 11100 B with byte-for-byte readback verification.
- MP157 local build, tests, runtime verification script, and ARM cross-build passed after adding ping/ack support.
- MP157 raw shell ping/ack validation passed: writing `command_ping` bytes to `/dev/ttySTM1` returned a `command_ack` frame with `request_type=0x80`, `status=0`, and nonce `0x50494E47`.
- F407 firmware build passed after switching `board_uart_send_bytes()` to UART4.
- F407 UART Bootloader flashing over `COM6` passed with Bootloader version `0x31`, chip ID `0x0413`, and byte-for-byte readback verification.
- MP157 `/dev/ttySTM1` raw capture showed continuous `A5 5A` MCU frame headers from `F407 PC10 UART4_TX -> MP157 PD9 USART3_RX`.
- MP157 Runtime serial validation passed: `mcu.heartbeat_seen=true`, `mcu.imu_seen=true`, `mcu.status_flags=1`, and `imu.seen=true` in `runtime_status.json`.

### Deferred

- MP157 -> F407 downlink command handling currently only covers the minimal ping/ack prototype; running the new ARM Runtime `--mcu-command ping` on MP157 remains pending because COM3 serial console package upload hit input overrun and SHA256 mismatches.

## 2026-06-19

### Added

- Added MP157 onboard ICM20608 character-device reader, IIO reader, and Runtime service.
- Added `board_imu` status output to `runtime_status.json`, separate from F407 MCU `imu` frames.
- Added board IMU configuration keys: `board_imu_enabled`, `board_imu_source`, `board_imu_device_path`, `board_imu_iio_path`, `board_imu_sample_count`, and `board_imu_sample_interval_ms`.
- Added fake character-device and fake-IIO unit tests for ICM20608 accel/gyro/temp conversion.
- Added ADR-0009 to document the decision to validate MP157 onboard ICM20608 before F407 -> MP157 serial integration.

### Changed

- Reordered the near-term Sensor Integration plan: MP157 onboard ICM20608 validation now comes before F407 -> MP157 live serial integration.
- Changed the MP157 board IMU default source to `char_device` after board-side verification showed the current image exposes `/dev/icm20608`, not an IIO IMU node.
- Updated README, project design, Stage 1 plan, and repository structure docs for the MP157 board IMU path.
- Added MSVC `/FS` for the MP157 CMake target to avoid Debug PDB write conflicts during local Visual Studio builds.

### Verified

- `cmake -S mp157/outdoor-core-service -B mp157/outdoor-core-service/build`
- `cmake --build mp157/outdoor-core-service/build`
- `ctest --test-dir mp157/outdoor-core-service/build -C Debug --output-on-failure`
- `powershell -ExecutionPolicy Bypass -File mp157/outdoor-core-service/scripts/verify_runtime.ps1`
- Fake-IIO Runtime smoke test with `--board-imu` reported `board_imu.enabled=true`, `board_imu.seen=true`, and `sample_count=2`.
- MP157 serial console confirmed Linux `5.4.31-g886e225be`, OpenSTLinux, device tree node `/soc/spi@44004000/icm20608@0`, SPI device `spi0.0`, and `modprobe icm20608` printing `ICM20608 ID = 0XAE`.
- Temporarily transferred and ran ALIENTEK `22_spi/icm20608App` on MP157; `/dev/icm20608` returned live samples with Z acceleration around `0.97g` and temperature around `39.5°C`.
- Cross-compiled the project Runtime with Windows host `arm-none-linux-gnueabihf-g++ 9.2.1` and deployed it to MP157 over the CH340 serial console.
- MP157 onboard Runtime validation passed: `board_imu.enabled=true`, `board_imu.seen=true`, `source=icm20608_char`, `sample_count=5`, `last_error=""`, Z acceleration around `0.983g`, and temperature around `36.5°C`.

## 2026-06-19

### Added

- Added F407 I2C2 initialization on PB10/PB11 and enabled the STM32F4 HAL I2C driver sources.
- Added a board-level I2C memory read/write BSP for `HAL_I2C_Mem_Read/Write`.
- Added an ICM42688 firmware data source with `WHO_AM_I` validation, 100 Hz accel/gyro configuration, temperature conversion, and 14-byte burst sample reads.
- Added ICM42688 INT1 PB12 GPIO BSP wrapper.
- Verified the real ICM42688 data path on F407 after correcting SCL/SDA wiring.

### Changed

- Replaced the temporary module naming and planning assumption with ICM42688.
- F407 IMU frames now prefer real ICM42688 samples and fall back to Mock IMU when initialization or reads fail.
- Heartbeat `status_flags` now report ICM42688 ready, IMU fallback, and IMU error states.
- Refreshed README and Stage 1 docs to mark ICM42688 board validation complete and keep MP157 live serial integration as the next milestone.
- Changed the F407 IMU polling and reporting period from 100 ms to 10 ms for a 100 Hz Sensor Hub target.
- Updated the F407 UART verification script to report IMU frame rate and fail below the configured minimum IMU rate.
- Documented the F407/ICM42688 power-on sequence, firmware initialization flow, runtime workflow, and UART Bootloader verification sequence.

### Verified

- UART capture after flashing reported 55 frames in 5 seconds, 5 heartbeat frames, 50 IMU frames, zero CRC errors, and `last_heartbeat_status_flags=0x0001`.
- 100 Hz UART capture after flashing reported 506 frames in 5 seconds, 5 heartbeat frames, 501 IMU frames, `imu_rate_hz=100.2`, zero CRC errors, and `last_heartbeat_status_flags=0x0001`.

## 2026-06-17

### Added

- Reserved F407 pins for the planned IMU integration: PB10, PB11, and PB12.
- Added a minimal PB12 data-ready GPIO BSP wrapper for future IMU driver work.

### Changed

- Restored the USART1 Sensor Hub runtime entry after CubeMX regeneration so heartbeat and Mock IMU reporting remain the firmware baseline.
- Replaced the previous incorrect module planning assumption pending updated module documentation.
- Extended the F407 UART flashing script erase-stage ACK timeout to make normal global erase reliable after CubeMX pin updates.

### Not Implemented

- Real register/protocol access was intentionally deferred until the correct module documentation was available.

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
