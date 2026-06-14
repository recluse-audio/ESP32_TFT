/**
 * NotesBrowser — Phase 1 thin notes browser for NOTES_SYNC.
 * Made by Ryan Devens on 2026-06-13
 *
 * A touch-driven directory browser over the SD card's NOTES mirror.
 *  - Lists the current folder (sub-folders first, then files, A-Z).
 *  - Tap a folder row to descend; tap a file row to open it.
 *  - Text files (.md/.txt/.org/...) render via the vendored renderer's
 *    drawText(); .png renders via drawImage(); anything else shows a
 *    "cannot display" placeholder (richer file types are Phase 5).
 *  - Footer buttons: BROWSE mode [UP] [^] [v]; VIEW mode [BACK] [^] [v].
 *
 * Read-only: this class never writes the SD card. The receive server
 * (NotesSyncServer) is the only writer. After a WiFi sync, tap UP at the
 * root (or re-enter a folder) to re-list and see newly arrived files.
 */
#pragma once

#include <TFT_eSPI.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include <string.h>

#include "ENGINE/ESP32GraphicsRenderer.h"

class NotesBrowser
{
public:
    NotesBrowser(TFT_eSPI& tft, ESP32GraphicsRenderer& renderer, const char* dataRoot)
    : mTft(tft), mRenderer(renderer), mRoot(dataRoot), mCurPath(dataRoot)
    {
    }

    // List the root folder and request a first draw.
    void begin()
    {
        listDir();
        mDirty = true;
    }

    // Call every loop(): redraws only when something changed.
    void tick()
    {
        if (!mDirty) return;
        if (mMode == BROWSING) drawBrowsing();
        else                   drawViewing();
        mDirty = false;
    }

    // Leading-edge tap at screen coords (already mapped to 320x240).
    void handleTap(int x, int y)
    {
        if (mMode == BROWSING) tapBrowsing(x, y);
        else                   tapViewing(x, y);
    }

    // Re-list the current folder (e.g. after a WiFi sync). Browsing mode only.
    void refresh()
    {
        if (mMode != BROWSING) return;
        listDir();
        mDirty = true;
    }

private:
    struct Entry
    {
        String name;
        bool   isDir;
    };

    enum Mode { BROWSING, VIEWING };

    static const int TOP_H    = 16;   // path bar height
    static const int LIST_Y   = 18;   // first row top
    static const int ROW_H    = 24;   // row height (fits size-2 text)
    static const int VIS_ROWS = 8;    // (FOOTER_Y - LIST_Y) / ROW_H
    static const int FOOTER_Y = 218;  // footer button row top
    static const int FOOTER_H = 22;

    // --- helpers ----------------------------------------------------------

    static String basename(const String& full)
    {
        int s = full.lastIndexOf('/');
        return (s < 0) ? full : full.substring(s + 1);
    }

    static String extOf(const String& name)
    {
        int d = name.lastIndexOf('.');
        if (d < 0) return "";
        String e = name.substring(d + 1);
        e.toLowerCase();
        return e;
    }

    static bool isTextExt(const String& e)
    {
        static const char* kText[] = {
            "md", "markdown", "txt", "org", "csv", "log", "json",
            "yml", "yaml", "ini", "cfg", "conf", "c", "h", "cpp",
            "hpp", "py", "sh", "js", "css", "html", "xml"
        };
        for (const char* t : kText)
            if (e == t) return true;
        return false;
    }

    // --- directory listing ------------------------------------------------

    void listDir()
    {
        mEntries.clear();
        File dir = SD.open(mCurPath.c_str());
        if (dir)
        {
            for (File e = dir.openNextFile(); e; e = dir.openNextFile())
            {
                String n = basename(String(e.name()));
                bool   d = e.isDirectory();
                e.close();
                if (n.length() == 0 || n[0] == '.')   // hide dotfiles/.staging/.tmp
                    continue;
                mEntries.push_back({ n, d });
            }
            dir.close();
        }

        std::sort(mEntries.begin(), mEntries.end(),
                  [](const Entry& a, const Entry& b)
                  {
                      if (a.isDir != b.isDir) return a.isDir;   // folders first
                      String an = a.name; an.toLowerCase();
                      String bn = b.name; bn.toLowerCase();
                      return an.compareTo(bn) < 0;
                  });

        int maxScroll = (int)mEntries.size() - VIS_ROWS;
        if (maxScroll < 0) maxScroll = 0;
        if (mScroll > maxScroll) mScroll = maxScroll;
        if (mScroll < 0) mScroll = 0;
    }

    // --- drawing ----------------------------------------------------------

    void drawFooter(const char* left)
    {
        mRenderer.drawButton(left, 2,   FOOTER_Y, 100, FOOTER_H);
        mRenderer.drawButton("^",  110, FOOTER_Y, 100, FOOTER_H);
        mRenderer.drawButton("v",  218, FOOTER_Y, 100, FOOTER_H);
    }

