param(
    [string]$PortName = "COM6",
    [int]$Seconds = 5,
    [int]$MinImuHz = 90,
    [int]$MinMagnetometerHz = 15,
    [int]$MinBarometerHz = 8
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
$badVersion = 0
$badPayload = 0

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
            $sensorUptimeMs = $null
            $accelXMg = $null
            $accelYMg = $null
            $accelZMg = $null
            $gyroXMdps = $null
            $gyroYMdps = $null
            $gyroZMdps = $null
            $imuTemperatureCentiC = $null
            $magneticXNt = $null
            $magneticYNt = $null
            $magneticZNt = $null
            $pressurePa = $null
            $barometerTemperatureCentiC = $null
            $i2cRecoveryCount = $null
            $i2cFailureCount = $null
            $i2cLastHalError = $null
            $fifoOverflowCount = $null
            $fifoMalformedCount = $null
            $fifoEmptyEventCount = $null
            $fifoDrainStallCount = $null
            $fifoSkippedPacketCount = $null
            $i2cLastLength = $null
            $i2cLastDeviceAddress = $null
            $i2cLastRegisterAddress = $null
            $i2cLastOperation = $null
            $i2cLastHalStatus = $null
            $icm42688InitErrorStep = $null
            $diagnosticsSchemaVersion = $null
            $uart4RxDropCount = $null
            $payloadOffset = $offset + 8

            if ($data[$offset + 2] -ne 0x01) {
                ++$badVersion
            }

            switch ($frameType) {
                "0x01" {
                    if ($payloadLength -eq 6) {
                        $sensorUptimeMs = [BitConverter]::ToUInt32($data, $payloadOffset)
                        $heartbeatStatusFlags = [BitConverter]::ToUInt16($data, $payloadOffset + 4)
                    } else {
                        ++$badPayload
                    }
                }
                "0x02" {
                    if ($payloadLength -eq 44 -or $payloadLength -eq 48) {
                        $sensorUptimeMs = [BitConverter]::ToUInt32($data, $payloadOffset)
                        $i2cRecoveryCount = [BitConverter]::ToUInt32($data, $payloadOffset + 4)
                        $i2cFailureCount = [BitConverter]::ToUInt32($data, $payloadOffset + 8)
                        $i2cLastHalError = [BitConverter]::ToUInt32($data, $payloadOffset + 12)
                        $fifoOverflowCount = [BitConverter]::ToUInt32($data, $payloadOffset + 16)
                        $fifoMalformedCount = [BitConverter]::ToUInt32($data, $payloadOffset + 20)
                        $fifoEmptyEventCount = [BitConverter]::ToUInt32($data, $payloadOffset + 24)
                        $fifoDrainStallCount = [BitConverter]::ToUInt32($data, $payloadOffset + 28)
                        $fifoSkippedPacketCount = [BitConverter]::ToUInt32($data, $payloadOffset + 32)
                        $i2cLastLength = [BitConverter]::ToUInt16($data, $payloadOffset + 36)
                        $i2cLastDeviceAddress = $data[$payloadOffset + 38]
                        $i2cLastRegisterAddress = $data[$payloadOffset + 39]
                        $i2cLastOperation = $data[$payloadOffset + 40]
                        $i2cLastHalStatus = $data[$payloadOffset + 41]
                        $icm42688InitErrorStep = $data[$payloadOffset + 42]
                        $diagnosticsSchemaVersion = $data[$payloadOffset + 43]
                        if ($payloadLength -eq 48 -and $diagnosticsSchemaVersion -eq 1) {
                            $uart4RxDropCount = [BitConverter]::ToUInt32($data, $payloadOffset + 44)
                        } elseif ($payloadLength -eq 44 -and $diagnosticsSchemaVersion -eq 0) {
                            $uart4RxDropCount = 0
                        } else {
                            ++$badPayload
                        }
                    } else {
                        ++$badPayload
                    }
                }
                "0x11" {
                    if ($payloadLength -eq 24) {
                        $sensorUptimeMs = [BitConverter]::ToUInt32($data, $payloadOffset)
                        $accelXMg = [BitConverter]::ToInt16($data, $payloadOffset + 4)
                        $accelYMg = [BitConverter]::ToInt16($data, $payloadOffset + 6)
                        $accelZMg = [BitConverter]::ToInt16($data, $payloadOffset + 8)
                        $gyroXMdps = [BitConverter]::ToInt32($data, $payloadOffset + 10)
                        $gyroYMdps = [BitConverter]::ToInt32($data, $payloadOffset + 14)
                        $gyroZMdps = [BitConverter]::ToInt32($data, $payloadOffset + 18)
                        $imuTemperatureCentiC = [BitConverter]::ToInt16($data, $payloadOffset + 22)
                    } else {
                        ++$badPayload
                    }
                }
                "0x12" {
                    if ($payloadLength -eq 16) {
                        $sensorUptimeMs = [BitConverter]::ToUInt32($data, $payloadOffset)
                        $magneticXNt = [BitConverter]::ToInt32($data, $payloadOffset + 4)
                        $magneticYNt = [BitConverter]::ToInt32($data, $payloadOffset + 8)
                        $magneticZNt = [BitConverter]::ToInt32($data, $payloadOffset + 12)
                    } else {
                        ++$badPayload
                    }
                }
                "0x13" {
                    if ($payloadLength -eq 10) {
                        $sensorUptimeMs = [BitConverter]::ToUInt32($data, $payloadOffset)
                        $pressurePa = [BitConverter]::ToUInt32($data, $payloadOffset + 4)
                        $barometerTemperatureCentiC = [BitConverter]::ToInt16($data, $payloadOffset + 8)
                    } else {
                        ++$badPayload
                    }
                }
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
                SensorUptimeMs = $sensorUptimeMs
                AccelXMg = $accelXMg
                AccelYMg = $accelYMg
                AccelZMg = $accelZMg
                GyroXMdps = $gyroXMdps
                GyroYMdps = $gyroYMdps
                GyroZMdps = $gyroZMdps
                ImuTemperatureCentiC = $imuTemperatureCentiC
                MagneticXNt = $magneticXNt
                MagneticYNt = $magneticYNt
                MagneticZNt = $magneticZNt
                PressurePa = $pressurePa
                BarometerTemperatureCentiC = $barometerTemperatureCentiC
                I2cRecoveryCount = $i2cRecoveryCount
                I2cFailureCount = $i2cFailureCount
                I2cLastHalError = $i2cLastHalError
                FifoOverflowCount = $fifoOverflowCount
                FifoMalformedCount = $fifoMalformedCount
                FifoEmptyEventCount = $fifoEmptyEventCount
                FifoDrainStallCount = $fifoDrainStallCount
                FifoSkippedPacketCount = $fifoSkippedPacketCount
                I2cLastLength = $i2cLastLength
                I2cLastDeviceAddress = $i2cLastDeviceAddress
                I2cLastRegisterAddress = $i2cLastRegisterAddress
                I2cLastOperation = $i2cLastOperation
                I2cLastHalStatus = $i2cLastHalStatus
                Icm42688InitErrorStep = $icm42688InitErrorStep
                DiagnosticsSchemaVersion = $diagnosticsSchemaVersion
                Uart4RxDropCount = $uart4RxDropCount
            }

            $offset += $frameLength
            continue
        }
    }

    ++$offset
}

