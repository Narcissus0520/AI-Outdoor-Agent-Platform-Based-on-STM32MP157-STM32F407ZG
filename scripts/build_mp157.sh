#!/usr/bin/env sh
set -eu

case "$0" in
    */*) SCRIPT_DIR=${0%/*} ;;
    *) SCRIPT_DIR=. ;;
esac
SCRIPT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
SERVICE_DIR="$REPO_ROOT/mp157/outdoor-core-service"

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake not found in PATH" >&2
    exit 1
fi

cmake -S "$SERVICE_DIR" -B "$SERVICE_DIR/build"
cmake --build "$SERVICE_DIR/build"
