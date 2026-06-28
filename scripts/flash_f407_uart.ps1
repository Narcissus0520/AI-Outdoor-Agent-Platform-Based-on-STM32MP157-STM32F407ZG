param(
    [string]$PortName = "COM3",
    [string]$FirmwarePath = "",
    [uint32]$BaseAddress = 0x08000000,
    [switch]$ReadoutUnprotectFirst
)

$ErrorActionPreference = "Stop"

$Ack = 0x79
$Nack = 0x1F
$ChunkSize = 256

if ([string]::IsNullOrWhiteSpace($FirmwarePath)) {
    $repoRoot = Split-Path -Parent $PSScriptRoot
    $FirmwarePath = Join-Path $repoRoot "f407/sensor-hub/firmware/build/sensor_hub.bin"
}

$FirmwarePath = (Resolve-Path -LiteralPath $FirmwarePath).Path
$firmware = [System.IO.File]::ReadAllBytes($FirmwarePath)
if ($firmware.Length -eq 0) {
    throw "Firmware is empty: $FirmwarePath"
}

$port = [System.IO.Ports.SerialPort]::new(
    $PortName,
    115200,
    [System.IO.Ports.Parity]::Even,
    8,
    [System.IO.Ports.StopBits]::One
)
$port.ReadTimeout = 2000
$port.WriteTimeout = 2000

function Write-Bytes {
    param([byte[]]$Bytes)
    $port.Write($Bytes, 0, $Bytes.Length)
}

function Read-Byte {
    return [byte]$port.ReadByte()
}

function Read-Bytes {
    param([int]$Length)

    $result = [byte[]]::new($Length)
    $offset = 0
    while ($offset -lt $Length) {
        $read = $port.Read($result, $offset, $Length - $offset)
        if ($read -le 0) {
            throw "Serial read returned no data"
        }
        $offset += $read
    }
    return $result
}

function Expect-Ack {
    param([string]$Operation)

    $response = Read-Byte
    if ($response -eq $Nack) {
        throw "$Operation returned NACK"
    }
    if ($response -ne $Ack) {
        throw ("{0} returned unexpected response 0x{1:X2}" -f $Operation, $response)
    }
}

function Send-Command {
    param([byte]$Command, [string]$Operation)

    Write-Bytes ([byte[]]@($Command, ($Command -bxor 0xFF)))
    Expect-Ack $Operation
}

function Enter-Bootloader {
    $port.DtrEnable = $false
    $port.RtsEnable = $true
    Start-Sleep -Milliseconds 150
    $port.DtrEnable = $true
    $port.RtsEnable = $true
    Start-Sleep -Milliseconds 250
    $port.DiscardInBuffer()

    Write-Bytes ([byte[]]@(0x7F))
    Expect-Ack "Bootloader synchronization"
}

function Get-AddressPacket {
    param([uint32]$Address)

    $packet = [byte[]]::new(5)
    $packet[0] = [byte](($Address -shr 24) -band 0xFF)
    $packet[1] = [byte](($Address -shr 16) -band 0xFF)
    $packet[2] = [byte](($Address -shr 8) -band 0xFF)
    $packet[3] = [byte]($Address -band 0xFF)
    $packet[4] = [byte]($packet[0] -bxor $packet[1] -bxor $packet[2] -bxor $packet[3])
    return $packet
}

function Read-Memory {
    param([uint32]$Address, [int]$Length)

    Send-Command 0x11 "Read Memory command"
    Write-Bytes (Get-AddressPacket $Address)
    Expect-Ack "Read Memory address"

    $count = [byte]($Length - 1)
    Write-Bytes ([byte[]]@($count, ($count -bxor 0xFF)))
    Expect-Ack "Read Memory length"
    return Read-Bytes $Length
}

function Start-Application {
    param([uint32]$Address)

    Send-Command 0x21 "Go command"
    Write-Bytes (Get-AddressPacket $Address)
    Expect-Ack "Go address"
}

