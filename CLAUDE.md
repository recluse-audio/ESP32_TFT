# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32 Arduino project for controlling a 2.8" ILI9341 TFT display (320x240) with PNG image decoding support. The project includes examples for both internal flash (LittleFS) and SD card storage, plus utilities for image processing.

## Hardware Configuration

**Display Module**: ESP32-WROOM-32E + 2.8" ILI9341 TFT (model: E32R28T/E32N28T from LCD Wiki)

**Product Variants**:
- **E32R28T**: With XPT2046 resistive touchscreen
- **E32N28T**: Display only (no touchscreen)

**Display Specifications**:
- Size: 2.8 inches diagonal
- Resolution: 240×320 pixels
- Driver IC: ILI9341V
- Color Depth: 65K (RGB565) / 262K (RGB666)
- Brightness: 260 cd/m²
- Backlight: 4× White LEDs
- Interface: 4-line SPI

**ESP32 Module** (ESP32-WROOM-32E):
- CPU: Dual-core Xtensa LX6, 240 MHz
- RAM: 520 KB SRAM
- Flash: 4 MB
- WiFi: 802.11 b/g/n (2.4 GHz)
- Bluetooth: V4.2 BR/EDR and BLE

**Onboard Hardware**:
- Type-C USB with CH340C (auto-download circuit)
- MicroSD card slot (VSPI interface)
- TP4054 battery charging IC (optional 3.7V LiPo)
- FM8002E audio amplifier
- RGB LED indicator (Red, Green, Blue)
- BOOT and RESET buttons

**TFT Display Pins (HSPI)**:
- MOSI: 13
- MISO: 12
- SCLK: 14
- CS: 15
- DC: 2
- RST: -1 (not used)
- BL: 21 (backlight, active HIGH)
- SPI Frequency: 20MHz

**SD Card Pins (VSPI)** (used in SDCARD_* sketches):
- SCK: 18
- MISO: 19
- MOSI: 23
- CS: 5
- Frequency: 4MHz

**Touchscreen Pins** (E32R28T variant only, XPT2046):
- Interface: SPI (shared with VSPI)
- CS: (Refer to Touch_Test sketch or CONFIG/User_Setup_TFT_eSPI.h)

**Power**:
- Input: 5V via Type-C USB
- Regulated: 3.3V (ME6217C33M5G)
- Total Current: Up to 480 mA (all features active)
- Battery: Optional 3.7V lithium polymer

## Hardware Documentation

