param(
    [string]$PortName = "COM9",
    [string]$RuntimePath = "",
    [string]$ConfigPath = "",
    [string]$InstallRoot = "/opt/outdoor-agent",
    [int]$BaudRate = 115200
)

$ErrorActionPreference = "Stop"

if ($PortName -notmatch '^COM[0-9]+$') {
    throw "PortName must use the COM<number> form."
}
if ($InstallRoot -notmatch '^/[A-Za-z0-9._/-]+$' -or $InstallRoot.EndsWith('/')) {
    throw "InstallRoot must be an absolute Linux path without spaces or a trailing slash."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($RuntimePath)) {
    $RuntimePath = Join-Path $repoRoot "mp157/outdoor-core-service/build-arm/outdoor_core_runtime"
}
if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
    $ConfigPath = Join-Path $repoRoot "mp157/outdoor-core-service/config/runtime.conf"
}

$xmodemScript = Join-Path $PSScriptRoot "send_xmodem.ps1"
$servicePath = Join-Path $repoRoot "mp157/outdoor-core-service/deploy/systemd/outdoor-agent-icm20608.service"
$healthPreflightPath = Join-Path $repoRoot "mp157/outdoor-core-service/scripts/run_board_health_preflight.sh"
$healthMonitorPath = Join-Path $repoRoot "mp157/outdoor-core-service/scripts/monitor_board_runtime_health.sh"
$longTestPath = Join-Path $repoRoot "mp157/outdoor-core-service/scripts/run_board_long_validation.sh"
$auditPath = Join-Path $repoRoot "mp157/outdoor-core-service/scripts/audit_board_long_validation.sh"
$crashRecoveryPath = Join-Path $repoRoot "mp157/outdoor-core-service/scripts/run_board_crash_recovery_validation.sh"
$gnssFixPath = Join-Path $repoRoot "mp157/outdoor-core-service/scripts/run_board_gnss_fix_validation.sh"
$nmeaSamplePath = Join-Path $repoRoot "mp157/outdoor-core-service/data/nmea_sample.txt"
$mcuMockFramesPath = Join-Path $repoRoot "mp157/outdoor-core-service/data/mcu_mock_frames.txt"
$mcuValidMockFramesPath = Join-Path $repoRoot "mp157/outdoor-core-service/data/mcu_mock_frames_valid.txt"
$mcuCompassMockFramesPath = Join-Path $repoRoot "mp157/outdoor-core-service/data/mcu_mock_frames_compass.txt"

$localFiles = @(
    @{ Local = $RuntimePath; Remote = "$InstallRoot/bin/outdoor_core_runtime"; Mode = "0755" },
    @{ Local = $ConfigPath; Remote = "$InstallRoot/config/runtime.conf"; Mode = "0644" },
    @{ Local = $healthPreflightPath; Remote = "$InstallRoot/scripts/run_board_health_preflight.sh"; Mode = "0755" },
    @{ Local = $healthMonitorPath; Remote = "$InstallRoot/scripts/monitor_board_runtime_health.sh"; Mode = "0755" },
    @{ Local = $longTestPath; Remote = "$InstallRoot/scripts/run_board_long_validation.sh"; Mode = "0755" },
    @{ Local = $auditPath; Remote = "$InstallRoot/scripts/audit_board_long_validation.sh"; Mode = "0755" },
    @{ Local = $crashRecoveryPath; Remote = "$InstallRoot/scripts/run_board_crash_recovery_validation.sh"; Mode = "0755" },
    @{ Local = $gnssFixPath; Remote = "$InstallRoot/scripts/run_board_gnss_fix_validation.sh"; Mode = "0755" },
    @{ Local = $nmeaSamplePath; Remote = "$InstallRoot/data/nmea_sample.txt"; Mode = "0644" },
    @{ Local = $mcuMockFramesPath; Remote = "$InstallRoot/data/mcu_mock_frames.txt"; Mode = "0644" },
    @{ Local = $mcuValidMockFramesPath; Remote = "$InstallRoot/data/mcu_mock_frames_valid.txt"; Mode = "0644" },
    @{ Local = $mcuCompassMockFramesPath; Remote = "$InstallRoot/data/mcu_mock_frames_compass.txt"; Mode = "0644" },
    @{ Local = $servicePath; Remote = "/etc/systemd/system/outdoor-agent-icm20608.service"; Mode = "0644" }
)

foreach ($entry in $localFiles) {
    $entry.Local = (Resolve-Path -LiteralPath $entry.Local).Path
    if ((Get-Item -LiteralPath $entry.Local).PSIsContainer) {
        throw "Deployment input must be a file: $($entry.Local)"
    }
    $entry.Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Local).Hash.ToLowerInvariant()
}

