# Changelog

## 2026-07-21

### Added

- Added an explicit-confirmation Stage 2 board acceptance harness that preserves/restores Runtime active/enabled state and records socket, Agent context, SIGTERM, failure-restart, hash, and journal evidence.
- Added a host contract verifier with six negative fixtures, ADR-0028, and deployment of the ARM `unix_status_service_tests` binary for isolated stale/active socket lifecycle checks.
- Added schema-v1 bounded Local Agent request/response types, a replaceable `IAgentBackend`, synchronous `LocalAgentService`, deterministic `mock_no_inference`, and JSON serialization.
- Added `outdoor_agent_terminal` with JSON/text output and opt-in read-only Runtime status context, plus boundary/error/backend/CLI tests and ADR-0027.
- Added a systemd `Type=simple` Runtime unit, headless live-source service config, volatile `/run/outdoor-agent` status/socket paths, bounded failure restart, SIGTERM handling, sandboxing, and a closed device allow-list.
- Added Runtime supervision contract verification plus negative fixtures for disabled restart, unbounded restart, widened device policy, and disabled service socket.
- Added ADR-0026 and a deployment flow that installs the Runtime unit/config by default but requires explicit switches to enable or start it.
- Added an optional cooperative Unix domain socket Runtime status service with bounded `GET_STATUS`/`PING` protocol, 0660 permissions, stale/active path safeguards, fixed client/request/idle limits, and no third-party dependency.
- Added `outdoor_status_query`, shared file/socket JSON serialization, `ipc` status fields, configuration/CLI switches, deployment packaging, cross-platform tests, Stage 2 plan, and ADR-0025.
- Added an always-on F407 host-test check so Release builds execute assertions and assertion-contained calls instead of losing them to `NDEBUG`.
- Added a validated `SENSOR_HUB_I2C2_CLOCK_HZ` CMake cache setting for controlled 100 kHz/400 kHz F407 sensor-bus A/B builds. The default 400 kHz path injects no override; unsupported values fail configuration.
- Added F407 documentation for building an ICM-only 100 kHz non-production image and requiring a full power cycle before board conclusions.

### Changed

- MP157 deployment now creates `/opt/outdoor-agent/tests`, packages the socket self-test and Stage 2 acceptance script, and syntax-checks the deployed shell script; it still does not execute acceptance or enable/start Runtime by default.
- `UnixStatusClient` now accepts a caller-selected response limit; the Agent terminal constrains Runtime context reads to 64 KiB while the standalone query client retains its 1 MiB default.
- MP157 deployment now packages `outdoor_agent_terminal`; this does not enable a model runtime or start any Agent daemon.
- MP157 deployment now packages `outdoor-agent-runtime.service` and `outdoor_agent_service.conf`; `-StartRuntimeService` implies `-EnableRuntimeService`, while the default performs no Runtime enable/start action.
- MP157 deployment packaging now includes both `outdoor_core_runtime` and `outdoor_status_query`; the status socket remains disabled by default so existing finite file/mock runs retain their prior lifecycle.

### Verification

- Stage 2 acceptance contract and its six weakening fixtures passed; PowerShell parser, Git for Windows `sh -n`, and the no-confirm exit-2 guard passed. GNU ARM Linux 9.2.1 linked the 64336-byte socket self-test. The board acceptance script was not executed.
- MP157 Windows GCC Release CTest passed 14/14, including Local Agent boundary, terminal help, and deterministic mock smoke; GNU ARM Linux 9.2.1 linked the Agent terminal and all targets. No real inference or board execution was performed.
- Runtime supervision contract and four negative fixtures passed; the full Runtime verifier loaded the service config and completed a file/mock no-hardware smoke. No deployment or systemd state change was executed.
- MP157 Windows GCC Release CTest passed 11/11, Runtime verifier passed, and GNU ARM Linux 9.2.1 linked all Runtime, query-client, and Unix socket test targets. No COM3/COM9, deployment, reset, power-cycle, or board action was performed.
- F407 GCC Release and MSVC Debug CTest passed 7/7 after replacing standard `assert`; the F407 ARM firmware build also passed.
- F407 host CTest passed 7/7. The formal 400 kHz image remained byte-identical at 19384 bytes and SHA256 `f0addc29...b9d1e`; the 400 kHz ICM-only baseline remained 15072 bytes and SHA256 `c07e235a...2f80a`.
- Built the ICM-only 100 kHz image at 15072 bytes with SHA256 `56b31d76...c3e56`; its Ninja rules contain the 100 kHz definition while the 400 kHz baseline contains no override. Invalid clock `123456` was rejected as designed.
- Did not claim deployment success: the first COM6 session returned `0xF9` at the first write address, while the second read chip ID as `0x0493` instead of `0x0413` and failed byte-for-byte verification at `0x0800013D`. The F407 application Flash is currently untrusted pending a physical COM6 reconnect and a fresh verified programming session.
- After physically reconnecting the COM6 USB/USB-UART and keeping terminals closed, a fresh session reported Bootloader `0x31` and chip ID `0x0413`, then completed erase, programming, byte-for-byte readback, and GO for the 100 kHz image.
- Completed the full-power single-device A/B in `/run/media/mmcblk1p1/outdoor-agent-health-preflight-wB2xhs`. The 100 kHz image degraded to `0xD206/init step 1`; recovery/final-failure rates were `13.104/2.831 per second`, or `3.29/3.67` times the comparable 400 kHz baseline. The lower FIFO counters came from lost readiness/fallback and were not treated as an improvement. This falsifies the simple “400 kHz is too fast” hypothesis.
- The first rollback attempt stopped before opening the serial port because COM6 no longer existed, so no erase or write occurred. The user identified the re-enumerated F407 USB-UART as COM3; PnP and consumer checks passed, and a COM3 session restored the 15072-byte 400 kHz ICM-only image with Bootloader `0x31`, chip ID `0x0413`, byte-for-byte readback, and GO. Electrical/waveform evidence is still required before 30/60-second or endurance validation.

