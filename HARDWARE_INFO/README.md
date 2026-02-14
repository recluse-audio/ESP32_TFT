# HARDWARE_INFO Directory

This directory contains technical documentation and resources for the **2.8" ESP32 Display Module** (E32R28T/E32N28T) obtained from [LCD Wiki](http://www.lcdwiki.com/).

## Directory Contents

### Documentation Files

#### **2.8inch_ESP32_32E_Display_LCD_wiki.htm**
Comprehensive web documentation from LCD Wiki containing:
- Complete hardware specifications
- Pin assignment references
- Interface definitions and electrical parameters
- Troubleshooting guides and community documentation

#### **E32R28T_E32N28T_Specification_V1.0.pdf** (Primary Specification)
**Document Code:** CR2024-MI2873 | **Version:** V1.0 (Aug 15, 2024)

Complete product specifications for both module variants:
- Electrical characteristics and power specifications
- Display and touchscreen detailed specifications
- ESP32-WROOM-32E module parameters
- Physical dimensions and weight
- Interface descriptions (USB-C, SD card, battery, speaker, etc.)
- Operating and storage temperature ranges

#### **2.8inch_ESP32-32E_E32R28T_E32N28T_User_Manual.pdf** (Implementation Guide)
**Document Code:** CR2024-MI2875

User manual and implementation guide containing:
- Hardware resource directory and component identification
- Pin mapping and connection diagrams
- Onboard circuit descriptions (voltage regulator, USB-serial, charge management, audio amplifier)
- Frequently asked questions (FAQs)
- Setup and troubleshooting guidance

#### **2.8inch_ESP32-32_Display_Schematic.pdf** (Circuit Design)
Hardware schematic diagram showing:
- Complete circuit design and component layout
- Pin routing and connections
- Power distribution and regulation circuits
- Peripheral interface connections

#### **QD-TFT2803_specification_v1.1.pdf** (LCD Panel Specification)
**Module:** QD-TFT2803 | **Version:** V1.1 (March 21, 2018)

ILI9341 TFT LCD panel detailed specifications:
- 240×320 resolution RGB display specifications
- Color depth options (262K/65K colors)
- Optical characteristics (viewing angle, brightness)
- SPI interface timing diagrams
- LCD driver IC (ILI9341V) specifications

#### **esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf** (ESP32 Datasheet)
**Source:** Espressif Systems | **Version:** V1.6

Official ESP32-WROOM-32E module datasheet:
- ESP32-D0WD-V3 chip specifications (dual-core Xtensa LX6)
- Memory configuration (ROM, SRAM, Flash, PSRAM)
- WiFi and Bluetooth specifications
- GPIO capabilities and peripheral interfaces
- Electrical characteristics and operating conditions

#### **E32R28T_Size.pdf** (Mechanical Dimensions)
Physical outline and dimensional specifications:
- Module outline dimensions with tolerances
- LCD and touchscreen physical dimensions
- Mechanical integration measurements

### 3D Model Files

#### **E32R28T_3D/**
Contains mechanical design files:
- **E32R28T_3D.step** (17 MB) - STEP format 3D CAD model for mechanical integration planning and enclosure design

---

## Hardware Overview

### Display Module Specifications

| Specification | Details |
|---------------|---------|
| **Display Size** | 2.8 inches diagonal |
| **Resolution** | 240×320 pixels (RGB565) |
| **Display Driver** | ILI9341V |
| **Color Depth** | 262K colors (18-bit) / 65K colors (16-bit) |
| **Interface** | 4-Line SPI |
| **Backlight** | 4× White LEDs, 75mA current |
| **Brightness** | 260 cd/m² (typical) |
| **Viewing Angle** | 120° |

### Touchscreen Specifications (E32R28T variant only)

| Specification | Details |
|---------------|---------|
| **Type** | Resistive (pressure-sensitive) |
| **Driver IC** | XPT2046 |
| **Interface** | SPI |
| **Resolution** | 240×320 pixels (matches display) |
| **Material** | ITO film + ITO glass |
| **Accessory** | Resistive stylus (87mm × 5mm) |

### ESP32 Module (ESP32-WROOM-32E)

| Specification | Details |
|---------------|---------|
| **CPU** | Dual-core Xtensa LX6, 240 MHz |
| **ROM** | 448 KB |
| **SRAM** | 520 KB |
| **Flash** | 4 MB (variants: 4/8/16 MB) |
| **WiFi** | 802.11 b/g/n (2.4 GHz, up to 150 Mbps) |
| **Bluetooth** | V4.2 BR/EDR and BLE |
| **GPIO Pins** | 26 pins with multiple peripheral support |

### Power Specifications

| Parameter | Value |
|-----------|-------|
| **Input Voltage** | 5.0V (USB Type-C) |
| **Regulated Voltage** | 3.3V (ME6217C33M5G) |
| **Display Current** | 130 mA |
| **Backlight Current** | 75 mA |
| **Total Current** | Up to 480 mA (all features) |
| **Battery Support** | 3.7V lithium polymer (optional) |
| **Charging Current** | 500 mA max (TP4054 IC) |

### Physical Dimensions

| Component | Dimensions (W × H × D) |
|-----------|------------------------|
| **LCD Panel** | 50.00 × 69.20 × 2.3 mm |
| **Module (with touch)** | 50.00 × 86.00 × 5.60 mm |
| **Module (no touch)** | 50.00 × 86.00 × 4.40 mm |
| **Weight (with packaging)** | E32R28T: 90g, E32N28T: 80g |

### Onboard Hardware Features

- **Type-C USB** - Power and programming with auto-download circuit (CH340C)
- **MicroSD Card Slot** - FAT32 storage expansion via SPI (VSPI)
- **RGB LED** - 3-color status indicator (Red, Green, Blue)
- **Audio Amplifier** - FM8002E for speaker output
- **Battery Charger** - TP4054 charge management IC
- **BOOT Button** - Download mode / GPIO
- **RESET Button** - System reset
- **Serial Port** - 1.25mm 4P connector for debugging
- **Speaker Connector** - 1.25mm 2P audio output
- **Expansion Header** - 1.25mm 2P (GND, 3.3V, IO35)

---

## Product Variants

- **E32R28T** - Complete module **with** resistive touchscreen (XPT2046)
- **E32N28T** - Display module **without** touchscreen (lower cost alternative)

---

## Temperature Ranges

| Component | Operating | Storage |
|-----------|-----------|---------|
| **Display** | -10°C to +50°C | -20°C to +60°C |
| **Touchscreen** | -10°C to +60°C | -20°C to +70°C |
| **ESP32 Module** | -40°C to +85°C | -40°C to +150°C |

---

## Documentation Version Information

All documentation current as of **August 2024**. Specification documents reference design code CR2024-MI2873/MI2875.

For the latest updates and community support, visit [LCD Wiki](http://www.lcdwiki.com/).
