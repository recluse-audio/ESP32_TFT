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