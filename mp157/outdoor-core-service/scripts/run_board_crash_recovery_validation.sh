#!/bin/sh

set -u

capture_seconds="${1:-15}"
recovery_seconds="${2:-15}"
runtime_binary="${3:-/opt/outdoor-agent/bin/outdoor_core_runtime}"
runtime_config="${4:-/opt/outdoor-agent/config/runtime.conf}"
storage_mount="${5:-/run/media/mmcblk1p1}"
script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
health_preflight_script="$script_dir/run_board_health_preflight.sh"

runtime_process_active()
{
    for proc_dir in /proc/[0-9]*; do
        if [ ! -r "$proc_dir/cmdline" ]; then
            continue
        fi
        executable="$(tr '\000' '\n' < "$proc_dir/cmdline" 2>/dev/null | sed -n '1p')"
        case "${executable##*/}" in
            outdoor_core_runtime*) return 0 ;;
        esac
    done
    return 1
}

case "$capture_seconds:$recovery_seconds" in
    *[!0-9:]*|0:*|*:0|:*|*:)
        echo "capture and recovery seconds must be positive integers" >&2
        exit 2
        ;;
esac

if runtime_process_active; then
    echo "another outdoor_core_runtime process is active" >&2
    exit 2
fi

for required_path in "$runtime_binary" "$runtime_config" "$storage_mount" \
    /dev/ttySTM1 /dev/ttySTM2 /dev/icm20608 /dev/fb0; do
    if [ ! -e "$required_path" ]; then
        echo "required path is missing: $required_path" >&2
        exit 2
    fi
done

if [ ! -x "$health_preflight_script" ]; then
    echo "health preflight script is missing or not executable: $health_preflight_script" >&2
    exit 2
fi

if ! "$health_preflight_script" 8 "$runtime_binary" "$runtime_config" "$storage_mount"; then
    echo "crash recovery validation was not started because board health preflight failed" >&2
    exit 3
fi

health_preflight_root="$(cat /tmp/outdoor_agent_health_preflight_root)"

test_root="$(mktemp -d "$storage_mount/outdoor-agent-crash-recovery-XXXXXX")"
report_path="$test_root/crash_recovery_report.txt"
failures=0

: > "$report_path"

record_pass()
{
    echo "PASS $*" | tee -a "$report_path"
}

record_fail()
{
    failures=$((failures + 1))
    echo "FAIL $*" | tee -a "$report_path" >&2
}

start_runtime()
{
    run_seconds="$1"
    nohup "$runtime_binary" \
        --config "$runtime_config" \
        --gnss-input-mode serial \
        --gnss-serial-device /dev/ttySTM2 \
        --gnss-serial-baud 38400 \
        --gnss-serial-capture-seconds 0 \
        --mcu-input-mode serial \
        --mcu-serial-device /dev/ttySTM1 \
        --mcu-serial-baud 115200 \
        --mcu-serial-capture-seconds 0 \
        --mcu-command ping \
        --runtime-run-seconds "$run_seconds" \
        --storage \
        --storage-root "$test_root" \
        --history \
        --board-imu \
        --board-imu-source char_device \
        --board-imu-device-path /dev/icm20608 \
        --board-imu-samples 0 \
        --dashboard-output-mode both \
        --dashboard-framebuffer-device /dev/fb0 \
        --dashboard-refresh-count 0 \
        --dashboard-refresh-interval-ms 1000 \
        --log-level info \
        >/dev/null 2>&1 &
    started_pid="$!"
}

csv_rows()
{
    awk 'END { if (NR > 0) print NR - 1; else print 0 }' "$1"
}

validate_csv_integrity()
{
    csv_name="$1"
    expected_fields="$2"
    csv_path="$test_root/data/history/$csv_name.csv"

    if [ ! -s "$csv_path" ]; then
        record_fail "csv_$csv_name missing_or_empty"
        return
    fi

    if awk -F, -v fields="$expected_fields" 'NF != fields { exit 1 }' "$csv_path"; then
        record_pass "csv_$csv_name field_count=$expected_fields"
    else
        record_fail "csv_$csv_name malformed_row=true"
    fi

    header_count="$(grep -c '^host_time_utc,' "$csv_path")"
    if [ "$header_count" = "1" ]; then
        record_pass "csv_$csv_name header_count=1"
    else
        record_fail "csv_$csv_name header_count=$header_count"
    fi

    last_byte="$(tail -c 1 "$csv_path" | od -An -t u1 | tr -d '[:space:]')"
    if [ "$last_byte" = "10" ]; then
        record_pass "csv_$csv_name newline_terminated=true"
    else
        record_fail "csv_$csv_name last_byte=$last_byte"
    fi
}

{
    echo "capture_seconds=$capture_seconds"
    echo "recovery_seconds=$recovery_seconds"
    echo "runtime_binary=$runtime_binary"
    echo "runtime_config=$runtime_config"
    echo "health_preflight_root=$health_preflight_root"
    echo "board_date=$(date)"
    sha256sum "$runtime_binary"
} > "$test_root/crash_recovery_metadata.txt"

