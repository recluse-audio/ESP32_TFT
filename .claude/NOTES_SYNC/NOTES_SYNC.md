# NOTES_SYNC — PLAN

Goal: Mirror the desktop NOTES Obsidian vault to an ESP32 + SD card over WiFi, triggered by
an on-screen Sync button, by vendoring the fundamental RD_FileGameEngine abstractions
(FileOperator / GraphicsRenderer + touch wiring) into the ESP32_TFT repo. The desktop NOTES
repo is strictly read-only; only the ESP32 writes its own SD card.

Effort: medium

## Status
- activeFocus: "NOTES_SYNC :: Phase 4 — Sync button + status UI :: next; Phase 1 hardware-verified"
- last commit: 844e9d7 (Phase 0 + Phases 2/3 committed; Phase 1 code committed alongside this update)
- updated: 2026-06-13 20:59 CDT / 2026-06-14T01:59Z
- Phases 0-3 complete and hardware-verified. Phase 1 browser (NOTES_SYNC/NotesBrowser.h + NOTES_SYNC.ino wiring) verified on-device: 359 files list, tap-navigate folders, open a RECIPES .md.
- NEXT: Phase 4 (on-screen Sync button + live received/total status UI), then Phase 5 (more file types: images via PNGdec, placeholders for pdf/xlsx/zip).

## Display config gotcha (resolved — see also project memory)
- The per-sketch `#define USER_SETUP_LOADED` + include of CONFIG/User_Setup_TFT_eSPI.h left the panel SOLID WHITE on a fresh machine, even though it compiled and reported using the config.
- Fix: configure the TFT_eSPI library's own `User_Setup.h` directly (`cp CONFIG/User_Setup_TFT_eSPI.h ~/Arduino/libraries/TFT_eSPI/User_Setup.h`) and have the sketch just `#include <TFT_eSPI.h>` (matches the known-good Basic_ESP32_TFT_Test.ino). NOTES_SYNC.ino now does this.
- Separate symptom: dark screen / no backlight needs GPIO 21 driven HIGH before tft.init() (already in the sketch).
- Consequence: per-new-device checklist MUST install the library User_Setup.h (added below).

## Hard constraints (from the user)
- NEVER write to the desktop NOTES repo. Every sync only READS NOTES. This is the data-loss guard.
- The SD card is NEVER removed from the board. It is written in place — therefore the ESP32 itself is the only possible writer, and that is allowed and safe (SD is a disposable mirror).
- Home repo is **ESP32_TFT**. Copy the needed engine pieces from RD_FileGameSystem into ESP32_TFT and duplicate them here. TODO (later, not this plan): have RD_FileGameSystem inherit these fundamentals FROM ESP32_TFT instead of holding its own copy — reverse the duplication once stable.

## Objectives
1. [must] The NOTES vault (whole vault, all file types) mirrors onto the ESP32 SD card preserving folder structure, including additions, updates, and deletions.
2. [must] A Sync button on the TFT screen triggers a sync with no cable swap and no card removal.
3. [must] Transport is WiFi with the **board as its own access point (SoftAP)**; the desktop joins the board's network and pushes. USB/Bluetooth deferred.
4. [must] All SD writes reuse a vendored `ESP32FileOperator`; all UI reuses a vendored `GraphicsRenderer` + the existing touch loop. Lean vendor set, no game machinery.
5. [must] NOTES on the desktop is opened read-only end to end; a half-finished sync never corrupts the existing SD mirror (temp-write + rename).
6. [nice] Browse/read synced notes on-device; rendering every file type (pdf/xlsx/zip) is an explicit later goal, not required for first ship.

## Decided stack
- WiFi transport — `WiFi.h` SoftAP + `WebServer.h` (both built into the ESP32 Arduino core; no extra library). Board hosts its own network; default AP IP is `192.168.4.1`.
- ESP32 receive server — hosts `/manifest`, `/file`, `/commit`; writes via vendored `ESP32FileOperator`.
- Desktop push script — Python, reads NOTES read-only, diffs against the ESP32 manifest, POSTs changes. Adapts the mirror logic in `RD_FileGameSystem/build_and_install_sd.py`.
- UI — vendored `ESP32GraphicsRenderer` (`drawButton`, `drawLabel`, `drawCenteredText`) + the touch loop from `FileGame.ino`.
- SD access — vendored `ESP32FileOperator` (read/write/append/list already implemented upstream).

