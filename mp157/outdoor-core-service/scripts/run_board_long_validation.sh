#!/bin/sh

set -eu

duration_seconds="${1:-3600}"
test_label="${2:-hour}"
runtime_binary="${3:-/opt/outdoor-agent/bin/outdoor_core_runtime}"
runtime_config="${4:-/opt/outdoor-agent/config/runtime.conf}"
storage_mount="${5:-/run/media/mmcblk1p1}"
script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
health_preflight_script="$script_dir/run_board_health_preflight.sh"
health_monitor_script="$script_dir/monitor_board_runtime_health.sh"

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

case "$duration_seconds" in
    ''|*[!0-9]*)
        echo "duration_seconds must be a positive integer" >&2
        exit 2
        ;;
    0)
        echo "duration_seconds must be greater than zero" >&2
        exit 2
        ;;
esac

if [ ! -x "$runtime_binary" ]; then
    echo "runtime binary is missing or not executable: $runtime_binary" >&2
    exit 3
fi

if [ ! -f "$runtime_config" ]; then
    echo "runtime config is missing: $runtime_config" >&2
    exit 3
fi

if [ ! -d "$storage_mount" ]; then
    echo "storage mount is missing: $storage_mount" >&2
    exit 3
fi

if runtime_process_active; then
    echo "another outdoor_core_runtime process is active" >&2
    exit 3
fi

for device in /dev/ttySTM1 /dev/ttySTM2 /dev/icm20608 /dev/fb0; do
    if [ ! -e "$device" ]; then
        echo "required device is missing: $device" >&2
        exit 3
    fi
done

if [ ! -x "$health_preflight_script" ]; then
    echo "health preflight script is missing or not executable: $health_preflight_script" >&2
    exit 3
fi

if [ ! -x "$health_monitor_script" ]; then
    echo "health monitor script is missing or not executable: $health_monitor_script" >&2
    exit 3
fi

if ! "$health_preflight_script" 8 "$runtime_binary" "$runtime_config" "$storage_mount"; then
    echo "long validation was not started because board health preflight failed" >&2
    exit 4
fi

health_preflight_root="$(cat /tmp/outdoor_agent_health_preflight_root)"

test_root="$(mktemp -d "$storage_mount/outdoor-agent-${test_label}-XXXXXX")"
metadata_path="$test_root/validation_metadata.txt"

{
    echo "test_label=$test_label"
    echo "duration_seconds=$duration_seconds"
    echo "runtime_binary=$runtime_binary"
    echo "runtime_config=$runtime_config"
    echo "storage_mount=$storage_mount"
    echo "health_preflight_root=$health_preflight_root"
    echo "board_date=$(date)"
    echo "kernel=$(uname -a)"
    sha256sum "$runtime_binary"
} > "$metadata_path"

df -k "$storage_mount" > "$test_root/df_before.txt"

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
    --runtime-run-seconds "$duration_seconds" \
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

test_pid="$!"
health_timeline_path="$test_root/health_timeline.csv"
nohup "$health_monitor_script" \
    "$test_pid" \
    "$test_root/status/runtime_status.json" \
    "$health_timeline_path" \
    1 \
    >"$test_root/health_monitor_console.log" 2>&1 &
health_monitor_pid="$!"
printf '%s\n' "$test_root" > /tmp/outdoor_agent_hour_root
printf '%s\n' "$test_pid" > /tmp/outdoor_agent_hour_pid
printf '%s\n' "$health_monitor_pid" > /tmp/outdoor_agent_health_monitor_pid
printf 'pid=%s\n' "$test_pid" >> "$metadata_path"
printf 'health_monitor_pid=%s\n' "$health_monitor_pid" >> "$metadata_path"
printf 'health_timeline_path=%s\n' "$health_timeline_path" >> "$metadata_path"
sync

sleep 3
if ! kill -0 "$test_pid" 2>/dev/null; then
    echo "LONG_TEST_START_FAILED pid=$test_pid root=$test_root" >&2
    if [ -f "$test_root/logs/outdoor_core_runtime.log" ]; then
        tail -n 20 "$test_root/logs/outdoor_core_runtime.log" >&2
    fi
    kill "$health_monitor_pid" 2>/dev/null || true
    exit 4
fi

echo "LONG_TEST_STARTED pid=$test_pid root=$test_root duration_seconds=$duration_seconds"