df -k "$storage_mount" > "$test_root/df_before.txt"

start_runtime 0
crash_pid="$started_pid"
echo "crash_pid=$crash_pid" >> "$test_root/crash_recovery_metadata.txt"
sleep 3
if ! kill -0 "$crash_pid" 2>/dev/null; then
    echo "crash-phase Runtime failed to start root=$test_root" >&2
    exit 3
fi

sleep "$capture_seconds"

for csv_path in "$test_root"/data/history/*.csv; do
    csv_name="$(basename "$csv_path" .csv)"
    echo "$csv_name=$(csv_rows "$csv_path")" >> "$test_root/rows_before_kill.txt"
done

kill -9 "$crash_pid"
wait "$crash_pid" 2>/dev/null
sync
sleep 1

if kill -0 "$crash_pid" 2>/dev/null; then
    record_fail "forced_exit pid_still_alive=$crash_pid"
else
    record_pass "forced_exit signal=SIGKILL pid=$crash_pid"
fi

status_path="$test_root/status/runtime_status.json"
if [ -s "$status_path" ]; then
    cp "$status_path" "$test_root/status_after_sigkill.json"
    if grep -Eq '"state":[[:space:]]+"running"' "$status_path"; then
        record_pass "sigkill_status expected_stale_state=running"
    else
        record_fail "sigkill_status expected_stale_state_missing"
    fi
else
    record_fail "sigkill_status missing_or_empty"
fi

validate_csv_integrity gnss 17
validate_csv_integrity mcu_imu 9
validate_csv_integrity magnetometer 5
validate_csv_integrity barometer 4
validate_csv_integrity board_imu 10

for csv_path in "$test_root"/data/history/*.csv; do
    csv_name="$(basename "$csv_path" .csv)"
    echo "$csv_name=$(csv_rows "$csv_path")" >> "$test_root/rows_after_kill.txt"
done

start_runtime "$recovery_seconds"
recovery_pid="$started_pid"
echo "recovery_pid=$recovery_pid" >> "$test_root/crash_recovery_metadata.txt"
wait "$recovery_pid"
recovery_rc="$?"
sync

if [ "$recovery_rc" = "0" ]; then
    record_pass "recovery_runtime controlled_exit_rc=0 pid=$recovery_pid"
else
    record_fail "recovery_runtime exit_rc=$recovery_rc pid=$recovery_pid"
fi

if [ -s "$status_path" ]; then
    cp "$status_path" "$test_root/status_after_recovery.json"
    if grep -Eq '"state":[[:space:]]+"stopped"' "$status_path"; then
        record_pass "recovery_status state=stopped"
    else
        record_fail "recovery_status state_not_stopped"
    fi
    if grep -Eq '"last_error":[[:space:]]+"[^"]+"' "$status_path"; then
        record_fail "recovery_status non_empty_last_error=true"
    else
        record_pass "recovery_status all_last_errors_empty=true"
    fi
    if grep -Eq '"command_ack_status":[[:space:]]+0' "$status_path"; then
        record_pass "recovery_status command_ack_status=0"
    else
        record_fail "recovery_status command_ack_status_not_zero"
    fi
    if grep -Eq '"command_ack_seen":[[:space:]]+true' "$status_path"; then
        record_pass "recovery_status command_ack_seen=true"
    else
        record_fail "recovery_status command_ack_seen=false"
    fi
    if grep -Eq '"status_flags":[[:space:]]+425' "$status_path"; then
        record_pass "recovery_status status_flags=425"
    else
        record_fail "recovery_status status_flags_not_425"
    fi
else
    record_fail "recovery_status missing_or_empty"
fi

validate_csv_integrity gnss 17
validate_csv_integrity mcu_imu 9
validate_csv_integrity magnetometer 5
validate_csv_integrity barometer 4
validate_csv_integrity board_imu 10

for csv_path in "$test_root"/data/history/*.csv; do
    csv_name="$(basename "$csv_path" .csv)"
    final_rows="$(csv_rows "$csv_path")"
    after_kill_rows="$(sed -n "s/^$csv_name=//p" "$test_root/rows_after_kill.txt")"
    echo "$csv_name=$final_rows" >> "$test_root/rows_after_recovery.txt"
    if [ "$final_rows" -gt "$after_kill_rows" ]; then
        record_pass "csv_$csv_name append_recovery rows_after_kill=$after_kill_rows final_rows=$final_rows"
    else
        record_fail "csv_$csv_name append_recovery rows_after_kill=$after_kill_rows final_rows=$final_rows"
    fi
done

df -k "$storage_mount" > "$test_root/df_after.txt"
du -sk "$test_root" > "$test_root/directory_size_kib.txt"
echo "failure_count=$failures" >> "$report_path"
sync

if [ "$failures" -ne 0 ]; then
    echo "CRASH_RECOVERY_FAILED failures=$failures root=$test_root report=$report_path" >&2
    exit 1
fi

echo "CRASH_RECOVERY_PASSED root=$test_root report=$report_path"