Complete technical documentation is available in the **HARDWARE_INFO/** directory, including:

- **Product specifications** (E32R28T_E32N28T_Specification_V1.0.pdf) - Complete electrical and physical specs
- **User manual** (2.8inch_ESP32-32E_E32R28T_E32N28T_User_Manual.pdf) - Implementation guide, FAQs, hardware resources
- **Schematics** (2.8inch_ESP32-32_Display_Schematic.pdf) - Circuit design reference
- **LCD panel specs** (QD-TFT2803_specification_v1.1.pdf) - ILI9341 display detailed specifications
- **ESP32 datasheet** (esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf) - Official Espressif documentation
- **Mechanical drawings** (E32R28T_Size.pdf) - Physical dimensions and outlines
- **3D CAD model** (E32R28T_3D/E32R28T_3D.step) - STEP format for mechanical integration
- **LCD Wiki documentation** (2.8inch_ESP32_32E_Display_LCD_wiki.htm) - Comprehensive web documentation

See **HARDWARE_INFO/README.md** for detailed summaries of each document and complete hardware specifications.

## Development Environment Setup

**Arduino IDE**: Version 2.x standalone installer (NOT Microsoft Store version - extensions are disabled there)

**Board Package**: `esp32` by Espressif Systems
**Board Selection**: ESP32 Dev Module

**Required Libraries** (install via Library Manager):
- `TFT_eSPI` by Bodmer
- `PNGdec`

**Critical Configuration**: The `TFT_eSPI` library requires hardware-specific configuration. **Use the shared config file instead of modifying the library:**

```cpp
// At the top of your sketch, before including TFT_eSPI.h:
#define USER_SETUP_LOADED
#include "../CONFIG/User_Setup_TFT_eSPI.h"
#include <TFT_eSPI.h>
```

This loads `CONFIG/User_Setup_TFT_eSPI.h` which contains the correct pin configuration for this hardware. This approach:
- Keeps configuration in the repository
- Avoids modifying the TFT_eSPI library files
- Makes the project portable

**Alternative (not recommended)**: Copy `CONFIG/User_Setup.h` to your Arduino libraries folder at:
```
C:\Users\<username>\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
```

**Sketchbook Location**: Recommended to use local drive (e.g., `C:\Arduino`) rather than OneDrive to avoid upload failures and path issues.

## Building and Uploading

### Standard Arduino Sketch Upload
1. Open any `.ino` file in Arduino IDE
2. Select Board: "ESP32 Dev Module"
3. Select correct COM port
4. Click Upload or use Ctrl+U

### LittleFS Filesystem Upload (for LittleFS_PNG sketch)

The `LittleFS_PNG` sketch requires PNG files to be uploaded to the ESP32's internal flash filesystem.

**Required Plugin**: `arduino-littlefs-upload` (v1.6.3 included as `arduino-littlefs-upload-1.6.3.vsix`)

**Plugin Installation**:
1. Create directory: `C:\Users\<username>\.arduinoIDE\plugins` (if not exists)
2. Copy the `.vsix` file into the `plugins` folder (NOT `plugin-storage`)
3. Restart Arduino IDE
4. Press `Ctrl+Shift+P`
5. Run: "Upload LittleFS to Pico/ESP8266/ESP32"

**How it works**: Files in the `data/` subdirectory of a sketch are packaged into a LittleFS image and flashed to a dedicated partition on the ESP32. The sketch then accesses these via `LittleFS.begin()` and standard file operations.

### SD Card Usage

For `SDCARD_*` sketches:
1. Format SD card as FAT32
2. Create the required folder structure (e.g., `/PHOTOS` for SDCARD_IMAGES)
3. Copy PNG files to the SD card
4. Insert SD card into module
5. Upload sketch normally

## Project Structure

### Arduino Sketches
- **Basic_ESP32_TFT_Test/**: Basic TFT functionality test (color fill, text display)
- **LittleFS_PNG/**: Displays PNG files stored in ESP32 internal flash via LittleFS
  - `LittleFS_PNG.ino`: Main sketch
  - `PNG_FS_Support.ino`: PNGdec callback functions for LittleFS
  - `data/`: PNG files to upload to flash (must use LittleFS upload tool)
- **SDCARD_IMAGES/**: Slideshow of PNG images from SD card `/PHOTOS` folder
- **SDCARD_TEXT/**: SD card text file operations
- **SDCARD_FILE_STRUCTURE/**: SD card directory listing and structure exploration
- **SDCARD_UPLOAD/**: File upload operations to SD card
- **SDCARD_TEST/**: Basic SD card initialization and testing
- **Touch_Test/**: Touchscreen input testing

### Image Processing Pipeline

**Python Scripts** (in `SCRIPTS/`):
1. **convert_to_png.py**: Converts various image formats (JPEG, BMP, RAW formats like ARW/CR2/NEF) to PNG
   - Source: `ASSETS/IMAGES/RAW/`
   - Output: `ASSETS/IMAGES/PNG/ORIGINAL_SIZE/`
   - Requires: `pip install pillow` (and optionally `rawpy` for RAW format support)

2. **resize_png.py**: Resizes PNG images to specific dimensions for display
   - Source: `ASSETS/IMAGES/PNG/ORIGINAL_SIZE/`
   - Output: `ASSETS/IMAGES/PNG/{width}x{height}/` (default: 320x240)
   - Options: Maintain aspect ratio (default) or stretch
   - Usage examples:
     ```bash
     python SCRIPTS/resize_png.py                           # 320x240, aspect ratio preserved
     python SCRIPTS/resize_png.py --width 480 --height 320  # Custom dimensions
     python SCRIPTS/resize_png.py --stretch                 # Stretch to exact size
     ```

**Workflow**: RAW/JPEG images → convert_to_png.py → PNG/ORIGINAL_SIZE → resize_png.py → PNG/320x240 → Copy to sketch data/ folder or SD card

### Configuration Files
- **CONFIG/User_Setup.h**: Original TFT_eSPI configuration (reference only)
- **CONFIG/User_Setup_TFT_eSPI.h**: Shared TFT_eSPI configuration for all sketches
  - Contains display pin mappings
  - Touch screen pin (uncomment `TOUCH_CS` after running Touch_Test)
  - Font settings
  - Include this in sketches instead of modifying library files

## Code Architecture

### Display Initialization Pattern
All sketches follow this pattern:
```cpp
#include <TFT_eSPI.h>
TFT_eSPI tft;

void setup() {
  tft.init();
  tft.setRotation(1);  // Landscape mode (320x240)
  tft.fillScreen(TFT_BLACK);
}
```

### PNG Decoding Architecture
PNG decoding uses the PNGdec library with callback functions for file operations:

**Callbacks Required**:
- `pngOpen()`: Open file, return handle and size
- `pngClose()`: Close file handle
- `pngRead()`: Read bytes from file
- `pngSeek()`: Seek to position in file
- `pngDraw()`: Render decoded line to display

**Two Implementations**:
1. **LittleFS version** (`LittleFS_PNG/PNG_FS_Support.ino`): Callbacks use LittleFS file operations
2. **SD Card version** (`SDCARD_IMAGES/SDCARD_IMAGES.ino`): Callbacks use SD library file operations

**Key difference**: SD card examples use VSPI explicitly (`SPIClass sdSPI(VSPI)`) while TFT uses HSPI (configured in User_Setup.h with `USE_HSPI_PORT`).

### Image Rendering
- `MAX_IMAGE_WIDTH` constant defines line buffer size (typically 240-320)
- PNGdec decodes line-by-line to conserve memory
- Each line is converted to RGB565 format via `png.getLineAsRGB565()`
- Lines are pushed to display via `tft.pushImage()`
- Full decode wrapped in `tft.startWrite()` / `tft.endWrite()` for performance

## Common Development Commands

### Python Environment (for image scripts)
```bash
# Activate virtual environment (if using venv/)
source venv/Scripts/activate  # Git Bash
# or
venv\Scripts\activate.bat     # Windows CMD

# Install dependencies
pip install pillow           # Basic image conversion
pip install rawpy            # RAW format support (optional)
```

### Running Python Scripts
```bash
# Convert images to PNG
python SCRIPTS/convert_to_png.py

# Resize PNGs for display (default 320x240)
python SCRIPTS/resize_png.py

# Resize with custom dimensions
python SCRIPTS/resize_png.py --width 240 --height 240
```

## Hardware-Specific Notes

- **Dual SPI Busses**: This setup uses both ESP32 SPI busses simultaneously - HSPI for TFT, VSPI for SD card. This allows independent operation without bus conflicts.

- **Display Dimensions**: After `setRotation(1)`, the display is 320x240 (landscape). Most sketches expect PNGs sized appropriately (≤320 width).

- **Backlight Control**: Pin 21 controls backlight (HIGH=ON). Can be PWM controlled for brightness adjustment.

- **Memory Constraints**: PNGdec requires ~40KB RAM. Line-by-line decoding is used to minimize memory usage. Images wider than MAX_IMAGE_WIDTH will fail to decode or display corrupted.

## Troubleshooting

**Display shows nothing**: Check that User_Setup.h is correctly installed in TFT_eSPI library folder with the pin configuration from CONFIG/User_Setup.h.

**LittleFS upload fails**: Ensure using standalone Arduino IDE 2.x (not Microsoft Store version) and plugin is in `.arduinoIDE/plugins/` (not `plugin-storage/`).

**SD card not detected**: Verify VSPI pin connections (SCK=18, MISO=19, MOSI=23, CS=5) and that SD is formatted as FAT32.

**PNG decoding fails**: Ensure PNG width ≤ MAX_IMAGE_WIDTH defined in sketch. Check serial output for detailed error messages.

**Images look wrong**: Verify `tft.setSwapBytes(false)` is set correctly for your display. Some displays need this set to `true`.
