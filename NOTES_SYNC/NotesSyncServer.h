/**
 * NotesSyncServer — ESP32 SoftAP + HTTP receiver for NOTES_SYNC (Phase 2)
 * Made by Ryan Devens on 2026-06-13
 *
 * The board hosts its own WiFi access point and a small web server. The desktop
 * push script (SCRIPTS/sync_notes_to_esp32.py) sends changed files here; this
 * class writes them to the SD card under a single root (default "/NOTES").
 *
 * Safety model:
 *  - Only ever WRITES the SD card; it never touches the desktop.
 *  - Every upload streams to ONE temp file, then is renamed into place, so an
 *    interrupted transfer leaves the previous copy intact.
 *  - All paths are forced under mRoot; nothing else on the card is touched.
 *
 * Endpoints:
 *  GET  /manifest            -> the JSON manifest the desktop last stored ({} if none)
 *  POST /upload?path=/REL    -> multipart file body; written to mRoot + /REL
 *  POST /delete?path=/REL    -> remove mRoot + /REL
 *  POST /manifest            -> raw JSON body stored as the new manifest
 */

#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

class NotesSyncServer
{
public:
    NotesSyncServer() : mServer(80) {}

    // Bring up the access point and start the web server. Returns the AP IP.
    IPAddress begin(const char* ssid, const char* password, const String& root)
    {
        mRoot = root;
        if (!SD.exists(mRoot))
            SD.mkdir(mRoot);

        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid, password);
        IPAddress ip = WiFi.softAPIP();

        mServer.on("/manifest", HTTP_GET,  [this]() { handleManifestGet(); });
        mServer.on("/manifest", HTTP_POST, [this]() { handleManifestPost(); });
        mServer.on("/delete",   HTTP_POST, [this]() { handleDelete(); });
        mServer.on("/upload",   HTTP_POST,
                   [this]() { mServer.send(200, "text/plain", "OK"); },
                   [this]() { handleUpload(); });
        mServer.onNotFound([this]() { mServer.send(404, "text/plain", "not found"); });
        mServer.begin();

        return ip;
    }

    void handle() { mServer.handleClient(); }

    int  filesReceived() const { return mFilesReceived; }
    int  filesDeleted()  const { return mFilesDeleted; }

private:
    String manifestPath() const { return mRoot + "/.sync_manifest.json"; }
    String tempPath()     const { return mRoot + "/.upload.tmp"; }

    // Force a desktop-relative path ("/A/B/x.md") to live under mRoot.
    String fullPath(const String& rel) const
    {
        if (rel.length() && rel[0] == '/')
            return mRoot + rel;
        return mRoot + "/" + rel;
    }

    // Create every intermediate directory for a final file path under mRoot.
    void ensureParentDirs(const String& full)
    {
        if (!SD.exists(mRoot))
            SD.mkdir(mRoot);
        int start = mRoot.length() + 1;            // first char after "mRoot/"
        int idx   = full.indexOf('/', start);
        while (idx >= 0)
        {
            String sub = full.substring(0, idx);
            if (sub.length() && !SD.exists(sub))
                SD.mkdir(sub);
            idx = full.indexOf('/', idx + 1);
        }
    }

    void handleManifestGet()
    {
        File f = SD.open(manifestPath().c_str(), FILE_READ);
        if (!f)
        {
            mServer.send(200, "application/json", "{}");
            return;
        }
        mServer.streamFile(f, "application/json");
        f.close();
    }

    void handleManifestPost()
    {
        String body = mServer.arg("plain");
        File f = SD.open(manifestPath().c_str(), FILE_WRITE);
        if (f)
        {
            f.print(body);
            f.close();
            mServer.send(200, "text/plain", "OK");
        }
        else
        {
            mServer.send(500, "text/plain", "manifest write failed");
        }
    }

    void handleDelete()
    {
        String rel  = mServer.arg("path");
        String full = fullPath(rel);
        bool ok = SD.remove(full.c_str());
        if (ok) mFilesDeleted++;
        mServer.send(200, "text/plain", ok ? "DELETED" : "MISSING");
    }

    // Streaming upload: START opens the temp file, WRITE appends chunks, END
    // renames the temp into the final path (after making parent dirs).
    void handleUpload()
    {
        HTTPUpload& up = mServer.upload();

        if (up.status == UPLOAD_FILE_START)
        {
            mUploadRel = mServer.arg("path");      // URL query, already decoded
            SD.remove(tempPath().c_str());
            mTempFile = SD.open(tempPath().c_str(), FILE_WRITE);
        }
        else if (up.status == UPLOAD_FILE_WRITE)
        {
            if (mTempFile)
                mTempFile.write(up.buf, up.currentSize);
        }
        else if (up.status == UPLOAD_FILE_END)
        {
            if (mTempFile)
                mTempFile.close();

            String full = fullPath(mUploadRel);
            ensureParentDirs(full);
            SD.remove(full.c_str());               // rename fails if dest exists
            if (SD.rename(tempPath().c_str(), full.c_str()))
                mFilesReceived++;
        }
        else if (up.status == UPLOAD_FILE_ABORTED)
        {
            if (mTempFile)
                mTempFile.close();
            SD.remove(tempPath().c_str());          // leave the live copy intact
        }
    }

    WebServer mServer;
    String    mRoot;
    File      mTempFile;
    String    mUploadRel;
    int       mFilesReceived = 0;
    int       mFilesDeleted  = 0;
};