$heartbeatFrames = @($frames | Where-Object { $_.Type -eq "0x01" })
$diagnosticFrames = @($frames | Where-Object { $_.Type -eq "0x02" })
$heartbeatCount = $heartbeatFrames.Count
$diagnosticsCount = $diagnosticFrames.Count
$imuFrames = @($frames | Where-Object { $_.Type -eq "0x11" })
$magnetometerFrames = @($frames | Where-Object { $_.Type -eq "0x12" })
$barometerFrames = @($frames | Where-Object { $_.Type -eq "0x13" })
$imuCount = $imuFrames.Count
$magnetometerCount = $magnetometerFrames.Count
$barometerCount = $barometerFrames.Count
$frameRateHz = if ($Seconds -gt 0) { [math]::Round($frames.Count / $Seconds, 2) } else { 0 }
$imuRateHz = if ($Seconds -gt 0) { [math]::Round($imuCount / $Seconds, 2) } else { 0 }
$magnetometerRateHz = if ($Seconds -gt 0) { [math]::Round($magnetometerCount / $Seconds, 2) } else { 0 }
$barometerRateHz = if ($Seconds -gt 0) { [math]::Round($barometerCount / $Seconds, 2) } else { 0 }
$minImuFrames = [math]::Max(1, $Seconds * $MinImuHz)
$minMagnetometerFrames = [math]::Max(1, $Seconds * $MinMagnetometerHz)
$minBarometerFrames = [math]::Max(1, $Seconds * $MinBarometerHz)
$lastHeartbeat = @($heartbeatFrames | Select-Object -Last 1)
$lastHeartbeatValue = if ($lastHeartbeat.Count -gt 0 -and $null -ne $lastHeartbeat[0].HeartbeatStatusFlags) {
    [int]$lastHeartbeat[0].HeartbeatStatusFlags
} else {
    $null
}
$lastHeartbeatStatusFlags = if ($null -ne $lastHeartbeatValue) { "0x{0:X4}" -f $lastHeartbeatValue } else { "n/a" }
$heartbeatStatusHistory = ($heartbeatFrames | ForEach-Object {
    "{0}:0x{1:X4}" -f $_.SensorUptimeMs, [int]$_.HeartbeatStatusFlags
}) -join ","
$lastDiagnostics = @($diagnosticFrames | Select-Object -Last 1)
$requiredReadyFlags = 0x01A9
$unhealthyFlags = 0x7E56
$readyFlagsOk = $null -ne $lastHeartbeatValue -and
                (($lastHeartbeatValue -band $requiredReadyFlags) -eq $requiredReadyFlags) -and
                (($lastHeartbeatValue -band $unhealthyFlags) -eq 0)
