param(
    [string]$UnitPath = "",
    [string]$ConfigPath = ""
)

$ErrorActionPreference = "Stop"

$moduleRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($UnitPath)) {
    $UnitPath = Join-Path $moduleRoot "deploy/systemd/outdoor-agent-runtime.service"
}
if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
    $ConfigPath = Join-Path $moduleRoot "config/outdoor_agent_service.conf"
}

$UnitPath = (Resolve-Path -LiteralPath $UnitPath).Path
$ConfigPath = (Resolve-Path -LiteralPath $ConfigPath).Path

function Read-SystemdUnit {
    param([string]$Path)

    $sections = @{}
    $sectionName = ""
    $lineNumber = 0
    foreach ($rawLine in Get-Content -LiteralPath $Path -Encoding utf8) {
        $lineNumber++
        $line = $rawLine.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith("#") -or $line.StartsWith(";")) {
            continue
        }
        if ($line -match '^\[([A-Za-z]+)\]$') {
            $sectionName = $Matches[1]
            if (-not $sections.ContainsKey($sectionName)) {
                $sections[$sectionName] = @{}
            }
            continue
        }
        if ([string]::IsNullOrWhiteSpace($sectionName) -or -not $line.Contains('=')) {
            throw "invalid systemd unit line ${lineNumber}: $rawLine"
        }

        $separator = $line.IndexOf('=')
        $key = $line.Substring(0, $separator).Trim()
        $value = $line.Substring($separator + 1).Trim()
        if ([string]::IsNullOrWhiteSpace($key)) {
            throw "empty systemd directive at line $lineNumber"
        }
        if (-not $sections[$sectionName].ContainsKey($key)) {
            $sections[$sectionName][$key] = @()
        }
        $sections[$sectionName][$key] += $value
    }
    return $sections
}

function Read-KeyValueConfig {
    param([string]$Path)

    $values = @{}
    $lineNumber = 0
    foreach ($rawLine in Get-Content -LiteralPath $Path -Encoding utf8) {
        $lineNumber++
        $line = $rawLine.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith("#")) {
            continue
        }
        if (-not $line.Contains('=')) {
            throw "invalid Runtime config line ${lineNumber}: $rawLine"
        }
        $separator = $line.IndexOf('=')
        $key = $line.Substring(0, $separator).Trim()
        $value = $line.Substring($separator + 1).Trim()
        if ($values.ContainsKey($key)) {
            throw "duplicate Runtime config key at line ${lineNumber}: $key"
        }
        $values[$key] = $value
    }
    return $values
}

function Assert-Directive {
    param(
        [hashtable]$Unit,
        [string]$Section,
        [string]$Key,
        [string]$Expected
    )

    if (-not $Unit.ContainsKey($Section) -or -not $Unit[$Section].ContainsKey($Key)) {
        throw "missing systemd directive [$Section] $Key"
    }
    if ($Unit[$Section][$Key] -notcontains $Expected) {
        $actual = $Unit[$Section][$Key] -join ', '
        throw "unexpected systemd directive [$Section] ${Key}: expected '$Expected', actual '$actual'"
    }
}

function Assert-DirectiveContainsToken {
    param(
        [hashtable]$Unit,
        [string]$Section,
        [string]$Key,
        [string]$ExpectedToken
    )

    if (-not $Unit.ContainsKey($Section) -or -not $Unit[$Section].ContainsKey($Key)) {
        throw "missing systemd directive [$Section] $Key"
    }
    $tokens = @($Unit[$Section][$Key] | ForEach-Object { $_ -split '\s+' })
    if ($tokens -notcontains $ExpectedToken) {
        throw "systemd directive [$Section] $Key does not contain '$ExpectedToken'"
    }
}

function Assert-ConfigValue {
    param(
        [hashtable]$Config,
        [string]$Key,
        [string]$Expected
    )

    if (-not $Config.ContainsKey($Key)) {
        throw "missing Runtime service config key: $Key"
    }
    if ($Config[$Key] -ne $Expected) {
        throw "unexpected Runtime service config ${Key}: expected '$Expected', actual '$($Config[$Key])'"
    }
}

