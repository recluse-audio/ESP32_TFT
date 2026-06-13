/**
 * Made by Ryan Devens on 2026-02-26
 */

#pragma once
#include "GraphicsRenderer.h"

#include <TFT_eSPI.h>
#include <string>

/**
 * ESP32 implementation of GraphicsRenderer.
 * Uses TFT_eSPI for display output and PNGdec for PNG decoding.
 * drawSVG is a no-op (SVG rendering not supported on ESP32).
 */
class ESP32GraphicsRenderer : public GraphicsRenderer
{
public:
    explicit ESP32GraphicsRenderer(TFT_eSPI& tft);

    void setDataRoot(const std::string& root) { mDataRoot = root; }

    void beginContentArea(int x, int y, int w, int h) override;
    void endContentArea() override;

    void drawImage(const std::string& path) override;
    void drawImage(const std::string& path, int x, int y, int w, int h) override;
    void drawText(const std::string& path, int x, int y) override;
    void drawSVG(const std::string& path, int x, int y, int w = 0, int h = 0) override;
    void drawButton(const std::string& label, int x, int y, int w, int h) override;
    void drawFilledRect(int x, int y, int w, int h, int r, int g, int b, int alpha) override;
    void drawLabel(const std::string& text, int x, int y) override;
    void drawCenteredLabel(const std::string& text, int x, int y, int w, int h) override;

private:
    std::string sdPath(const std::string& path) const;
    void        drawPng(const std::string& fullPath, int originX, int originY);

    TFT_eSPI&   mTft;
    std::string mDataRoot;
};
