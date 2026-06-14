#!/usr/bin/env bash
#
# notes_sync_wifi.sh — full offline NOTES_SYNC workflow with diagnostics.
#
# Does the whole round trip so you don't have to remember the steps:
#   1. join the ESP32 SoftAP  (SSID ESP32_NOTES)
#   2. run the Python sync     (uploads changed notes to the board)
#   3. rejoin your normal WiFi (RD_WIFI)
#
# The board WiFi only exists while the ESP32 is powered AND running the
# NOTES_SYNC Phase 2 sketch (the TFT screen shows "WiFi: ESP32_NOTES" when
# the access point is up). If the SSID is not found, the fix is on the board,
# not the laptop.
#
# Usage:
#   ./notes_sync_wifi.sh            normal run (join, sync, rejoin)
#   ./notes_sync_wifi.sh --dry-run  join, preview changes, rejoin (sends nothing)
#   ./notes_sync_wifi.sh --diag     diagnostics only (radio, rescan, list)
#   ./notes_sync_wifi.sh --stay     join + sync but DO NOT rejoin RD_WIFI

set -u

AP_SSID="ESP32_NOTES"
AP_PASS="notesync123"
HOME_SSID="RD_WIFI"
BOARD_IP="192.168.4.1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYNC_PY="$SCRIPT_DIR/sync_notes_to_esp32.py"

say() { printf '\n=== %s ===\n' "$1"; }

rejoin_home() {
    say "Rejoining $HOME_SSID"
    nmcli connection up "$HOME_SSID" \
        || nmcli device wifi connect "$HOME_SSID"
}

diag() {
    say "Diagnostics"
    echo "+ nmcli radio wifi on"
    nmcli radio wifi on
    echo "+ nmcli device wifi rescan"
    nmcli device wifi rescan
    sleep 2
    echo "+ nmcli device wifi list (looking for $AP_SSID)"
    nmcli device wifi list | grep -E "SSID|$AP_SSID" || true
    echo
    if nmcli -t -f SSID device wifi list | grep -qx "$AP_SSID"; then
        echo "OK: $AP_SSID is visible."
    else
        echo "NOT FOUND: $AP_SSID is not broadcasting."
        echo "  -> Check the ESP32 is powered (USB)."
        echo "  -> Check the TFT shows 'WiFi: $AP_SSID' (SoftAP up)."
        echo "  -> If not, reflash NOTES_SYNC:"
        echo "     arduino-cli compile --upload -p /dev/ttyUSB0 \\"
        echo "       --fqbn esp32:esp32:esp32 \\"
        echo "       $(dirname "$SCRIPT_DIR")/NOTES_SYNC"
    fi
}

case "${1:-}" in
    --diag)
        diag
        exit 0
        ;;
esac

# Make sure the radio is on and the AP is visible before trying to join.
nmcli radio wifi on
nmcli device wifi rescan
sleep 2
if ! nmcli -t -f SSID device wifi list | grep -qx "$AP_SSID"; then
    echo "Error: SSID '$AP_SSID' not found."
    diag
    exit 1
fi

say "Joining $AP_SSID"
join_ap() {
    # Try the simple form first.
    nmcli device wifi connect "$AP_SSID" password "$AP_PASS" && return 0
    # Fall back: a stale/incomplete profile triggers
    # "802-11-wireless-security.key-mgmt: property is missing".
    # Delete it and recreate a complete WPA2-PSK profile explicitly.
    echo "  retrying with an explicit WPA2-PSK profile..."
    nmcli connection delete "$AP_SSID" 2>/dev/null
    nmcli connection add type wifi con-name "$AP_SSID" ifname '*' ssid "$AP_SSID" \
        wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$AP_PASS" \
        && nmcli connection up "$AP_SSID"
}
if ! join_ap; then
    echo "Error: could not join $AP_SSID."
    rejoin_home
    exit 1
fi

# Confirm the board answers before syncing.
say "Checking board at $BOARD_IP"
if ! ping -c 2 -W 2 "$BOARD_IP" >/dev/null 2>&1; then
    echo "Warning: $BOARD_IP did not respond to ping (continuing anyway)."
fi

say "Syncing notes"
# Pass through any sync flags except --stay (handled by this script).
PY_ARGS=()
for a in "$@"; do
    [ "$a" = "--stay" ] && continue
    PY_ARGS+=("$a")
done
python3 "$SYNC_PY" "${PY_ARGS[@]}"
SYNC_RC=$?

STAY=0
for a in "$@"; do [ "$a" = "--stay" ] && STAY=1; done
if [ "$STAY" -eq 0 ]; then
    rejoin_home
else
    say "Staying on $AP_SSID (--stay); run: nmcli connection up $HOME_SSID"
fi

exit $SYNC_RC
