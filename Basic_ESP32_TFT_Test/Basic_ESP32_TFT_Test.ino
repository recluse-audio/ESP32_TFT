#include <TFT_eSPI.h>
TFT_eSPI tft;

static const int BL_PIN = 21; // you said HIGH turns it on

void setup()
{
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, HIGH);
  delay(50);

  tft.init();
  tft.setRotation(1);

  tft.fillScreen(TFT_RED);   delay(200);
  tft.fillScreen(TFT_GREEN); delay(200);
  tft.fillScreen(TFT_BLUE);  delay(200);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("TFT OK");
}

void loop() 
{
  
}
