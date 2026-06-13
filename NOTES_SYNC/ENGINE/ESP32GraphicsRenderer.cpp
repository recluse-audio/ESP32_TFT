#include "ESP32GraphicsRenderer.h"
#include <PNGdec.h>
#include <SD.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// PNG decode state — global to avoid large stack allocations.
// sPngOriginX/Y are set before each decode to position the image on screen.
// sOutW/H, sSrcW/H, sOutOffX/Y are used for nearest-neighbour scaling.

static TFT_eSPI*  sDrawTft    = nullptr;
static PNG        sPng;
static File       sPngFile;
static uint16_t   sLineBuffer[320];
static int        sPngOriginX = 0;
static int        sPngOriginY = 0;
static int        sSrcW       = 320;
static int        sSrcH       = 240;
static int        sOutW       = 320;
static int        sOutH       = 240;
static int        sOutOffX    = 0;
static int        sOutOffY    = 0;
static int        sLastOutY   = -1;

static void* pngOpen(const char* filename, int32_t* size)
{
    sPngFile = SD.open(filename, FILE_READ);
    if (!sPngFile) { *size = 0; return nullptr; }
    *size = sPngFile.size();
    Serial.printf("[PNG] opened %s  size=%d\n", filename, (int)*size);
    return &sPngFile;
}

static void pngClose(void* handle)
{
    File* f = (File*)handle;
    if (f && *f) f->close();
}

static int32_t pngRead(PNGFILE* /*handle*/, uint8_t* buf, int32_t size)
{
    if (!sPngFile) return 0;
    return sPngFile.read(buf, size);
}

static int32_t pngSeek(PNGFILE* /*handle*/, int32_t pos)
{
    if (!sPngFile) return 0;
    return sPngFile.seek(pos) ? pos : 0;
}

static int pngDraw(PNGDRAW* pDraw)
{
    if (!sDrawTft || (int)pDraw->iWidth > 320) return 0;

    // Map source row → output row (nearest-neighbour vertical)
    int outY = (int)pDraw->y * sOutH / sSrcH;
    if (outY == sLastOutY) return 1;  // skip duplicate source rows
    sLastOutY = outY;

    sPng.getLineAsRGB565(pDraw, sLineBuffer, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);

    // Nearest-neighbour horizontal downsample in-place.
    // srcX = i * sSrcW / sOutW >= i when sSrcW >= sOutW, so reads never
    // overtake writes and no second buffer is needed.
    for (int i = 0; i < sOutW; i++)
        sLineBuffer[i] = sLineBuffer[i * sSrcW / sOutW];

    sDrawTft->pushImage(sPngOriginX + sOutOffX, sPngOriginY + sOutOffY + outY,
                        sOutW, 1, sLineBuffer);
    return 1;
}

// ---------------------------------------------------------------------------
// Resolve an image path: if the path has no file extension (asset folder),
// append the 320x240 variant filename.
static std::string resolveImagePath(const std::string& sdFullPath)
{
    size_t slash = sdFullPath.rfind('/');
    std::string name = (slash == std::string::npos) ? sdFullPath : sdFullPath.substr(slash + 1);
    if (name.find('.') == std::string::npos)
        return sdFullPath + "/" + name + "_320x240.png";
    return sdFullPath;
}

// ---------------------------------------------------------------------------

ESP32GraphicsRenderer::ESP32GraphicsRenderer(TFT_eSPI& tft)
: mTft(tft)
{
    sDrawTft = &tft;
}

std::string ESP32GraphicsRenderer::sdPath(const std::string& path) const
{
    if (!path.empty() && path[0] == '/')
        return mDataRoot + path;
    return path;
}

// ---------------------------------------------------------------------------

void ESP32GraphicsRenderer::beginContentArea(int x, int y, int w, int h)
{
    // vpDatum=false keeps all draw coordinates in absolute screen space.
    mTft.setViewport(x, y, w, h, false);
}

void ESP32GraphicsRenderer::endContentArea()
{
    mTft.resetViewport();
}

// ---------------------------------------------------------------------------

