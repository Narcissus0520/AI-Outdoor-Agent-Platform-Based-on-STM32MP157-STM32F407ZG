$ErrorActionPreference = "Stop"

$serviceRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$output = Join-Path $serviceRoot "outdoor_core_runtime_verify.exe"
$statusOutput = Join-Path $serviceRoot "runtime\runtime_status_verify.json"
$edgeStatusOutput = Join-Path $serviceRoot "runtime\runtime_status_edge_verify.json"
$dashboardOutput = Join-Path $serviceRoot "runtime\dashboard_verify.txt"
$storageRoot = Join-Path $serviceRoot "runtime\storage_verify"
$storageStatusOutput = Join-Path $storageRoot "status\runtime_status.json"
$storageDashboardOutput = Join-Path $storageRoot "dashboard\dashboard.txt"
$storageLogOutput = Join-Path $storageRoot "logs\outdoor_core_runtime.log"

function Remove-TemporaryFile {
    param (
        [string]$Path
    )

    if (!(Test-Path $Path)) {
        return
    }

    for ($attempt = 0; $attempt -lt 5; $attempt++) {
        try {
            Remove-Item -LiteralPath $Path -Force
            return
        } catch {
            if ($attempt -eq 4) {
                Write-Warning "Failed to remove temporary file: $Path"
                return
            }

            Start-Sleep -Milliseconds 300
        }
    }
}

