#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <TFT_eSPI.h>
#include <PNGdec.h>

static const int kSdCsPin = 5;
static const uint32_t kSdHz = 4000000;

static const char* kImageDir = "/PHOTOS";   // <-- your folder
static const uint32_t kImageDelayMs = 2000;

#define MAX_IMAGE_WIDTH  320   // set to >= the widest PNG you intend to display
static uint16_t lineBuffer[MAX_IMAGE_WIDTH];

TFT_eSPI tft;
PNG png;

SPIClass sdSPI(VSPI);
static File gPngFile;

// ---------- PNGdec file callbacks ----------
static void* PngOpen(const char* filename, int32_t* size)
{
  gPngFile = SD.open(filename, FILE_READ);
  if (!gPngFile)
  {
    *size = 0;
    return nullptr;
  }
  *size = (int32_t)gPngFile.size();
  return &gPngFile;
}

static void PngClose(void* handle)
{
  File* f = (File*)handle;
  if (f && *f) f->close();
}

static int32_t PngRead(PNGFILE*, uint8_t* buffer, int32_t length)
{
  if (!gPngFile) return 0;
  return (int32_t)gPngFile.read(buffer, length);
}

static int32_t PngSeek(PNGFILE*, int32_t position)
{
  if (!gPngFile) return 0;
  return gPngFile.seek(position) ? position : 0;
}

static int PngDraw(PNGDRAW* pDraw)
{
  if ((int)pDraw->iWidth > MAX_IMAGE_WIDTH)
  {
    return 0; // stop: buffer would overflow -> random colors
  }

  // Convert this line into RGB565 pixels in our buffer
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);

  // Draw the full line (or crop yourself if you want)
  tft.pushImage(0, (int)pDraw->y, (int)pDraw->iWidth, 1, lineBuffer);
  return 1;
}


// ---------- Helpers ----------
static bool IsPngName(const String& name)
{
  String lower = name;
  lower.toLowerCase();
  return lower.endsWith(".png");
}

static void CollectRecursive(const char* dirPath, String* outPaths, int maxCount, int& count)
{
    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory())
    {
        return;
    }

    for (File f = dir.openNextFile(); f && count < maxCount; f = dir.openNextFile())
    {
        String path = String(f.name());

        // On ESP32 SD, name() often returns full path already.
        // If not, prepend current directory.
        if (!path.startsWith("/"))
        {
        path = String(dirPath) + "/" + path;
        }

        if (f.isDirectory())
        {
        f.close();
        CollectRecursive(path.c_str(), outPaths, maxCount, count);
        }
        else
        {
        if (IsPngName(path))
        {
            outPaths[count++] = path;
        }
        f.close();
        }
    }

    dir.close();
}

static int CollectPngPaths(const char* rootDir, String* outPaths, int maxCount)
{
  int count = 0;
  CollectRecursive(rootDir, outPaths, maxCount, count);
  return count;
}

static bool DrawPng(const char* path)
{
    int rc = png.open(path, PngOpen, PngClose, PngRead, PngSeek, PngDraw);
    if (rc != PNG_SUCCESS) return false;


    const int imgW = (int)png.getWidth();
    const int imgH = (int)png.getHeight();
    const int scrW = tft.width();
    const int scrH = tft.height();

    if (imgW <= 0 || imgH <= 0)
    {
        png.close();
        return false;
    }

    // Only draw if it fully fits (no scaling, no cropping)
    if (imgW > scrW || imgH > scrH || imgW > MAX_IMAGE_WIDTH)
    {
        png.close();
        return false; // caller can treat as "skip"
    }


    tft.startWrite();
    rc = png.decode(nullptr, 0);
    tft.endWrite();

    png.close();
    return (rc == PNG_SUCCESS);
}

static bool InitSd()
{
    // Proven-good VSPI pins for your board
    sdSPI.begin(/*sck=*/18, /*miso=*/19, /*mosi=*/23, /*ss=*/kSdCsPin);
    return SD.begin(kSdCsPin, sdSPI, kSdHz);
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(false);

    if (!InitSd())
    {
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.println("SD init failed");
        while (true) delay(1000);
    }

    tft.setCursor(0, 0);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("SD OK");
    tft.println(kImageDir);
}

void loop()
{
  static String paths[128];
  static int pathCount = -1;

    if (pathCount < 0)
    {
        pathCount = CollectPngPaths(kImageDir, paths, 128);
        Serial.printf("Found %d PNG(s) in %s\n", pathCount, kImageDir);

        if (pathCount == 0)
        {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.println("No PNGs found");
        delay(2000);
        return;
        }
    }

    for (int i = 0; i < pathCount; ++i)
    {
        Serial.printf("Showing %s\n", paths[i].c_str());

        tft.fillScreen(TFT_BLACK);
        bool ok = DrawPng(paths[i].c_str());

        if (!ok)
        {
            tft.setCursor(0, 0);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.println("PNG decode fail:");
            tft.println(paths[i]);
        }
        else
        {
            delay(kImageDelayMs);
        }
    }
}