### Why not the alternatives (so we don't relitigate)
- ❌ USB transfer — ESP32-WROOM-32E has no native USB, only CH340 serial. No mass-storage; serial transfer is slow/fiddly. Hardest, not easiest.
- ❌ Bluetooth first — classic `BluetoothSerial` is slow for ~359 files and Windows-specific desktop-side. Possible later.
- ❌ Desktop writes the SD directly (`build_and_install_sd.py` to `E:\`) — needs the card mounted on the PC. Violates the no-removal constraint.
- ❌ WiFi join-home-LAN — chose SoftAP instead so no router/credentials dependency; the board is self-contained and works anywhere.
- ❌ Vendoring the whole engine (GameRunner/Scene/Zone/Level/overlays) — coupled to the puzzle-game concept we don't need. Vendor only the two abstractions + renderer + config.

## The interface(s) we wire to (real signatures, from the engine being vendored)
- `ESP32FileOperator` (`RD_FileGameEngine/SOURCE/ESP32/ESP32FileOperator.h`):
  - `void setDataRoot(const std::string& root)` — SD root prefix for relative paths.
  - `std::string load(path)` / `void writeToFile(path, content)` / `void appendToFile(path, content)`
  - `void writeAbsolute(path, content)` / `std::string loadAbsolute(path)` — bypass data root.
  - `std::vector<std::string> listDirectory(path)`
- `GraphicsRenderer` (`RD_FileGameEngine/SOURCE/SHARED/GRAPHICS_RENDERER/GraphicsRenderer.h`):
  - `void drawButton(const std::string& label, int x, int y, int w, int h)`
  - `void drawLabel(const std::string& text, int x, int y)` / `drawCenteredText(...)` / `drawFilledRect(...)`
  - `void drawText(const std::string& path, int x, int y)` — renders a markdown/text file by path.
- Touch + frame loop (`RD_FileGameEngine/SOURCE/ESP32/FileGame/FileGame.ino`):
  - SD on VSPI (`SD_CS 5, SCK 18, MISO 19, MOSI 23`), TFT on HSPI, XPT2046 touch via software SPI (`T_CLK 25, T_DIN 32, T_DO 39, T_CS 33, T_IRQ 36`).
  - Leading-edge hit detection; redraw-on-demand. We mirror this wiring verbatim.
- Desktop mirror helpers to adapt (`RD_FileGameSystem/build_and_install_sd.py`):
  - `sync_tree`, `remove_extra`, `sync_file` — mtime-based copy + prune. Replace the filesystem destination with HTTP POSTs to the board.

## Architecture / flow
```
  Desktop (read-only)                         ESP32 (its own WiFi AP, writes only its own SD)
  -----------------                           ----------------------------------------------
  join board's WiFi (SoftAP)
  NOTES vault --read--> sync_notes.py          WiFi.softAP() + WebServer.h @ 192.168.4.1
   (whole vault)             |                   GET  /manifest  -> {path: {size,hash}}
                            diff <-------------- (ESP32 walks SD /NOTES via FileOperator)
                             |
                 POST /file (path + bytes) ----> write /NOTES/.staging/<path> then rename
                 POST /commit (delete list) ---> unlink extras under /NOTES
                                               Sync button on TFT shows AP SSID/IP + progress
```
- Writes land in a staging path and are renamed on success → an interrupted sync never breaks the live mirror.
- Everything the ESP32 writes is under one SD root (`/NOTES`). Nothing else is touched.

## Config that lives in THIS repo (ESP32_TFT)
- `NOTES_SYNC/NOTES_SYNC.ino` — new sketch folder at the repo root (sibling of `SDCARD_*`, `Basic_ESP32_TFT_Test`).
- `NOTES_SYNC/NotesSyncServer.h/.cpp` — SoftAP + WebServer endpoints (`/manifest`, `/file`, `/commit`).
- `NOTES_SYNC/ENGINE/` — vendored copies (lean set): `FileOperator.h`, `ESP32FileOperator.h`, `GraphicsRenderer.h`, `ESP32GraphicsRenderer.h/.cpp`.
- `NOTES_SYNC/NOTES_SYNC_Sources.cpp` — single TU that `#include`s the vendored `.cpp` files so Arduino compiles them in-tree (same pattern as `FileGame_Sources.cpp`).
- `NOTES_SYNC/CONFIG/User_Setup_TFT_eSPI.h` — copied from the engine CONFIG (or reuse existing `CONFIG/User_Setup_TFT_eSPI.h` already in ESP32_TFT).
- `SCRIPTS/sync_notes_to_esp32.py` — desktop push script (ESP32_TFT already has a `SCRIPTS/` dir).
- Add a `VENDOR_TODO.md` note in `NOTES_SYNC/ENGINE/` recording the source commit copied from and the future inversion (RD_FileGameSystem inherits from here).

## Phased steps (each independently verifiable)
0. **Vendor + boot** (effort M, obj 4) — Copy the lean engine set into `NOTES_SYNC/ENGINE/`; record source commit in `VENDOR_TODO.md`. Create `NOTES_SYNC.ino` from `FileGame.ino` wiring (SD + TFT + touch). Set data root `/NOTES`. List the SD `/NOTES` tree to Serial. Verify: board boots, prints the folder listing.
1. **Thin notes browser** (effort M, obj 6) — Render a scrollable directory listing with `drawButton`/`drawLabel`; tap a folder to descend, tap a `.md` to `drawText` it. No game classes. Verify: navigate a seeded card on-screen.
2. **WiFi SoftAP receive server, safe writes** (effort L, obj 1,3,5) — `WiFi.softAP(ssid,pass)` + `WebServer.h`. Endpoints: `/manifest` (walk `/NOTES`, JSON path→size+hash), `/file` (write to `/NOTES/.staging/<path>` then rename), `/commit` (delete listed extras). Writes confined under `/NOTES`. Verify: `curl` a file to `192.168.4.1`, lands intact; a killed transfer leaves the prior copy intact.
3. **Desktop push script** (effort M, obj 1,5) — `SCRIPTS/sync_notes_to_esp32.py`: open NOTES read-only, GET `/manifest`, diff (add/update by hash, delete extras), POST changed files, POST `/commit`. Reuse `sync_tree`/`remove_extra` with HTTP destination. `--dry-run` prints the diff. Verify: full first sync, no-op second run, edit+delete reflected.
4. **Sync button + status UI** (effort M, obj 2,4) — `drawButton("Sync")`; on hit, bring up the AP and show SSID/IP + live progress (received/total) via `drawLabel`. Verify: press, run script, watch progress, return to browser.
5. **Render more file types** (effort M, obj 6, later) — images via PNGdec, graceful placeholder for pdf/xlsx/zip. Verify: open a note with an image.

## Decided defaults (resolved open questions)
1. AP network name (SSID) is `ESP32_NOTES`, with a throwaway password hardcoded in the sketch (the AP is local and short-lived, not your home WiFi).
2. Manifest change-detection uses content **hashing**, computed desktop-side; the board reports what it stored. Only changed files are sent. (Avoids missing same-size edits.)

## Local-only (NOT tracked)
- `WiFiSecrets.h` — AP SSID/password if we keep them out of git.
- The SD card contents themselves.
- Board-specific touch recalibration constants if a different unit is used.

## Per-new-device checklist
- Install the display config into the TFT_eSPI library: `cp CONFIG/User_Setup_TFT_eSPI.h ~/Arduino/libraries/TFT_eSPI/User_Setup.h` (REQUIRED, or the screen is solid white) · ensure your user is in the `dialout` group for `/dev/ttyUSB0` access (`sudo usermod -aG dialout $USER`) · flash `NOTES_SYNC.ino` (`arduino-cli compile --upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 NOTES_SYNC`) · (optional) create local `WiFiSecrets.h` · insert a FAT32 SD card · on the desktop, join the board's `ESP32_NOTES` WiFi network · run `python SCRIPTS/sync_notes_to_esp32.py 192.168.4.1` for the first full mirror.