# NOTES_SYNC — PLAN HISTORY
Append-only. Newest entries at the bottom. Source of truth for /rd_plan --resume.

## 2026-06-13 12:02 CDT (2026-06-13T17:02Z) — focus-set
- activeFocus: "NOTES_SYNC :: Phase 0 — Vendor + boot :: New NOTES_SYNC sketch in ESP32_TFT boots, lists SD /NOTES tree"
- commit: 18fc707
- note: Plan finalized and moved to ESP32_TFT (was drafted under RD_FileGameSystem). Decisions locked: home repo ESP32_TFT (vendor lean engine subset; TODO invert duplication later); WiFi SoftAP (board hosts its own network); mirror whole vault; render-all-filetypes deferred to Phase 5. NOTES read-only, SD written in place by ESP32 only. 2 open questions remain (AP creds, manifest hashing) before Phase 0 build.

## 2026-06-13 12:20 CDT (2026-06-13T17:20Z) — note
- activeFocus: "NOTES_SYNC :: Phase 0 — Vendor + boot :: Code written; awaiting flash-to-hardware verification"
- commit: 18fc707
- note: Phase 0 code written (uncommitted). Created NOTES_SYNC/ with vendored ENGINE/ (FileOperator.h, ESP32FileOperator.h, GraphicsRenderer.h, ESP32GraphicsRenderer.h/.cpp, VENDOR_TODO.md), NOTES_SYNC.ino (SD+TFT+touch wiring from FileGame.ino, sets data root /NOTES, recursive serial dump of the tree, exercises ESP32FileOperator + drawLabel), and NOTES_SYNC_Sources.cpp shim to compile the vendored .cpp in-tree. Open questions 1 and 2 resolved earlier (SSID ESP32_NOTES, hash-based diff). Next: flash to board and confirm it boots, lists /NOTES, registers taps.

## 2026-06-13 12:32 CDT (2026-06-13T17:32Z) — step-done
- activeFocus: "NOTES_SYNC :: Phase 0 — Vendor + boot :: Compiles via arduino-cli; awaiting flash-to-hardware verification"
- commit: 18fc707
- note: Phase 0 compiles clean with arduino-cli 1.5.1 (esp32 core 3.3.10; installed TFT_eSPI 2.5.43, PNGdec 1.1.6). FQBN esp32:esp32:esp32. Flash 31% (415714 B), RAM 21%. Fixed a stray </content> line that the editor had appended to NOTES_SYNC.ino, NOTES_SYNC_Sources.cpp, and 4 doc files. Build verified; hardware boot/list/tap still to be confirmed on-device.

## 2026-06-13 13:44 CDT (2026-06-13T18:44Z) — phase-done
- activeFocus: "NOTES_SYNC :: Phase 1 — Thin notes browser :: next; Phase 0 hardware-verified"
- commit: 18fc707
- committed as 56bb526 ("got notes sync working again") by the user.
- note: Phase 0 VERIFIED on hardware (/dev/ttyUSB0). Serial: "SD ready", "/NOTES not found ... Total files: 0", "FileOperator.listDirectory(/) -> 0 entries", "Touch ready". Display shows white "NOTES_SYNC" + "NOTES files: 0" correctly. The 0 is expected — SD card has no /NOTES content yet (sync is Phase 2). White-screen blocker resolved: switched NOTES_SYNC.ino from per-sketch USER_SETUP_LOADED include to the library User_Setup.h mechanism (installed CONFIG/User_Setup_TFT_eSPI.h into ~/Arduino/libraries/TFT_eSPI/User_Setup.h; original backed up as User_Setup.h.orig_backup) after confirming Basic_ESP32_TFT_Test.ino works that way. Added manual GPIO21 backlight drive. Saved project memories (white-screen fix, serial access, plan). Per-device checklist updated. Next: Phase 1 thin notes browser, or commit Phase 0 first.

## 2026-06-13 20:38 CDT (2026-06-14T01:38Z) — note
- activeFocus: "NOTES_SYNC :: Phase 1 — Thin notes browser :: code written + compiles; awaiting flash/hardware verification"
- commit: 56bb526 (uncommitted working tree changes)
- note: Phase 1 (thin notes browser) BUILT. New header-only NOTES_SYNC/NotesBrowser.h: touch-driven SD directory browser. BROWSE mode lists current folder (sub-folders first then files, A-Z, dotfiles/.staging hidden), 8 rows of size-2 text, footer [UP][^][v]; tap a folder row to descend, tap a file row to open. VIEW mode renders text exts (md/txt/org/csv/json/code...) via ESP32GraphicsRenderer::drawText with vertical scroll, .png via drawImage(bounded), everything else a "Cannot display <name>" placeholder; footer [BACK][^][v]; body clipped above footer via beginContentArea(0,0,320,218). UP at root re-lists (manual refresh after a WiFi sync). NOTES_SYNC.ino wiring: include NotesBrowser.h, global gBrowser, setup() now creates+begin()s the browser as the main UI (removed the old static status drawLabels for NOTES-count/WiFi/IP — SSID/IP still print to Serial), loop() services gSync.handle() then routes leading-edge taps to gBrowser->handleTap and calls gBrowser->tick() (redraw-on-demand). Removed the on-screen recv/del counter (was full-screen redraw per file; would fight the browser and slow the web server). String->std::string conversion needed at drawText/drawImage call sites (renderer takes std::string, Arduino gives String). Compiles clean with arduino-cli (esp32 core 3.3.10): flash 82% (1077738 B), RAM 28%. NOT yet flashed/verified on hardware. Side work this session: SCRIPTS/sync_notes_to_esp32.py gained --only PREFIX (sync one subpath, no deletes outside it) and --max-mb N (skip huge files, e.g. the 146 MB arduino-ide zip in the vault); manifest now starts from the board's stored manifest so filtered runs never drop other files; delete loop now updates the stored manifest. New SCRIPTS/notes_sync_wifi.sh (join ESP32_NOTES AP w/ WPA2-PSK fallback, sync, rejoin RD_WIFI; --dry-run/--diag/--stay, forwards sync flags) and .claude/NOTES_SYNC/WIFI_SYNC_CHEATSHEET.md. NEXT (after /clear): flash NOTES_SYNC to /dev/ttyUSB0, verify browsing the 359 synced files on-screen (open a RECIPES .md), then commit Phase 1.

## 2026-06-13 20:59 CDT (2026-06-14T01:59Z) — phase-done
- activeFocus: "NOTES_SYNC :: Phase 4 — Sync button + status UI :: next; Phase 1 hardware-verified"
- commit: 844e9d7 (Phase 1 code committed in this entry)
- note: Phase 1 VERIFIED on hardware (/dev/ttyUSB0). Flashed NOTES_SYNC (flash 82%, 1077888 B; hash verified). Serial boot clean: "SD ready", all 359 files listed under /NOTES, "FileOperator.listDirectory(/) -> 15 entries", "Touch ready", "SoftAP up: SSID=ESP32_NOTES IP=192.168.4.1", "Phase 1 browser ready. Tap to navigate; UP at root refreshes." User confirmed on-screen: tapping navigates folders and opens a note (RECIPES .md). Phases 2+3 (SoftAP receive server + desktop push) already functional — used to mirror the 359 files (HEAD 844e9d7 "files uploaded over wifi"). NEXT: Phase 4 — on-screen Sync button + live status UI (drawButton("Sync"), show SSID/IP + received/total progress), then Phase 5 (more file types: images via PNGdec, placeholders for pdf/xlsx/zip).