# Configuration Files

This folder contains TFT_eSPI library configuration for the ESP32 + ILI9341 2.8" display.

## Files

### User_Setup_TFT_eSPI.h
**The main configuration file to use in your sketches.**

Contains:
- Display driver (ILI9341)
- ESP32 pin mappings (HSPI)
- Touch screen configuration (uncomment TOUCH_CS after finding correct pin)
- Font settings

**Usage in sketches:**
```cpp
#define USER_SETUP_LOADED
#include "../CONFIG/User_Setup_TFT_eSPI.h"
#include <TFT_eSPI.h>
```

### User_Setup.h
Original TFT_eSPI configuration (reference only). This was the initial configuration copied from the Arduino libraries folder.

## Touch Screen Setup

The touch screen pin is currently not configured. To enable touch:

1. Run the `Touch_Test` sketch to find the correct TOUCH_CS pin
2. Edit `User_Setup_TFT_eSPI.h`
3. Uncomment and update the TOUCH_CS line:
   ```cpp
   #define TOUCH_CS 4  // Replace 4 with the pin found by Touch_Test
   ```

## Why Use This Approach?

- **Portable**: Configuration is part of the repository
- **Clean**: Doesn't modify TFT_eSPI library files
- **Flexible**: Easy to update for all sketches at once
- **Version controlled**: Changes are tracked in git
