// Shared User_Setup.h for ESP32 + ILI9341 2.8" TFT Display
// Include this in your sketches instead of modifying the TFT_eSPI library's User_Setup.h
//
// Usage in sketch:
//   #define USER_SETUP_LOADED
//   #include "../CONFIG/User_Setup_TFT_eSPI.h"
//   #include <TFT_eSPI.h>

#define USER_SETUP_INFO "ESP32_TFT_Shared_Setup"

// ==================== Display Driver ====================
#define ILI9341_DRIVER

// ==================== ESP32 Pins (HSPI) ====================
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1      // Not connected

// ==================== Backlight ====================
#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH

// ==================== SPI Configuration ====================
#define USE_HSPI_PORT
#define SPI_FREQUENCY  20000000
#define SPI_READ_FREQUENCY  20000000

// ==================== Touch Screen (XPT2046 on E32R28T) ====================
// The E32R28T variant uses custom SPI pins for touch (NOT standard HSPI/VSPI)
// Note: TFT_eSPI's built-in touch support may not work with custom pins
// Use manual bit-bang SPI or custom touch library instead

// Touch pins for E32R28T (from LCD Wiki documentation)
#define TOUCH_CS 33   // T_CS
// Additional touch pins (for reference - not used by TFT_eSPI):
//   T_CLK  = IO25
//   T_DIN  = IO32 (MOSI)
//   T_DO   = IO39 (MISO - input only)
//   T_IRQ  = IO36 (interrupt - input only)

// Touch SPI runs at lower frequency
#define SPI_TOUCH_FREQUENCY  2500000

// Optional: Touch calibration data (run calibration sketch to get these)
// #define TOUCH_CALIBRATION_DATA { 300, 3600, 300, 3600, 1 }

// IMPORTANT: E32N28T variant does NOT have touchscreen - comment out TOUCH_CS if using E32N28T

// ==================== Fonts ====================
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Smooth font rendering
#define SMOOTH_FONT
