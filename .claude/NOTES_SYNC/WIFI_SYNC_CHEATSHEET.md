# NOTES_SYNC WiFi cheatsheet (offline reference)

Board SoftAP: SSID `ESP32_NOTES`, password `notesync123`, board IP `192.168.4.1`.
Home WiFi: `RD_WIFI`. Vault (read-only): `/home/artie/REPOS/NOTES`.

## One command (does everything)

    cd ~/REPOS/PROJECTS/ESP32_TFT
    ./SCRIPTS/notes_sync_wifi.sh            # join, sync, rejoin RD_WIFI
    ./SCRIPTS/notes_sync_wifi.sh --dry-run  # preview only, sends nothing
    ./SCRIPTS/notes_sync_wifi.sh --diag     # just check if the board WiFi is visible

## Sync options (sync_notes_to_esp32.py)

    --only /RECIPES   sync ONLY that subpath; leaves all other board files
                      untouched (no deletes outside the prefix). Use this to
                      push one folder fast.
    --max-mb 5        skip any vault file larger than 5 MB. Use this on full
                      runs to avoid huge files that stall the sync.
    --dry-run         preview the plan, send nothing.

Examples:
    # just the recipes (9 tiny files, finishes instantly)
    python3 SCRIPTS/sync_notes_to_esp32.py --only /RECIPES
    # full vault but skip anything over 5 MB
    python3 SCRIPTS/sync_notes_to_esp32.py --max-mb 5

Known huge file that stalls a full sync:
    PROJECTS/ARDUINO/IDE/arduino-ide_2.3.3_Windows_64bit.zip  (~146 MB)
    -> use --max-mb 5 (or smaller) to skip it.

Card capacity: the board has no WiFi endpoint for SD free space yet; it prints
card size to serial at boot. (A /stat HTTP route could be added later.)

## Manual steps (if the script is unavailable)

1. Join the board:
       nmcli device wifi connect ESP32_NOTES password notesync123
2. Sync:
       python3 ~/REPOS/PROJECTS/ESP32_TFT/SCRIPTS/sync_notes_to_esp32.py
3. Rejoin home WiFi:
       nmcli connection up RD_WIFI

## Issue: "No network with SSID 'ESP32_NOTES' found"

The board's WiFi is not broadcasting, or your laptop's scan is stale.

Laptop side (try first):
    nmcli radio wifi on        # make sure WiFi radio is enabled
    nmcli device wifi rescan   # force a fresh scan (cached scans miss new APs)
    nmcli device wifi list     # is ESP32_NOTES in the list?

If still not listed, the board is the problem:
- ESP32 powered on (USB plugged in)?
- TFT screen shows the line `WiFi: ESP32_NOTES`? That prints only when the
  SoftAP starts. If missing, the board is not running the Phase 2 sketch.
- Reflash it (board on /dev/ttyUSB0):
      cd ~/REPOS/PROJECTS/ESP32_TFT
      arduino-cli compile --upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 NOTES_SYNC
- If /dev/ttyUSB0 is missing or "permission denied", see the serial-access
  notes: user must be in the `dialout` group, then log out/in.

## Issue: "802-11-wireless-security.key-mgmt: property is missing"

A stale/incomplete saved profile exists; NetworkManager can't infer WPA2.
Delete it and recreate the profile explicitly:
    nmcli connection delete ESP32_NOTES 2>/dev/null
    nmcli connection add type wifi con-name ESP32_NOTES ifname '*' ssid ESP32_NOTES \
      wifi-sec.key-mgmt wpa-psk wifi-sec.psk notesync123
    nmcli connection up ESP32_NOTES
(The notes_sync_wifi.sh script now does this fallback automatically.)

## Issue: joined the board but sync fails to reach it

- Confirm the board answers:
      ping -c 2 192.168.4.1
- Re-run the sync; it is resumable and only resends what failed.
- Make sure you are actually ON ESP32_NOTES, not auto-reconnected to RD_WIFI:
      nmcli -t -f NAME,DEVICE connection show --active

## Issue: stuck on the board's WiFi (no internet)

The board AP has no internet. Get back with:
    nmcli connection up RD_WIFI

## Safety reminders

- The desktop NOTES vault is READ-ONLY in this flow; nothing writes to it.
- The SD card stays in the board; the ESP32 writes it in place (staging + rename).