## 2026-07-20

### Changed

- Completed the full-power ICM-only 8-second preflight in `/run/media/mmcblk1p1/outdoor-agent-health-preflight-gsVcum`: the diagnostic image reported exactly `0x8181`, init step 0, real IMU data, and no magnetometer/barometer frames, confirming both initialization recovery and software isolation.
- Kept the validation failed because cumulative diagnostics still reported I2C recovery/failure `660/105`, HAL timeout `0x20`, and FIFO overflow/malformed/empty/skipped `707/126/39/2933`. The 30/60-second runs were intentionally not started; diagnosis is now bounded to the ICM-only electrical/FIFO-recovery path pending voltage, pull-up, continuity, and waveform evidence.
- Reclassified `gsVcum` as contaminated rather than a clean full-power result after the user reported that COM6 had remained open. The raw evidence and failure counters are retained, but a new full power cycle with COM6 closed is required before repeating the 8-second gate or drawing an electrical root-cause conclusion.
- Repeated the full-power ICM-only gate with COM6 closed in `outdoor-agent-health-preflight-XMWvGC`. It again reported `0x8181` and init step 0 but accumulated I2C recovery/failure `232/45` plus FIFO overflow/malformed/empty/skipped `186/21/20/671` by 58.298 seconds; the last failure was a 112-byte `0x69/FIFO_DATA` read with HAL ACK failure `0x04`. This falsifies COM6-open as a necessary condition and moves the investigation to single-device electrical and waveform evidence.
- Recorded a host PowerShell quoting failure that occurred before COM9 opened, then reran with a single-quoted remote command and captured the only valid board preflight without duplicating Runtime execution.

## 2026-07-19

### Added

