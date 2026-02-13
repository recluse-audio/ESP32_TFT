This repo is related to an ESP32 + integrated TFT screen I bought on Amazon, and getting it to connect properly.

This required customization of the User_Setup.h file, which actually needs to exist in
```
"C:\Users\<username>\OneDrive\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h"
```
Or wherever your Arduino libraries are located.


From here, the Basic_ESP32_TFT_Test sketch is a good jumping off point.

### Board
`ESP32 + 2.8" ILI9341 (HSPI 14/13/12, BL=21)`

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