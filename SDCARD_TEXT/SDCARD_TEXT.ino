#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

static const int kSdCsPin = 5;
static const uint32_t kSdHz = 4000000;

SPIClass sdSPI(VSPI);
TFT_eSPI tft;

// ----- UI layout (320x240) -----
static const int kMarginX = 2;
static const int kMarginY = 2;
static const int kLineGap = 1;        // extra pixels between lines
static const int kFont = 2;           // TFT_eSPI built-in font 2 (~16 px height)
static const uint32_t kPagePauseMs = 1200; // pause when page fills (0 = no pause)

static int LineHeightPx()
{
    return 16 + kLineGap; // font 2 ~16px
}

static void NewPage(int& cursorY)
{
    if (kPagePauseMs > 0)
    {
        delay(kPagePauseMs);
    }
    tft.fillScreen(TFT_BLACK);
    cursorY = kMarginY;
}

static void PrintLine(const String& s, int& cursorY)
{
    if (cursorY + LineHeightPx() > tft.height())
    {
        NewPage(cursorY);
    }

    tft.setCursor(kMarginX, cursorY);
    tft.println(s);
    cursorY += LineHeightPx();
}

static bool InitSd()
{
    sdSPI.begin(/*sck=*/18, /*miso=*/19, /*mosi=*/23, /*ss=*/kSdCsPin);
    return SD.begin(kSdCsPin, sdSPI, kSdHz);
}

// Build an indentation prefix like "  |-- " etc (simple and readable)
static String IndentPrefix(int depth, bool isLast)
{
    String p;
    for (int i = 0; i < depth; ++i)
    {
        p += "  ";
    }
    p += isLast ? "\\- " : "|- ";
    return p;
}

// Recursively print directory tree
static void PrintTree(const char* dirPath, int depth, int& cursorY, int maxDepth)
{
    if (depth > maxDepth)
    {
        PrintLine(String(IndentPrefix(depth, true)) + "...", cursorY);
        return;
    }

    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory())
    {
        PrintLine(String("Open failed: ") + dirPath, cursorY);
        return;
    }

    // First pass: count entries (so we can mark last item)
    int entryCount = 0;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile())
    {
        ++entryCount;
        f.close();
    }
    dir.close();

    dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory())
    {
        PrintLine(String("Reopen failed: ") + dirPath, cursorY);
        return;
    }

    int index = 0;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile())
    {
        const bool isLast = (index == entryCount - 1);

        String name = String(f.name());
        // On ESP32 SD, f.name() is often full path already; keep it short for display
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;

        if (f.isDirectory())
        {
            PrintLine(IndentPrefix(depth, isLast) + "[" + base + "]", cursorY);

            // Recurse into this subdir using full path
            String childPath = name;
            if (!childPath.startsWith("/"))
            {
                // If name isn't full path, build it from parent
                childPath = String(dirPath);
                if (!childPath.endsWith("/")) childPath += "/";
                childPath += base;
            }

            f.close();
            PrintTree(childPath.c_str(), depth + 1, cursorY, maxDepth);
        }
        else
        {
            uint32_t sz = (uint32_t)f.size();
            // Keep the line short to fit font 2 on 320px
            PrintLine(IndentPrefix(depth, isLast) + base + "  (" + String(sz) + "B)", cursorY);
            f.close();
        }

        ++index;
    }

    dir.close();
}

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(kFont);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextWrap(false);

    if (!InitSd())
    {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("SD init failed");
        while (true) delay(1000);
    }

    int cursorY = kMarginY;
    NewPage(cursorY);

    PrintLine("SD Tree:", cursorY);
    PrintLine("/", cursorY);

    // Print entire SD tree starting at root
    // maxDepth prevents insane recursion on huge cards
    const int kMaxDepth = 8;
    PrintTree("/", 0, cursorY, kMaxDepth);

    PrintLine("Done.", cursorY);
}

void loop() {}