- Added a default-off `SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC` firmware option that compiles out MMC5603/BMP390 initialization and polling and marks the heartbeat with bit `0x8000`.
- Added an `icm42688_only` health-preflight profile requiring `0x8181`, absent magnetometer/barometer frames, and zero I2C/FIFO/init/UART4-drop diagnostics; the existing `full` profile remains the default used by formal endurance tests.
- Added a dependency-free C++17 `tools/compass_calibrator` that reads History Recorder magnetometer CSV, fits hard-iron bias and a full 3x3 soft-iron correction, and emits Runtime-compatible candidate configuration.
- Added normalized full-ellipsoid least squares, symmetric 3x3 Jacobi eigendecomposition, determinant-preserving scale, two-stage robust outlier rejection, orientation/axis/residual quality gates, and optional independently measured sensor-to-body rotation composition.
- Added ADR-0024, tool documentation, and always-on synthetic tests for full-matrix/bias recovery, planar rejection, gross magnetic outliers, mount rotation, invalid rotation, CSV loading, and forced `compass_calibration_valid=false` output.
- Added a backward-compatible 48-byte Sensor Hub diagnostics schema 1 tail extension with the UART4 RX cumulative drop count, while retaining legacy 44-byte schema 0 parsing and rejecting unknown extension versions.
- Added F407 frame-builder coverage and MP157 parser/status JSON coverage for the new diagnostics fields; F407 PC CTest now passes 7/7 and MP157 CTest passes 10/10.
- Added preflight, sampled timeline, offline audit, and COM6-verifier gates requiring diagnostics schema 1 and zero UART4 RX drops for formal validation.
- Added cumulative Runtime logger rotation/write failure counters and a structured last error, published in the storage status and aggregated into `storage.last_error`.
- Added deterministic logger rotation-failure injection and non-zero status-publisher JSON tests; MP157 CTest now passes 10/10.
- Added a configurable MP157 `CompassEstimator` with hard-iron bias, a full 3x3 soft-iron/sensor-to-body transform, accelerometer tilt compensation, magnetic declination, and input quality gates.
- Added a `compass` runtime status object, focused MCU fixture, compass boundary tests, ADR-0022, and troubleshooting item TRB-025.
- Added ADR-0021 and a dedicated MMC5603 provider test for the shared-I2C2 transaction-storm root cause; that step raised F407 PC CTest to 6/6, before the diagnostics frame-builder test raised it to 7/7.
- Added an 8-second real-board health preflight that gates endurance and crash-recovery runs on GNSS/MCU/board-IMU presence, ping ACK, F407 `0x01A9`, controlled Runtime stop, and empty core errors.
- Added a one-second board health timeline and an offline audit for sample coverage, fatal heartbeat bits, core errors, healthy ratio, maximum degradation streak, final health, and monotonic Sensor Hub diagnostics.
- Added ADR-0019 and ADR-0020 for sampled endurance health evidence and UART4 Sensor Hub diagnostics respectively.
- Published the existing 44-byte `0x02` Sensor Hub diagnostics frame on UART4 as well as USART1, parsed it on MP157, and exposed I2C/FIFO counters plus last-failure context in `runtime_status.json`.
- Added `deploy_mp157_runtime.ps1` for repeatable COM9/XMODEM deployment to `/opt/outdoor-agent`, including permissions, ordered ICM20608 service installation, SHA256 verification, shell syntax checks, and device/service gates.
- Added a bounded GNSS fix verifier with status/RMC/GGA/CSV evidence gates and a dedicated valid-only MCU mock fixture; a real indoor negative run correctly rejects NMEA reception without a satellite fix.
- Added a versioned MP157 board endurance script that checks required devices, records Runtime SHA256/kernel/SD metadata, and starts bounded dual-serial/board-IMU/framebuffer/history validation in a unique evidence directory.
- Added and soft-reboot-verified `outdoor-agent-icm20608.service`, ordered after the vendor `link-modules.service`; it replaces an initial modules-load.d attempt that ran before `/lib/modules/<kernel>` existed.
- Added a `HistoryRecorder` that writes separate GNSS, MCU IMU, magnetometer, barometer, and board IMU CSV files with change-based deduplication and periodic flushes.
- Added `history_enabled`, `history_output_path`, `history_flush_interval_ms`, `--history`, and `--history-output`; relative history paths resolve under the storage root when SD storage is enabled.
- Added size-based Runtime log rotation with configurable maximum bytes and backup count, plus rotation/history fields in `runtime_status.json`.
- Added History Recorder and Logger unit tests; MP157 CTest now passes 8/8.
- Added ADR-0018 comparing CSV, JSON Lines, SQLite, date-based logs, and size-based rotation.
- Added Runtime Manager scheduler tests covering cooperative interleaving, completion, service failure, stop predicates, loop callbacks, and startup rollback.
- Added periodic runtime status publishing with active/completed service counts, SIGINT/SIGTERM handling, and `runtime_run_seconds` for bounded endurance runs.
- Added ADR-0017 for the MP157 cooperative service scheduler and recorded the implementation/verification evidence in troubleshooting item `TRB-20260719-007`.
- Added `docs/troubleshooting_log.md` with a problem index, evidence levels, detailed retrospectives, failed/reverted experiment history, and a reusable issue template for interview-ready engineering records.
- Added a repository-wide requirement to create and maintain a troubleshooting record while problems are being analyzed, rather than documenting only the final fix.
- Added a `0x02` Sensor Hub diagnostics frame with cumulative I2C/FIFO counters and last-failure context; it was initially USART1-only and is now also published on UART4.
- Added a shared real/Mock IMU timestamp normalizer that preserves in-batch spacing while keeping the published 100 Hz timeline continuous across FIFO batches and fallback transitions.
- Added ICM42688 byte-count FIFO support with a 64-byte watermark, threshold/full INT1 routing, variable 16/8/message packet parsing, batch timestamps, and FIFO recovery diagnostics.
- Added fixed 1024-byte UART4 and USART1 TX queues driven by `HAL_UART_Transmit_IT()` and completion callbacks.
- Added host tests for FIFO packet/stream parsing, provider register configuration and recovery, and UART queue wrap/atomic frame rejection; F407 PC CTest now passes 5/5.
- Added heartbeat diagnostics for FIFO active/error, UART4/USART1 TX overflow, ICM initialization failure, UART4 RX overflow, and I2C runtime transaction failure.
- Added ADR-0016 documenting the FIFO byte-count and interrupt-driven UART TX queue decisions.
- Expanded `scripts/verify_f407_uart.ps1` to decode heartbeat, IMU, magnetometer, and barometer payloads and validate rates, timestamps, protocol fields, CRC, ready/error flags, and broad physical ranges.
- Added ADR-0014 documenting the F407 I2C2 runtime recovery decision.
- Added PB12 EXTI12 event accounting and heartbeat bit `0x0080` so board validation can prove the ICM42688 INT1 data-ready path is active.
- Added fixed-point first-order IIR filters for IMU, magnetometer, and barometer samples plus dedicated host tests.
- Added ADR-0015 documenting the INT1 event-driven read and lightweight filtering decisions.

### Changed

