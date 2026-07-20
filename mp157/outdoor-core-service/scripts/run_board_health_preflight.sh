#!/bin/sh

set -u

duration_seconds="${1:-8}"
runtime_binary="${2:-/opt/outdoor-agent/bin/outdoor_core_runtime}"
runtime_config="${3:-/opt/outdoor-agent/config/runtime.conf}"
storage_mount="${4:-/run/media/mmcblk1p1}"
validation_profile="${5:-full}"

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

section_has_pattern()
{
    section="$1"
    pattern="$2"
    path="$3"
    awk -v section="\"$section\"" -v pattern="$pattern" '
        index($0, section) { inside = 1; next }
        inside && $0 ~ /^[[:space:]]*},?[[:space:]]*$/ { inside = 0; closed = 1 }
        inside && $0 ~ pattern { found = 1 }
        END { exit !(found && closed) }
    ' "$path"
}

case "$duration_seconds" in
    ''|*[!0-9]*|0)
        echo "duration_seconds must be a positive integer" >&2
        exit 2
        ;;
esac

case "$validation_profile" in
    full|icm42688_only) ;;
    *)
        echo "validation_profile must be full or icm42688_only" >&2
        exit 2
        ;;
esac

if [ ! -x "$runtime_binary" ]; then
    echo "runtime binary is missing or not executable: $runtime_binary" >&2
    exit 2
fi

if [ ! -f "$runtime_config" ]; then
    echo "runtime config is missing: $runtime_config" >&2
    exit 2
fi

if [ ! -d "$storage_mount" ]; then
    echo "storage mount is missing: $storage_mount" >&2
    exit 2
fi

if runtime_process_active; then
    echo "another outdoor_core_runtime process is active" >&2
    exit 2
fi

for device in /dev/ttySTM1 /dev/ttySTM2 /dev/icm20608; do
    if [ ! -e "$device" ]; then
        echo "required device is missing: $device" >&2
        exit 2
    fi
done

test_root="$(mktemp -d "$storage_mount/outdoor-agent-health-preflight-XXXXXX")"
report_path="$test_root/health_preflight_report.txt"
status_path="$test_root/status/runtime_status.json"
failures=0

printf '%s\n' "$test_root" > /tmp/outdoor_agent_health_preflight_root

{
    echo "duration_seconds=$duration_seconds"
    echo "runtime_binary=$runtime_binary"
    echo "runtime_config=$runtime_config"
    echo "storage_mount=$storage_mount"
    echo "validation_profile=$validation_profile"
    echo "board_date=$(date)"
    echo "kernel=$(uname -a)"
    sha256sum "$runtime_binary"
} > "$test_root/health_preflight_metadata.txt"

record_pass()
{
    echo "PASS $*" | tee -a "$report_path"
}

record_fail()
{
    failures=$((failures + 1))
    echo "FAIL $*" | tee -a "$report_path" >&2
}

require_section_pattern()
{
    label="$1"
    section="$2"
    pattern="$3"
    if section_has_pattern "$section" "$pattern" "$status_path"; then
        record_pass "$label"
    else
        record_fail "$label section=$section pattern_not_found=$pattern"
    fi
}

: > "$report_path"

if "$runtime_binary" \
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
    --runtime-run-seconds "$duration_seconds" \
    --storage \
    --storage-root "$test_root" \
    --history \
    --board-imu \
    --board-imu-source char_device \
    --board-imu-device-path /dev/icm20608 \
    --board-imu-samples 0 \
    --dashboard-output-mode text \
    --dashboard-refresh-count 0 \
    --dashboard-refresh-interval-ms 1000 \
    --log-level info \
    >"$test_root/runtime_console.log" 2>&1; then
    record_pass "runtime controlled_exit_rc=0"
else
    runtime_rc="$?"
    record_fail "runtime exit_rc=$runtime_rc"
fi

sync

