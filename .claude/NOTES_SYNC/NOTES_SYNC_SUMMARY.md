# NOTES_SYNC — SUMMARY

## What this does
- Copies your desktop NOTES vault onto the ESP32's SD card over WiFi.
- The board makes its own WiFi network; your computer joins it and pushes the files.
- You press a Sync button on the screen to start receiving.
- Your NOTES repo is only ever READ. Nothing writes back to it. The SD card never leaves the board.

## What gets created / used
| What | Why | Where |
|------|-----|-------|
| `NOTES_SYNC.ino` (new sketch) | On-board app: SD + screen + touch + WiFi receiver | `ESP32_TFT/NOTES_SYNC/` |
| `NotesSyncServer.h/.cpp` | Web server on the board that receives files | same folder |
| `ENGINE/` (vendored) | Copied `FileOperator` + `GraphicsRenderer` from the game engine; only the fundamentals | `ESP32_TFT/NOTES_SYNC/ENGINE/` |
| `sync_notes_to_esp32.py` | Desktop script: reads NOTES, sends changed files to the board | `ESP32_TFT/SCRIPTS/` |
| `WiFiSecrets.h` | Board's WiFi name + password (optional, kept off git) | local only |
| `WiFi.h`, `WebServer.h`, `SD`, `TFT_eSPI`, `PNGdec` | WiFi AP + server, SD, display, image decode | already in the ESP32 Arduino core / Library Manager |

## Acronyms / terms
- **SD** — the removable memory card (stays in the board).
- **SoftAP (access point)** — the board hosts its own WiFi network; you connect to it directly, no router needed. Default board address is `192.168.4.1`.
- **WiFi push** — your computer sends files to the board; the board does not pull.
- **Manifest** — list the board reports of what it holds, so the script only sends what changed.
- **Staging + rename** — write to a temp name, rename on success, so a failed sync can't corrupt the existing copy.
- **Vendoring** — copying a few source files from the engine into this repo instead of depending on it.

## In repo vs. on machine
- **Repo (synced):** the new sketch, the vendored engine files, the server source, the desktop push script.
- **Local-only (per machine):** optional `WiFiSecrets.h`, the SD card contents.

## Safety guarantees
- The desktop NOTES vault is opened read-only — the script can physically only read it.
- The board only writes under one SD folder (`/NOTES`); nothing else is touched.
- An interrupted sync leaves the previous mirror intact (temp-write then rename).

## Note for later (not this plan)
- The vendored `ENGINE/` files are duplicates of RD_FileGameSystem. TODO: later make RD_FileGameSystem inherit these fundamentals FROM ESP32_TFT, so there is one source of truth.

## How to use it
- On the desktop, join the board's WiFi network named `ESP32_NOTES`.
- Press **Sync** on the TFT screen — board shows its network name + IP and waits.
- Run `python SCRIPTS/sync_notes_to_esp32.py 192.168.4.1` — pushes changed files; `--dry-run` previews.
- Watch progress on the screen; it returns to the browser when done.