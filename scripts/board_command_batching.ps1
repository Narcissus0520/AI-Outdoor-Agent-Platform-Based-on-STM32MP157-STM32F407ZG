function Split-BoardCommandBatches {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [string[]]$Commands,
        [ValidateRange(64, 4096)]
        [int]$MaximumLength = 600
    )

    $batches = [System.Collections.Generic.List[string]]::new()
    $current = ""

    foreach ($command in $Commands) {
        if ([string]::IsNullOrWhiteSpace($command)) {
            throw "Board command batch contains an empty command."
        }
        if ($command.Length -gt $MaximumLength) {
            throw "Board command exceeds the $MaximumLength-character limit: $command"
        }

        if ([string]::IsNullOrEmpty($current)) {
            $current = $command
            continue
        }

        $candidate = "$current; $command"
        if ($candidate.Length -gt $MaximumLength) {
            $batches.Add($current)
            $current = $command
        } else {
            $current = $candidate
        }
    }

    if (-not [string]::IsNullOrEmpty($current)) {
        $batches.Add($current)
    }

    return $batches.ToArray()
}
