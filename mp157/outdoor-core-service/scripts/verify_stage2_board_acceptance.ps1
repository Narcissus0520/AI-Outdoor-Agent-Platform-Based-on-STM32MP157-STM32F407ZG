param(
    [string]$ScriptPath = "",
    [string]$DeploymentScriptPath = ""
)

$ErrorActionPreference = "Stop"

$moduleRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $moduleRoot "../..")).Path
if ([string]::IsNullOrWhiteSpace($ScriptPath)) {
    $ScriptPath = Join-Path $PSScriptRoot "run_stage2_board_acceptance.sh"
}
if ([string]::IsNullOrWhiteSpace($DeploymentScriptPath)) {
    $DeploymentScriptPath = Join-Path $repoRoot "scripts/deploy_mp157_runtime.ps1"
}

$ScriptPath = (Resolve-Path -LiteralPath $ScriptPath).Path
$DeploymentScriptPath = (Resolve-Path -LiteralPath $DeploymentScriptPath).Path
$scriptText = (Get-Content -LiteralPath $ScriptPath -Raw -Encoding utf8).Replace("`r`n", "`n")
$deploymentText = (Get-Content -LiteralPath $DeploymentScriptPath -Raw -Encoding utf8).Replace("`r`n", "`n")

function Assert-ContainsLiteral {
    param(
        [string]$Text,
        [string]$Expected,
        [string]$Description
    )

    if (-not $Text.Contains($Expected)) {
        throw "Stage 2 acceptance contract is missing ${Description}: $Expected"
    }
}

function Assert-Matches {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Description
    )

    if ($Text -notmatch $Pattern) {
        throw "Stage 2 acceptance contract does not match ${Description}: $Pattern"
    }
}

function Assert-DoesNotMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Description
    )

    if ($Text -match $Pattern) {
        throw "Stage 2 acceptance contract contains forbidden ${Description}: $Pattern"
    }
}

Assert-Matches $scriptText '^#!/bin/sh\n' "POSIX shell header"
Assert-ContainsLiteral $scriptText 'set -u' "unset-variable protection"
Assert-ContainsLiteral $scriptText 'if [ "$confirmation" != "--confirm" ] || [ "$#" -ne 1 ]; then' "explicit destructive-test confirmation"
Assert-ContainsLiteral $scriptText 'if [ "$(id -u)" != "0" ]; then' "root precondition"
Assert-ContainsLiteral $scriptText 'STAGE2_ACCEPTANCE_WAIT_SECONDS' "bounded-wait override"
Assert-ContainsLiteral $scriptText 'while [ "$elapsed" -lt "$wait_seconds" ]; do' "bounded wait loops"

Assert-ContainsLiteral $scriptText 'service_name="outdoor-agent-runtime.service"' "fixed Runtime unit"
Assert-ContainsLiteral $scriptText 'runtime_binary="/opt/outdoor-agent/bin/outdoor_core_runtime"' "deployed Runtime binary"
Assert-ContainsLiteral $scriptText 'status_query="/opt/outdoor-agent/bin/outdoor_status_query"' "deployed status query"
Assert-ContainsLiteral $scriptText 'agent_terminal="/opt/outdoor-agent/bin/outdoor_agent_terminal"' "deployed Agent terminal"
Assert-ContainsLiteral $scriptText 'socket_self_test="/opt/outdoor-agent/tests/unix_status_service_tests"' "deployed Unix socket self-test"
Assert-ContainsLiteral $scriptText 'status_socket="$runtime_directory/outdoor_core.sock"' "volatile status socket"

Assert-ContainsLiteral $scriptText 'initial_active="$(service_active_state)"' "initial active-state capture"
Assert-ContainsLiteral $scriptText 'initial_enabled="$(service_enabled_state)"' "initial enable-state capture"
Assert-ContainsLiteral $scriptText 'restore_initial_state()' "state restoration function"
Assert-ContainsLiteral $scriptText 'trap ''finish $?'' EXIT' "EXIT restoration trap"
Assert-ContainsLiteral $scriptText 'restoration_required=1' "mutation boundary"
Assert-ContainsLiteral $scriptText 'systemctl enable "$service_name"' "enabled-state restoration"
Assert-ContainsLiteral $scriptText 'systemctl disable "$service_name"' "disabled-state restoration"
Assert-ContainsLiteral $scriptText 'initial_state_restored' "restoration evidence"