- Isolated shared-I2C diagnosis from manual wiring assumptions: a reported ICM-only setup still emitted BMP390 data and accessed MMC5603 `0x30/STATUS1`, so that preflight is retained as invalid isolation evidence rather than attributed to the IMU.
- Reverted the unsuccessful 100 kHz and fixed 128/64-byte partial-FIFO experiments after real-board A/B testing; restored 400 kHz record-count reads that fetch only the reported number of complete 16-byte records, and removed temporary raw-header diagnostic reuse.
- Versioned the Sensor Hub diagnostics tail instead of silently changing the 44-byte payload meaning; the F407 now emits schema 1 and the MP157 reports both `schema_version` and `uart4_rx_drop_count`.
- Made board preflight and endurance audit reject non-zero logger rotation/write failures or a non-empty logger error.
- Replaced the framebuffer direction card's fixed 62.3-degree placeholder with a valid compass heading, falling back to GNSS course only for a valid fix moving at least 1 km/h.
- Updated derived compass state immediately after each applied MCU frame so the dashboard and status publisher observe the same sensor update.
- Changed ICM42688 FIFO from byte-count/64-byte watermark to big-endian record-count/4-record watermark, stop parsing on the FIFO-empty message header, and read only complete 16-byte records.
- Increased shared I2C2 from 100 kHz to 400 kHz and removed the runtime ICM42688 `INT_STATUS` transaction.
- Bounded MMC5603 one-shot completion handling: wait 7 ms after trigger, then perform at most three `STATUS1` reads with 1 ms spacing instead of tight-polling for up to 20 ms.
- Kept a single FIFO read failure observable without immediately entering Mock fallback; fallback now requires three consecutive failures.
- Preserved ICM42688 cumulative FIFO diagnostics across runtime reinitialization while retaining an explicit cold-start reset, so one F407 boot produces monotonic evidence.
- Made the board preflight require a decoded Sensor Hub diagnostics frame, intentionally rejecting mismatched old F407 firmware before a formal run starts.
- Changed GNSS/MCU serial history recording from one scheduler-round snapshot to a callback after each successfully applied sentence/frame, preventing same-read updates from overwriting earlier history samples.
- Extended the Runtime loop callback to observe sensor state changes for history recording without adding a permanently active service.
- Extended storage verification to create all five history files, require GNSS/MCU IMU records, and validate history/rotation status fields.
- Replaced the MP157 Runtime's blocking sequential `IService::run()` model with bounded `poll()` steps in a single-thread cooperative scheduler.
- Made GNSS/MCU serial input, board IMU sampling, dashboard refresh, and evdev launcher handling coexist without blocking the Runtime loop; zero capture/sample/refresh values now mean continuous operation.
- Enabled `FIFO_WM_GT_TH`, initialized ICM42688 after the other I2C sensors, and rate-limited FIFO event consumption to a minimum 30 ms interval.
- Made FIFO_DATA reads non-replayable: failures recover I2C2 and flush the FIFO instead of repeating a destructive stream read.
- Changed heartbeat FIFO/I2C bits to report current unrecovered state while USART1 diagnostics retain cumulative historical failures.
- Replaced blocking UART4/USART1 application transmission with atomic frame enqueue and interrupt-driven draining; UART4 RX remains a separate 64-byte interrupt ring buffer.
- Increased the I2C transaction timeout from 20 ms to 30 ms so a 256-byte FIFO read at 100 kHz has sufficient transfer time.
- Reset I2C runtime diagnostics after initialization so expected alternate-address probe NACKs do not report a runtime fault.
- Prevented Mock IMU, MMC5603, and BMP390 polling from catch-up bursts; Mock output now has one scheduler so failed FIFO events or INT noise cannot double-publish the same timestamp.
- Removed the unused PC mock `Icm42688Driver` placeholder to keep the real F407 provider authoritative.
- Replaced the ICM42688 fixed 10 ms polling read with INT1 data-ready events; I2C reads remain in the main loop rather than the ISR.
- Configured the ICM42688 internal accel/gyro UI LPF to approximately 25 Hz while MMC5603 and BMP390 remain polling-based.
- Added a 250 ms INT1 timeout, Mock IMU fallback, and 1-second ICM42688 reinitialization path.

### Fixed

- Fixed the first compass-calibrator fit being vulnerable to a few extreme magnetic points by adding a median/radial-MAD gross prefilter before the ellipsoid solve and residual trimming after it.
- Replaced calibrator tests' raw `assert` checks, which opened an MSVC modal dialog in Debug and compiled away under GCC Release, with always-on checks that print the failed expression and return non-zero in every build type.
- Corrected a synthetic test that incorrectly required near-uniform directional variance after intentionally applying a strong full-matrix distortion; the metric now proves three-dimensional coverage while matrix and residual assertions prove recovery accuracy.
- Fixed the compass integration verifier first assuming the default negative MCU fixture contained magnetometer data, then sampling a finite-refresh dashboard before the focused fixture had been consumed.
- Fixed the parser continuing past the ICM42688 FIFO-empty message header and amplifying one marker into many skipped entries.
- Bounded MMC5603 tight status polling so it no longer performs an unbounded shared-I2C2 status loop; full-power regression later proved this improvement did not eliminate the remaining runtime I2C/FIFO fault.
- Fixed the endurance audit blind spot where a recovered final `0x01A9` could hide periodic `0x022E/0x03AE` FIFO fallback during the run.
- Fixed the diagnostics parser test fixture from 43 to the required 44 bytes and configured MSVC tests to report failed assertions without blocking on a modal dialog.
- Fixed MP157 MCU IMU history dropping from an upstream 104.2 Hz to roughly 57-65 Hz because multiple decoded frames shared one end-of-round status snapshot; the framebuffer board recheck now records about 98.1 Hz (`TRB-20260719-011`).
- Fixed a new History Recorder test fixture that used the converted-unit name `accelZG` instead of the protocol field `accelZMg`; the failure and regression evidence are recorded as `TRB-20260719-008`.
- Removed the false FIFO drain-stall heuristic that compared counts across a concurrently growing FIFO.
- Fixed batch-boundary and real/Mock transition timestamp regressions and large gaps.
- Fixed I2C recovery classification so a transient GPIO line sample does not suppress the retry after I2C2 reinitializes successfully.
- Fixed a shared I2C2 runtime lockup that changed the heartbeat from `0x0029` to `0x0056` and stopped all three real sensor streams.
- I2C2 transfers now hard-reset the STM32 peripheral, release a stuck bus with up to 18 SCL pulses plus STOP, and retry the failed transaction once.
- Changed the F407 verification script default port from stale `COM3` to current `COM6`.

