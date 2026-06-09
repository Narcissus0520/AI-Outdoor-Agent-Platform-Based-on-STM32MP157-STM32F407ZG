$ErrorActionPreference = "Stop"

$output = Join-Path $PSScriptRoot "..\outdoor_core_runtime_verify.exe"
$statusOutput = Join-Path $PSScriptRoot "..\runtime\status_verify.txt"

try {
    g++ -std=c++17 -Wall -Wextra -Wpedantic `
        -I"$PSScriptRoot\..\include" `
        "$PSScriptRoot\..\src\main.cpp" `
        "$PSScriptRoot\..\src\config\config_loader.cpp" `
        "$PSScriptRoot\..\src\gnss\nmea_parser.cpp" `
        "$PSScriptRoot\..\src\input\file_replay_input.cpp" `
        "$PSScriptRoot\..\src\ipc\status_publisher.cpp" `
        "$PSScriptRoot\..\src\log\logger.cpp" `
        "$PSScriptRoot\..\src\runtime\runtime_manager.cpp" `
        "$PSScriptRoot\..\src\runtime\runtime_status.cpp" `
        "$PSScriptRoot\..\src\services\gnss_replay_service.cpp" `
        -o $output

    $defaultOutput = & $output --config "$PSScriptRoot\..\config\runtime.conf" --status-output $statusOutput 2>&1
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
    if ($statusText -notmatch "state=stopped") {
        throw "runtime status output did not report stopped state"
    }

    if ($statusText -notmatch "service_count=1") {
        throw "runtime status output did not report service count"
    }

    $warnOutput = & $output --config "$PSScriptRoot\..\config\runtime.conf" --status-output $statusOutput --log-level warn 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "warn-level runtime execution failed"
    }

    if (($warnOutput -join "`n") -match "\[INFO\]") {
        throw "warn log level did not suppress INFO output"
    }

    Write-Host "Stage 0.5 runtime verification passed."
} finally {
    Start-Sleep -Milliseconds 300
    if (Test-Path $output) {
        Remove-Item -LiteralPath $output -Force
    }
    if (Test-Path $statusOutput) {
        Remove-Item -LiteralPath $statusOutput -Force
    }
}
