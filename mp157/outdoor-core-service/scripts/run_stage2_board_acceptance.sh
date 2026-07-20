#!/bin/sh

set -u

service_name="outdoor-agent-runtime.service"
unit_path="/etc/systemd/system/outdoor-agent-runtime.service"
runtime_binary="/opt/outdoor-agent/bin/outdoor_core_runtime"
status_query="/opt/outdoor-agent/bin/outdoor_status_query"
agent_terminal="/opt/outdoor-agent/bin/outdoor_agent_terminal"
socket_self_test="/opt/outdoor-agent/tests/unix_status_service_tests"
runtime_config="/opt/outdoor-agent/config/outdoor_agent_service.conf"
runtime_directory="/run/outdoor-agent"
status_socket="$runtime_directory/outdoor_core.sock"
wait_seconds="${STAGE2_ACCEPTANCE_WAIT_SECONDS:-20}"
confirmation="${1:-}"

case "$wait_seconds" in
    ''|*[!0-9]*|0)
        echo "STAGE2_ACCEPTANCE_WAIT_SECONDS must be a positive integer" >&2
        exit 2
        ;;
esac

if [ "$confirmation" != "--confirm" ] || [ "$#" -ne 1 ]; then
    echo "Usage: $0 --confirm" >&2
    echo "This explicitly starts/stops the Runtime and injects one SIGKILL for restart validation." >&2
    exit 2
fi

if [ "$(id -u)" != "0" ]; then
    echo "Stage 2 board acceptance must run as root" >&2
    exit 2
fi

for required_command in systemctl systemd-analyze stat grep sed tr mktemp sha256sum tee; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        echo "required command is missing: $required_command" >&2
        exit 2
    fi
done

report_root="$(mktemp -d /tmp/outdoor-agent-stage2-acceptance-XXXXXX)" || exit 2
report_path="$report_root/acceptance_report.txt"
: > "$report_path"

failures=0
restoration_required=0
initial_active=""
initial_enabled=""

record_pass()
{
    echo "PASS $*" | tee -a "$report_path"
}

record_fail()
{
    failures=$((failures + 1))
    echo "FAIL $*" | tee -a "$report_path" >&2
}

service_active_state()
{
    systemctl is-active "$service_name" 2>/dev/null || true
}

service_enabled_state()
{
    systemctl is-enabled "$service_name" 2>/dev/null || true
}

service_main_pid()
{
    systemctl show -p MainPID "$service_name" 2>/dev/null |
        sed -n 's/^MainPID=//p'
}

service_restart_count()
{
    systemctl show -p NRestarts "$service_name" 2>/dev/null |
        sed -n 's/^NRestarts=//p'
}