    void drawBrowsing()
    {
        mTft.fillScreen(TFT_BLACK);

        String rel = mCurPath.substring(strlen(mRoot));
        if (rel.length() == 0) rel = "/";
        mTft.setTextSize(1);
        mTft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        mTft.drawString(rel.c_str(), 2, 4);
        mTft.drawFastHLine(0, TOP_H, 320, TFT_DARKGREY);

        if (mEntries.empty())
        {
            mTft.setTextSize(2);
            mTft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            mTft.drawString("(empty)", 8, LIST_Y + 8);
        }

        for (int i = 0; i < VIS_ROWS; i++)
        {
            int idx = mScroll + i;
            if (idx >= (int)mEntries.size()) break;
            const Entry& en = mEntries[idx];
            int y = LIST_Y + i * ROW_H;

            mTft.drawRect(1, y, 318, ROW_H - 2, TFT_NAVY);
            mTft.setTextSize(2);
            mTft.setTextColor(en.isDir ? TFT_CYAN : TFT_WHITE, TFT_BLACK);

            String lbl = (en.isDir ? "/" : " ") + en.name;
            if (lbl.length() > 25) lbl = lbl.substring(0, 24) + "~";
            mTft.drawString(lbl.c_str(), 6, y + 4);
        }

        drawFooter("UP");
    }

    void drawViewing()
    {
        mTft.fillScreen(TFT_BLACK);

        String ext  = extOf(mViewName);
        bool   text = isTextExt(ext);
        bool   png  = (ext == "png");

        // Clip body to above the footer so long text/images never overdraw it.
        mRenderer.beginContentArea(0, 0, 320, FOOTER_Y);
        std::string rel(mViewRel.c_str());
        if (text)
        {
            mRenderer.setScrollOffset(mViewScroll);
            mRenderer.drawText(rel, 0, 0);
        }
        else if (png)
        {
            mRenderer.drawImage(rel, 0, 0, 320, FOOTER_Y);
        }
        mRenderer.endContentArea();

        if (!text && !png)
        {
            mTft.setTextSize(2);
            mTft.setTextColor(TFT_ORANGE, TFT_BLACK);
            mTft.drawString("Cannot display", 8, 90);
            mTft.setTextSize(1);
            mTft.setTextColor(TFT_WHITE, TFT_BLACK);
            mTft.drawString(mViewName.c_str(), 8, 120);
        }

        drawFooter("BACK");
    }

    // --- tap handling -----------------------------------------------------

    void scrollBy(int delta)
    {
        int maxScroll = (int)mEntries.size() - VIS_ROWS;
        if (maxScroll < 0) maxScroll = 0;
        mScroll += delta;
        if (mScroll > maxScroll) mScroll = maxScroll;
        if (mScroll < 0) mScroll = 0;
        mDirty = true;
    }

    void goUp()
    {
        if (mCurPath == mRoot)   // already at root: treat UP as refresh
        {
            listDir();
            mDirty = true;
            return;
        }
        int s = mCurPath.lastIndexOf('/');
        mCurPath = mCurPath.substring(0, s);
        if (mCurPath.length() < (int)strlen(mRoot))
            mCurPath = mRoot;
        mScroll = 0;
        listDir();
        mDirty = true;
    }

    void open(int idx)
    {
        const Entry& en = mEntries[idx];
        if (en.isDir)
        {
            mCurPath = mCurPath + "/" + en.name;
            mScroll  = 0;
            listDir();
            mDirty = true;
        }
        else
        {
            String full = mCurPath + "/" + en.name;
            mViewRel    = full.substring(strlen(mRoot));   // data-root-relative, leading '/'
            mViewName   = en.name;
            mViewScroll = 0;
            mMode       = VIEWING;
            mDirty      = true;
        }
    }

    void tapBrowsing(int x, int y)
    {
        if (y < TOP_H) return;
        if (y >= FOOTER_Y)
        {
            if      (x < 106) goUp();
            else if (x < 214) scrollBy(-VIS_ROWS);
            else              scrollBy(VIS_ROWS);
            return;
        }
        int row = (y - LIST_Y) / ROW_H;
        int idx = mScroll + row;
        if (idx >= 0 && idx < (int)mEntries.size())
            open(idx);
    }

    void tapViewing(int x, int y)
    {
        if (y < FOOTER_Y) return;
        if (x < 106)
        {
            mMode  = BROWSING;
            mDirty = true;
        }
        else if (x < 214)
        {
            mViewScroll -= 120;
            if (mViewScroll < 0) mViewScroll = 0;
            mDirty = true;
        }
        else
        {
            mViewScroll += 120;
            mDirty = true;
        }
    }

    // --- state ------------------------------------------------------------

    TFT_eSPI&             mTft;
    ESP32GraphicsRenderer& mRenderer;
    const char*           mRoot;
    String                mCurPath;
    std::vector<Entry>    mEntries;
    int                   mScroll     = 0;
    Mode                  mMode       = BROWSING;
    String                mViewRel;
    String                mViewName;
    int                   mViewScroll = 0;
    bool                  mDirty      = true;
};