### Verified

- The default-off diagnostic option leaves the formal F407 image unchanged at 19384 bytes and SHA256 `f0addc29...b9d1e`. The ICM-only image builds at 15072 bytes with SHA256 `c07e235a...2f80a`, and passed COM6 programming plus byte-for-byte readback. The updated MP157 preflight script passed host/board shell syntax and matching SHA256 deployment; a new full power cycle is still required before isolation results can be claimed.
- A hot-reset `icm42688_only` smoke run correctly reported no magnetometer/barometer frames, zero FIFO counters, schema 1/drop 0, and the diagnostic bit, while rejecting `0xD006` plus 898 ICM initialization failures. This validates the isolation profile but intentionally does not count as the required full-power result.
- A full power cycle restored ICM42688 initialization and let preflight `rjgpa6` reach `0x01A9`, but cumulative diagnostics already showed recovery 234/overflow 56; about 73 seconds later preflight `Hq1vCJ` reached `0x03A9` and blocked formal testing. The 100 kHz, fixed 128-byte, and fixed 64-byte A/B images also failed. A temporary parse-failure capture produced candidate headers `06 FF FF FF`; all experimental code was then removed.
- The restored formal F407 image builds as 19384 bytes with text 19364 bytes, data 20 bytes, BSS 4408 bytes, and SHA256 `f0addc29...b9d1e`; F407 CTest 7/7, ARM clean build, COM6 programming, and byte-for-byte readback passed. This hot reset is not counted as a new sensor power-cycle validation.
- Compass calibrator MSVC Debug CTest passes 3/3 and GCC 16.1 Release CTest passes 3/3 with `-Wall -Wextra -Wpedantic` clean. Synthetic 1600-point data recovers the known hard-iron bias within 0.05 uT, every full-matrix element within 0.01, and RMS residual below 0.2%; a separate gross-outlier case is accepted only after rejecting all injected extreme points, and a CSV-to-candidate-config CLI fixture passes end to end.
- The schema 1 baseline image built as 19376 bytes with text 19356 bytes, data 20 bytes, BSS 4408 bytes, and SHA256 `365fb9c3...129c`; F407 CTest 7/7, ARM build, COM6 programming, and byte-for-byte readback passed. The current 258560-byte MP157 Runtime SHA256 is `47c4bcc9...b739`.
- A four-sample board monitor probe produced 23 columns per row with schema 1/drop 0. A 40960-byte controlled UART4 downlink then produced heartbeat `0x302E` and `uart4_rx_drop_count=2231`; preflight `/run/media/mmcblk1p1/outdoor-agent-health-preflight-jYMnDJ` rejected the non-zero count. After an F407 application reset, `/run/media/mmcblk1p1/outdoor-agent-health-preflight-XOXgzn` returned to schema 1/drop 0 and still rejected only the independent ICM42688 `0x102E` sensor-health failure.
- The 258560-byte ARM Runtime with SHA256 `330b7bc4...a645` was deployed to `/opt/outdoor-agent`; a real MP157 fixture published zero logger failures and an empty logger error. Preflight `/run/media/mmcblk1p1/outdoor-agent-health-preflight-FImCIn` passed all three logger gates and still rejected only the independent F407 `0x102E` sensor-health failure.
- MP157 Debug CTest passes 9/9, including cardinal headings, declination, hard-iron correction, 30-degree tilt compensation, stale samples, field rejection, and singular-matrix rejection. Runtime verification and ARMv7 hard-float cross-build pass. The 254152-byte ARM Runtime deployed with SHA256 `8f4c1423...10d5`; a deterministic MP157 fixture run produced matching JSON/dashboard heading `30.303°` with uncalibrated quality. Real sensor calibration remains pending.
- The first PID 2005 endurance run was invalidated after a BusyBox `ps` truncation let a second Runtime compete for serial devices; `/proc/<pid>/cmdline` guards now reject concurrent Runtime processes. PID 3127 was invalidated when an experimental COM6 open reset F407, and PID 4868 was invalidated after transient FIFO fallback exposed the missing health timeline. No hour-long result is currently claimed.
- Two real 8-second preflights passed at `0x01A9`; the first automatically released a 60-second joint smoke test whose five CSV rates, final state, ping ACK, controlled stop, and three log backups passed audit.
- A 20-second real-board timeline smoke correctly failed its new audit after recording five unhealthy samples, `healthy_permille=750`, maximum unhealthy streak 1, and final `0x03AE`, proving that recovered periodic fallback is no longer hidden.
- A real MP157 60-second dual-serial/board-IMU/framebuffer/SD-history smoke run stopped cleanly with all five CSV files, four MCU sensor states, and ping ACK status 0; the corrected 20-second framebuffer run recorded MCU IMU at about 98.1 Hz.
- A real MP157 VFAT SD-card 30-second info-log run created a 718 KiB active log and a 1.0 MiB `.1` backup, proving the default size-based rotation path.
- MP157 Debug build and CTest pass 9/9; Runtime storage/history/compass verification and the ARMv7 hard-float cross-build succeed. Real SD-card hour-long, power-loss, and compass calibration validation remain pending.
- FIFO parser/provider, MMC5603 provider, UART TX queue, diagnostics frame builder, and the existing mock/filter host tests pass 7/7.
- The previously board-validated FIFO/TX queue ARM Release firmware built as 19368 bytes with 4424 bytes BSS. Two real 30-second UART4-diagnostics runs correctly failed on cumulative I2C/FIFO deltas, first reducing `skipped` from 9281 to 290 and then exposing MMC5603 tight polling as the shared-bus root cause.
- An intermediate record-count/bounded-MMC image built as 19356 bytes with 4408 bytes BSS and SHA256 `d220cbc7...ad56`; programming and byte-for-byte readback passed. A subsequent `0x102E` preflight correctly blocked formal testing because ICM42688 did not ACK after hot flashing; the later full-power regression restored ACK but still failed on cumulative runtime diagnostics.
- A final 10-second COM6 capture received 1290 frames with 0 CRC/protocol/payload errors: IMU 97.8 Hz, magnetometer 20.1 Hz, barometer 10.1 Hz, monotonic IMU timestamps, and no UART4/USART1 TX overflow. The heartbeat remained `0x502E` because the unpowered-reset ICM42688 still did not ACK, so Mock fallback output is not counted as FIFO proof.
- The MP157 `/dev/ttySTM1` raw capture received 4096 bytes of continuous MCU frames, and ARM Runtime confirmed heartbeat/IMU/magnetometer/barometer plus `command_ping -> command_ack` with status 0 and nonce `0x50494E47`; this verifies the UART4 TX queue, RX ring, and ack TX path.
- Final real-sensor FIFO validation passed after a full F407/ICM42688 power cycle and two independent final-image programming resets.
- The final 60-second COM6 gate passed with 7988 frames: heartbeat 60, IMU 6086, magnetometer 1187, barometer 595; rates were 101.43/19.78/9.92 Hz, final heartbeat was `0x01A9`, maximum IMU timestamp gap was 10 ms, and CRC/protocol/payload/TX-overflow checks were clean.
- MP157 `/dev/ttySTM1` revalidation saw all four upstream states with `status_flags=0x01A9` and received a ping ACK with status 0 and nonce `0x50494E47`.
- The 18808-byte firmware image built, programmed through the COM6 ROM Bootloader, and passed byte-for-byte readback verification.
- A 60-second COM6 capture passed with 7532 frames: heartbeat 60, IMU 5666, magnetometer 1204, and barometer 602; the final heartbeat remained `0x0029`.
- Two additional independent-reset 30-second captures also passed with final heartbeat `0x0029`.
- Measured rates were 94.43 Hz IMU, 20.07 Hz magnetometer, and 10.03 Hz barometer; CRC, protocol-version, payload-length, timestamp, and physical-range checks all passed.
- The 20356-byte INT1/filter firmware image built, programmed through COM6, and passed byte-for-byte readback after one failed download transaction was retried from a fresh Bootloader session.
- A 60-second INT1 capture passed with 7412 frames and final heartbeat `0x00A9`; IMU, magnetometer, and barometer rates were 92.45, 20.05, and 10.03 Hz.
- `icm42688_int1_active=true`; CRC, protocol, timestamp, and physical-range checks passed, and F407 PC CTest passed 2/2.
- Three additional independent-reset 10-second INT1 captures passed with final heartbeat `0x00A9`; all streams changed over time and no CRC or protocol errors were observed.

