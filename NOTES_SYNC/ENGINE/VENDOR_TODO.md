# Vendored engine files — provenance and TODO

These files are COPIES taken from the RD_FileGameEngine, not a dependency.

## Source
- Repo: RD_FileGameSystem / SUBMODULES/RD_FileGameEngine
- Source commit: 9ca2118 (RD_FileGameSystem), copied 2026-06-13
- Original locations:
  - `SOURCE/SHARED/FILE_OPERATOR/FileOperator.h`        -> `FileOperator.h`
  - `SOURCE/ESP32/ESP32FileOperator.h`                 -> `ESP32FileOperator.h`
  - `SOURCE/SHARED/GRAPHICS_RENDERER/GraphicsRenderer.h`-> `GraphicsRenderer.h`
  - `SOURCE/ESP32/ESP32GraphicsRenderer.h`             -> `ESP32GraphicsRenderer.h`
  - `SOURCE/ESP32/ESP32GraphicsRenderer.cpp`           -> `ESP32GraphicsRenderer.cpp`

## Local edits after copying
- Include paths flattened for this folder:
  - `ESP32FileOperator.h`: `"../SHARED/FILE_OPERATOR/FileOperator.h"` -> `"FileOperator.h"`
  - `ESP32GraphicsRenderer.h`: `"../SHARED/GRAPHICS_RENDERER/GraphicsRenderer.h"` -> `"GraphicsRenderer.h"`

## TODO (later, not this plan)
- Invert the duplication: make RD_FileGameSystem inherit these fundamentals FROM
  ESP32_TFT so there is one source of truth instead of two copies.
- Until then, if the upstream engine changes these files, re-copy and re-apply the
  include-path edits above.