$icm42688Int1Active = $null -ne $lastHeartbeatValue -and
                      (($lastHeartbeatValue -band 0x0080) -ne 0)
$icm42688FifoActive = $null -ne $lastHeartbeatValue -and
                      (($lastHeartbeatValue -band 0x0100) -ne 0)
$icm42688FifoError = $null -ne $lastHeartbeatValue -and
                     (($lastHeartbeatValue -band 0x0200) -ne 0)
$uart4TxOverflow = $null -ne $lastHeartbeatValue -and
                   (($lastHeartbeatValue -band 0x0400) -ne 0)
$usart1TxOverflow = $null -ne $lastHeartbeatValue -and
                    (($lastHeartbeatValue -band 0x0800) -ne 0)
$icm42688InitError = $null -ne $lastHeartbeatValue -and
                     (($lastHeartbeatValue -band 0x1000) -ne 0)
$uart4RxOverflow = $null -ne $lastHeartbeatValue -and
                   (($lastHeartbeatValue -band 0x2000) -ne 0)
$i2cTransactionFailure = $null -ne $lastHeartbeatValue -and
                          (($lastHeartbeatValue -band 0x4000) -ne 0)

$invalidImuCount = @($imuFrames | Where-Object {
    $null -eq $_.SensorUptimeMs -or
    [math]::Abs([int]$_.AccelXMg) -gt 8000 -or
    [math]::Abs([int]$_.AccelYMg) -gt 8000 -or
    [math]::Abs([int]$_.AccelZMg) -gt 8000 -or
    [math]::Abs([long]$_.GyroXMdps) -gt 2000000 -or
    [math]::Abs([long]$_.GyroYMdps) -gt 2000000 -or
    [math]::Abs([long]$_.GyroZMdps) -gt 2000000 -or
    $_.ImuTemperatureCentiC -lt -4000 -or $_.ImuTemperatureCentiC -gt 8500
}).Count

$magneticMagnitudesUt = @($magnetometerFrames | ForEach-Object {
    [math]::Sqrt(
        [math]::Pow([double]$_.MagneticXNt, 2) +
        [math]::Pow([double]$_.MagneticYNt, 2) +
        [math]::Pow([double]$_.MagneticZNt, 2)
    ) / 1000.0
})
$invalidMagnetometerCount = @($magnetometerFrames | Where-Object {
    $null -eq $_.SensorUptimeMs -or
    [math]::Abs([long]$_.MagneticXNt) -gt 2000000 -or
    [math]::Abs([long]$_.MagneticYNt) -gt 2000000 -or
    [math]::Abs([long]$_.MagneticZNt) -gt 2000000 -or
    ($_.MagneticXNt -eq 0 -and $_.MagneticYNt -eq 0 -and $_.MagneticZNt -eq 0)
}).Count

