#!/bin/sh

set -u

test_root="${1:-}"
duration_seconds="${2:-3600}"

if [ -z "$test_root" ] && [ -f /tmp/outdoor_agent_hour_root ]; then
    test_root="$(cat /tmp/outdoor_agent_hour_root)"
fi

case "$duration_seconds" in
    ''|*[!0-9]*|0)
        echo "duration_seconds must be a positive integer" >&2
        exit 2
        ;;
esac

if [ -z "$test_root" ] || [ ! -d "$test_root" ]; then
    echo "validation root is missing: $test_root" >&2
    exit 2
fi

current_test_root=""
if [ -f /tmp/outdoor_agent_hour_root ]; then
    current_test_root="$(cat /tmp/outdoor_agent_hour_root)"
fi

if [ "$current_test_root" = "$test_root" ] && [ -f /tmp/outdoor_agent_hour_pid ]; then
    test_pid="$(cat /tmp/outdoor_agent_hour_pid)"
    if kill -0 "$test_pid" 2>/dev/null; then
        echo "AUDIT_PENDING pid=$test_pid root=$test_root"
        exit 2
    fi
fi

if [ "$current_test_root" = "$test_root" ] && [ -f /tmp/outdoor_agent_health_monitor_pid ]; then
    health_monitor_pid="$(cat /tmp/outdoor_agent_health_monitor_pid)"
    wait_count=0
    while kill -0 "$health_monitor_pid" 2>/dev/null && [ "$wait_count" -lt 5 ]; do
        sleep 1
        wait_count=$((wait_count + 1))
    done
fi

report_path="$test_root/validation_audit.txt"
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

require_file()
{
    label="$1"
    path="$2"
    if [ -s "$path" ]; then
        record_pass "$label path=$path"
    else
        record_fail "$label missing_or_empty path=$path"
    fi
}

require_pattern()
{
    label="$1"
    pattern="$2"
    path="$3"
    if grep -Eq "$pattern" "$path" 2>/dev/null; then
        record_pass "$label"
    else
        record_fail "$label pattern_not_found=$pattern"
    fi
}

require_section_pattern()
{
    label="$1"
    section="$2"
    pattern="$3"
    path="$4"
    if awk -v section="\"$section\"" -v pattern="$pattern" '
        index($0, section) { inside = 1; next }
        inside && $0 ~ /^[[:space:]]*},?[[:space:]]*$/ { inside = 0; closed = 1 }
        inside && $0 ~ pattern { found = 1 }
        END { exit !(found && closed) }
    ' "$path"; then
        record_pass "$label"
    else
        record_fail "$label section=$section pattern_not_found=$pattern"
    fi
}

check_csv()
{
    csv_name="$1"
    expected_fields="$2"
    key_column="$3"
    minimum_rate="$4"
    maximum_rate="$5"
    csv_path="$test_root/data/history/$csv_name.csv"

    if [ ! -s "$csv_path" ]; then
        record_fail "csv_$csv_name missing_or_empty path=$csv_path"
        return
    fi

    data_rows="$(awk 'END { if (NR > 0) print NR - 1; else print 0 }' "$csv_path")"
    rate="$(awk -v rows="$data_rows" -v duration="$duration_seconds" \
        'BEGIN { printf "%.3f", rows / duration }')"

    if awk -F, -v fields="$expected_fields" -v key="$key_column" '
        NR == 1 { next }
        NF != fields { exit 1 }
        NR == 2 { previous = $key + 0; next }
        ($key + 0) <= previous { exit 1 }
        { previous = $key + 0 }
        END { if (NR < 2) exit 1 }
    ' "$csv_path"; then
        record_pass "csv_$csv_name rows=$data_rows fields=$expected_fields key_column=$key_column strictly_increasing=true"
    else
        record_fail "csv_$csv_name malformed_or_non_monotonic rows=$data_rows"
    fi

    if awk -v rate="$rate" -v minimum="$minimum_rate" -v maximum="$maximum_rate" \
        'BEGIN { exit !(rate >= minimum && rate <= maximum) }'; then
        record_pass "csv_${csv_name}_rate hz=$rate expected=$minimum_rate..$maximum_rate"
    else
        record_fail "csv_${csv_name}_rate hz=$rate expected=$minimum_rate..$maximum_rate"
    fi

    last_byte="$(tail -c 1 "$csv_path" | od -An -t u1 | tr -d '[:space:]')"
    if [ "$last_byte" = "10" ]; then
        record_pass "csv_${csv_name}_final_line newline_terminated=true"
    else
        record_fail "csv_${csv_name}_final_line last_byte=$last_byte"
    fi
}

