$ErrorActionPreference = "Stop"

$moduleRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $moduleRoot "../..")).Path
$verifier = Join-Path $PSScriptRoot "verify_stage2_board_acceptance.ps1"
$sourceScript = Join-Path $PSScriptRoot "run_stage2_board_acceptance.sh"
$sourceDeployment = Join-Path $repoRoot "scripts/deploy_mp157_runtime.ps1"
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("outdoor-stage2-acceptance-" + [System.IO.Path]::GetRandomFileName())

function Invoke-Verifier {
    param(
        [string]$ScriptPath,
        [string]$DeploymentPath,
        [bool]$ExpectSuccess,
        [string]$CaseName
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass `
            -File $verifier `
            -ScriptPath $ScriptPath `
            -DeploymentScriptPath $DeploymentPath 2>&1
        $succeeded = $LASTEXITCODE -eq 0
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    if ($succeeded -ne $ExpectSuccess) {
        throw "Stage 2 acceptance verifier case '$CaseName' expected success=$ExpectSuccess but success=$succeeded`n$($output -join "`n")"
    }
}

function Write-Fixture {
    param(
        [string]$Name,
        [string]$Content
    )

    $path = Join-Path $testRoot $Name
    [System.IO.File]::WriteAllText($path, $Content, [System.Text.UTF8Encoding]::new($false))
    return $path
}

try {
    New-Item -ItemType Directory -Path $testRoot | Out-Null
    $validScriptText = Get-Content -LiteralPath $sourceScript -Raw -Encoding utf8
    $validDeploymentText = Get-Content -LiteralPath $sourceDeployment -Raw -Encoding utf8

    Invoke-Verifier $sourceScript $sourceDeployment $true "valid acceptance contract"

    $confirmationBypass = $validScriptText.Replace(
        'if [ "$confirmation" != "--confirm" ] || [ "$#" -ne 1 ]; then',
        'if [ "$confirmation" = "--confirm" ] && [ "$#" -eq 1 ]; then')
    Invoke-Verifier (Write-Fixture "confirmation-bypass.sh" $confirmationBypass) `
        $sourceDeployment $false "confirmation bypassed"

    $missingTrap = $validScriptText.Replace("trap 'finish `$?' EXIT", "# EXIT trap removed")
    Invoke-Verifier (Write-Fixture "missing-trap.sh" $missingTrap) `
        $sourceDeployment $false "restoration trap removed"

    $softFailure = $validScriptText.Replace("--signal=SIGKILL", "--signal=SIGTERM")
    Invoke-Verifier (Write-Fixture "soft-failure.sh" $softFailure) `
        $sourceDeployment $false "failure injection weakened"

    $singleQuery = $validScriptText.Replace(
        'while [ "$query_index" -le 3 ]; do',
        'while [ "$query_index" -le 1 ]; do')
    Invoke-Verifier (Write-Fixture "single-query.sh" $singleQuery) `
        $sourceDeployment $false "repeat query coverage reduced"

    $powerMutation = $validScriptText + "`nreboot`n"
    Invoke-Verifier (Write-Fixture "power-mutation.sh" $powerMutation) `
        $sourceDeployment $false "power command added"

    $missingDeployment = $validDeploymentText.Replace(
        'Remote = "$InstallRoot/tests/unix_status_service_tests"',
        'Remote = "$InstallRoot/tests/socket_test_missing"')
    Invoke-Verifier $sourceScript `
        (Write-Fixture "missing-self-test-deployment.ps1" $missingDeployment) `
        $false "self-test deployment removed"

    Write-Host "Stage 2 board acceptance verifier tests passed."
} finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
