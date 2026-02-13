### Load a PNG onto ESP32/screen ###
This sketch is for loading PNGs onto the ESP32 and displaying them.
This required some steps for actually getting the image to upload. (an extension)

ChatGPT was telling me to use an extensions sidebar... not working.

I ended up succeeding. Should be able to Build/Verify LittleFS_PNG sketch 
as long as you have requirements installed.

### LittleFS ###
"Little File System" - lightweight embedded file system stored in dedicated partition of ESP32 flash memory.

Below is ChatGPT summary of how I got it working.

---

# LittleFS_PNG – ESP32 + TFT + PNGdec

## Environment Setup

This project uses:

* **Arduino IDE 2.x (standalone installer)**

  * Downloaded from: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
  * Do **not** use the Microsoft Store version (extensions are disabled).

* **Sketchbook relocated to local drive**

  * Moved from OneDrive to:

    ```
    C:\Arduino
    ```
  * Set via:

    ```
    File → Preferences → Sketchbook location
    ```

Keeping Arduino off OneDrive avoids filesystem upload failures and path issues.

---

## Required Libraries

Installed via Library Manager:

* `TFT_eSPI` (Bodmer)
* `PNGdec`

ESP32 board package:

* `esp32 by Espressif Systems` (Boards Manager)

---

## LittleFS Upload Tool (Required)

The example includes PNG files in a `data/` folder, but they are **not automatically uploaded** to the ESP32.

To upload them, we installed the LittleFS uploader plugin:

Repository:
[https://github.com/earlephilhower/arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload)

### Installation (Arduino IDE 2.x)

1. Download the `.vsix` from the releases page.
2. Create this folder (if it does not exist):

   ```
   C:\Users\<username>\.arduinoIDE\plugins
   ```
3. Place the `.vsix` file inside that `plugins` folder.
4. Restart Arduino IDE.
5. Press:

   ```
   Ctrl + Shift + P
   ```

   Run:

   ```
   Upload LittleFS to Pico/ESP8266/ESP32
   ```

The `.vsix` must be in `plugins`, not `plugin-storage`.

---

## Where the PNG Data Comes From

The example sketch includes a `data/` directory:

```
LittleFS_PNG/
    LittleFS_PNG.ino
    PNG_FS_Support.ino
    data/
        test.png
```

The files inside `data/` are copied into the ESP32’s flash filesystem (LittleFS) when running the **Upload LittleFS** command.

At runtime:

* `LittleFS.begin()` mounts the filesystem.
* The sketch scans `/` for `.png` files.
* `PNGdec` decodes the image.
* `TFT_eSPI` renders it to the display.

---

## Why the LittleFS Plugin Was Needed

Arduino IDE does not automatically upload filesystem data.

Without the LittleFS uploader:

* The ESP32 filesystem is empty.
* The example compiles and runs.
* No PNG files are found.
* Nothing renders.

The plugin builds a LittleFS image from the `data/` folder and flashes it into the ESP32’s filesystem partition.

---

## Board Configuration Notes

ESP32 + ILI9341 2.8" TFT (HSPI):

```
MOSI = 13
MISO = 12
SCLK = 14
CS   = 15
DC   = 2
RST  = -1
BL   = 21 (active HIGH)
SPI  = 20MHz
```

---

This setup is now fully local (no OneDrive dependency) and reproducible.
