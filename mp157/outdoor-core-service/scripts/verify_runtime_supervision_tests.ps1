$ErrorActionPreference = "Stop"

$moduleRoot = Split-Path -Parent $PSScriptRoot
$verifier = Join-Path $PSScriptRoot "verify_runtime_supervision.ps1"
$sourceUnit = Join-Path $moduleRoot "deploy/systemd/outdoor-agent-runtime.service"
$sourceConfig = Join-Path $moduleRoot "config/outdoor_agent_service.conf"
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("outdoor-supervision-" + [System.IO.Path]::GetRandomFileName())

function Invoke-Verifier {
    param(
        [string]$UnitPath,
        [string]$ConfigPath,
        [bool]$ExpectSuccess,
        [string]$CaseName
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass `
            -File $verifier -UnitPath $UnitPath -ConfigPath $ConfigPath 2>&1
        $succeeded = $LASTEXITCODE -eq 0
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    if ($succeeded -ne $ExpectSuccess) {
        throw "supervision verifier case '$CaseName' expected success=$ExpectSuccess but success=$succeeded`n$($output -join "`n")"
    }
}

try {
    New-Item -ItemType Directory -Path $testRoot | Out-Null
    $validUnit = Join-Path $testRoot "valid.service"
    $validConfig = Join-Path $testRoot "valid.conf"
    Copy-Item -LiteralPath $sourceUnit -Destination $validUnit
    Copy-Item -LiteralPath $sourceConfig -Destination $validConfig

    Invoke-Verifier $validUnit $validConfig $true "valid supervision contract"

    $unsafeRestartUnit = Join-Path $testRoot "unsafe-restart.service"
    (Get-Content -LiteralPath $sourceUnit -Raw -Encoding utf8).Replace(
        "Restart=on-failure", "Restart=no") |
        Set-Content -LiteralPath $unsafeRestartUnit -Encoding utf8
    Invoke-Verifier $unsafeRestartUnit $validConfig $false "restart disabled"

    $unboundedRestartUnit = Join-Path $testRoot "unbounded-restart.service"
    (Get-Content -LiteralPath $sourceUnit -Raw -Encoding utf8).Replace(
        "StartLimitBurst=5", "StartLimitBurst=0") |
        Set-Content -LiteralPath $unboundedRestartUnit -Encoding utf8
    Invoke-Verifier $unboundedRestartUnit $validConfig $false "restart burst unbounded"

    $unsafeDeviceUnit = Join-Path $testRoot "unsafe-device.service"
    (Get-Content -LiteralPath $sourceUnit -Raw -Encoding utf8).Replace(
        "DevicePolicy=closed", "DevicePolicy=auto") |
        Set-Content -LiteralPath $unsafeDeviceUnit -Encoding utf8
    Invoke-Verifier $unsafeDeviceUnit $validConfig $false "device policy widened"

    $disabledSocketConfig = Join-Path $testRoot "disabled-socket.conf"
    (Get-Content -LiteralPath $sourceConfig -Raw -Encoding utf8).Replace(
        "status_socket_enabled = true", "status_socket_enabled = false") |
        Set-Content -LiteralPath $disabledSocketConfig -Encoding utf8
    Invoke-Verifier $validUnit $disabledSocketConfig $false "service socket disabled"

    Write-Host "Runtime supervision verifier tests passed."
} finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
