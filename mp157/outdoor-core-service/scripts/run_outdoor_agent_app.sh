#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SERVICE_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SERVICE_ROOT"

RUNTIME_BIN="${OUTDOOR_RUNTIME_BIN:-./outdoor_core_runtime}"
APP_CONFIG="${OUTDOOR_AGENT_APP_CONFIG:-config/outdoor_agent_app.conf}"

exec "$RUNTIME_BIN" --config "$APP_CONFIG" "$@"