## 2026-07-18

### Added

- Added UART4 interrupt-driven single-byte receive with a fixed 64-byte ring buffer for F407 downlink commands.
- Added `scripts/send_xmodem.ps1` for reliable XMODEM-CRC uploads through the MP157 serial console, including remote padding truncation.
- Added ADR-0013 documenting the UART4 polling, interrupt ring-buffer, and DMA alternatives.

### Fixed

- Fixed intermittent loss of MP157 `command_ping` frames while the F407 main loop was occupied by blocking sensor-frame transmission and USART1 diagnostic mirroring.

### Verified

- F407 COM6 UART Bootloader programming and byte-for-byte verification passed for the 18156-byte firmware image.
- COM6 capture passed with valid heartbeat/IMU/magnetometer/barometer traffic and zero CRC errors; the final all-sensors-ready heartbeat was `status_flags=0x0029`.
- Three consecutive MP157 raw pings produced three valid F407 command acknowledgements.
- Current MP157 ARM Runtime board validation passed with heartbeat, IMU, magnetometer, barometer, and command ACK all seen; ACK status was 0 and nonce was `0x50494E47`.
- XMODEM transfer size and SHA256 verification passed on the board.
- MP157 CTest passed 5/5, F407 PC CTest passed 1/1, Runtime verification passed, and both ARM builds passed.