check_health_timeline()
{
    timeline_path="$test_root/health_timeline.csv"
    completion_path="$timeline_path.complete"

    require_file "health_timeline" "$timeline_path"
    require_file "health_timeline_complete" "$completion_path"
    if [ ! -s "$timeline_path" ]; then
        return
    fi

    if health_result="$(awk -F, -v duration="$duration_seconds" '
        function bit_set(value, bit) { return int(value / bit) % 2 }
        NR == 1 { next }
        {
            ++samples
            flags = $3 + 0
            if (flags < 0) {
                ++missing
                current_unhealthy_streak = 0
                next
            }

            ++evaluated
            final_flags = flags
            if ($4 != 1 || $5 != 1 || $6 != 1 || $7 != 1) {
                ++error_samples
            }
            if ($8 != 1) {
                ++diagnostics_missing
            } else {
                if (($22 + 0) != 1) {
                    ++diagnostics_schema_mismatch
                }
                if (!diagnostics_started) {
                    diagnostics_started = 1
                    first_i2c_recovery = $9 + 0
                    first_i2c_failure = $10 + 0
                    first_fifo_overflow = $11 + 0
                    first_fifo_malformed = $12 + 0
                    first_fifo_empty = $13 + 0
                    first_fifo_stall = $14 + 0
                    first_fifo_skipped = $15 + 0
                    first_uart4_rx_drop = $23 + 0
                } else if (($9 + 0) < last_i2c_recovery ||
                           ($10 + 0) < last_i2c_failure ||
                           ($11 + 0) < last_fifo_overflow ||
                           ($12 + 0) < last_fifo_malformed ||
                           ($13 + 0) < last_fifo_empty ||
                           ($14 + 0) < last_fifo_stall ||
                           ($15 + 0) < last_fifo_skipped ||
                           ($23 + 0) < last_uart4_rx_drop) {
                    ++diagnostics_regressions
                }
                last_i2c_recovery = $9 + 0
                last_i2c_failure = $10 + 0
                last_fifo_overflow = $11 + 0
                last_fifo_malformed = $12 + 0
                last_fifo_empty = $13 + 0
                last_fifo_stall = $14 + 0
                last_fifo_skipped = $15 + 0
                final_init_error_step = $21 + 0
                final_diagnostics_schema_version = $22 + 0
                last_uart4_rx_drop = $23 + 0
            }
            if (bit_set(flags, 1024) || bit_set(flags, 2048) ||
                bit_set(flags, 4096) || bit_set(flags, 8192) ||
                bit_set(flags, 16384)) {
                ++fatal_samples
            }
            if (flags == 425) {
                ++healthy
                current_unhealthy_streak = 0
            } else {
                ++unhealthy
                ++current_unhealthy_streak
                if (current_unhealthy_streak > max_unhealthy_streak) {
                    max_unhealthy_streak = current_unhealthy_streak
                }
            }
        }
        END {
            minimum_samples = duration > 5 ? duration - 5 : 1
            healthy_permille = evaluated > 0 ? int(healthy * 1000 / evaluated) : 0
            i2c_recovery_delta = diagnostics_started ? last_i2c_recovery - first_i2c_recovery : -1
            i2c_failure_delta = diagnostics_started ? last_i2c_failure - first_i2c_failure : -1
            fifo_overflow_delta = diagnostics_started ? last_fifo_overflow - first_fifo_overflow : -1
            fifo_malformed_delta = diagnostics_started ? last_fifo_malformed - first_fifo_malformed : -1
            fifo_empty_delta = diagnostics_started ? last_fifo_empty - first_fifo_empty : -1
            fifo_stall_delta = diagnostics_started ? last_fifo_stall - first_fifo_stall : -1
            fifo_skipped_delta = diagnostics_started ? last_fifo_skipped - first_fifo_skipped : -1
            uart4_rx_drop_delta = diagnostics_started ? last_uart4_rx_drop - first_uart4_rx_drop : -1
            printf "samples=%d evaluated=%d healthy=%d unhealthy=%d healthy_permille=%d missing=%d fatal=%d error_samples=%d max_unhealthy_streak=%d final_flags=%d diagnostics_missing=%d diagnostics_schema_mismatch=%d diagnostics_regressions=%d i2c_recovery_delta=%d i2c_failure_delta=%d fifo_overflow_delta=%d fifo_malformed_delta=%d fifo_empty_delta=%d fifo_stall_delta=%d fifo_skipped_delta=%d final_init_error_step=%d diagnostics_schema_version=%d uart4_rx_drop_delta=%d final_uart4_rx_drop=%d",
                   samples,
                   evaluated,
                   healthy,
                   unhealthy,
                   healthy_permille,
                   missing,
                   fatal_samples,
                   error_samples,
                   max_unhealthy_streak,
                   final_flags,
                   diagnostics_missing,
                   diagnostics_schema_mismatch,
                   diagnostics_regressions,
                   i2c_recovery_delta,
                   i2c_failure_delta,
                   fifo_overflow_delta,
                   fifo_malformed_delta,
                   fifo_empty_delta,
                   fifo_stall_delta,
                   fifo_skipped_delta,
                   final_init_error_step,
                   final_diagnostics_schema_version,
                   uart4_rx_drop_delta,
                   last_uart4_rx_drop
            failed = samples < minimum_samples ||
                     missing > 3 ||
                     fatal_samples > 0 ||
                     error_samples > 0 ||
                     diagnostics_missing > 3 ||
                     diagnostics_schema_mismatch > 0 ||
                     diagnostics_regressions > 0 ||
                     i2c_failure_delta != 0 ||
                     fifo_overflow_delta != 0 ||
                     fifo_malformed_delta != 0 ||
                     fifo_stall_delta != 0 ||
                     final_init_error_step != 0 ||
                     final_diagnostics_schema_version != 1 ||
                     uart4_rx_drop_delta != 0 ||
                     last_uart4_rx_drop != 0 ||
                     healthy_permille < 990 ||
                     max_unhealthy_streak > 5 ||
                     final_flags != 425
            exit failed
        }
    ' "$timeline_path")"; then
        record_pass "health_timeline $health_result"
    else
        record_fail "health_timeline $health_result expected=coverage>=duration-5,missing<=3,fatal=0,error_samples=0,diagnostics_missing<=3,diagnostics_schema_mismatch=0,diagnostics_regressions=0,i2c_failure_delta=0,fifo_overflow_delta=0,fifo_malformed_delta=0,fifo_stall_delta=0,final_init_error_step=0,diagnostics_schema_version=1,uart4_rx_drop_delta=0,final_uart4_rx_drop=0,healthy_permille>=990,max_unhealthy_streak<=5,final_flags=425"
    fi
}