$invalidBarometerCount = @($barometerFrames | Where-Object {
    $null -eq $_.SensorUptimeMs -or
    $_.PressurePa -lt 30000 -or $_.PressurePa -gt 120000 -or
    $_.BarometerTemperatureCentiC -lt -4000 -or $_.BarometerTemperatureCentiC -gt 8500
}).Count

$imuUptimeAdvances = $imuFrames.Count -ge 2 -and $imuFrames[-1].SensorUptimeMs -gt $imuFrames[0].SensorUptimeMs
$magnetometerUptimeAdvances = $magnetometerFrames.Count -ge 2 -and $magnetometerFrames[-1].SensorUptimeMs -gt $magnetometerFrames[0].SensorUptimeMs
$barometerUptimeAdvances = $barometerFrames.Count -ge 2 -and $barometerFrames[-1].SensorUptimeMs -gt $barometerFrames[0].SensorUptimeMs
$imuUniqueSampleCount = @($imuFrames | ForEach-Object {
    "$($_.AccelXMg),$($_.AccelYMg),$($_.AccelZMg),$($_.GyroXMdps),$($_.GyroYMdps),$($_.GyroZMdps)"
} | Sort-Object -Unique).Count
$magnetometerUniqueSampleCount = @($magnetometerFrames | ForEach-Object {
    "$($_.MagneticXNt),$($_.MagneticYNt),$($_.MagneticZNt)"
} | Sort-Object -Unique).Count
$barometerUniqueSampleCount = @($barometerFrames | ForEach-Object {
    "$($_.PressurePa),$($_.BarometerTemperatureCentiC)"
} | Sort-Object -Unique).Count
$imuTimestampNonMonotonicCount = 0
$imuTimestampMaxGapMs = 0
$imuTimestampMaxGapDetail = ""
$imuTimestampNonMonotonicDetails = New-Object System.Collections.Generic.List[string]
for ($index = 1; $index -lt $imuFrames.Count; ++$index) {
    $gap = [long]$imuFrames[$index].SensorUptimeMs - [long]$imuFrames[$index - 1].SensorUptimeMs
    if ($gap -le 0) {
        ++$imuTimestampNonMonotonicCount
        if ($imuTimestampNonMonotonicDetails.Count -lt 10) {
            $imuTimestampNonMonotonicDetails.Add(
                "$($imuFrames[$index - 1].Seq):$($imuFrames[$index - 1].SensorUptimeMs)->$($imuFrames[$index].Seq):$($imuFrames[$index].SensorUptimeMs)($gap)")
        }
    } elseif ($gap -gt $imuTimestampMaxGapMs) {
        $imuTimestampMaxGapMs = $gap
        $imuTimestampMaxGapDetail =
            "$($imuFrames[$index - 1].Seq):$($imuFrames[$index - 1].SensorUptimeMs)->$($imuFrames[$index].Seq):$($imuFrames[$index].SensorUptimeMs)"
    }
}

$imuAccelNormAvgG = if ($imuFrames.Count -gt 0) {
    [math]::Round((($imuFrames | ForEach-Object {
        [math]::Sqrt(
            [math]::Pow([double]$_.AccelXMg, 2) +
            [math]::Pow([double]$_.AccelYMg, 2) +
            [math]::Pow([double]$_.AccelZMg, 2)
        ) / 1000.0
    } | Measure-Object -Average).Average), 3)
} else { 0 }
$imuTemperatureAvgC = if ($imuFrames.Count -gt 0) {
    [math]::Round((($imuFrames | Measure-Object -Property ImuTemperatureCentiC -Average).Average / 100.0), 2)
} else { 0 }
$magneticMagnitudeAvgUt = if ($magneticMagnitudesUt.Count -gt 0) {
    [math]::Round((($magneticMagnitudesUt | Measure-Object -Average).Average), 2)
} else { 0 }
$pressureAvgPa = if ($barometerFrames.Count -gt 0) {
    [math]::Round((($barometerFrames | Measure-Object -Property PressurePa -Average).Average), 1)
} else { 0 }
$barometerTemperatureAvgC = if ($barometerFrames.Count -gt 0) {
    [math]::Round((($barometerFrames | Measure-Object -Property BarometerTemperatureCentiC -Average).Average / 100.0), 2)
} else { 0 }
$preview = ($data | Select-Object -First 80 | ForEach-Object { $_.ToString("X2") }) -join " "