## 2026-07-05

### Added

- Added an explicit `outdoor-agent` APP icon to the framebuffer dashboard main screen.
- Added text dashboard markers `app_icon` and `app_icon_visible: true` so PC verification can cover the icon display requirement.
- Added Runtime verification checks for APP icon visibility in both the default dashboard output and the board-side APP profile smoke test.
- Added a minimal fbdev/evdev APP launcher for MP157: the screen now starts on a central `OUTDOOR AGENT` icon/tile and can enter the dashboard after the launcher condition is satisfied.
- Added `launcher_enabled`, `launcher_input_device`, and `launcher_auto_start_seconds` config keys plus CLI overrides for board-side validation.

### Changed

- Reworked the left sidebar title block so the icon and `OUTDOOR / AGENT / APP` labels fit inside the header area.
- Updated README, project design, Stage 1 plan, and module README to document the screen main interface icon.
- Expanded the launcher touch target to the full icon/tile area and accepted coordinate-only evdev frames for better touchscreen compatibility.

### Verified

- MP157 local CMake configure and build passed.
- CTest passed: 5/5 tests.
- Runtime verification script passed, including the new APP icon marker checks.
- `git diff --check` passed.
- MP157 ARM cross-build passed; deployed package over COM3 with XMODEM checksum mode.
- MP157 `/dev/fb0` reported 1024x600 at 16 bpp.
- Board-side `outdoor-agent` APP profile rendered to `/dev/fb0` successfully; `run_outdoor_agent_app.sh` returned `APP_SCRIPT_EXIT:0`.
- Text dashboard markers confirmed `app_icon_visible: true`; framebuffer sample was nonblank.
- MP157 board confirmed Goodix touch input at `/dev/input/touchscreen0 -> event0`.
- Board-side launcher auto-start validation passed with `--launcher-auto-start-seconds 2`; logs confirmed launcher display, timeout transition, and framebuffer dashboard rendering.
- Board-side real finger-tap validation passed; logs confirmed `Launcher icon tapped at x=530, y=292`, followed by framebuffer dashboard rendering and `TOUCH_RETEST_DONE:0`.

### Deferred

- Multi-control touch UI, coordinate calibration, and screen-rotation handling are still deferred beyond the minimal APP launcher.

## 2026-06-29

### Added

- Added `config/outdoor_agent_app.conf` as the board-side `outdoor-agent` APP profile for the 7-inch RGB framebuffer dashboard.
- Added `scripts/run_outdoor_agent_app.sh` to launch the Runtime with the APP profile.
- Added Runtime verification coverage for loading the APP profile while overriding framebuffer output to text and one refresh on PC.

### Changed

- Changed `DashboardService` so `dashboard_refresh_count = 0` means a long-lived refresh loop, suitable for the screen APP process.
- Documented the split between `config/runtime.conf` for PC/default verification and `config/outdoor_agent_app.conf` for the board-side APP process.
- Improved F407 UART Bootloader flashing by issuing `GO 0x08000000` after byte-for-byte verification and releasing the one-key download circuit to the application-run state.
- Mirrored F407 Sensor Hub uplink frames to USART1 for COM6 diagnostics while keeping UART4 as the MP157 application link.
- Added I2C2 bus recovery before HAL I2C initialization and initialized UARTs before I2C2 so sensor-bus faults do not remove UART diagnostics.
- Added ICM42688 address fallback from `0x69` to `0x68`.
- Adjusted BMP390 probing to prefer `0x76`, accept Bosch BMP3/BMP390 chip IDs, and read latest compensated data in the 10 Hz polling path.

### Verified

- F407 build passed.
- COM6 UART Bootloader flashing passed with bootloader version `0x31`, chip ID `0x0413`, byte-for-byte verification, and application start through `GO 0x08000000`.
- COM6 USART1 protocol mirror confirmed application frames and zero CRC errors during capture.
- MP157 local build, CTest, and Runtime verification passed, including the new `outdoor-agent` APP profile smoke test.

### Current Status

- ICM42688 is still not ready in the current wiring state: latest capture reported `status_flags=0x0056`, meaning IMU fallback/error remains active.
- MMC5603 was intentionally disconnected during I2C isolation, so magnetometer frames are currently absent.
- BMP390 still has no successful I2C ACK / barometer frame in the current board setup.

## 2026-06-28

### Added

- Added MP157 Runtime SD card storage mode with `storage_enabled`, `storage_root_path`, storage output paths, and `--storage-root`.
- Added optional file logging while preserving stdout/stderr logs.
- Added a top-level `storage` object to `runtime_status.json`.
- Added ADR-0012 for the SD card file-storage decision.

### Verified

- MP157 local CMake build passed.
- Runtime verification script passed, including storage directory, status, dashboard, and log-file creation.
- CTest passed: 5/5 tests.
- MP157 board check confirmed the SD card is mounted at `/run/media/mmcblk1p1`; `/mnt/sdcard` does not currently exist.
- MP157 board storage validation passed with `--storage-root /run/media/mmcblk1p1/outdoor-agent`, producing status, dashboard, and log files on the SD card.

