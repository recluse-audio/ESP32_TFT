/*
 * XPT2046 Touch Test for E32R28T
 * Uses custom SPI pins as documented in LCD Wiki specs
 *
 * Touch Pins:
 * T_CS  = IO33
 * T_CLK = IO25
 * T_DIN = IO32 (MOSI)
 * T_DO  = IO39 (MISO)
 * T_IRQ = IO36
 */

// Load shared User_Setup from CONFIG folder
#define USER_SETUP_LOADED
#include "../CONFIG/User_Setup_TFT_eSPI.h"
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// XPT2046 Touch pins (custom SPI - NOT standard HSPI/VSPI)
#define T_CS   33
#define T_CLK  25
#define T_DIN  32  // MOSI
#define T_DO   39  // MISO (input only)
#define T_IRQ  36  // Interrupt (input only)

// Display dimensions
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== XPT2046 Touch Test (E32R28T) ===");

  // Initialize TFT
  tft.init();
  tft.setRotation(1);  // Landscape
  tft.fillScreen(TFT_BLACK);

  // Header (smaller - will show coordinates here)
  tft.fillRect(0, 0, SCREEN_WIDTH, 25, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(1);
  tft.setCursor(5, 9);
  tft.println("Touch to begin...");

  // Initialize touch pins
  pinMode(T_CS, OUTPUT);
  pinMode(T_CLK, OUTPUT);
  pinMode(T_DIN, OUTPUT);
  pinMode(T_DO, INPUT);   // IO39 is input-only
  pinMode(T_IRQ, INPUT);  // IO36 is input-only

  digitalWrite(T_CS, HIGH);
  digitalWrite(T_CLK, LOW);

  Serial.println("\nTouch Pin Configuration:");
  Serial.printf("T_CS  = IO%d\n", T_CS);
  Serial.printf("T_CLK = IO%d\n", T_CLK);
  Serial.printf("T_DIN = IO%d (MOSI)\n", T_DIN);
  Serial.printf("T_DO  = IO%d (MISO)\n", T_DO);
  Serial.printf("T_IRQ = IO%d\n\n", T_IRQ);

  // Test touch controller
  Serial.println("Testing XPT2046 communication...");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 50);

  delay(500);

  // Try to read a value from touch controller
  uint16_t x = readTouchRaw(0x90);  // Read X position
  uint16_t y = readTouchRaw(0xD0);  // Read Y position

  Serial.printf("Initial read: X=%d, Y=%d\n", x, y);
  Serial.printf("IRQ pin state: %s\n\n", digitalRead(T_IRQ) ? "HIGH (not touched)" : "LOW (touched)");

  Serial.println("✓ Touch controller initialized!");
  Serial.println("\nTouch the screen with stylus...\n");
}

void loop() {
  // Read touch with interrupt pin
  if (digitalRead(T_IRQ) == LOW) {  // Touch detected (IRQ goes low)
    uint16_t rawX, rawY, rawZ;

    // Read touch coordinates
    rawX = readTouchRaw(0x90);  // X position
    rawY = readTouchRaw(0xD0);  // Y position
    rawZ = readTouchRaw(0xB0);  // Z1 (pressure)

    // Valid touch has reasonable values
    if (rawZ > 400) {  // Pressure threshold
      // Map to screen coordinates (adjusted calibration - shifted right slightly)
      int screenX = map(rawX, 200, 3700, 0, SCREEN_WIDTH);
      int screenY = map(rawY, 300, 3800, 0, SCREEN_HEIGHT);

      screenX = constrain(screenX, 0, SCREEN_WIDTH - 1);
      screenY = constrain(screenY, 0, SCREEN_HEIGHT - 1);

      // Draw touch point
      tft.fillCircle(screenX, screenY, 3, TFT_RED);

      // Display coordinates in header bar
      tft.fillRect(0, 0, SCREEN_WIDTH, 25, TFT_BLUE);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);
      tft.setTextSize(1);
      tft.setCursor(5, 4);
      tft.printf("X:%3d Y:%3d", screenX, screenY);
      tft.setCursor(5, 14);
      tft.setTextColor(TFT_CYAN, TFT_BLUE);
      tft.printf("Raw:%4d,%4d Z=%d", rawX, rawY, rawZ);

      Serial.printf("Touch: Screen(%d,%d) Raw(%d,%d) Z=%d\n",
                    screenX, screenY, rawX, rawY, rawZ);

      delay(50);  // Debounce
    }
  }
}

// Bit-bang SPI read from XPT2046
uint16_t readTouchRaw(uint8_t command) {
  uint16_t data = 0;

  // Start transaction
  digitalWrite(T_CS, LOW);
  delayMicroseconds(10);

  // Send command byte
  spiWrite(command);

  // Read 12-bit response (sent as 16 bits)
  data = spiRead() << 8;
  data |= spiRead();

  // End transaction
  digitalWrite(T_CS, HIGH);
  delayMicroseconds(10);

  // Convert to 12-bit value
  data >>= 3;

  return data;
}

// Bit-bang SPI write
void spiWrite(uint8_t data) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(T_CLK, LOW);
    digitalWrite(T_DIN, (data >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(1);
  }
}

// Bit-bang SPI read
uint8_t spiRead() {
  uint8_t data = 0;

  for (int i = 7; i >= 0; i--) {
    digitalWrite(T_CLK, LOW);
    delayMicroseconds(1);
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(1);

    if (digitalRead(T_DO)) {
      data |= (1 << i);
    }
  }

  return data;
}