try {
    Push-Location $serviceRoot

    g++ -std=c++17 -Wall -Wextra -Wpedantic `
        -I"include" `
        -I"..\..\common" `
        "src\main.cpp" `
        "src\config\config_loader.cpp" `
        "src\gnss\gnss_status.cpp" `
        "src\gnss\nmea_parser.cpp" `
        "src\input\file_replay_input.cpp" `
        "src\ipc\status_publisher.cpp" `
        "src\log\logger.cpp" `
        "src\mcu\imu_payload_parser.cpp" `
        "src\mcu\mcu_command.cpp" `
        "src\mcu\mcu_frame_parser.cpp" `
        "src\mcu\mcu_frame_stream_decoder.cpp" `
        "src\mcu\mcu_protocol.cpp" `
        "src\mcu\mcu_status.cpp" `
        "src\runtime\runtime_manager.cpp" `
        "src\runtime\runtime_status.cpp" `
        "src\sensors\icm20608_char_reader.cpp" `
        "src\sensors\icm20608_iio_reader.cpp" `
        "src\services\dashboard_service.cpp" `
        "src\services\gnss_replay_service.cpp" `
        "src\services\gnss_serial_service.cpp" `
        "src\services\icm20608_service.cpp" `
        "src\services\mcu_mock_service.cpp" `
        "src\services\mcu_serial_service.cpp" `
        -o $output

    $defaultOutput = & $output --config "config\runtime.conf" --status-output $statusOutput --dashboard-output $dashboardOutput --log-level debug 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "default runtime execution failed"
    }

    if (($defaultOutput -join "`n") -notmatch "NMEA:") {
        throw "default runtime output did not contain NMEA lines"
    }

    if (($defaultOutput -join "`n") -notmatch "GNSS fix:") {
        throw "default runtime output did not contain parsed GNSS fix"
    }

    if (!(Test-Path $statusOutput)) {
        throw "runtime status output file was not created"
    }

    $statusText = Get-Content $statusOutput -Raw
    if ($statusText -notmatch '"state": "stopped"') {
        throw "runtime status output did not report stopped state"
    }

    if ($statusText -notmatch '"service_count": 3') {
        throw "runtime status output did not report service count"
    }

    if ($statusText -notmatch '"gnss": \{') {
        throw "runtime status output did not contain GNSS status object"
    }

    if ($statusText -notmatch '"source": "file"') {
        throw "runtime status output did not report GNSS file source"
    }

    if ($statusText -notmatch '"fix_valid": true') {
        throw "runtime status output did not report valid GNSS fix"
    }

    if ($statusText -notmatch '"satellites_in_view": 12') {
        throw "runtime status output did not report GNSS satellites in view"
    }

    if ($statusText -notmatch '"board_imu": \{') {
        throw "runtime status output did not contain board IMU status object"
    }

    if ($statusText -notmatch '"enabled": false') {
        throw "runtime status output did not report disabled board IMU by default"
    }

    if ($statusText -notmatch '"source": "icm20608_char"') {
        throw "runtime status output did not report board IMU source"
    }

    if ($statusText -notmatch '"heartbeat_seen": true') {
        throw "runtime status output did not report MCU heartbeat"
    }

    if ($statusText -notmatch '"mock_sensor_seen": true') {
        throw "runtime status output did not report MCU mock sensor"
    }

    if ($statusText -notmatch '"imu_seen": true') {
        throw "runtime status output did not report MCU IMU frame"
    }

    if ($statusText -notmatch '"temperature_celsius": 25\.340') {
        throw "runtime status output did not report MCU mock temperature"
    }

    if ($statusText -notmatch '"imu": \{') {
        throw "runtime status output did not contain IMU status object"
    }

    if ($statusText -notmatch '"gyro_y_dps": -2\.194') {
        throw "runtime status output did not report parsed IMU gyro"
    }

    if ($statusText -notmatch '"accel_z_g": 1\.001') {
        throw "runtime status output did not report parsed IMU acceleration"
    }

    if (($defaultOutput -join "`n") -notmatch "MCU frame CRC mismatch") {
        throw "default runtime output did not report invalid MCU CRC"
    }

    if (!(Test-Path $dashboardOutput)) {
        throw "dashboard output file was not created"
    }

    $dashboardText = Get-Content $dashboardOutput -Raw
    if ($dashboardText -notmatch "outdoor-agent") {
        throw "dashboard output did not contain title"
    }

    if ($dashboardText -notmatch "u-blox M10") {
        throw "dashboard output did not contain GNSS section"
    }

    if ($dashboardText -notmatch "AI Local Agent") {
        throw "dashboard output did not contain AI agent placeholder"
    }

    $edgeOutput = & $output --config "config\runtime.conf" --input "data\nmea_edge_cases.txt" --status-output $edgeStatusOutput --dashboard-output $dashboardOutput --log-level debug 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "edge-case runtime execution failed"
    }

    $edgeText = $edgeOutput -join "`n"
    if ($edgeText -notmatch "lat=-37\.860833") {
        throw "edge-case runtime output did not contain parsed southern latitude"
    }

    if ($edgeText -notmatch "lon=-145\.122667") {
        throw "edge-case runtime output did not contain parsed western longitude"
    }

    if ($edgeText -notmatch "NMEA checksum mismatch") {
        throw "edge-case runtime output did not report invalid checksum"
    }

    $warnOutput = & $output --config "config\runtime.conf" --status-output $statusOutput --dashboard-output $dashboardOutput --log-level warn 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "warn-level runtime execution failed"
    }

    if (($warnOutput -join "`n") -match "\[INFO\]") {
        throw "warn log level did not suppress INFO output"
    }

    if (Test-Path $storageRoot) {
        Remove-Item -LiteralPath $storageRoot -Recurse -Force
    }

    $storageOutput = & $output --config "config\runtime.conf" --storage-root $storageRoot --log-level info 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "storage runtime execution failed"
    }

    if (!(Test-Path $storageStatusOutput)) {
        throw "storage status output file was not created"
    }

    if (!(Test-Path $storageDashboardOutput)) {
        throw "storage dashboard output file was not created"
    }

    if (!(Test-Path $storageLogOutput)) {
        throw "storage log output file was not created"
    }

    $storageStatusText = Get-Content $storageStatusOutput -Raw
    if ($storageStatusText -notmatch '"storage": \{') {
        throw "storage status output did not contain storage object"
    }

    if ($storageStatusText -notmatch '"enabled": true') {
        throw "storage status output did not report enabled storage"
    }

    if (($storageOutput -join "`n") -notmatch "Storage enabled: true") {
        throw "storage runtime output did not report enabled storage"
    }

    $storageLogText = Get-Content $storageLogOutput -Raw
    if ($storageLogText -notmatch "Outdoor Core Runtime starting") {
        throw "storage log output did not contain runtime logs"
    }

    Write-Host "Stage 1 runtime verification passed."
} finally {
    Pop-Location
    Start-Sleep -Milliseconds 300
    Remove-TemporaryFile $output
    Remove-TemporaryFile $statusOutput
    Remove-TemporaryFile $edgeStatusOutput
    Remove-TemporaryFile $dashboardOutput
    if (Test-Path $storageRoot) {
        Remove-Item -LiteralPath $storageRoot -Recurse -Force
    }
}
