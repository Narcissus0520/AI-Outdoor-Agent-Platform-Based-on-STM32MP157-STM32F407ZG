param(
    [string]$PortName = "COM3",
    [int]$Seconds = 5,
    [int]$MinImuHz = 80
)

$ErrorActionPreference = "Stop"

$port = [System.IO.Ports.SerialPort]::new($PortName, 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.ReadTimeout = 100
$port.WriteTimeout = 100

function Invoke-AppReset {
    param($SerialPort)

    $SerialPort.DtrEnable = $true
    $SerialPort.RtsEnable = $false
    Start-Sleep -Milliseconds 150
    $SerialPort.RtsEnable = $true
    Start-Sleep -Milliseconds 500
}

function Get-Crc16Modbus {
    param(
        [byte[]]$Data,
        [int]$Start,
        [int]$Length
    )

    $crc = 0xFFFF
    for ($index = 0; $index -lt $Length; ++$index) {
        $crc = $crc -bxor ([int]$Data[$Start + $index])
        for ($bit = 0; $bit -lt 8; ++$bit) {
            if (($crc -band 1) -ne 0) {
                $crc = ($crc -shr 1) -bxor 0xA001
            } else {
                $crc = $crc -shr 1
            }
            $crc = $crc -band 0xFFFF
        }
    }
    return $crc
}

$bytes = New-Object System.Collections.Generic.List[byte]

try {
    $port.Open()
    Invoke-AppReset $port
    $port.DiscardInBuffer()

    $deadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline) {
        try {
            $value = $port.ReadByte()
            if ($value -ge 0) {
                $bytes.Add([byte]$value)
            }
        } catch [System.TimeoutException] {
        }
    }
} finally {
    if ($port.IsOpen) {
        $port.Close()
    }
    $port.Dispose()
}

$data = $bytes.ToArray()
$offset = 0
$frames = @()
$badCrc = 0

while ($offset -le ($data.Length - 10)) {
    if ($data[$offset] -eq 0xA5 -and $data[$offset + 1] -eq 0x5A) {
        $payloadLength = ([int]$data[$offset + 6]) -bor (([int]$data[$offset + 7]) -shl 8)
        $frameLength = 8 + $payloadLength + 2

        if ($payloadLength -le 64 -and ($offset + $frameLength) -le $data.Length) {
            $calculatedCrc = Get-Crc16Modbus $data ($offset + 2) (6 + $payloadLength)
            $receivedCrc = ([int]$data[$offset + 8 + $payloadLength]) -bor (([int]$data[$offset + 9 + $payloadLength]) -shl 8)
            $crcOk = $calculatedCrc -eq $receivedCrc
            $frameType = ("0x{0:X2}" -f $data[$offset + 3])
            $heartbeatStatusFlags = $null

            if ($frameType -eq "0x01" -and $payloadLength -ge 6) {
                $heartbeatStatusFlags = ([int]$data[$offset + 12]) -bor (([int]$data[$offset + 13]) -shl 8)
            }

            if (-not $crcOk) {
                ++$badCrc
            }

            $frames += [pscustomobject]@{
                Type = $frameType
                Seq = (([int]$data[$offset + 4]) -bor (([int]$data[$offset + 5]) -shl 8))
                PayloadLength = $payloadLength
                CrcOk = $crcOk
                Offset = $offset
                HeartbeatStatusFlags = $heartbeatStatusFlags
            }

            $offset += $frameLength
            continue
        }
    }

    ++$offset
}

$heartbeatCount = @($frames | Where-Object { $_.Type -eq "0x01" }).Count
$imuCount = @($frames | Where-Object { $_.Type -eq "0x11" }).Count
$frameRateHz = if ($Seconds -gt 0) { [math]::Round($frames.Count / $Seconds, 2) } else { 0 }
$imuRateHz = if ($Seconds -gt 0) { [math]::Round($imuCount / $Seconds, 2) } else { 0 }
$minImuFrames = [math]::Max(1, $Seconds * $MinImuHz)
$lastHeartbeat = @($frames | Where-Object { $_.Type -eq "0x01" } | Select-Object -Last 1)
$lastHeartbeatStatusFlags = if ($lastHeartbeat.Count -gt 0 -and $null -ne $lastHeartbeat[0].HeartbeatStatusFlags) {
    "0x{0:X4}" -f $lastHeartbeat[0].HeartbeatStatusFlags
} else {
    "n/a"
}
$preview = ($data | Select-Object -First 80 | ForEach-Object { $_.ToString("X2") }) -join " "

Write-Host "bytes_read=$($data.Length)"
Write-Host "frames=$($frames.Count)"
Write-Host "heartbeat=$heartbeatCount"
Write-Host "imu=$imuCount"
Write-Host "frame_rate_hz=$frameRateHz"
Write-Host "imu_rate_hz=$imuRateHz"
Write-Host "crc_bad=$badCrc"
Write-Host "last_heartbeat_status_flags=$lastHeartbeatStatusFlags"
Write-Host "preview_hex=$preview"
Write-Host "first_frames:"
$frames | Select-Object -First 10 | Format-Table -AutoSize

if ($frames.Count -eq 0 -or $badCrc -ne 0 -or $heartbeatCount -lt 1 -or $imuCount -lt $minImuFrames) {
    exit 1
}