Write-Host "bytes_read=$($data.Length)"
Write-Host "frames=$($frames.Count)"
Write-Host "heartbeat=$heartbeatCount"
Write-Host "sensor_hub_diagnostics=$diagnosticsCount"
Write-Host "imu=$imuCount"
Write-Host "magnetometer=$magnetometerCount"
Write-Host "barometer=$barometerCount"
Write-Host "frame_rate_hz=$frameRateHz"
Write-Host "imu_rate_hz=$imuRateHz"
Write-Host "magnetometer_rate_hz=$magnetometerRateHz"
Write-Host "barometer_rate_hz=$barometerRateHz"
Write-Host "crc_bad=$badCrc"
Write-Host "version_bad=$badVersion"
Write-Host "payload_bad=$badPayload"
Write-Host "last_heartbeat_status_flags=$lastHeartbeatStatusFlags"
Write-Host "heartbeat_status_history=$heartbeatStatusHistory"
if ($lastDiagnostics.Count -gt 0) {
    $diagnostics = $lastDiagnostics[0]
    Write-Host "i2c_recovery_count=$($diagnostics.I2cRecoveryCount)"
    Write-Host "i2c_failure_count=$($diagnostics.I2cFailureCount)"
    Write-Host ("i2c_last_hal_error=0x{0:X8}" -f [uint32]$diagnostics.I2cLastHalError)
    Write-Host ("i2c_last_device=0x{0:X2}" -f [byte]$diagnostics.I2cLastDeviceAddress)
    Write-Host ("i2c_last_register=0x{0:X2}" -f [byte]$diagnostics.I2cLastRegisterAddress)
    Write-Host "i2c_last_length=$($diagnostics.I2cLastLength)"
    Write-Host "i2c_last_operation=$($diagnostics.I2cLastOperation)"
    Write-Host "i2c_last_hal_status=$($diagnostics.I2cLastHalStatus)"
    Write-Host "fifo_overflow_count=$($diagnostics.FifoOverflowCount)"
    Write-Host "fifo_malformed_count=$($diagnostics.FifoMalformedCount)"
    Write-Host "fifo_empty_event_count=$($diagnostics.FifoEmptyEventCount)"
    Write-Host "fifo_drain_stall_count=$($diagnostics.FifoDrainStallCount)"
    Write-Host "fifo_skipped_packet_count=$($diagnostics.FifoSkippedPacketCount)"
    Write-Host "icm42688_init_error_step=$($diagnostics.Icm42688InitErrorStep)"
    Write-Host "diagnostics_schema_version=$($diagnostics.DiagnosticsSchemaVersion)"
    Write-Host "uart4_rx_drop_count=$($diagnostics.Uart4RxDropCount)"
}
Write-Host "sensor_ready_flags_ok=$readyFlagsOk"
Write-Host "icm42688_int1_active=$icm42688Int1Active"
Write-Host "icm42688_fifo_active=$icm42688FifoActive"
Write-Host "icm42688_fifo_error=$icm42688FifoError"
Write-Host "uart4_tx_overflow=$uart4TxOverflow"
Write-Host "usart1_tx_overflow=$usart1TxOverflow"
Write-Host "icm42688_init_error=$icm42688InitError"
Write-Host "uart4_rx_overflow=$uart4RxOverflow"
Write-Host "i2c_transaction_failure=$i2cTransactionFailure"
Write-Host "imu_invalid_samples=$invalidImuCount"
Write-Host "imu_uptime_advances=$imuUptimeAdvances"
Write-Host "imu_unique_samples=$imuUniqueSampleCount"
Write-Host "imu_timestamp_nonmonotonic=$imuTimestampNonMonotonicCount"
Write-Host "imu_timestamp_nonmonotonic_details=$($imuTimestampNonMonotonicDetails -join ',')"
Write-Host "imu_timestamp_max_gap_ms=$imuTimestampMaxGapMs"
Write-Host "imu_timestamp_max_gap_detail=$imuTimestampMaxGapDetail"
Write-Host "imu_accel_norm_avg_g=$imuAccelNormAvgG"
Write-Host "imu_temperature_avg_c=$imuTemperatureAvgC"
Write-Host "magnetometer_invalid_samples=$invalidMagnetometerCount"
Write-Host "magnetometer_uptime_advances=$magnetometerUptimeAdvances"
Write-Host "magnetometer_unique_samples=$magnetometerUniqueSampleCount"
Write-Host "magnetic_magnitude_avg_ut=$magneticMagnitudeAvgUt"
Write-Host "barometer_invalid_samples=$invalidBarometerCount"
Write-Host "barometer_uptime_advances=$barometerUptimeAdvances"
Write-Host "barometer_unique_samples=$barometerUniqueSampleCount"
Write-Host "pressure_avg_pa=$pressureAvgPa"
Write-Host "barometer_temperature_avg_c=$barometerTemperatureAvgC"
Write-Host "preview_hex=$preview"
Write-Host "first_frames:"
$frames | Select-Object -First 10 | Format-Table -AutoSize

