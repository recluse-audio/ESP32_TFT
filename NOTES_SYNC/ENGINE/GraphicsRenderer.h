/**
 * Made by Ryan Devens on 2026-02-26
 */

#pragma once
#include <string>
#include <utility>
#include <vector>

/**
 * Abstract base class for platform-specific drawing primitives.
 * Platform implementations handle file I/O and rendering internally.
 * nanosvg rasterization and texture caching are internal platform details.
 */
class GraphicsRenderer
{
public:
    virtual ~GraphicsRenderer() = default;

    void setScrollOffset(int offset) { mScrollOffset = offset; }

    virtual void beginContentArea(int x, int y, int w, int h) {}
    virtual void endContentArea() {}

protected:
    int mScrollOffset = 0;

public:
    /**
     * Load and render an image asset from the given data-root-relative path,
     * filling the full game canvas (320x240).
     */
    virtual void drawImage(const std::string& path) = 0;

    /**
     * Load and render an image asset scaled to fit within the given game-space
     * bounds, preserving aspect ratio and centering within the area.
     */
    virtual void drawImage(const std::string& path, int x, int y, int w, int h) {}

    /**
     * Load and render a text/markdown asset from the given data-root-relative path
     * at the given screen coordinates.
     */
    virtual void drawText(const std::string& path, int x, int y) = 0;

    /**
     * Load and render an SVG asset from the given path at the given
     * screen coordinates.
     */
    virtual void drawSVG(const std::string& path, int x, int y, int w = 0, int h = 0) = 0;

    /**
     * Draw a labeled button rectangle at the given screen coordinates.
     * Used for programmatically rendered menus (no image assets).
     */
    virtual void drawButton(const std::string& label, int x, int y, int w, int h) = 0;

    /**
     * Draw a rectangle outline at the given game-space coordinates.
     * Used by the zone display debug overlay. Default is a no-op.
     */
    virtual void drawRect(int x, int y, int w, int h) {}

    /**
     * Draw a filled rectangle with an explicit RGBA color.
     * alpha is 0 (transparent) – 255 (opaque). Default is a no-op.
     */
    virtual void drawFilledRect(int x, int y, int w, int h,
                                int r, int g, int b, int alpha) {}

    /**
     * Draw a text string directly at the given screen coordinates.
     * Unlike drawText(), the string is the literal content to render, not a path.
     * Default is a no-op.
     */
    virtual void drawLabel(const std::string& text, int x, int y) {}

    /**
     * Draw a text string centered within the given rectangle, with no background.
     * Default is a no-op.
     */
    virtual void drawCenteredLabel(const std::string& text, int x, int y, int w, int h) {}

    /**
     * Draw a closed polygon outline through the given game-space points.
     * Used by the zone display debug overlay. Default is a no-op.
     */
    virtual void drawPolygon(const std::vector<std::pair<int, int>>& points) {}
};