Assert-ContainsLiteral $scriptText 'systemd-analyze verify "$unit_path"' "systemd unit verification"
Assert-ContainsLiteral $scriptText 'validate_runtime_ownership' "unmanaged Runtime exclusion"
Assert-ContainsLiteral $scriptText '/tmp/outdoor_agent_hour_pid' "long-test collision guard"
Assert-ContainsLiteral $scriptText '"$socket_self_test" > "$report_root/unix_status_service_tests.txt"' "stale/active socket self-test"
Assert-ContainsLiteral $scriptText 'stale_socket=true active_collision=true query=true unlink=true' "socket self-test evidence"

Assert-ContainsLiteral $scriptText 'directory_mode="$(stat -c ''%a'' "$runtime_directory")"' "Runtime directory permission check"
Assert-ContainsLiteral $scriptText 'socket_mode="$(stat -c ''%a'' "$status_socket")"' "socket permission check"
Assert-ContainsLiteral $scriptText 'while [ "$query_index" -le 3 ]; do' "three repeated status queries"
Assert-ContainsLiteral $scriptText '"state":[[:space:]]+"running"' "running-state query assertion"
Assert-ContainsLiteral $scriptText '"status_socket_enabled":[[:space:]]+true' "socket-enabled query assertion"

Assert-ContainsLiteral $scriptText '--request-id stage2-no-context --format json' "Agent without-context invocation"
Assert-ContainsLiteral $scriptText '--request-id stage2-with-context --status-socket "$status_socket"' "Agent with-context invocation"
Assert-ContainsLiteral $scriptText '"backend":[[:space:]]+"mock_no_inference"' "mock backend assertion"
Assert-ContainsLiteral $scriptText 'agent_without_context_completed' "without-context completed-state assertion"
Assert-ContainsLiteral $scriptText 'agent_with_context_completed' "with-context completed-state assertion"
Assert-ContainsLiteral $scriptText '"runtime_context_available":[[:space:]]+false' "without-context assertion"
Assert-ContainsLiteral $scriptText '"runtime_context_available":[[:space:]]+true' "with-context assertion"
Assert-ContainsLiteral $scriptText 'no AI inference was executed' "no-inference assertion"

Assert-ContainsLiteral $scriptText 'systemctl stop "$service_name"' "controlled SIGTERM stop"
Assert-ContainsLiteral $scriptText 'socket_not_removed_after_stop' "socket cleanup assertion"
Assert-ContainsLiteral $scriptText 'service_pid_not_changed_after_controlled_restart' "controlled restart PID assertion"
Assert-ContainsLiteral $scriptText 'systemctl kill --kill-who=main --signal=SIGKILL "$service_name"' "failure injection"
Assert-ContainsLiteral $scriptText 'wait_for_pid_change "$crash_pid"' "automatic restart PID assertion"
Assert-ContainsLiteral $scriptText 'status_after_failure_restart.json' "post-recovery query"
Assert-ContainsLiteral $scriptText 'STAGE2_BOARD_ACCEPTANCE_PASSED' "success marker"
Assert-ContainsLiteral $scriptText 'STAGE2_BOARD_ACCEPTANCE_FAILED' "failure marker"

Assert-DoesNotMatch $scriptText '(?im)^\s*(reboot|poweroff|halt|shutdown)\b' "power-state command"
Assert-DoesNotMatch $scriptText '(?i)COM[0-9]+|flash_f407|send_xmodem' "host serial or flashing action"
Assert-DoesNotMatch $scriptText '(?i)rm\s+-[^\n]*(rf|fr)\b' "recursive forced deletion"

Assert-ContainsLiteral $deploymentText '[string]$UnixStatusTestPath = ""' "Unix socket self-test deployment parameter"
Assert-ContainsLiteral $deploymentText 'build-arm/unix_status_service_tests' "Unix socket self-test build path"
Assert-ContainsLiteral $deploymentText 'Remote = "$InstallRoot/tests/unix_status_service_tests"' "Unix socket self-test remote path"
Assert-ContainsLiteral $deploymentText 'run_stage2_board_acceptance.sh' "Stage 2 acceptance deployment entry"
Assert-ContainsLiteral $deploymentText '$InstallRoot/tests' "deployment tests directory"
Assert-ContainsLiteral $deploymentText 'sh -n $InstallRoot/scripts/run_stage2_board_acceptance.sh' "deployed shell syntax check"
Assert-ContainsLiteral $deploymentText 'verify_stage2_board_acceptance.ps1' "host acceptance verifier path"
Assert-ContainsLiteral $deploymentText '& $stage2AcceptanceVerifierPath -ScriptPath $stage2AcceptancePath -DeploymentScriptPath $PSCommandPath' "host acceptance verifier invocation"

Write-Host "Stage 2 board acceptance contract verification passed."