if [ -s "$status_path" ]; then
    record_pass "runtime_status path=$status_path"
    require_section_pattern "runtime_stopped" "runtime" '"state":[[:space:]]+"stopped"'
    require_section_pattern "runtime_last_error_empty" "runtime" '"last_error":[[:space:]]+""'
    require_section_pattern "storage_enabled" "storage" '"enabled":[[:space:]]+true'
    require_section_pattern "storage_log_rotation_failures_zero" "storage" '"log_rotation_failure_count":[[:space:]]+0'
    require_section_pattern "storage_log_write_failures_zero" "storage" '"log_write_failure_count":[[:space:]]+0'
    require_section_pattern "storage_log_last_error_empty" "storage" '"log_last_error":[[:space:]]+""'
    require_section_pattern "storage_last_error_empty" "storage" '"last_error":[[:space:]]+""'
    require_section_pattern "gnss_seen" "gnss" '"seen":[[:space:]]+true'
    require_section_pattern "mcu_heartbeat_seen" "mcu" '"heartbeat_seen":[[:space:]]+true'
    require_section_pattern "mcu_imu_seen" "mcu" '"imu_seen":[[:space:]]+true'
    require_section_pattern "mcu_command_ack_seen" "mcu" '"command_ack_seen":[[:space:]]+true'
    require_section_pattern "mcu_command_ack_status" "mcu" '"command_ack_status":[[:space:]]+0'
    require_section_pattern "mcu_last_error_empty" "mcu" '"last_error":[[:space:]]+""'
    require_section_pattern "sensor_hub_diagnostics_seen" "sensor_hub_diagnostics" '"seen":[[:space:]]+true'
    require_section_pattern "sensor_hub_diagnostics_schema" "sensor_hub_diagnostics" '"schema_version":[[:space:]]+1'
    require_section_pattern "uart4_rx_drop_count_zero" "sensor_hub_diagnostics" '"uart4_rx_drop_count":[[:space:]]+0'
    if [ "$validation_profile" = "full" ]; then
        require_section_pattern "mcu_magnetometer_seen" "mcu" '"magnetometer_seen":[[:space:]]+true'
        require_section_pattern "mcu_barometer_seen" "mcu" '"barometer_seen":[[:space:]]+true'
        require_section_pattern "mcu_real_sensor_status" "mcu" '"status_flags":[[:space:]]+425'
    else
        require_section_pattern "mcu_magnetometer_absent" "mcu" '"magnetometer_seen":[[:space:]]+false'
        require_section_pattern "mcu_barometer_absent" "mcu" '"barometer_seen":[[:space:]]+false'
        require_section_pattern "mcu_icm42688_only_status" "mcu" '"status_flags":[[:space:]]+33153'
        require_section_pattern "i2c_recovery_count_zero" "sensor_hub_diagnostics" '"i2c_recovery_count":[[:space:]]+0'
        require_section_pattern "i2c_transaction_failure_count_zero" "sensor_hub_diagnostics" '"i2c_transaction_failure_count":[[:space:]]+0'
        require_section_pattern "i2c_last_hal_error_zero" "sensor_hub_diagnostics" '"i2c_last_hal_error":[[:space:]]+0'
        require_section_pattern "fifo_overflow_count_zero" "sensor_hub_diagnostics" '"fifo_overflow_count":[[:space:]]+0'
        require_section_pattern "fifo_malformed_packet_count_zero" "sensor_hub_diagnostics" '"fifo_malformed_packet_count":[[:space:]]+0'
        require_section_pattern "fifo_empty_event_count_zero" "sensor_hub_diagnostics" '"fifo_empty_event_count":[[:space:]]+0'
        require_section_pattern "fifo_drain_stall_count_zero" "sensor_hub_diagnostics" '"fifo_drain_stall_count":[[:space:]]+0'
        require_section_pattern "fifo_skipped_packet_count_zero" "sensor_hub_diagnostics" '"fifo_skipped_packet_count":[[:space:]]+0'
        require_section_pattern "icm42688_init_error_step_zero" "sensor_hub_diagnostics" '"icm42688_init_error_step":[[:space:]]+0'
    fi
    require_section_pattern "board_imu_enabled" "board_imu" '"enabled":[[:space:]]+true'
    require_section_pattern "board_imu_seen" "board_imu" '"seen":[[:space:]]+true'
    require_section_pattern "board_imu_last_error_empty" "board_imu" '"last_error":[[:space:]]+""'
else
    record_fail "runtime_status missing_or_empty path=$status_path"
fi

echo "failure_count=$failures" >> "$report_path"
sync

if [ "$failures" -ne 0 ]; then
    echo "BOARD_HEALTH_PREFLIGHT_FAILED failures=$failures root=$test_root report=$report_path" >&2
    exit 1
fi

echo "BOARD_HEALTH_PREFLIGHT_PASSED root=$test_root report=$report_path"
