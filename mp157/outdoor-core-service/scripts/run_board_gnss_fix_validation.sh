#!/bin/sh

set -u

timeout_seconds="${1:-600}"
runtime_binary="${2:-/opt/outdoor-agent/bin/outdoor_core_runtime}"
runtime_config="${3:-/opt/outdoor-agent/config/runtime.conf}"
mock_frames="${4:-/opt/outdoor-agent/data/mcu_mock_frames_valid.txt}"
storage_mount="${5:-/run/media/mmcblk1p1}"

case "$timeout_seconds" in
    ''|*[!0-9]*|0)
        echo "timeout_seconds must be a positive integer" >&2
        exit 2
        ;;
esac

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

if runtime_process_active; then
    echo "another outdoor_core_runtime process is active" >&2
    exit 2
fi

for required_path in "$runtime_binary" "$runtime_config" "$mock_frames" \
    "$storage_mount" /dev/ttySTM2; do
    if [ ! -e "$required_path" ]; then
        echo "required path is missing: $required_path" >&2
        exit 2
    fi
done

test_root="$(mktemp -d "$storage_mount/outdoor-agent-gnss-fix-XXXXXX")"
report_path="$test_root/gnss_fix_report.txt"
status_path="$test_root/status/runtime_status.json"
history_path="$test_root/data/history/gnss.csv"

{
    echo "timeout_seconds=$timeout_seconds"
    echo "runtime_binary=$runtime_binary"
    echo "runtime_config=$runtime_config"
    echo "mock_frames=$mock_frames"
    echo "board_date=$(date)"
    sha256sum "$runtime_binary"
} > "$test_root/gnss_fix_metadata.txt"
df -k "$storage_mount" > "$test_root/df_before.txt"

nohup "$runtime_binary" \
    --config "$runtime_config" \
    --gnss-input-mode serial \
    --gnss-serial-device /dev/ttySTM2 \
    --gnss-serial-baud 38400 \
    --gnss-serial-capture-seconds 0 \
    --mcu-input-mode mock_file \
    --mcu-mock-input "$mock_frames" \
    --mcu-command none \
    --runtime-run-seconds "$timeout_seconds" \
    --storage \
    --storage-root "$test_root" \
    --history \
    --no-board-imu \
    --no-dashboard \
    --log-level info \
    >/dev/null 2>&1 &

runtime_pid="$!"
echo "runtime_pid=$runtime_pid" >> "$test_root/gnss_fix_metadata.txt"

sleep 2
if ! kill -0 "$runtime_pid" 2>/dev/null; then
    echo "GNSS_FIX_START_FAILED pid=$runtime_pid root=$test_root" >&2
    exit 3
fi

elapsed_seconds=0
fix_detected=false
while kill -0 "$runtime_pid" 2>/dev/null && [ "$elapsed_seconds" -lt "$timeout_seconds" ]; do
    if [ -s "$status_path" ] && grep -Eq '"fix_valid":[[:space:]]+true' "$status_path"; then
        fix_detected=true
        break
    fi
    sleep 1
    elapsed_seconds=$((elapsed_seconds + 1))
done

if [ "$fix_detected" = "true" ]; then
    kill -TERM "$runtime_pid" 2>/dev/null || true
fi

wait "$runtime_pid"
runtime_rc="$?"
sync

{
    echo "fix_detected=$fix_detected"
    echo "elapsed_seconds=$elapsed_seconds"
    echo "runtime_exit_code=$runtime_rc"
} > "$report_path"

failures=0
record_pass()
{
    echo "PASS $*" | tee -a "$report_path"
}
record_fail()
{
    failures=$((failures + 1))
    echo "FAIL $*" | tee -a "$report_path" >&2
}

section_has_nonempty_error()
{
    section_name="$1"
    json_path="$2"
    awk -v section="\"$section_name\"" '
        index($0, section) { inside = 1; next }
        inside && $0 ~ /^[[:space:]]*},?[[:space:]]*$/ { inside = 0; closed = 1 }
        inside && $0 ~ /"last_error":[[:space:]]+"[^"]+"/ { found = 1 }
        END { exit !(found && closed) }
    ' "$json_path"
}

if [ "$runtime_rc" = "0" ]; then
    record_pass "runtime controlled_exit=true"
else
    record_fail "runtime exit_code=$runtime_rc"
fi

if [ -s "$status_path" ]; then
    cp "$status_path" "$test_root/runtime_status_final.json"
else
    record_fail "runtime_status missing_or_empty"
fi

if [ "$fix_detected" = "true" ]; then
    record_pass "gnss_fix detected_within_seconds=$elapsed_seconds"
else
    record_fail "gnss_fix timeout_seconds=$timeout_seconds"
fi

if [ -s "$status_path" ] && grep -Eq '"fix_valid":[[:space:]]+true' "$status_path"; then
    record_pass "status fix_valid=true"
else
    record_fail "status fix_valid_not_true"
fi

for core_section in runtime storage mcu; do
    if [ -s "$status_path" ] && section_has_nonempty_error "$core_section" "$status_path"; then
        record_fail "status section=$core_section non_empty_last_error=true"
    else
        record_pass "status section=$core_section last_error_empty=true"
    fi
done

if [ -s "$history_path" ]; then
    data_rows="$(awk 'END { if (NR > 0) print NR - 1; else print 0 }' "$history_path")"
    record_pass "gnss_history rows=$data_rows"

    if awk -F, 'NR > 1 && $3 == "RMC" && $4 == 1 && $6 != 0 && $7 != 0 { found = 1 } END { exit !found }' "$history_path"; then
        record_pass "gnss_history valid_rmc_with_coordinates=true"
    else
        record_fail "gnss_history valid_rmc_with_coordinates=false"
    fi

    if awk -F, 'NR > 1 && $3 == "GGA" && $4 == 1 && $11 > 0 && $13 > 0 { found = 1 } END { exit !found }' "$history_path"; then
        record_pass "gnss_history valid_gga_satellites_and_quality=true"
    else
        record_fail "gnss_history valid_gga_satellites_and_quality=false"
    fi

    last_byte="$(tail -c 1 "$history_path" | od -An -t u1 | tr -d '[:space:]')"
    if [ "$last_byte" = "10" ]; then
        record_pass "gnss_history newline_terminated=true"
    else
        record_fail "gnss_history last_byte=$last_byte"
    fi
else
    record_fail "gnss_history missing_or_empty"
fi

df -k "$storage_mount" > "$test_root/df_after.txt"
du -sk "$test_root" > "$test_root/directory_size_kib.txt"
echo "failure_count=$failures" >> "$report_path"
sync

if [ "$failures" -ne 0 ]; then
    echo "GNSS_FIX_FAILED failures=$failures root=$test_root report=$report_path" >&2
    exit 1
fi

echo "GNSS_FIX_PASSED root=$test_root report=$report_path"
