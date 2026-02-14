#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include <SPI.h>
#include <SD.h>

static const int kSdCsPin = 5;
static const uint32_t kSdHz = 4000000;
SPIClass sdSPI(VSPI);

WebServer server(80);

// --- Hotspot config ---
static const char* kApSsid = "ESP32_SD";
static const char* kApPass = "change_me_123";  // >= 8 chars, or set nullptr for open AP

// --- Upload destination ---
static const char* kUploadDir = "/UPLOADS";

// Current upload state
static File gUploadFile;
static String gUploadPath;

static bool InitSd()
{
    sdSPI.begin(/*sck=*/18, /*miso=*/19, /*mosi=*/23, /*ss=*/kSdCsPin);
    if (!SD.begin(kSdCsPin, sdSPI, kSdHz))
    {
        return false;
    }

    if (!SD.exists(kUploadDir))
    {
        SD.mkdir(kUploadDir);
    }

    return true;
}

static String HtmlPage()
{
    String s;
    s += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    s += "<title>ESP32 SD Upload</title></head><body>";
    s += "<h3>Upload to SD</h3>";
    s += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    s += "<input type='file' name='file' required>";
    s += "<button type='submit'>Upload</button>";
    s += "</form>";
    s += "<p>Saved to <code>";
    s += kUploadDir;
    s += "/</code></p>";
    s += "<p><a href='/list'>List files</a></p>";
    s += "</body></html>";
    return s;
}

static String SanitizeFilename(const String& in)
{
    // keep it simple: strip any path separators
    String out = in;
    out.replace("\\", "_");
    out.replace("/", "_");
    out.replace("..", "_");
    return out;
}

static String UniquePathFor(const String& dir, const String& filename)
{
    String base = dir + "/" + filename;
    if (!SD.exists(base.c_str()))
    {
        return base;
    }

    // add _001, _002...
    int dot = filename.lastIndexOf('.');
    String stem = (dot >= 0) ? filename.substring(0, dot) : filename;
    String ext  = (dot >= 0) ? filename.substring(dot) : "";

    for (int i = 1; i < 1000; ++i)
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "_%03d", i);
        String candidate = dir + "/" + stem + String(buf) + ext;
        if (!SD.exists(candidate.c_str()))
        {
            return candidate;
        }
    }

    // fallback (overwrite) if somehow exhausted
    return base;
}

static void HandleRoot()
{
    server.send(200, "text/html", HtmlPage());
}

static void HandleList()
{
    File dir = SD.open(kUploadDir);
    if (!dir || !dir.isDirectory())
    {
        server.send(500, "text/plain", "UPLOADS dir missing");
        return;
    }

    String s;
    s += "<!doctype html><html><body><h3>Files in ";
    s += kUploadDir;
    s += "</h3><ul>";

    for (File f = dir.openNextFile(); f; f = dir.openNextFile())
    {
        if (!f.isDirectory())
        {
            String name = String(f.name());
            s += "<li><a href='/download?path=";
            s += name;
            s += "'>";
            s += name;
            s += "</a> (";
            s += (uint32_t)f.size();
            s += " bytes)</li>";
        }
        f.close();
    }

    s += "</ul><p><a href='/'>Back</a></p></body></html>";
    dir.close();

    server.send(200, "text/html", s);
}

static void HandleDownload()
{
    if (!server.hasArg("path"))
    {
        server.send(400, "text/plain", "missing path");
        return;
    }

    String path = server.arg("path");
    if (!path.startsWith("/"))
    {
        server.send(400, "text/plain", "bad path");
        return;
    }

    File f = SD.open(path.c_str(), FILE_READ);
    if (!f)
    {
        server.send(404, "text/plain", "not found");
        return;
    }

    // Force download
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + path.substring(path.lastIndexOf('/') + 1) + "\"");
    server.streamFile(f, "application/octet-stream");
    f.close();
}

// POST /upload handler (final response)
static void HandleUploadDone()
{
    // The actual upload is handled in server.onUpload callback below.
    server.send(200, "text/plain", gUploadPath.length() ? ("OK: " + gUploadPath) : "OK");
}

// Called repeatedly during multipart upload
static void HandleUploadStream()
{
    HTTPUpload& up = server.upload();

    if (up.status == UPLOAD_FILE_START)
    {
        String fname = SanitizeFilename(String(up.filename));
        gUploadPath = UniquePathFor(kUploadDir, fname);

        gUploadFile = SD.open(gUploadPath.c_str(), FILE_WRITE);
        if (!gUploadFile)
        {
            Serial.printf("Failed to open for write: %s\n", gUploadPath.c_str());
        }
        else
        {
            Serial.printf("Upload start: %s\n", gUploadPath.c_str());
        }
    }
    else if (up.status == UPLOAD_FILE_WRITE)
    {
        if (gUploadFile)
        {
            // write the chunk
            gUploadFile.write(up.buf, up.currentSize);
        }
    }
    else if (up.status == UPLOAD_FILE_END)
    {
        if (gUploadFile)
        {
            gUploadFile.close();
            Serial.printf("Upload done: %s (%u bytes)\n", gUploadPath.c_str(), (unsigned)up.totalSize);
        }
    }
    else if (up.status == UPLOAD_FILE_ABORTED)
    {
        if (gUploadFile)
        {
            gUploadFile.close();
        }
        if (gUploadPath.length())
        {
            SD.remove(gUploadPath.c_str()); // discard partial
        }
        Serial.println("Upload aborted");
    }
}

void setup()
{
    Serial.begin(115200);
    delay(200);

    if (!InitSd())
    {
        Serial.println("SD init failed");
        while (true) delay(1000);
    }
    Serial.println("SD OK");

    // Start hotspot
    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP(kApSsid, kApPass);
    if (!apOk)
    {
        Serial.println("SoftAP failed");
        while (true) delay(1000);
    }

    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP SSID: ");
    Serial.println(kApSsid);
    Serial.print("AP IP: ");
    Serial.println(ip);

    // Routes
    server.on("/", HTTP_GET, HandleRoot);
    server.on("/list", HTTP_GET, HandleList);
    server.on("/download", HTTP_GET, HandleDownload);

    // Upload: POST endpoint + streaming callback
    server.on("/upload", HTTP_POST, HandleUploadDone, HandleUploadStream);

    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    server.handleClient();
}
