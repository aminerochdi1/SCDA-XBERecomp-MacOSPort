#!/bin/zsh
set -e

# Tom Clancy's Splinter Cell: Double Agent (Xbox Static Recompilation for Apple Silicon macOS)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-macos"

MODE="${1:-sp}"

if [[ "$MODE" == "versus" || "$MODE" == "mp" || "$MODE" == "multiplayer" ]]; then
    EXE="${BUILD_DIR}/SCDA_Versus"
    echo "=== Launching Splinter Cell: Double Agent - Versus Multiplayer (arm64 Metal/Vulkan) ==="
else
    EXE="${BUILD_DIR}/SCDA"
    echo "=== Launching Splinter Cell: Double Agent - Single Player / Co-op (arm64 Metal/Vulkan) ==="
fi

if [[ ! -f "$EXE" ]]; then
    echo "Executable $EXE not found! Building..."
    ninja -C "$BUILD_DIR"
fi

cd "$BUILD_DIR"
exec "$EXE"
