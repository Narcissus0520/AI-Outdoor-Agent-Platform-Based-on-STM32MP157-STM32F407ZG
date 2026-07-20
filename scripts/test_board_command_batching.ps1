$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "board_command_batching.ps1")

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

$commands = @(
    "chmod 0755 /opt/outdoor-agent/bin/outdoor_core_runtime",
    "chmod 0755 /opt/outdoor-agent/bin/outdoor_status_query",
    "chmod 0755 /opt/outdoor-agent/bin/outdoor_agent_terminal",
    "chmod 0755 /opt/outdoor-agent/tests/unix_status_service_tests"
)
$batches = @(Split-BoardCommandBatches -Commands $commands -MaximumLength 120)
Assert-True ($batches.Count -gt 1) "commands should be split into more than one batch"
foreach ($batch in $batches) {
    Assert-True ($batch.Length -le 120) "batch exceeds the configured maximum: $batch"
}
$roundTrip = @($batches | ForEach-Object { $_ -split '; ' })
Assert-True (($roundTrip -join "`n") -eq ($commands -join "`n")) `
    "batching must preserve command order and content"

$exact = "x" * 64
$exactBatch = @(Split-BoardCommandBatches -Commands @($exact) -MaximumLength 64)
Assert-True ($exactBatch.Count -eq 1 -and $exactBatch[0] -eq $exact) `
    "a command at the exact limit should be accepted"

$emptyBatches = @(Split-BoardCommandBatches -Commands @() -MaximumLength 64)
Assert-True ($emptyBatches.Count -eq 0) "an empty command list should produce no batches"

$oversizedRejected = $false
try {
    [void](Split-BoardCommandBatches -Commands @("y" * 65) -MaximumLength 64)
} catch {
    $oversizedRejected = $_.Exception.Message -match "exceeds the 64-character limit"
}
Assert-True $oversizedRejected "an oversized individual command should be rejected"

$emptyRejected = $false
try {
    [void](Split-BoardCommandBatches -Commands @("valid", "") -MaximumLength 64)
} catch {
    $emptyRejected = $_.Exception.Message -match "empty command"
}
Assert-True $emptyRejected "an empty individual command should be rejected"

Write-Host "Board command batching tests passed."
