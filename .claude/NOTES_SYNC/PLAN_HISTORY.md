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
- note: Phase 0 VERIFIED on hardware (/dev/ttyUSB0). Serial: "SD ready", "/NOTES not found ... Total files: 0", "FileOperator.listDirectory(/) -> 0 entries", "Touch ready". Display shows white "NOTES_SYNC" + "NOTES files: 0" correctly. The 0 is expected — SD card has no /NOTES content yet (sync is Phase 2). White-screen blocker resolved: switched NOTES_SYNC.ino from per-sketch USER_SETUP_LOADED include to the library User_Setup.h mechanism (installed CONFIG/User_Setup_TFT_eSPI.h into ~/Arduino/libraries/TFT_eSPI/User_Setup.h; original backed up as User_Setup.h.orig_backup) after confirming Basic_ESP32_TFT_Test.ino works that way. Added manual GPIO21 backlight drive. Saved project memories (white-screen fix, serial access, plan). Per-device checklist updated. Next: Phase 1 thin notes browser, or commit Phase 0 first.