### Deferred

- Long-term sensor history files and log rotation are not implemented yet.

## 2026-06-26

### Added

- 集成 Bosch BMP3 Sensor API v2.0.5 和 F407 `bmp390_provider_c`。
- 新增 `sensor_barometer` (`0x13`) 协议帧、heartbeat BMP390 状态位和 MP157 barometer 状态输出。
- BMP390 自动探测 I2C 地址 `0x77/0x76`，配置 25 Hz normal mode，并以 10 Hz 发布补偿气压和温度。
- 新增 ADR-0011，记录采用 Bosch 官方补偿驱动的原因和边界。

### Verified

- F407 GNU ARM 固件构建通过。
- MP157 本机构建、5 项 CTest 和 ARM 交叉编译通过。
- `frame_decoder` 构建和协议解码路径通过。

### Deferred

- 按当前任务要求未烧录固件，BMP390 真实 I2C、补偿值和 10 Hz 帧上板验证待完成。

## 2026-06-25

### Added

- Added F407 MMC5603 I2C2 provider at address `0x30`.
- Added `sensor_magnetometer` frame type, fixed-point nanotesla payload, MP157 parsing, status output, and dashboard magnetic-strength display.

### Verified

- F407 firmware build and byte-for-byte UART Bootloader flashing passed.
- Five-second board capture reported 100 MMC5603 frames at `19.8 Hz`.
- Average field was approximately `(-35.60, -25.18, 18.16) uT`, magnitude `47.24 uT`.
- MMC5603 ready was set while ICM42688 remained in fallback/error, yielding heartbeat `status_flags=0x000E`.
- UBLOX-M10 UART5 communication was confirmed at 38400 baud with valid NMEA checksums; satellite fix remained invalid in the current environment.

## 2026-06-22

### Changed

- Updated the `outdoor-agent` fbdev framebuffer dashboard to follow the provided dark outdoor terminal reference layout.
- Added custom fbdev drawing primitives for panel borders, line art, circular gauges, arcs, map-style grid lines, bar charts, and bottom status tiles.
- Reworked the 7-inch RGB screen layout with a left navigation rail, top status bar, direction compass, large speed gauge, location/map panel, temperature panel, light-intensity demo panel, and footer status strip.

### Notes

- GNSS, F407 Sensor Hub, and MP157 Board IMU sections continue to use Runtime data.
- Light, air-quality, battery, and signal widgets are UI placeholder/demo metrics until the corresponding real sensors or system status providers are integrated.

### Verified

- Local MP157 build, CTest, runtime verification script, ARM cross-build, and `git diff --check` passed.
- MP157 board validation passed on COM3: current ARM package `91737` bytes matched SHA256 `f32a2aa049a4be72535894ef1922eb7e81a79aae3f8f00319ddb10a62d546086`; the runtime rendered to `/dev/fb0` for 2 refresh cycles and preserved text dashboard markers including `outdoor-agent` and `source: icm20608_char`.

## 2026-06-21

### Added

- Added MP157 UART5 GNSS serial input software path for UBLOX-M10 NMEA, defaulting to `/dev/ttySTM2` at 9600 8N1.
- Added `GnssStatus` and `gnss` output in `runtime_status.json`.
- Extended NMEA parsing from RMC/GGA to RMC/GGA/VTG/GSA/GSV.
- Added text dashboard prototype output at `runtime/dashboard.txt`.
- Added `outdoor-agent` fbdev framebuffer dashboard APP prototype for the 7-inch RGB screen.
- Added GNSS and dashboard configuration keys: `gnss_input_mode`, `gnss_serial_device`, `gnss_serial_baud`, `gnss_serial_capture_seconds`, `dashboard_enabled`, `dashboard_output_path`, `dashboard_output_mode`, `dashboard_framebuffer_device`, `dashboard_refresh_count`, and `dashboard_refresh_interval_ms`.
- Added an `AI LOCAL AGENT: PLANNED` placeholder panel in the screen dashboard for future local AI deployment and interaction.
- Added ADR-0010 for UART5 NMEA input and text dashboard prototype.
- Added NMEA parser tests covering RMC/GGA/VTG/GSA/GSV and checksum rejection.

### Verified

- MP157 local build passed.
- CTest passed with 5 tests, including the new NMEA parser test.
- Runtime verification script passed and now checks `gnss` status plus dashboard output.
- MP157 ARM cross-build passed.
- MP157 board validation passed on COM3: `/dev/fb0` reported `1024,600` at 16 bpp; current ARM package `80025` bytes matched SHA256 `c27907d312d95e1278537e8d8bff20555d81a0d05be8f9c8848d70d6859079c1`; the runtime rendered `outdoor-agent` to `runtime/dashboard.txt` and `/dev/fb0` for 3 refresh cycles; dashboard markers confirmed `ai_agent_state: planned` and `source: icm20608_char`.

### Deferred

- UBLOX-M10 UART5 real board validation is pending.
- `/dev/ttySTM2` and 9600 baud are current software defaults; both must be confirmed on MP157 with the actual module.
- The dashboard is currently a lightweight fbdev APP prototype, not a full GUI toolkit, touch UI, or real AI Agent interaction layer.

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