echo "validation_root=$test_root" >> "$report_path"
echo "duration_seconds=$duration_seconds" >> "$report_path"
echo "audit_board_date=$(date)" >> "$report_path"

sync
df -k "$(dirname "$test_root")" > "$test_root/df_after.txt"
du -sk "$test_root" > "$test_root/directory_size_kib.txt"

metadata_path="$test_root/validation_metadata.txt"
status_path="$test_root/status/runtime_status.json"
dashboard_path="$test_root/dashboard/dashboard.txt"
log_path="$test_root/logs/outdoor_core_runtime.log"

require_file "metadata" "$metadata_path"
require_file "df_before" "$test_root/df_before.txt"
require_file "df_after" "$test_root/df_after.txt"
require_file "runtime_status" "$status_path"
require_file "dashboard" "$dashboard_path"
require_file "runtime_log" "$log_path"
check_health_timeline

if [ -s "$status_path" ]; then
    require_section_pattern "runtime_stopped" "runtime" '"state":[[:space:]]+"stopped"' "$status_path"
    require_section_pattern "runtime_service_count" "runtime" '"service_count":[[:space:]]+4' "$status_path"
    require_section_pattern "storage_enabled" "storage" '"enabled":[[:space:]]+true' "$status_path"
    require_section_pattern "history_enabled" "storage" '"history_enabled":[[:space:]]+true' "$status_path"
    require_section_pattern "storage_log_rotation_failures_zero" "storage" '"log_rotation_failure_count":[[:space:]]+0' "$status_path"
    require_section_pattern "storage_log_write_failures_zero" "storage" '"log_write_failure_count":[[:space:]]+0' "$status_path"
    require_section_pattern "storage_log_last_error_empty" "storage" '"log_last_error":[[:space:]]+""' "$status_path"
    require_section_pattern "gnss_seen" "gnss" '"seen":[[:space:]]+true' "$status_path"
    require_section_pattern "mcu_heartbeat_seen" "mcu" '"heartbeat_seen":[[:space:]]+true' "$status_path"
    require_section_pattern "mcu_imu_seen" "mcu" '"imu_seen":[[:space:]]+true' "$status_path"
    require_section_pattern "mcu_magnetometer_seen" "mcu" '"magnetometer_seen":[[:space:]]+true' "$status_path"
    require_section_pattern "mcu_barometer_seen" "mcu" '"barometer_seen":[[:space:]]+true' "$status_path"
    require_section_pattern "mcu_command_ack_seen" "mcu" '"command_ack_seen":[[:space:]]+true' "$status_path"
    require_section_pattern "mcu_command_ack_status" "mcu" '"command_ack_status":[[:space:]]+0' "$status_path"
    require_section_pattern "mcu_status_flags" "mcu" '"status_flags":[[:space:]]+425' "$status_path"
    require_section_pattern "sensor_hub_diagnostics_seen" "sensor_hub_diagnostics" '"seen":[[:space:]]+true' "$status_path"
    require_section_pattern "sensor_hub_diagnostics_schema" "sensor_hub_diagnostics" '"schema_version":[[:space:]]+1' "$status_path"
    require_section_pattern "uart4_rx_drop_count_zero" "sensor_hub_diagnostics" '"uart4_rx_drop_count":[[:space:]]+0' "$status_path"
    require_section_pattern "board_imu_enabled" "board_imu" '"enabled":[[:space:]]+true' "$status_path"
    require_section_pattern "board_imu_seen" "board_imu" '"seen":[[:space:]]+true' "$status_path"

    if grep -Eq '"last_error":[[:space:]]+"[^"]+"' "$status_path"; then
        record_fail "status_last_errors non_empty=true"
    else
        record_pass "status_last_errors all_empty=true"
    fi
