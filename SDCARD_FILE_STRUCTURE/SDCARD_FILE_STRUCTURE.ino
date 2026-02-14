// SDCARD_FILE_BROWSER_TOUCH_XPT2046.ino
// - SD file browser (one directory at a time)
// - Tap to select/highlight
// - Tap selected directory again to enter
// - Tap ".." to go up
//
// SD: VSPI SCK=18 MISO=19 MOSI=23 CS=5 (your proven working config)
// TFT: your existing TFT_eSPI config (already working)
// TOUCH: XPT2046_Touchscreen on HSPI pins that match your TFT bus (SCK=14 MISO=12 MOSI=13)
//        You MUST set kTouchCsPin to your board's touch CS (T_CS / TP_CS)

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ---------------- SD ----------------
static const int kSdCsPin = 5;
static const uint32_t kSdHz = 4000000;
SPIClass sdSPI(VSPI);

// ---------------- TFT ----------------
TFT_eSPI tft;
static const int kW = 320;
static const int kH = 240;

// ---------------- TOUCH (XPT2046) ----------------
// CHANGE THIS to your real touch CS pin (board silkscreen: T_CS / TP_CS)
static const int kTouchCsPin = 22;
static const int kTouchIrqPin = 255; // 255 = none
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(kTouchCsPin, kTouchIrqPin);

// Touch calibration placeholders (you WILL tune these)
// Read raw p.x/p.y in Serial and set min/max; then adjust swap/invert if needed.
static int kTouchXMin = 200;
static int kTouchXMax = 3800;
static int kTouchYMin = 200;
static int kTouchYMax = 3800;

static const bool kTouchSwapXY = false;
static const bool kTouchInvertX = false;
static const bool kTouchInvertY = false;

// ---------------- UI ----------------
static const int kHeaderH = 24;
static const int kRowH = 18;
static const int kListTop = kHeaderH;
static const int kRowsPerPage = (kH - kListTop) / kRowH;

static const int kMaxEntries = 256;
static const int kNameMax = 64;

struct Entry
{
    char name[kNameMax];
    bool isDir;
    uint32_t size;
};

static Entry gEntries[kMaxEntries];
static int gEntryCount = 0;

static String gCwd = "/";
static int gSelected = -1;
static int gScroll = 0;

// ---------------- Helpers ----------------
static bool InitSd()
{
    sdSPI.begin(/*sck=*/18, /*miso=*/19, /*mosi=*/23, /*ss=*/kSdCsPin);
    return SD.begin(kSdCsPin, sdSPI, kSdHz);
}

static bool InitTouch()
{
    // Match your TFT bus pins from User_Setup: SCK=14 MISO=12 MOSI=13
    touchSPI.begin(/*sck=*/14, /*miso=*/12, /*mosi=*/13, /*ss=*/kTouchCsPin);
    return ts.begin(touchSPI);
}