void ESP32GraphicsRenderer::drawPng(const std::string& fullPath, int originX, int originY)
{
    Serial.printf("[IMG] drawPng: %s  origin=(%d,%d)\n", fullPath.c_str(), originX, originY);
    sPngOriginX = originX;
    sPngOriginY = originY;

    int rc = sPng.open(fullPath.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    Serial.printf("[IMG] sPng.open rc=%d\n", rc);

    if (rc == PNG_SUCCESS)
    {
        mTft.startWrite();
        int dec = sPng.decode(nullptr, 0);
        mTft.endWrite();
        sPng.close();
        Serial.printf("[IMG] decode rc=%d\n", dec);
    }
}

void ESP32GraphicsRenderer::drawImage(const std::string& path)
{
    mTft.fillScreen(TFT_BLACK);
    sSrcW = sOutW = 320;  sSrcH = sOutH = 240;
    sOutOffX = sOutOffY = 0;  sLastOutY = -1;
    drawPng(resolveImagePath(sdPath(path)), 0, 0);
}

void ESP32GraphicsRenderer::drawImage(const std::string& path, int x, int y, int w, int h)
{
    std::string fullPath = resolveImagePath(sdPath(path));
    Serial.printf("[IMG] drawImage(bounded): %s at=(%d,%d) bounds=(%d,%d)\n",
                  fullPath.c_str(), x, y, w, h);

    int rc = sPng.open(fullPath.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (rc != PNG_SUCCESS) return;

    sSrcW = sPng.getWidth();
    sSrcH = sPng.getHeight();
    float scale = std::min((float)w / sSrcW, (float)h / sSrcH);
    sOutW    = (int)(sSrcW * scale);
    sOutH    = (int)(sSrcH * scale);
    sOutOffX = (w - sOutW) / 2;
    sOutOffY = (h - sOutH) / 2;
    sLastOutY   = -1;
    sPngOriginX = x;
    sPngOriginY = y;

    mTft.startWrite();
    int dec = sPng.decode(nullptr, 0);
    mTft.endWrite();
    sPng.close();
    Serial.printf("[IMG] decode rc=%d  scale=%.3f out=(%d,%d)\n", dec, scale, sOutW, sOutH);
}

// ---------------------------------------------------------------------------

void ESP32GraphicsRenderer::drawText(const std::string& path, int x, int y)
{
    File file = SD.open(sdPath(path).c_str(), FILE_READ);
    if (!file) return;

    mTft.setTextWrap(false);
    int curY = y - mScrollOffset;
    String line;

    while (file.available())
    {
        char c = file.read();
        if (c != '\n') { line += c; continue; }

        int hashes = 0;
        while (hashes < (int)line.length() && line[hashes] == '#') hashes++;
        String display = (hashes > 0) ? line.substring(hashes + 1) : line;

        uint8_t sz    = (hashes == 1) ? 3 : (hashes == 2) ? 2 : 1;
        int     lineH = sz * 8 + 4;

        if (curY + lineH > y && curY < y + 240) // skip lines outside the visible band
        {
            mTft.setTextSize(sz);
            mTft.setTextColor(hashes > 0 ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
            mTft.drawString(display, x + 4, curY + 4);
        }

        curY += lineH;
        line = "";
    }
    file.close();
}

// ---------------------------------------------------------------------------

void ESP32GraphicsRenderer::drawSVG(const std::string& /*path*/,
                                    int /*x*/, int /*y*/,
                                    int /*w*/, int /*h*/) {}

// ---------------------------------------------------------------------------

void ESP32GraphicsRenderer::drawButton(const std::string& label, int x, int y, int w, int h)
{
    mTft.fillRect(x, y, w, h, TFT_BLACK);
    mTft.drawRect(x, y, w, h, TFT_WHITE);
    mTft.setTextSize(1);
    mTft.setTextColor(TFT_WHITE, TFT_BLACK);
    int textX = x + (w - (int)label.size() * 6) / 2;
    int textY = y + (h - 8) / 2;
    mTft.drawString(label.c_str(), textX, textY);
}

// ---------------------------------------------------------------------------

void ESP32GraphicsRenderer::drawFilledRect(int x, int y, int w, int h,
                                           int r, int g, int b, int /*alpha*/)
{
    mTft.fillRect(x, y, w, h, mTft.color565(r, g, b));
}

void ESP32GraphicsRenderer::drawLabel(const std::string& text, int x, int y)
{
    mTft.setTextSize(1);
    mTft.setTextColor(TFT_WHITE, TFT_BLACK);
    mTft.drawString(text.c_str(), x, y);
}

void ESP32GraphicsRenderer::drawCenteredLabel(const std::string& text,
                                              int x, int y, int w, int h)
{
    int textW = (int)text.size() * 6; // size-1 font: ~6px per char
    int textX = x + (w - textW) / 2;
    int textY = y + (h - 8) / 2;     // size-1 font: 8px tall
    mTft.setTextSize(1);
    mTft.setTextColor(TFT_WHITE, TFT_BLACK);
    mTft.drawString(text.c_str(), textX, textY);
}
