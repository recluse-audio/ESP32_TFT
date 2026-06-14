/**
 * NOTES_SYNC — ESP32 startup program (Phase 0)
 * Made by Ryan Devens on 2026-06-13
 *
 * Goal of this phase: bring up the display, SD card, and touch, point the
 * vendored ESP32FileOperator at the SD "/NOTES" folder, and list that folder
 * tree to the serial monitor. No WiFi and no writing yet.
 *
 * Board: ESP32-WROOM-32E with ILI9341 2.8" TFT (E32R28T).
 * Libraries: TFT_eSPI (configured via CONFIG/User_Setup_TFT_eSPI.h), SD, SPI, PNGdec.
 *
 * Vendored engine files (copies, see ENGINE/VENDOR_TODO.md):
 *   ENGINE/FileOperator.h, ENGINE/ESP32FileOperator.h
 *   ENGINE/GraphicsRenderer.h, ENGINE/ESP32GraphicsRenderer.h/.cpp
 *   (the .cpp is compiled in-tree via NOTES_SYNC_Sources.cpp)
 */

// Display config comes from the TFT_eSPI library's own User_Setup.h, NOT a
// per-sketch include. On a fresh machine the per-sketch
// "#define USER_SETUP_LOADED + #include ../CONFIG/..." approach left the panel
// a solid WHITE screen (config not honored), while installing the board config
// into the library folder works (matches Basic_ESP32_TFT_Test.ino). Setup:
// copy CONFIG/User_Setup_TFT_eSPI.h to <Arduino>/libraries/TFT_eSPI/User_Setup.h.
#include <TFT_eSPI.h>

#include <SPI.h>
#include <SD.h>

#include "ENGINE/ESP32FileOperator.h"
#include "ENGINE/ESP32GraphicsRenderer.h"
#include "NotesSyncServer.h"

// --- Backlight: this board needs pin 21 driven HIGH manually before init ---
// (TFT_eSPI's automatic TFT_BL handling does not light it on this hardware;
//  matches Basic_ESP32_TFT_Test.ino which is known to work.)
static const int BL_PIN = 21;

// --- Data root on the SD card: the mirror of the NOTES vault lives here ---
static const char* DATA_ROOT = "/NOTES";

// --- WiFi access point the desktop joins to push notes (Phase 2) ----------
static const char* AP_SSID = "ESP32_NOTES";
static const char* AP_PASS = "notesync123";   // >= 8 chars for WPA2; local-only

// --- SD pin config (VSPI — separate bus from the TFT on HSPI) -------------
static const int SD_CS   =  5;
static const int SD_SCK  = 18;
static const int SD_MISO = 19;
static const int SD_MOSI = 23;

SPIClass sdSPI(VSPI);

// --- Touch pin config (XPT2046, software SPI) — same wiring as FileGame ----
static const int T_CLK = 25;
static const int T_DIN = 32;
static const int T_DO  = 39;   // input-only GPIO
static const int T_CS  = 33;
static const int T_IRQ = 36;   // input-only GPIO

// Landscape (setRotation 1): the XPT2046 axes are transposed.
static const int TOUCH_RAW_X_TOP    = 169;
static const int TOUCH_RAW_X_BOTTOM = 1886;
static const int TOUCH_RAW_Y_LEFT   = 161;
static const int TOUCH_RAW_Y_RIGHT  = 1834;

// --- Globals --------------------------------------------------------------
static TFT_eSPI                gTft;
static ESP32FileOperator       gFileOperator;
static ESP32GraphicsRenderer*  gRenderer = nullptr;
static NotesSyncServer         gSync;

// --- Touch (XPT2046 software SPI) -----------------------------------------

static void touchInit()
{
    pinMode(T_CLK, OUTPUT);
    pinMode(T_DIN, OUTPUT);
    pinMode(T_DO,  INPUT);
    pinMode(T_CS,  OUTPUT);
    pinMode(T_IRQ, INPUT);
    digitalWrite(T_CS,  HIGH);
    digitalWrite(T_CLK, LOW);
}

static uint16_t xpt2046Read(uint8_t cmd)
{
    uint16_t val = 0;
    for (int i = 7; i >= 0; i--)
    {
        digitalWrite(T_DIN, (cmd >> i) & 1);
        digitalWrite(T_CLK, HIGH); delayMicroseconds(1);
        digitalWrite(T_CLK, LOW);  delayMicroseconds(1);
    }
    for (int i = 11; i >= 0; i--)
    {
        digitalWrite(T_CLK, HIGH); delayMicroseconds(1);
        val |= (uint16_t)digitalRead(T_DO) << i;
        digitalWrite(T_CLK, LOW);  delayMicroseconds(1);
    }
    return val;
}

static bool touchRead(int& screenX, int& screenY)
{
    if (digitalRead(T_IRQ) == HIGH) return false;

    digitalWrite(T_CS, LOW);
    delayMicroseconds(10);
    uint16_t rawX = xpt2046Read(0xD0); // X channel
    uint16_t rawY = xpt2046Read(0x90); // Y channel
    digitalWrite(T_CS, HIGH);

    if (digitalRead(T_IRQ) == HIGH) return false; // lifted during read

    // Axes transposed in landscape: raw Y → screen X, raw X → screen Y
    screenX = constrain(map(rawY, TOUCH_RAW_Y_LEFT, TOUCH_RAW_Y_RIGHT, 0, 319), 0, 319);
    screenY = constrain(map(rawX, TOUCH_RAW_X_TOP,  TOUCH_RAW_X_BOTTOM, 0, 239), 0, 239);
    return true;
}