static void DrawHeader()
{
    tft.fillRect(0, 0, kW, kHeaderH, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setCursor(2, 4);
    tft.print(gCwd);

    tft.setCursor(250, 4);
    tft.print("tap");
}

static void DrawRow(int visibleRow, int entryIndex)
{
    const int y = kListTop + visibleRow * kRowH;
    const bool isSelected = (entryIndex == gSelected);

    const uint16_t bg = isSelected ? TFT_YELLOW : TFT_BLACK;
    const uint16_t fg = isSelected ? TFT_BLACK  : TFT_WHITE;

    tft.fillRect(0, y, kW, kRowH, bg);
    tft.setTextColor(fg, bg);

    tft.setCursor(2, y + 2);

    if (gEntries[entryIndex].isDir)
    {
        tft.print("[");
        tft.print(gEntries[entryIndex].name);
        tft.print("]");
    }
    else
    {
        tft.print(gEntries[entryIndex].name);
    }

    if (!gEntries[entryIndex].isDir)
    {
        tft.setCursor(230, y + 2);
        tft.print(gEntries[entryIndex].size);
        tft.print("B");
    }
}

static void DrawList()
{
    tft.fillRect(0, kListTop, kW, kH - kListTop, TFT_BLACK);

    const int start = gScroll;
    const int end = min(gEntryCount, gScroll + kRowsPerPage);

    for (int i = start; i < end; ++i)
    {
        DrawRow(i - start, i);
    }
}

static void Render()
{
    DrawHeader();
    DrawList();
}

static void ClampScroll()
{
    if (gScroll < 0) gScroll = 0;
    const int maxScroll = max(0, gEntryCount - kRowsPerPage);
    if (gScroll > maxScroll) gScroll = maxScroll;
}

static void EnsureVisible(int idx)
{
    if (idx < gScroll) gScroll = idx;
    if (idx >= gScroll + kRowsPerPage) gScroll = idx - (kRowsPerPage - 1);
    ClampScroll();
}

static int LoadDirectory(const String& path)
{
    gEntryCount = 0;
    gSelected = -1;
    gScroll = 0;

    File dir = SD.open(path.c_str());
    if (!dir || !dir.isDirectory())
    {
        return 0;
    }

    // ".." entry except at root
    if (path != "/" && gEntryCount < kMaxEntries)
    {
        strncpy(gEntries[gEntryCount].name, "..", kNameMax);
        gEntries[gEntryCount].name[kNameMax - 1] = '\0';
        gEntries[gEntryCount].isDir = true;
        gEntries[gEntryCount].size = 0;
        ++gEntryCount;
    }

    for (File f = dir.openNextFile(); f && gEntryCount < kMaxEntries; f = dir.openNextFile())
    {
        String full = String(f.name());
        int slash = full.lastIndexOf('/');
        String base = (slash >= 0) ? full.substring(slash + 1) : full;

        strncpy(gEntries[gEntryCount].name, base.c_str(), kNameMax);
        gEntries[gEntryCount].name[kNameMax - 1] = '\0';
        gEntries[gEntryCount].isDir = f.isDirectory();
        gEntries[gEntryCount].size = (uint32_t)f.size();
        ++gEntryCount;

        f.close();
    }

    dir.close();
    ClampScroll();
    return gEntryCount;
}

static String JoinPath(const String& dir, const String& base)
{
    if (dir == "/") return "/" + base;
    return dir + "/" + base;
}

static void NavigateIntoSelected()
{
    if (gSelected < 0 || gSelected >= gEntryCount) return;
    if (!gEntries[gSelected].isDir) return;

    String name = String(gEntries[gSelected].name);

    if (name == "..")
    {
        if (gCwd == "/") return;
        int slash = gCwd.lastIndexOf('/');
        gCwd = (slash <= 0) ? "/" : gCwd.substring(0, slash);
    }
    else
    {
        gCwd = JoinPath(gCwd, name);
    }

    LoadDirectory(gCwd);
    Render();
}

static int HitTestEntry(int16_t ty)
{
    if (ty < kListTop || ty >= kH) return -1;
    const int row = (ty - kListTop) / kRowH;
    const int idx = gScroll + row;
    if (idx < 0 || idx >= gEntryCount) return -1;
    return idx;
}

static bool TouchToScreen(int16_t& outX, int16_t& outY)
{
    if (!ts.touched()) return false;

    TS_Point p = ts.getPoint();

    int rx = p.x;
    int ry = p.y;

    if (kTouchSwapXY)
    {
        int tmp = rx; rx = ry; ry = tmp;
    }

    rx = max(kTouchXMin, min(kTouchXMax, rx));
    ry = max(kTouchYMin, min(kTouchYMax, ry));

    int16_t x = (int16_t)map(rx, kTouchXMin, kTouchXMax, 0, kW - 1);
    int16_t y = (int16_t)map(ry, kTouchYMin, kTouchYMax, 0, kH - 1);

    if (kTouchInvertX) x = (kW - 1) - x;
    if (kTouchInvertY) y = (kH - 1) - y;

    outX = x;
    outY = y;
    return true;
}

// ---------------- Arduino ----------------
void setup()
{
    Serial.begin(115200);
    delay(200);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextWrap(false);

    if (!InitSd())
    {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("SD init failed");
        while (true) delay(1000);
    }

    const bool touchOk = InitTouch();
    Serial.printf("Touch init: %s (CS=%d)\n", touchOk ? "OK" : "FAIL", kTouchCsPin);

    LoadDirectory(gCwd);
    Render();

    if (!touchOk)
    {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(2, 220);
        tft.print("Touch init FAIL");
    }
}

void loop()
{
    int16_t tx = 0;
    int16_t ty = 0;

    if (TouchToScreen(tx, ty))
    {
        delay(120);

        const int idx = HitTestEntry(ty);
        if (idx >= 0)
        {
            if (gSelected == idx)
            {
                NavigateIntoSelected();
            }
            else
            {
                gSelected = idx;
                EnsureVisible(gSelected);
                Render();
            }
        }

        // wait for release-ish
        uint32_t start = millis();
        while (ts.touched() && (millis() - start) < 400)
        {
            delay(10);
        }
    }
}