wait_for_active()
{
    elapsed=0
    while [ "$elapsed" -lt "$wait_seconds" ]; do
        current_pid="$(service_main_pid)"
        if [ "$(service_active_state)" = "active" ] &&
           [ -n "$current_pid" ] && [ "$current_pid" != "0" ]; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

wait_for_inactive()
{
    elapsed=0
    while [ "$elapsed" -lt "$wait_seconds" ]; do
        if [ "$(service_active_state)" = "inactive" ]; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

wait_for_socket()
{
    elapsed=0
    while [ "$elapsed" -lt "$wait_seconds" ]; do
        if [ -S "$status_socket" ]; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

wait_for_pid_change()
{
    previous_pid="$1"
    elapsed=0
    while [ "$elapsed" -lt "$wait_seconds" ]; do
        current_pid="$(service_main_pid)"
        if [ "$(service_active_state)" = "active" ] &&
           [ -n "$current_pid" ] && [ "$current_pid" != "0" ] &&
           [ "$current_pid" != "$previous_pid" ]; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

restore_initial_state()
{
    restore_failed=0

    if [ "$initial_active" = "active" ]; then
        if ! systemctl reset-failed "$service_name" >/dev/null 2>&1; then
            restore_failed=1
        fi
        if ! systemctl start "$service_name" >/dev/null 2>&1; then
            restore_failed=1
        elif ! wait_for_active; then
            restore_failed=1
        fi
    else
        if ! systemctl stop "$service_name" >/dev/null 2>&1; then
            restore_failed=1
        elif ! wait_for_inactive; then
            restore_failed=1
        fi
        systemctl reset-failed "$service_name" >/dev/null 2>&1 || true
    fi

    if [ "$initial_enabled" = "enabled" ]; then
        if ! systemctl enable "$service_name" >/dev/null 2>&1; then
            restore_failed=1
        fi
    else
        if ! systemctl disable "$service_name" >/dev/null 2>&1; then
            restore_failed=1
        fi
    fi

    if [ "$restore_failed" -ne 0 ]; then
        return 1
    fi
    if [ "$(service_active_state)" != "$initial_active" ]; then
        return 1
    fi
    if [ "$(service_enabled_state)" != "$initial_enabled" ]; then
        return 1
    fi
    return 0
}

finish()
{
    exit_code="$1"
    trap - EXIT INT TERM

    if [ "$restoration_required" -eq 1 ]; then
        if restore_initial_state; then
            record_pass "initial_state_restored active=$initial_active enabled=$initial_enabled"
        else
            record_fail "initial_state_restore active_expected=$initial_active enabled_expected=$initial_enabled active_actual=$(service_active_state) enabled_actual=$(service_enabled_state)"
        fi
    fi

    if command -v journalctl >/dev/null 2>&1; then
        journalctl -u "$service_name" -n 100 --no-pager > "$report_root/runtime_journal.txt" 2>&1 || true
    fi
    echo "failure_count=$failures" >> "$report_path"
    echo "report_root=$report_root" >> "$report_path"

    if [ "$failures" -ne 0 ] && [ "$exit_code" -eq 0 ]; then
        exit_code=1
    fi
    if [ "$exit_code" -eq 0 ]; then
        echo "STAGE2_BOARD_ACCEPTANCE_PASSED report=$report_path"
    else
        echo "STAGE2_BOARD_ACCEPTANCE_FAILED exit_code=$exit_code failures=$failures report=$report_path" >&2
    fi
    exit "$exit_code"
}

trap 'finish $?' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

require_executable()
{
    path="$1"
    if [ ! -x "$path" ]; then
        record_fail "executable_missing path=$path"
        return 1
    fi
    record_pass "executable path=$path"
    return 0
}

validate_runtime_ownership()
{
    expected_pid="0"
    if [ "$initial_active" = "active" ]; then
        expected_pid="$(service_main_pid)"
        case "$expected_pid" in
            ''|0|*[!0-9]*)
                record_fail "service_main_pid_invalid value=$expected_pid"
                return 1
                ;;
        esac
    fi

    found_expected=0
    for proc_dir in /proc/[0-9]*; do
        if [ ! -r "$proc_dir/cmdline" ]; then
            continue
        fi
        executable="$(tr '\000' '\n' < "$proc_dir/cmdline" 2>/dev/null | sed -n '1p')"
        case "${executable##*/}" in
            outdoor_core_runtime*)
                process_pid="${proc_dir##*/}"
                if [ "$expected_pid" = "0" ] || [ "$process_pid" != "$expected_pid" ]; then
                    record_fail "unmanaged_runtime_process pid=$process_pid expected_service_pid=$expected_pid"
                    return 1
                fi
                found_expected=1
                ;;
        esac
    done

    if [ "$expected_pid" != "0" ] && [ "$found_expected" -ne 1 ]; then
        record_fail "service_runtime_process_not_found pid=$expected_pid"
        return 1
    fi
    record_pass "runtime_process_ownership expected_service_pid=$expected_pid"
    return 0
}

require_json_pattern()
{
    label="$1"
    pattern="$2"
    path="$3"
    if grep -Eq "$pattern" "$path"; then
        record_pass "$label path=$path"
        return 0
    fi
    record_fail "$label pattern_not_found=$pattern path=$path"
    return 1
}

query_status()
{
    output_path="$1"
    if ! "$status_query" "$status_socket" > "$output_path" 2>&1; then
        record_fail "status_query path=$output_path"
        return 1
    fi
    require_json_pattern "status_runtime_running" '"state":[[:space:]]+"running"' "$output_path" || return 1
    require_json_pattern "status_socket_enabled" '"status_socket_enabled":[[:space:]]+true' "$output_path" || return 1
    require_json_pattern "status_socket_path" '"status_socket_path":[[:space:]]+"/run/outdoor-agent/outdoor_core.sock"' "$output_path" || return 1
    return 0
}

run_acceptance()
{
    require_executable "$runtime_binary" || return 1
    require_executable "$status_query" || return 1
    require_executable "$agent_terminal" || return 1
    require_executable "$socket_self_test" || return 1
    if [ ! -f "$unit_path" ] || [ ! -f "$runtime_config" ]; then
        record_fail "unit_or_config_missing unit=$unit_path config=$runtime_config"
        return 1
    fi

    if ! systemd-analyze verify "$unit_path" > "$report_root/systemd_analyze.txt" 2>&1; then
        record_fail "systemd_analyze_verify log=$report_root/systemd_analyze.txt"
        return 1
    fi
    record_pass "systemd_analyze_verify unit=$unit_path"

    initial_active="$(service_active_state)"
    initial_enabled="$(service_enabled_state)"
    case "$initial_active" in
        active|inactive) ;;
        *)
            record_fail "unsupported_initial_active_state state=$initial_active"
            return 1
            ;;
    esac
    case "$initial_enabled" in
        enabled|disabled) ;;
        *)
            record_fail "unsupported_initial_enabled_state state=$initial_enabled"
            return 1
            ;;
    esac
    record_pass "initial_state active=$initial_active enabled=$initial_enabled"

    for pid_file in /tmp/outdoor_agent_hour_pid /tmp/outdoor_agent_crash_recovery_pid; do
        if [ -r "$pid_file" ]; then
            tracked_pid="$(sed -n '1p' "$pid_file")"
            if [ -n "$tracked_pid" ] && kill -0 "$tracked_pid" 2>/dev/null; then
                record_fail "conflicting_validation_active pid_file=$pid_file pid=$tracked_pid"
                return 1
            fi
        fi
    done
    validate_runtime_ownership || return 1

    if ! {
        echo "board_date=$(date)"
        echo "kernel=$(uname -a)"
        echo "service=$service_name"
        echo "initial_active=$initial_active"
        echo "initial_enabled=$initial_enabled"
        sha256sum "$runtime_binary" "$status_query" "$agent_terminal" "$socket_self_test" "$runtime_config" "$unit_path"
    } > "$report_root/acceptance_metadata.txt"; then
        record_fail "acceptance_metadata_write path=$report_root/acceptance_metadata.txt"
        return 1
    fi

    if ! "$socket_self_test" > "$report_root/unix_status_service_tests.txt" 2>&1; then
        record_fail "unix_status_service_self_test log=$report_root/unix_status_service_tests.txt"
        return 1
    fi
    record_pass "unix_status_service_self_test stale_socket=true active_collision=true query=true unlink=true"

    restoration_required=1
    systemctl reset-failed "$service_name" >/dev/null 2>&1 || true
    if ! systemctl start "$service_name" > "$report_root/service_start.txt" 2>&1; then
        record_fail "service_start log=$report_root/service_start.txt"
        return 1
    fi
    if ! wait_for_active; then
        systemctl show "$service_name" > "$report_root/service_start_state.txt" 2>&1 || true
        record_fail "service_active timeout_seconds=$wait_seconds state=$report_root/service_start_state.txt"
        return 1
    fi
    if ! wait_for_socket; then
        systemctl show "$service_name" > "$report_root/service_socket_state.txt" 2>&1 || true
        record_fail "service_socket timeout_seconds=$wait_seconds path=$status_socket state=$report_root/service_socket_state.txt"
        return 1
    fi
    first_pid="$(service_main_pid)"
    record_pass "service_started pid=$first_pid socket=$status_socket"

    directory_mode="$(stat -c '%a' "$runtime_directory")"
    socket_mode="$(stat -c '%a' "$status_socket")"
    if [ "$directory_mode" != "750" ] || [ "$socket_mode" != "660" ]; then
        record_fail "runtime_permissions directory_mode=$directory_mode socket_mode=$socket_mode expected=750/660"
        return 1
    fi
    record_pass "runtime_permissions directory_mode=$directory_mode socket_mode=$socket_mode"

    query_index=1
    while [ "$query_index" -le 3 ]; do
        query_status "$report_root/status_query_${query_index}.json" || return 1
        query_index=$((query_index + 1))
    done
    record_pass "status_query_repeat count=3"

    no_context_path="$report_root/agent_without_context.json"
    if ! "$agent_terminal" --prompt "Report current outdoor status" \
        --request-id stage2-no-context --format json > "$no_context_path" 2>&1; then
        record_fail "agent_terminal_without_context"
        return 1
    fi
    require_json_pattern "agent_mock_backend" '"backend":[[:space:]]+"mock_no_inference"' "$no_context_path" || return 1
    require_json_pattern "agent_without_context_completed" '"state":[[:space:]]+"completed"' "$no_context_path" || return 1
    require_json_pattern "agent_no_context" '"runtime_context_available":[[:space:]]+false' "$no_context_path" || return 1
    require_json_pattern "agent_no_inference_claim" 'no AI inference was executed' "$no_context_path" || return 1

    with_context_path="$report_root/agent_with_context.json"
    if ! "$agent_terminal" --prompt "Report current outdoor status" \
        --request-id stage2-with-context --status-socket "$status_socket" \
        --format json > "$with_context_path" 2>&1; then
        record_fail "agent_terminal_with_context"
        return 1
    fi
    require_json_pattern "agent_with_context_backend" '"backend":[[:space:]]+"mock_no_inference"' "$with_context_path" || return 1
    require_json_pattern "agent_with_context_completed" '"state":[[:space:]]+"completed"' "$with_context_path" || return 1
    require_json_pattern "agent_with_context" '"runtime_context_available":[[:space:]]+true' "$with_context_path" || return 1
    require_json_pattern "agent_with_context_no_inference" 'no AI inference was executed' "$with_context_path" || return 1

    controlled_pid="$(service_main_pid)"
    if ! systemctl stop "$service_name" >/dev/null 2>&1 || ! wait_for_inactive; then
        record_fail "service_controlled_stop timeout_seconds=$wait_seconds"
        return 1
    fi
    if [ -e "$status_socket" ]; then
        record_fail "socket_not_removed_after_stop path=$status_socket"
        return 1
    fi
    record_pass "service_controlled_stop pid=$controlled_pid socket_removed=true"

    if ! systemctl start "$service_name" >/dev/null 2>&1 ||
       ! wait_for_active || ! wait_for_socket; then
        record_fail "service_restart_after_stop timeout_seconds=$wait_seconds"
        return 1
    fi
    restarted_pid="$(service_main_pid)"
    if [ "$restarted_pid" = "$controlled_pid" ]; then
        record_fail "service_pid_not_changed_after_controlled_restart pid=$restarted_pid"
        return 1
    fi
    query_status "$report_root/status_after_controlled_restart.json" || return 1
    record_pass "service_restart_after_stop old_pid=$controlled_pid new_pid=$restarted_pid"

    restart_count_before="$(service_restart_count)"
    crash_pid="$(service_main_pid)"
    if ! systemctl kill --kill-who=main --signal=SIGKILL "$service_name" >/dev/null 2>&1; then
        record_fail "service_sigkill_injection pid=$crash_pid"
        return 1
    fi
    if ! wait_for_pid_change "$crash_pid" || ! wait_for_socket; then
        record_fail "service_failure_restart timeout_seconds=$wait_seconds old_pid=$crash_pid"
        return 1
    fi
    recovered_pid="$(service_main_pid)"
    restart_count_after="$(service_restart_count)"
    query_status "$report_root/status_after_failure_restart.json" || return 1
    case "$restart_count_before:$restart_count_after" in
        *[!0-9:]*|:*|*:)
            record_pass "service_failure_restart old_pid=$crash_pid new_pid=$recovered_pid restart_counter=unavailable"
            ;;
        *)
            if [ "$restart_count_after" -le "$restart_count_before" ]; then
                record_fail "service_restart_counter before=$restart_count_before after=$restart_count_after"
                return 1
            fi
            record_pass "service_failure_restart old_pid=$crash_pid new_pid=$recovered_pid restart_count_before=$restart_count_before restart_count_after=$restart_count_after"
            ;;
    esac

    return 0
}

if run_acceptance; then
    exit 0
fi
exit 1