$failures = New-Object System.Collections.Generic.List[string]
if ($frames.Count -eq 0) { $failures.Add("no valid frame envelope received") }
if ($badCrc -ne 0) { $failures.Add("$badCrc frame(s) failed CRC") }
if ($badVersion -ne 0) { $failures.Add("$badVersion frame(s) used an unexpected protocol version") }
if ($badPayload -ne 0) { $failures.Add("$badPayload known frame(s) used an unexpected payload length") }
if ($heartbeatCount -lt 1) { $failures.Add("no heartbeat received") }
if ($diagnosticsCount -lt 1) { $failures.Add("no sensor hub diagnostics received") }
if ($lastDiagnostics.Count -gt 0 -and $lastDiagnostics[0].DiagnosticsSchemaVersion -ne 1) {
    $failures.Add("sensor hub diagnostics extension schema is not version 1")
}
if ($lastDiagnostics.Count -gt 0 -and $lastDiagnostics[0].Uart4RxDropCount -ne 0) {
    $failures.Add("UART4 RX ring dropped $($lastDiagnostics[0].Uart4RxDropCount) byte(s)")
}
if (-not $readyFlagsOk) { $failures.Add("sensor ready/error flags are unhealthy: $lastHeartbeatStatusFlags") }
if ($imuCount -lt $minImuFrames) { $failures.Add("IMU rate is below $MinImuHz Hz") }
if ($magnetometerCount -lt $minMagnetometerFrames) { $failures.Add("magnetometer rate is below $MinMagnetometerHz Hz") }
if ($barometerCount -lt $minBarometerFrames) { $failures.Add("barometer rate is below $MinBarometerHz Hz") }
if ($invalidImuCount -ne 0) { $failures.Add("$invalidImuCount IMU sample(s) are outside plausible ranges") }
if ($invalidMagnetometerCount -ne 0) { $failures.Add("$invalidMagnetometerCount magnetometer sample(s) are outside plausible ranges") }
if ($invalidBarometerCount -ne 0) { $failures.Add("$invalidBarometerCount barometer sample(s) are outside plausible ranges") }
if (-not $imuUptimeAdvances) { $failures.Add("IMU timestamps did not advance") }
if ($imuTimestampNonMonotonicCount -ne 0) { $failures.Add("$imuTimestampNonMonotonicCount IMU timestamp step(s) were non-monotonic") }
if ($imuTimestampMaxGapMs -gt 100) { $failures.Add("IMU timestamp gap exceeded 100 ms: $imuTimestampMaxGapMs ms") }
if (-not $magnetometerUptimeAdvances) { $failures.Add("magnetometer timestamps did not advance") }
if (-not $barometerUptimeAdvances) { $failures.Add("barometer timestamps did not advance") }
if ($imuUniqueSampleCount -lt 2) { $failures.Add("IMU filtered samples did not change") }
if ($magnetometerUniqueSampleCount -lt 2) { $failures.Add("magnetometer filtered samples did not change") }
if ($barometerUniqueSampleCount -lt 2) { $failures.Add("barometer filtered samples did not change") }

if ($failures.Count -ne 0) {
    Write-Host "verification=FAILED"
    $failures | ForEach-Object { Write-Host "failure=$_" }
    exit 1
}

Write-Host "verification=PASSED"
