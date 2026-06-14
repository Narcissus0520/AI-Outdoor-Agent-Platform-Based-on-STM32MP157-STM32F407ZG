$ErrorActionPreference = "Stop"

$serviceRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$output = Join-Path $serviceRoot "outdoor_core_runtime_verify.exe"
$statusOutput = Join-Path $serviceRoot "runtime\runtime_status_verify.json"
$edgeStatusOutput = Join-Path $serviceRoot "runtime\runtime_status_edge_verify.json"

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
        "src\main.cpp" `
        "src\config\config_loader.cpp" `
        "src\gnss\nmea_parser.cpp" `
        "src\input\file_replay_input.cpp" `
        "src\ipc\status_publisher.cpp" `
        "src\log\logger.cpp" `
        "src\mcu\mcu_frame_parser.cpp" `
        "src\mcu\mcu_protocol.cpp" `
        "src\mcu\mcu_status.cpp" `
        "src\runtime\runtime_manager.cpp" `
        "src\runtime\runtime_status.cpp" `
        "src\services\gnss_replay_service.cpp" `
        "src\services\mcu_mock_service.cpp" `
        -o $output

    $defaultOutput = & $output --config "config\runtime.conf" --status-output $statusOutput --log-level debug 2>&1
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

    if ($statusText -notmatch '"service_count": 2') {
        throw "runtime status output did not report service count"
    }

    if ($statusText -notmatch '"heartbeat_seen": true') {
        throw "runtime status output did not report MCU heartbeat"
    }

    if ($statusText -notmatch '"mock_sensor_seen": true') {
        throw "runtime status output did not report MCU mock sensor"
    }

    if ($statusText -notmatch '"temperature_celsius": 25\.340') {
        throw "runtime status output did not report MCU mock temperature"
    }

    if (($defaultOutput -join "`n") -notmatch "MCU frame CRC mismatch") {
        throw "default runtime output did not report invalid MCU CRC"
    }

    $edgeOutput = & $output --config "config\runtime.conf" --input "data\nmea_edge_cases.txt" --status-output $edgeStatusOutput --log-level debug 2>&1
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

    $warnOutput = & $output --config "config\runtime.conf" --status-output $statusOutput --log-level warn 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "warn-level runtime execution failed"
    }

    if (($warnOutput -join "`n") -match "\[INFO\]") {
        throw "warn log level did not suppress INFO output"
    }

    Write-Host "Stage 1 runtime verification passed."
} finally {
    Pop-Location
    Start-Sleep -Milliseconds 300
    Remove-TemporaryFile $output
    Remove-TemporaryFile $statusOutput
    Remove-TemporaryFile $edgeStatusOutput
}
