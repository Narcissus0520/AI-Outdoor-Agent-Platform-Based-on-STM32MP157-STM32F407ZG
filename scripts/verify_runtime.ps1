$ErrorActionPreference = "Stop"

$output = Join-Path $PSScriptRoot "..\outdoor_core_runtime_verify.exe"

try {
    g++ -std=c++17 -Wall -Wextra -Wpedantic `
        -I"$PSScriptRoot\..\include" `
        "$PSScriptRoot\..\src\main.cpp" `
        "$PSScriptRoot\..\src\config\config_loader.cpp" `
        "$PSScriptRoot\..\src\input\file_replay_input.cpp" `
        "$PSScriptRoot\..\src\log\logger.cpp" `
        "$PSScriptRoot\..\src\runtime\runtime_manager.cpp" `
        "$PSScriptRoot\..\src\services\gnss_replay_service.cpp" `
        -o $output

    $defaultOutput = & $output --config "$PSScriptRoot\..\config\runtime.conf" 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "default runtime execution failed"
    }

    if (($defaultOutput -join "`n") -notmatch "NMEA:") {
        throw "default runtime output did not contain NMEA lines"
    }

    $warnOutput = & $output --config "$PSScriptRoot\..\config\runtime.conf" --log-level warn 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "warn-level runtime execution failed"
    }

    if (($warnOutput -join "`n") -match "\[INFO\]") {
        throw "warn log level did not suppress INFO output"
    }

    Write-Host "Stage 0.3 runtime verification passed."
} finally {
    Start-Sleep -Milliseconds 300
    if (Test-Path $output) {
        Remove-Item -LiteralPath $output -Force
    }
}