function Invoke-BoardCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command,
        [int]$TimeoutSeconds = 15
    )

    $port = [System.IO.Ports.SerialPort]::new(
        $PortName,
        $BaudRate,
        [System.IO.Ports.Parity]::None,
        8,
        [System.IO.Ports.StopBits]::One
    )
    $port.ReadTimeout = 200
    $port.WriteTimeout = 3000

    try {
        $port.Open()
        $port.DiscardInBuffer()
        $suffix = '; deploy_rc=$?; printf ''__DEPLOY_RC_%s__\n'' "$deploy_rc"'
        $port.Write($Command + $suffix + "`r")

        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        $output = New-Object System.Text.StringBuilder
        $match = $null
        while ((Get-Date) -lt $deadline) {
            $chunk = $port.ReadExisting()
            if ($chunk.Length -gt 0) {
                [void]$output.Append($chunk)
                $match = [regex]::Match($output.ToString(), '__DEPLOY_RC_([0-9]+)__')
                if ($match.Success) {
                    break
                }
            }
            Start-Sleep -Milliseconds 50
        }

        if ($null -eq $match -or -not $match.Success) {
            throw "Board command timed out: $Command`n$($output.ToString())"
        }

        $result = [pscustomobject]@{
            ExitCode = [int]$match.Groups[1].Value
            Output = $output.ToString()
        }
        if ($result.ExitCode -ne 0) {
            throw "Board command failed with exit code $($result.ExitCode): $Command`n$($result.Output)"
        }
        return $result
    } finally {
        if ($port.IsOpen) {
            $port.Close()
        }
        $port.Dispose()
    }
}

Write-Host "Creating persistent MP157 deployment directories under $InstallRoot."
[void](Invoke-BoardCommand -Command "install -d -m 0755 $InstallRoot/bin $InstallRoot/config $InstallRoot/data $InstallRoot/scripts")

foreach ($entry in $localFiles) {
    Write-Host "Uploading $($entry.Local) -> $($entry.Remote)"
    & $xmodemScript `
        -PortName $PortName `
        -LocalPath $entry.Local `
        -RemotePath $entry.Remote `
        -BaudRate $BaudRate
}

$modeCommands = $localFiles | ForEach-Object { "chmod $($_.Mode) $($_.Remote)" }
[void](Invoke-BoardCommand -Command ($modeCommands -join '; '))

Write-Host "Installing and starting the ordered ICM20608 systemd unit."
$serviceCommand = @(
    'rm -f /etc/modules-load.d/outdoor-agent.conf',
    'systemctl daemon-reload',
    'systemctl enable outdoor-agent-icm20608.service',
    'systemctl restart outdoor-agent-icm20608.service',
    'test -c /dev/icm20608'
) -join '; '
[void](Invoke-BoardCommand -Command $serviceCommand -TimeoutSeconds 30)

$remotePaths = ($localFiles | ForEach-Object { $_.Remote }) -join ' '
$hashResult = Invoke-BoardCommand -Command "sha256sum $remotePaths" -TimeoutSeconds 20
$remoteHashes = @{}
foreach ($line in ($hashResult.Output -split "`r?`n")) {
    if ($line -match '^([0-9a-f]{64})\s+(/\S+)$') {
        $remoteHashes[$Matches[2]] = $Matches[1]
    }
}

foreach ($entry in $localFiles) {
    if (-not $remoteHashes.ContainsKey($entry.Remote)) {
        throw "Board SHA256 output did not contain $($entry.Remote).`n$($hashResult.Output)"
    }
    if ($remoteHashes[$entry.Remote] -ne $entry.Hash) {
        throw "SHA256 mismatch for $($entry.Remote): local=$($entry.Hash), remote=$($remoteHashes[$entry.Remote])"
    }
    Write-Host "sha256=PASS path=$($entry.Remote) hash=$($entry.Hash)"
}

$syntaxCommand = @(
    "sh -n $InstallRoot/scripts/run_board_health_preflight.sh",
    "sh -n $InstallRoot/scripts/monitor_board_runtime_health.sh",
    "sh -n $InstallRoot/scripts/run_board_long_validation.sh",
    "sh -n $InstallRoot/scripts/audit_board_long_validation.sh",
    "sh -n $InstallRoot/scripts/run_board_crash_recovery_validation.sh",
    "sh -n $InstallRoot/scripts/run_board_gnss_fix_validation.sh",
    'systemctl is-active --quiet outdoor-agent-icm20608.service',
    'systemctl is-enabled --quiet outdoor-agent-icm20608.service',
    'test -c /dev/icm20608'
) -join '; '
[void](Invoke-BoardCommand -Command $syntaxCommand -TimeoutSeconds 20)

Write-Host "deployment=PASSED root=$InstallRoot port=$PortName"