try {
    $port.Open()

    Enter-Bootloader

    if ($ReadoutUnprotectFirst) {
        Write-Host "Issuing Readout Unprotect. This may erase user Flash and take several seconds..."
        Send-Command 0x92 "Readout Unprotect command"
        $oldTimeout = $port.ReadTimeout
        $port.ReadTimeout = 20000
        try {
            Expect-Ack "Readout Unprotect completion"
            Write-Host "Readout Unprotect completion ACK received."
        } catch {
            Write-Host "Readout Unprotect completion ACK not received; continuing with bootloader resync."
        }
        $port.ReadTimeout = $oldTimeout
        Start-Sleep -Seconds 2
        Enter-Bootloader
    }

    Send-Command 0x00 "Get command"
    $getLength = [int](Read-Byte) + 1
    $getResponse = Read-Bytes $getLength
    Expect-Ack "Get command completion"
    $bootloaderVersion = $getResponse[0]
    $supportedCommands = $getResponse[1..($getResponse.Length - 1)]

    Send-Command 0x02 "Get ID command"
    $idLength = [int](Read-Byte) + 1
    $chipIdBytes = Read-Bytes $idLength
    Expect-Ack "Get ID completion"
    $chipId = ($chipIdBytes | ForEach-Object { $_.ToString("X2") }) -join ""

    Write-Host ("Bootloader version: 0x{0:X2}" -f $bootloaderVersion)
    Write-Host "Chip ID: 0x$chipId"
    Write-Host "Firmware: $FirmwarePath ($($firmware.Length) bytes)"

    if ($ReadoutUnprotectFirst) {
        Write-Host "Skipping explicit erase because Readout Unprotect already erased user Flash."
    } else {
        $oldTimeout = $port.ReadTimeout
        $port.ReadTimeout = 30000
        try {
            if ($supportedCommands -contains 0x44) {
                Send-Command 0x44 "Extended Erase command"
                Write-Bytes ([byte[]]@(0xFF, 0xFF, 0x00))
                Expect-Ack "Global erase"
            } elseif ($supportedCommands -contains 0x43) {
                Send-Command 0x43 "Erase command"
                Write-Bytes ([byte[]]@(0xFF, 0x00))
                Expect-Ack "Global erase"
            } else {
                throw "Bootloader does not advertise an erase command"
            }
        } finally {
            $port.ReadTimeout = $oldTimeout
        }
    }

    for ($offset = 0; $offset -lt $firmware.Length; $offset += $ChunkSize) {
        $length = [Math]::Min($ChunkSize, $firmware.Length - $offset)
        $address = $BaseAddress + [uint32]$offset

        Send-Command 0x31 "Write Memory command"
        Write-Bytes (Get-AddressPacket $address)
        Expect-Ack "Write Memory address"

        $packet = [byte[]]::new($length + 2)
        $packet[0] = [byte]($length - 1)
        [Array]::Copy($firmware, $offset, $packet, 1, $length)
        $checksum = $packet[0]
        for ($index = 0; $index -lt $length; ++$index) {
            $checksum = $checksum -bxor $packet[$index + 1]
        }
        $packet[$packet.Length - 1] = $checksum
        Write-Bytes $packet
        Expect-Ack "Write Memory data"

        $percent = [Math]::Floor((($offset + $length) * 100.0) / $firmware.Length)
        Write-Progress -Activity "Programming F407" -Status "$percent%" -PercentComplete $percent
    }
    Write-Progress -Activity "Programming F407" -Completed

    for ($offset = 0; $offset -lt $firmware.Length; $offset += $ChunkSize) {
        $length = [Math]::Min($ChunkSize, $firmware.Length - $offset)
        $address = $BaseAddress + [uint32]$offset
        $readBack = Read-Memory $address $length

        for ($index = 0; $index -lt $length; ++$index) {
            if ($readBack[$index] -ne $firmware[$offset + $index]) {
                throw ("Verification failed at address 0x{0:X8}" -f ($address + $index))
            }
        }
    }

    Write-Host "Programming and byte-for-byte verification succeeded."

    Write-Host ("Starting application at 0x{0:X8}." -f $BaseAddress)
    Start-Application $BaseAddress
    $port.RtsEnable = $false
    $port.DtrEnable = $true
    Start-Sleep -Milliseconds 300
} finally {
    if ($port.IsOpen) {
        $port.Close()
    }
    $port.Dispose()
}