// --- Recursively dump the SD tree under a directory to Serial -------------
// Returns the number of regular files seen.
static int dumpTree(File dir, int depth)
{
    int fileCount = 0;
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry) break;

        for (int i = 0; i < depth; i++) Serial.print("  ");
        if (entry.isDirectory())
        {
            Serial.printf("[dir]  %s\n", entry.name());
            fileCount += dumpTree(entry, depth + 1);
        }
        else
        {
            Serial.printf("       %s  (%u bytes)\n", entry.name(), (unsigned)entry.size());
            fileCount++;
        }
        entry.close();
    }
    return fileCount;
}

// -------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n[NOTES_SYNC] Phase 0 startup.");

    // Backlight ON before anything else, or the screen stays dark
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, HIGH);
    delay(50);

    // Display
    gTft.init();
    gTft.setRotation(1);
    gTft.fillScreen(TFT_BLACK);
    gTft.setTextSize(2);
    gTft.setTextColor(TFT_WHITE, TFT_BLACK);
    gTft.drawString("NOTES_SYNC", 10, 10);

    // SD card — explicit VSPI so it does not clash with TFT on HSPI
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    bool sdOk = false;
    int freqs[] = { 25000000, 10000000, 4000000 };
    for (int f : freqs)
    {
        if (SD.begin(SD_CS, sdSPI, f)) { sdOk = true; break; }
        delay(100);
    }
    if (!sdOk)
    {
        gTft.setTextSize(2);
        gTft.setTextColor(TFT_RED, TFT_BLACK);
        gTft.drawString("SD init failed", 10, 40);
        Serial.println("[NOTES_SYNC] SD init failed — halting.");
        while (true) {}
    }
    Serial.println("[NOTES_SYNC] SD ready.");

    // Point the vendored file operator at the NOTES root on the SD card
    gFileOperator.setDataRoot(DATA_ROOT);
    gRenderer = new ESP32GraphicsRenderer(gTft);
    gRenderer->setDataRoot(DATA_ROOT);

    // Full recursive dump of the NOTES tree to Serial
    Serial.printf("[NOTES_SYNC] Listing SD tree under %s:\n", DATA_ROOT);
    File root = SD.open(DATA_ROOT);
    int total = 0;
    if (!root)
    {
        Serial.printf("[NOTES_SYNC] %s not found on SD card (create it, then re-run).\n", DATA_ROOT);
    }
    else
    {
        total = dumpTree(root, 0);
        root.close();
    }
    Serial.printf("[NOTES_SYNC] Total files under %s: %d\n", DATA_ROOT, total);

    // Prove the vendored ESP32FileOperator API links and reads the top level
    std::vector<std::string> top = gFileOperator.listDirectory("/");
    Serial.printf("[NOTES_SYNC] FileOperator.listDirectory(\"/\") -> %d entries\n", (int)top.size());

    // Show the count on screen so the board reports liveness without a serial cable
    gRenderer->drawLabel(std::string("NOTES files: ") + std::to_string(total), 10, 50);

    touchInit();
    Serial.println("[NOTES_SYNC] Touch ready.");

    // --- WiFi access point + receive server (Phase 2) ---------------------
    IPAddress ip = gSync.begin(AP_SSID, AP_PASS, DATA_ROOT);
    Serial.printf("[NOTES_SYNC] SoftAP up: SSID=%s  IP=%s\n", AP_SSID, ip.toString().c_str());
    gRenderer->drawLabel(std::string("WiFi: ") + AP_SSID, 10, 70);
    gRenderer->drawLabel(std::string("IP: ") + ip.toString().c_str(), 10, 90);
    gRenderer->drawLabel("Run desktop sync to push notes", 10, 120);
    Serial.println("[NOTES_SYNC] Phase 2 setup complete. Waiting for desktop pushes.");
}

static bool gPrevTouched = false;
static int  gLastReceived = -1;
static int  gLastDeleted  = -1;

// -------------------------------------------------------------------------
void loop()
{
    // Service the web server (desktop file pushes).
    gSync.handle();

    // Touch — report taps (the on-screen Sync button comes in Phase 3).
    int tx, ty;
    bool touched = touchRead(tx, ty);
    if (touched && !gPrevTouched)
        Serial.printf("[NOTES_SYNC] tap (%d, %d)\n", tx, ty);
    gPrevTouched = touched;

    // Live receive/delete counters on screen as files arrive.
    int rec = gSync.filesReceived();
    int del = gSync.filesDeleted();
    if (rec != gLastReceived || del != gLastDeleted)
    {
        gLastReceived = rec;
        gLastDeleted  = del;
        gRenderer->drawFilledRect(0, 150, 320, 20, 0, 0, 0, 255);
        gRenderer->drawLabel(std::string("recv: ") + std::to_string(rec) +
                             "  del: " + std::to_string(del), 10, 150);
        Serial.printf("[NOTES_SYNC] progress: recv=%d del=%d\n", rec, del);
    }
}
