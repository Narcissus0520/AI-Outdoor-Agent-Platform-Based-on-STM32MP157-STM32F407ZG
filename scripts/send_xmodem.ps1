param(
    [Parameter(Mandatory = $true)]
    [string]$PortName,

    [Parameter(Mandatory = $true)]
    [string]$LocalPath,

    [Parameter(Mandatory = $true)]
    [string]$RemotePath,

    [int]$BaudRate = 115200,
    [int]$MaxRetries = 10
)

$ErrorActionPreference = "Stop"

if ($RemotePath -notmatch '^/[A-Za-z0-9._/-]+$') {
    throw "RemotePath must be an absolute Linux path containing only letters, numbers, '.', '_', '-', and '/'."
}
if ($MaxRetries -lt 1) {
    throw "MaxRetries must be at least 1."
}

$resolvedLocalPath = (Resolve-Path -LiteralPath $LocalPath).Path
$fileInfo = Get-Item -LiteralPath $resolvedLocalPath
if ($fileInfo.PSIsContainer) {
    throw "LocalPath must refer to a file."
}

$soh = [byte]0x01
$eot = [byte]0x04
$ack = [byte]0x06
$nak = [byte]0x15
$can = [byte]0x18
$crcRequest = [byte]0x43
$blockSize = 128

function Get-XmodemCrc16 {
    param([byte[]]$Data)

    [uint16]$crc = 0
    foreach ($value in $Data) {
        $crc = [uint16](($crc -bxor ([int]$value -shl 8)) -band 0xFFFF)
        for ($bit = 0; $bit -lt 8; ++$bit) {
            if (($crc -band 0x8000) -ne 0) {
                $crc = [uint16](((($crc -shl 1) -bxor 0x1021)) -band 0xFFFF)
            } else {
                $crc = [uint16](($crc -shl 1) -band 0xFFFF)
            }
        }
    }
    return $crc
}

function Wait-XmodemControl {
    param(
        [System.IO.Ports.SerialPort]$SerialPort,
        [int]$TimeoutMilliseconds
    )

    $deadline = (Get-Date).AddMilliseconds($TimeoutMilliseconds)
    while ((Get-Date) -lt $deadline) {
        try {
            $value = $SerialPort.ReadByte()
            if ($value -eq $ack -or $value -eq $nak -or $value -eq $can -or $value -eq $crcRequest) {
                return [byte]$value
            }
        } catch [System.TimeoutException] {
        }
    }
    return $null
}

$port = [System.IO.Ports.SerialPort]::new(
    $PortName,
    $BaudRate,
    [System.IO.Ports.Parity]::None,
    8,
    [System.IO.Ports.StopBits]::One
)
$port.ReadTimeout = 200
$port.WriteTimeout = 3000

$stream = $null
try {
    $port.Open()
    $port.DiscardInBuffer()
    $port.Write("`r")
    Start-Sleep -Milliseconds 150
    $port.DiscardInBuffer()
    $port.Write("rx -c -b $RemotePath`r")

    $handshake = Wait-XmodemControl $port 10000
    if ($handshake -ne $crcRequest) {
        throw "Receiver did not request XMODEM-CRC transfer."
    }

    $stream = [System.IO.File]::OpenRead($resolvedLocalPath)
    $totalBlocks = [math]::Ceiling($stream.Length / $blockSize)
    $blockNumber = 1
    $blockIndex = 0

    while ($stream.Position -lt $stream.Length) {
        $payload = [byte[]]::new($blockSize)
        for ($index = 0; $index -lt $payload.Length; ++$index) {
            $payload[$index] = [byte]0x1A
        }
        $bytesRead = $stream.Read($payload, 0, $payload.Length)
        if ($bytesRead -le 0) {
            break
        }

        $packet = [byte[]]::new(3 + $blockSize + 2)
        $packet[0] = $soh
        $packet[1] = [byte]$blockNumber
        $packet[2] = [byte](0xFF - $blockNumber)
        [Array]::Copy($payload, 0, $packet, 3, $payload.Length)
        $crc = Get-XmodemCrc16 $payload
        $packet[3 + $blockSize] = [byte](($crc -shr 8) -band 0xFF)
        $packet[4 + $blockSize] = [byte]($crc -band 0xFF)

        $accepted = $false
        for ($attempt = 1; $attempt -le $MaxRetries; ++$attempt) {
            $port.Write($packet, 0, $packet.Length)
            $response = Wait-XmodemControl $port 3000
            if ($response -eq $ack) {
                $accepted = $true
                break
            }
            if ($response -eq $can) {
                throw "Receiver cancelled the XMODEM transfer."
            }
        }
        if (-not $accepted) {
            throw "Block $blockNumber was not acknowledged after $MaxRetries attempts."
        }

        ++$blockIndex
        $percent = [math]::Floor(($blockIndex * 100.0) / $totalBlocks)
        Write-Progress -Activity "Uploading $($fileInfo.Name) over XMODEM" -Status "$blockIndex / $totalBlocks blocks" -PercentComplete $percent
        $blockNumber = ($blockNumber + 1) -band 0xFF
    }

    $completed = $false
    for ($attempt = 1; $attempt -le $MaxRetries; ++$attempt) {
        $port.Write([byte[]]@($eot), 0, 1)
        $response = Wait-XmodemControl $port 3000
        if ($response -eq $ack) {
            $completed = $true
            break
        }
        if ($response -eq $can) {
            throw "Receiver cancelled the XMODEM transfer during completion."
        }
    }
    if (-not $completed) {
        throw "Receiver did not acknowledge XMODEM completion."
    }

    Start-Sleep -Milliseconds 250
    $port.DiscardInBuffer()
    $port.Write("truncate -s $($fileInfo.Length) $RemotePath && sync && printf '__XMODEM_%s__\n' TRUNCATE_OK`r")
    $truncateDeadline = (Get-Date).AddSeconds(5)
    $truncateOutput = New-Object System.Text.StringBuilder
    while ((Get-Date) -lt $truncateDeadline) {
        $chunk = $port.ReadExisting()
        if ($chunk.Length -gt 0) {
            [void]$truncateOutput.Append($chunk)
            if ($truncateOutput.ToString().Contains('__XMODEM_TRUNCATE_OK__')) {
                break
            }
        }
        Start-Sleep -Milliseconds 50
    }
    if (-not $truncateOutput.ToString().Contains('__XMODEM_TRUNCATE_OK__')) {
        throw "XMODEM completed, but the receiver did not confirm removal of protocol padding."
    }

    Write-Progress -Activity "Uploading $($fileInfo.Name) over XMODEM" -Completed
    Write-Host "Uploaded $($stream.Length) bytes to $RemotePath over $PortName using XMODEM-CRC."
} catch {
    if ($port.IsOpen) {
        try {
            $port.Write([byte[]]@($can, $can), 0, 2)
        } catch {
        }
    }
    throw
} finally {
    if ($null -ne $stream) {
        $stream.Dispose()
    }
    if ($port.IsOpen) {
        $port.Close()
    }
    $port.Dispose()
}