$unit = Read-SystemdUnit -Path $UnitPath
$config = Read-KeyValueConfig -Path $ConfigPath

Assert-DirectiveContainsToken $unit "Unit" "Requires" "outdoor-agent-icm20608.service"
Assert-DirectiveContainsToken $unit "Unit" "After" "outdoor-agent-icm20608.service"
Assert-Directive $unit "Unit" "StartLimitIntervalSec" "60"
Assert-Directive $unit "Unit" "StartLimitBurst" "5"

Assert-Directive $unit "Service" "Type" "simple"
Assert-Directive $unit "Service" "User" "root"
Assert-Directive $unit "Service" "Group" "root"
Assert-Directive $unit "Service" "WorkingDirectory" "/opt/outdoor-agent"
Assert-Directive $unit "Service" "RuntimeDirectory" "outdoor-agent"
Assert-Directive $unit "Service" "RuntimeDirectoryMode" "0750"
Assert-Directive $unit "Service" "ExecStart" "/opt/outdoor-agent/bin/outdoor_core_runtime --config /opt/outdoor-agent/config/outdoor_agent_service.conf"
Assert-Directive $unit "Service" "Restart" "on-failure"
Assert-Directive $unit "Service" "RestartSec" "2s"
Assert-Directive $unit "Service" "TimeoutStopSec" "15s"
Assert-Directive $unit "Service" "KillSignal" "SIGTERM"
Assert-Directive $unit "Service" "UMask" "0027"
Assert-Directive $unit "Service" "NoNewPrivileges" "true"
Assert-Directive $unit "Service" "PrivateTmp" "true"
Assert-Directive $unit "Service" "ProtectSystem" "strict"
Assert-Directive $unit "Service" "ProtectHome" "true"
Assert-Directive $unit "Service" "ProtectKernelTunables" "true"
Assert-Directive $unit "Service" "ProtectKernelModules" "true"
Assert-Directive $unit "Service" "ProtectControlGroups" "true"
Assert-Directive $unit "Service" "LockPersonality" "true"
Assert-Directive $unit "Service" "MemoryDenyWriteExecute" "true"
Assert-Directive $unit "Service" "RestrictAddressFamilies" "AF_UNIX"
Assert-Directive $unit "Service" "ReadWritePaths" "/run/outdoor-agent"
Assert-Directive $unit "Service" "DevicePolicy" "closed"
Assert-Directive $unit "Service" "DeviceAllow" "/dev/ttySTM1 rw"
Assert-Directive $unit "Service" "DeviceAllow" "/dev/ttySTM2 rw"
Assert-Directive $unit "Service" "DeviceAllow" "/dev/icm20608 r"
Assert-Directive $unit "Install" "WantedBy" "multi-user.target"

Assert-ConfigValue $config "gnss_input_mode" "serial"
Assert-ConfigValue $config "gnss_serial_device" "/dev/ttySTM2"
Assert-ConfigValue $config "gnss_serial_capture_seconds" "0"
Assert-ConfigValue $config "mcu_input_mode" "serial"
Assert-ConfigValue $config "mcu_serial_device" "/dev/ttySTM1"
Assert-ConfigValue $config "mcu_serial_capture_seconds" "0"
Assert-ConfigValue $config "mcu_command" "ping"
Assert-ConfigValue $config "runtime_run_seconds" "0"
Assert-ConfigValue $config "status_output_path" "/run/outdoor-agent/runtime_status.json"
Assert-ConfigValue $config "status_socket_enabled" "true"
Assert-ConfigValue $config "status_socket_path" "/run/outdoor-agent/outdoor_core.sock"
Assert-ConfigValue $config "storage_enabled" "false"
Assert-ConfigValue $config "history_enabled" "false"
Assert-ConfigValue $config "board_imu_enabled" "true"
Assert-ConfigValue $config "board_imu_source" "char_device"
Assert-ConfigValue $config "board_imu_device_path" "/dev/icm20608"
Assert-ConfigValue $config "board_imu_sample_count" "0"
Assert-ConfigValue $config "dashboard_enabled" "false"
Assert-ConfigValue $config "launcher_enabled" "false"

Write-Host "Runtime supervision verification passed."
