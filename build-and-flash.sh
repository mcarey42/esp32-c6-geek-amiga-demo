#!/usr/bin/env bash
# build-and-flash.sh — one-shot build + flash for the ESP32-C6-Geek.
#
# Usage:
#   ./build-and-flash.sh                 # build + auto-detect port + flash + monitor
#   ./build-and-flash.sh --build-only    # build, don't flash
#   ./build-and-flash.sh --port /dev/ttyACM0   # use a specific serial port
#   ./build-and-flash.sh --no-monitor    # build + flash, skip monitor
#
# The script:
#   1. Sources ESP-IDF (looks in common install locations + common shims)
#   2. Sets the target to esp32c6
#   3. Builds the firmware
#   4. (optionally) Auto-detects the serial port the board enumerated as
#   5. (optionally) Flashes the device
#   6. (optionally) Opens the serial monitor so you can watch the boot log

set -euo pipefail

PORT=""
DO_FLASH=1
DO_MONITOR=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-only) DO_FLASH=0; DO_MONITOR=0; shift ;;
        --no-monitor) DO_MONITOR=0; shift ;;
        --port)       PORT="$2"; shift 2 ;;
        -h|--help)
            sed -n '2,12p' "$0" | sed 's/^# //;s/^#//'
            exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

cd "$(dirname "$0")"

# --- Locate and source ESP-IDF ---------------------------------------------

source_idf() {
    # Already sourced?
    if command -v idf.py >/dev/null 2>&1 && [[ -n "${IDF_PATH:-}" ]]; then
        echo "[idf] already in environment: $IDF_PATH"
        return 0
    fi

    # Repo-local shim (the form the development environment used)
    if [[ -f "scripts/activate.sh" ]]; then
        echo "[idf] sourcing ./scripts/activate.sh"
        # shellcheck disable=SC1091
        if . scripts/activate.sh && command -v idf.py >/dev/null 2>&1; then
            return 0
        fi
        echo "[idf] scripts/activate.sh did not produce a working idf.py — trying fallbacks"
    fi

    # Common installer paths to probe.
    local candidates=(
        "$HOME/esp/esp-idf/export.sh"
        "$HOME/esp/v6.0/esp-idf/export.sh"
        "$HOME/esp/v5.3/esp-idf/export.sh"
        "$HOME/esp/v5.2/esp-idf/export.sh"
        "$HOME/.espressif/v6.0/esp-idf/export.sh"
        "$HOME/.espressif/frameworks/esp-idf-v6.0/export.sh"
        "$HOME/.espressif/frameworks/esp-idf-v5.3/export.sh"
        "/opt/esp-idf/export.sh"
    )
    for c in "${candidates[@]}"; do
        if [[ -f "$c" ]]; then
            echo "[idf] sourcing $c"
            # shellcheck disable=SC1091
            if . "$c" >/dev/null 2>&1 && command -v idf.py >/dev/null 2>&1; then
                return 0
            fi
        fi
    done

    cat >&2 <<'EOF'

[idf] ESP-IDF not found.

This project requires ESP-IDF v5.3 or later (developed against v6.0).

Install IDF following Espressif's instructions:
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/

Then either:
  - source IDF's export.sh in your shell BEFORE running this script, OR
  - copy/edit scripts/activate.sh to source IDF from your install path

EOF
    return 1
}

# --- Auto-detect serial port -----------------------------------------------

detect_port() {
    local found=""
    for p in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyACM2 /dev/ttyUSB0 /dev/ttyUSB1 /dev/cu.usbmodem* /dev/cu.SLAB_USBtoUART; do
        if [[ -e "$p" ]]; then
            found="$p"
            break
        fi
    done
    if [[ -z "$found" ]]; then
        echo "[port] no serial device found — plug the board into USB and retry," >&2
        echo "       or pass --port /dev/ttyXXX explicitly." >&2
        return 1
    fi
    echo "$found"
}

# --- Build pipeline --------------------------------------------------------

source_idf

echo "[build] target = esp32c6"
idf.py set-target esp32c6 >/dev/null
echo "[build] building..."
idf.py build

if [[ "$DO_FLASH" -eq 0 ]]; then
    echo "[done] build complete, skipping flash (--build-only)."
    echo "       binary: build/esp32demo.bin"
    exit 0
fi

if [[ -z "$PORT" ]]; then
    PORT="$(detect_port)"
fi
echo "[flash] using port $PORT"
idf.py -p "$PORT" flash

if [[ "$DO_MONITOR" -eq 0 ]]; then
    echo "[done] flashed; skipping monitor (--no-monitor)."
    echo "       Reset the board to start the show."
    exit 0
fi

echo
echo "[monitor] opening serial monitor — Ctrl+] to exit."
exec idf.py -p "$PORT" monitor