fi

check_csv "gnss" 17 2 1 20
check_csv "mcu_imu" 9 2 80 130
check_csv "magnetometer" 5 2 15 30
check_csv "barometer" 4 2 8 20
check_csv "board_imu" 10 2 7 15

backup_count=0
for backup_path in "$test_root"/logs/outdoor_core_runtime.log.*; do
    if [ -f "$backup_path" ]; then
        backup_count=$((backup_count + 1))
    fi
done
if [ "$backup_count" -ge 1 ] && [ "$backup_count" -le 3 ]; then
    record_pass "log_rotation backup_count=$backup_count expected=1..3"
else
    record_fail "log_rotation backup_count=$backup_count expected=1..3"
fi

oversized_logs=0
for candidate_log in "$test_root"/logs/outdoor_core_runtime.log*; do
    if [ ! -f "$candidate_log" ]; then
        continue
    fi
    candidate_size="$(wc -c < "$candidate_log" | tr -d '[:space:]')"
    if [ "$candidate_size" -gt 1048576 ]; then
        oversized_logs=$((oversized_logs + 1))
    fi
done
if [ "$oversized_logs" = "0" ]; then
    record_pass "log_rotation max_file_bytes=1048576"
else
    record_fail "log_rotation oversized_file_count=$oversized_logs"
fi

echo "failure_count=$failures" >> "$report_path"
sync

if [ "$failures" -ne 0 ]; then
    echo "AUDIT_FAILED failures=$failures report=$report_path" >&2
    exit 1
fi

echo "AUDIT_PASSED report=$report_path"
