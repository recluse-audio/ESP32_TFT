#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "SD_MMC.h"

static void ListRoot(fs::FS& fs)
{
  File root = fs.open("/");
  if (!root)
  {
    Serial.println("open / failed");
    return;
  }

  Serial.println("root listing:");
  for (File f = root.openNextFile(); f; f = root.openNextFile())
  {
    Serial.print("  ");
    Serial.print(f.name());
    if (f.isDirectory()) Serial.print("/");
    Serial.println();
    f.close();
  }
  root.close();
}

static bool TrySpi(const char* label, int sck, int miso, int mosi, int cs, uint32_t hz)
{
  Serial.printf("[%s] SCK=%d MISO=%d MOSI=%d CS=%d Hz=%lu ... ",
                label, sck, miso, mosi, cs, (unsigned long)hz);

  SPIClass spi(HSPI);
  spi.begin(sck, miso, mosi, cs);

  if (!SD.begin(cs, spi, hz))
  {
    Serial.println("FAIL");
    return false;
  }

  Serial.println("OK");
  Serial.print("Card size MB: ");
  Serial.println((uint32_t)(SD.cardSize() / (1024 * 1024)));
  ListRoot(SD);
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("SD test starting...");

  Serial.println("Trying SD_MMC...");
  if (SD_MMC.begin())
  {
    Serial.println("SD_MMC OK");
    Serial.print("Card size MB: ");
    Serial.println((uint32_t)(SD_MMC.cardSize() / (1024 * 1024)));
    ListRoot(SD_MMC);
    return;
  }
  Serial.println("SD_MMC failed.");

  Serial.println("Trying SPI SD (SD.h) probe...");

  const int csPins[] = { 5, 4, 13, 15, 2, 21, 22 };

  // VSPI default pins
  for (int cs : csPins)
  {
    if (TrySpi("VSPI", 18, 19, 23, cs, 4000000)) return;
  }

  // HSPI default pins (also matches many integrated TFT busses)
  for (int cs : csPins)
  {
    if (TrySpi("HSPI", 14, 12, 13, cs, 4000000)) return;
  }

  Serial.println("No SPI config worked either.");
}

void loop() {}
