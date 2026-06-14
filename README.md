This repo is related to an ESP32 + integrated TFT screen (model E32R28T/E32N28T) purchased on Amazon. The module features a 2.8" ILI9341 display with an optional XPT2046 resistive touchscreen, along with integrated WiFi, Bluetooth, SD card slot, and battery charging support.

**Complete hardware documentation** from LCD Wiki is available in the **HARDWARE_INFO/** directory, including:
- Product specifications and datasheets
- Schematics and mechanical drawings
- LCD panel and ESP32 module specifications
- 3D CAD models for mechanical integration

See **HARDWARE_INFO/README.md** for detailed documentation summaries.

This required customization of the User_Setup.h file, which can exist in:
```
"C:\Users\<username>\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h"
```
Or use the shared configuration approach (see **CLAUDE.md** for details).

From here, the Basic_ESP32_TFT_Test sketch is a good jumping off point.

### Board
`ESP32-WROOM-32E + 2.8" ILI9341 Display (E32R28T/E32N28T from LCD Wiki)`
- HSPI: SCK=14, MOSI=13, MISO=12, CS=15, DC=2, BL=21
- VSPI (SD): SCK=18, MOSI=23, MISO=19, CS=5

### Packages Needed
`ESP32 by Espressif Systems`

### Board to select
`ESP32 Dev Module`

### Libraries
`TFT_eSPI`

### User_Setup config
```
#define ILI9341_DRIVER

#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1

#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH

#define SPI_FREQUENCY 20000000
#define USE_HSPI_PORT

```

## NOTES_SYNC: switching WiFi to push notes, and getting back

The NOTES_SYNC board hosts its own WiFi network. To sync you must join it, which
drops normal internet until you switch back. These commands make the round trip
safe. Run them in a terminal (your home network is the saved profile `RD_WIFI`).

### 1. Join the board's network
```sh
# Force a fresh scan first (the board's SSID is often missing from the cache).
nmcli device wifi rescan
sleep 5
nmcli device wifi list | grep -i ESP32_NOTES   # should show ESP32_NOTES

# Connect (WPA2 password is notesync123).
nmcli device wifi connect ESP32_NOTES password notesync123
```

### 2. Push the notes (board address is always 192.168.4.1)
```sh
python3 /home/artie/REPOS/PROJECTS/ESP32_TFT/SCRIPTS/sync_notes_to_esp32.py --dry-run
python3 /home/artie/REPOS/PROJECTS/ESP32_TFT/SCRIPTS/sync_notes_to_esp32.py
```
The sync is resumable: if it stops, just run it again and it sends only what's
left. It never writes the NOTES vault, only reads it.

### 3. Get back on home WiFi (RD_WIFI)
```sh
# Reconnect the saved home profile — no password needed.
nmcli connection up RD_WIFI

# If that ever fails, connect fresh (replace with your real password):
nmcli device wifi connect RD_WIFI password <YOUR_RD_WIFI_PASSWORD>

# Verify you are back online:
nmcli -t -f NAME connection show --active
ping -c 2 1.1.1.1
```

Notes:
- A VPN (ProtonVPN/WireGuard) can block reaching `192.168.4.1`. If the sync
  cannot connect, bring the VPN down first (`nmcli connection down proton0`),
  sync, then bring it back up after rejoining `RD_WIFI`.
- The board's network only exists while the board is powered. Unplugging it (or
  reconnecting `RD_WIFI`) returns you to normal internet.