#!/bin/sh

set -u

runtime_pid="${1:-}"
status_path="${2:-}"
output_path="${3:-}"
interval_seconds="${4:-1}"

case "$runtime_pid" in
    ''|*[!0-9]*)
        echo "runtime_pid must be a positive integer" >&2
        exit 2
        ;;
esac

case "$interval_seconds" in
    ''|*[!0-9]*|0)
        echo "interval_seconds must be a positive integer" >&2
        exit 2
        ;;
esac

if [ -z "$status_path" ] || [ -z "$output_path" ]; then
    echo "status_path and output_path are required" >&2
    exit 2
fi

sample_status()
{
    sample_epoch="$(date +%s)"
    if [ ! -s "$status_path" ]; then
        echo "$sample_epoch,missing,-1,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1" >> "$output_path"
        return
    fi

    awk -v epoch="$sample_epoch" '
        BEGIN {
            section = ""
            state = "unknown"
            flags = -1
            runtime_error_empty = 0
            storage_error_empty = 0
            mcu_error_empty = 0
            board_imu_error_empty = 0
            diagnostics_seen = 0
            i2c_recovery_count = -1
            i2c_transaction_failure_count = -1
            fifo_overflow_count = -1
            fifo_malformed_packet_count = -1
            fifo_empty_event_count = -1
            fifo_drain_stall_count = -1
            fifo_skipped_packet_count = -1
            i2c_last_length = -1
            i2c_last_device_address = -1
            i2c_last_register_address = -1
            i2c_last_operation = -1
            i2c_last_hal_status = -1
            icm42688_init_error_step = -1
            diagnostics_schema_version = -1
            uart4_rx_drop_count = -1
        }
        function json_number(line) {
            sub(/^.*:[[:space:]]*/, "", line)
            sub(/[^0-9].*$/, "", line)
            return line + 0
        }
        /^[[:space:]]*"runtime":[[:space:]]*\{/ { section = "runtime"; next }
        /^[[:space:]]*"storage":[[:space:]]*\{/ { section = "storage"; next }
        /^[[:space:]]*"mcu":[[:space:]]*\{/ { section = "mcu"; next }
        /^[[:space:]]*"sensor_hub_diagnostics":[[:space:]]*\{/ { section = "diagnostics"; next }
        /^[[:space:]]*"board_imu":[[:space:]]*\{/ { section = "board_imu"; next }
        section != "" && /^[[:space:]]*},?[[:space:]]*$/ { section = ""; next }
        section == "runtime" && /"state"[[:space:]]*:/ {
            line = $0
            sub(/^.*"state"[[:space:]]*:[[:space:]]*"/, "", line)
            sub(/".*$/, "", line)
            state = line
        }
        section == "mcu" && /"status_flags"[[:space:]]*:/ {
            line = $0
            sub(/^.*"status_flags"[[:space:]]*:[[:space:]]*/, "", line)
            sub(/[^0-9].*$/, "", line)
            flags = line + 0
        }
        /"last_error"[[:space:]]*:[[:space:]]*""/ {
            if (section == "runtime") runtime_error_empty = 1
            if (section == "storage") storage_error_empty = 1
            if (section == "mcu") mcu_error_empty = 1
            if (section == "board_imu") board_imu_error_empty = 1
        }
        section == "diagnostics" && /"seen"[[:space:]]*:[[:space:]]*true/ { diagnostics_seen = 1 }
        section == "diagnostics" && /"i2c_recovery_count"[[:space:]]*:/ { i2c_recovery_count = json_number($0) }
        section == "diagnostics" && /"i2c_transaction_failure_count"[[:space:]]*:/ { i2c_transaction_failure_count = json_number($0) }
        section == "diagnostics" && /"fifo_overflow_count"[[:space:]]*:/ { fifo_overflow_count = json_number($0) }
        section == "diagnostics" && /"fifo_malformed_packet_count"[[:space:]]*:/ { fifo_malformed_packet_count = json_number($0) }
        section == "diagnostics" && /"fifo_empty_event_count"[[:space:]]*:/ { fifo_empty_event_count = json_number($0) }
        section == "diagnostics" && /"fifo_drain_stall_count"[[:space:]]*:/ { fifo_drain_stall_count = json_number($0) }
        section == "diagnostics" && /"fifo_skipped_packet_count"[[:space:]]*:/ { fifo_skipped_packet_count = json_number($0) }
        section == "diagnostics" && /"i2c_last_length"[[:space:]]*:/ { i2c_last_length = json_number($0) }
        section == "diagnostics" && /"i2c_last_device_address"[[:space:]]*:/ { i2c_last_device_address = json_number($0) }
        section == "diagnostics" && /"i2c_last_register_address"[[:space:]]*:/ { i2c_last_register_address = json_number($0) }
        section == "diagnostics" && /"i2c_last_operation"[[:space:]]*:/ { i2c_last_operation = json_number($0) }
        section == "diagnostics" && /"i2c_last_hal_status"[[:space:]]*:/ { i2c_last_hal_status = json_number($0) }
        section == "diagnostics" && /"icm42688_init_error_step"[[:space:]]*:/ { icm42688_init_error_step = json_number($0) }
        section == "diagnostics" && /"schema_version"[[:space:]]*:/ { diagnostics_schema_version = json_number($0) }
        section == "diagnostics" && /"uart4_rx_drop_count"[[:space:]]*:/ { uart4_rx_drop_count = json_number($0) }
        END {
            printf "%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                   epoch,
                   state,
                   flags,
                   runtime_error_empty,
                   storage_error_empty,
                   mcu_error_empty,
                   board_imu_error_empty,
                   diagnostics_seen,
                   i2c_recovery_count,
                   i2c_transaction_failure_count,
                   fifo_overflow_count,
                   fifo_malformed_packet_count,
                   fifo_empty_event_count,
                   fifo_drain_stall_count,
                   fifo_skipped_packet_count,
                   i2c_last_length,
                   i2c_last_device_address,
                   i2c_last_register_address,
                   i2c_last_operation,
                   i2c_last_hal_status,
                   icm42688_init_error_step,
                   diagnostics_schema_version,
                   uart4_rx_drop_count
        }
    ' "$status_path" >> "$output_path"
}

echo "host_epoch,runtime_state,status_flags,runtime_error_empty,storage_error_empty,mcu_error_empty,board_imu_error_empty,diagnostics_seen,i2c_recovery_count,i2c_transaction_failure_count,fifo_overflow_count,fifo_malformed_packet_count,fifo_empty_event_count,fifo_drain_stall_count,fifo_skipped_packet_count,i2c_last_length,i2c_last_device_address,i2c_last_register_address,i2c_last_operation,i2c_last_hal_status,icm42688_init_error_step,diagnostics_schema_version,uart4_rx_drop_count" > "$output_path"

while kill -0 "$runtime_pid" 2>/dev/null; do
    sample_status
    sleep "$interval_seconds"
done

sample_status
{
    echo "completed_epoch=$(date +%s)"
    echo "runtime_pid=$runtime_pid"
    echo "status_path=$status_path"
    echo "interval_seconds=$interval_seconds"
} > "$output_path.complete"
sync
