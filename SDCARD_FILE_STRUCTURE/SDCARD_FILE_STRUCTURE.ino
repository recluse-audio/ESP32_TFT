/*
 * SD Card File Browser with Touch Navigation & File Viewer
 * - Browse SD card folders and files
 * - Tap to select/highlight (Yellow = viewable, Orange = too large/binary)
 * - Tap selected item twice to enter folder or view file
 * - Tap ".." to go up one level
 * - View text files and images (PNG/JPEG)
 *
 * Hardware:
 * - SD Card: VSPI (SCK=18, MISO=19, MOSI=23, CS=5)
 * - TFT Display: HSPI (from User_Setup)
 * - Touch: Custom SPI (CS=33, CLK=25, MOSI=32, MISO=39, IRQ=36)
 */

// Load shared TFT config
#define USER_SETUP_LOADED
#include "../CONFIG/User_Setup_TFT_eSPI.h"

#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <PNGdec.h>
#include <WiFi.h>
#include <WebServer.h>

// ============ Hardware Pin Definitions ============

// SD Card (VSPI)
#define SD_CS 5
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
SPIClass sdSPI(VSPI);

// Touch (Custom SPI)
#define T_CS   33
#define T_CLK  25
#define T_DIN  32
#define T_DO   39
#define T_IRQ  36

// TFT
TFT_eSPI tft = TFT_eSPI();

// WiFi & Web Server
WebServer server(80);
const char* AP_SSID = "ESP32_FileManager";
const char* AP_PASS = "esp32files";  // Min 8 chars
String wifiIP = "";

// Upload state
File uploadFile;
String uploadPath;

// ============ Display Constants ============
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;
const int HEADER_HEIGHT = 24;
const int ROW_HEIGHT = 18;
const int LIST_TOP = HEADER_HEIGHT;
const int ROWS_PER_PAGE = (SCREEN_HEIGHT - LIST_TOP) / ROW_HEIGHT;

// ============ File Browser State ============
const int MAX_ENTRIES = 256;
const int MAX_NAME_LEN = 64;

struct Entry 
{
  char name[MAX_NAME_LEN];
  bool isDir;
  uint32_t size;
};

Entry entries[MAX_ENTRIES];
int entryCount = 0;
String currentPath = "/";
int selectedIndex = -1;
int scrollOffset = 0;

// ============ Touch Calibration ============
const int TOUCH_X_MIN = 200;
const int TOUCH_X_MAX = 3700;
const int TOUCH_Y_MIN = 300;
const int TOUCH_Y_MAX = 3800;

// ============ File Viewer Limits ============
const uint32_t MAX_TEXT_SIZE = 100000;    // 100KB max for text files
const uint32_t MAX_IMAGE_SIZE = 500000;   // 500KB max for images
const int MAX_IMAGE_WIDTH = 320;

// ============ File Type Detection ============
enum FileType
{
  TYPE_UNKNOWN,
  TYPE_FOLDER,
  TYPE_TEXT,
  TYPE_PNG,
  TYPE_TOO_LARGE,
  TYPE_BINARY
};

FileType getFileType(const char* filename, uint32_t size, bool isDir)
{
  if (isDir) return TYPE_FOLDER;

  String name = String(filename);
  name.toLowerCase();

  // Check text files
  if (name.endsWith(".txt") || name.endsWith(".log") ||
      name.endsWith(".md") || name.endsWith(".json") ||
      name.endsWith(".csv") || name.endsWith(".ini"))
  {
    return (size <= MAX_TEXT_SIZE) ? TYPE_TEXT : TYPE_TOO_LARGE;
  }

  // Check PNG files
  if (name.endsWith(".png"))
  {
    return (size <= MAX_IMAGE_SIZE) ? TYPE_PNG : TYPE_TOO_LARGE;
  }

  // Unknown/binary file (JPEG support can be added if needed)
  return TYPE_BINARY;
}

bool isViewable(FileType type)
{
  return (type == TYPE_TEXT || type == TYPE_PNG);
}

// ============ Touch Functions (Bit-bang SPI) ============

void initTouch() 
{
  pinMode(T_CS, OUTPUT);
  pinMode(T_CLK, OUTPUT);
  pinMode(T_DIN, OUTPUT);
  pinMode(T_DO, INPUT);
  pinMode(T_IRQ, INPUT);

  digitalWrite(T_CS, HIGH);
  digitalWrite(T_CLK, LOW);
}

void spiWrite(uint8_t data) 
{
  for (int i = 7; i >= 0; i--) 
  {
    digitalWrite(T_CLK, LOW);
    digitalWrite(T_DIN, (data >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(1);
  }
}

uint8_t spiRead() 
{
  uint8_t data = 0;
  for (int i = 7; i >= 0; i--) 
  {
    digitalWrite(T_CLK, LOW);
    delayMicroseconds(1);
    digitalWrite(T_CLK, HIGH);
    delayMicroseconds(1);
    if (digitalRead(T_DO)) 
    {
      data |= (1 << i);
    }
  }
  return data;
}

uint16_t readTouchRaw(uint8_t command) 
{
  uint16_t data = 0;

  digitalWrite(T_CS, LOW);
  delayMicroseconds(10);

  spiWrite(command);
  data = spiRead() << 8;
  data |= spiRead();

  digitalWrite(T_CS, HIGH);
  delayMicroseconds(10);

  return data >> 3;  // Convert to 12-bit
}

bool getTouchPoint(int16_t& x, int16_t& y) 
{
  // Check if screen is being touched
  if (digitalRead(T_IRQ) == HIGH) 
  {
    return false;  // Not touched
  }

  // Read pressure
  uint16_t z = readTouchRaw(0xB0);
  if (z < 200) 
  {
    return false;  // Not enough pressure
  }

  // Read coordinates
  uint16_t rawX = readTouchRaw(0x90);
  uint16_t rawY = readTouchRaw(0xD0);

  // Map to screen coordinates
  x = map(rawX, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_WIDTH);
  y = map(rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_HEIGHT);

  x = constrain(x, 0, SCREEN_WIDTH - 1);
  y = constrain(y, 0, SCREEN_HEIGHT - 1);

  return true;
}

// ============ Display Functions ============

void drawHeader()
{
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(1);

  // Top line: Path
  tft.setCursor(4, 2);
  String displayPath = currentPath;
  if (displayPath.length() > 35)
  {
    displayPath = "..." + displayPath.substring(displayPath.length() - 32);
  }
  tft.print(displayPath);

  // Entry count
  tft.setCursor(250, 2);
  tft.printf("%d", entryCount);

  // Free space dot
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  if (totalBytes > 0)
  {
    float freePercent = ((float)(totalBytes - usedBytes) / totalBytes) * 100;
    uint16_t dotColor = (freePercent > 10) ? TFT_GREEN : (freePercent > 5) ? TFT_YELLOW : TFT_RED;
    tft.fillCircle(314, 6, 2, dotColor);
  }

  // Bottom line: WiFi info
  tft.setCursor(4, 14);
  tft.setTextColor(TFT_CYAN, TFT_DARKGREY);
  if (wifiIP.length() > 0)
  {
    tft.printf("WiFi: %s", wifiIP.c_str());
  }
  else
  {
    tft.print("WiFi: Off");
  }
}

void drawRow(int visibleRow, int entryIndex)
{
  const int y = LIST_TOP + visibleRow * ROW_HEIGHT;
  const bool isSelected = (entryIndex == selectedIndex);

  // Determine file type and color
  FileType ftype = getFileType(entries[entryIndex].name,
                                entries[entryIndex].size,
                                entries[entryIndex].isDir);

  uint16_t bg, fg;

  if (isSelected)
  {
    // Yellow for viewable files, orange for non-viewable
    if (ftype == TYPE_FOLDER)
    {
      bg = TFT_YELLOW;   // Yellow for folders
      fg = TFT_BLACK;
    }
    else if (isViewable(ftype))
    {
      bg = TFT_YELLOW;   // Yellow for viewable files
      fg = TFT_BLACK;
    }
    else
    {
      bg = TFT_ORANGE;   // Orange for too large or binary
      fg = TFT_BLACK;
    }
  }
  else
  {
    bg = TFT_BLACK;
    fg = TFT_WHITE;
  }

  tft.fillRect(0, y, SCREEN_WIDTH, ROW_HEIGHT, bg);
  tft.setTextColor(fg, bg);
  tft.setCursor(4, y + 2);

  if (entries[entryIndex].isDir)
  {
    tft.print("[");
    tft.print(entries[entryIndex].name);
    tft.print("]");
  }
  else
  {
    tft.print(entries[entryIndex].name);

    // Show file size
    tft.setCursor(230, y + 2);
    if (entries[entryIndex].size < 1024)
    {
      tft.printf("%dB", entries[entryIndex].size);
    }
    else if (entries[entryIndex].size < 1024*1024)
    {
      tft.printf("%dK", entries[entryIndex].size / 1024);
    }
    else
    {
      tft.printf("%dM", entries[entryIndex].size / (1024*1024));
    }
  }
}

void drawList() 
{
  tft.fillRect(0, LIST_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - LIST_TOP, TFT_BLACK);

  const int start = scrollOffset;
  const int end = min(entryCount, scrollOffset + ROWS_PER_PAGE);

  for (int i = start; i < end; i++) 
  {
    drawRow(i - start, i);
  }
}

void render() {
  drawHeader();
  drawList();
}

// ============ Web Server Functions ============

String sanitizeFilename(const String& in)
{
  String out = in;
  out.replace("\\", "_");
  out.replace("/", "_");
  out.replace("..", "_");
  return out;
}

String uniquePath(const String& dir, const String& filename)
{
  String base = dir + "/" + filename;
  if (!SD.exists(base.c_str())) return base;

  int dot = filename.lastIndexOf('.');
  String stem = (dot >= 0) ? filename.substring(0, dot) : filename;
  String ext  = (dot >= 0) ? filename.substring(dot) : "";

  for (int i = 1; i < 999; i++)
  {
    String candidate = dir + "/" + stem + "_" + String(i) + ext;
    if (!SD.exists(candidate.c_str())) return candidate;
  }
  return base;
}

void handleRoot()
{
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32 File Manager</title></head><body>";
  html += "<h2>ESP32 File Manager</h2>";
  html += "<p>Current path: <b>" + currentPath + "</b></p>";
  html += "<h3>Upload File</h3>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='file' required>";
  html += "<button type='submit'>Upload</button>";
  html += "</form>";
  html += "<h3><a href='/list'>Browse Files</a></h3>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleList()
{
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>File List</title></head><body>";
  html += "<h2>Files in " + currentPath + "</h2>";
  html += "<p><a href='/'>Upload</a> | <a href='/list'>Refresh</a></p><ul>";

  File dir = SD.open(currentPath.c_str());
  if (dir && dir.isDirectory())
  {
    File file = dir.openNextFile();
    while (file)
    {
      String fullPath = String(file.name());
      String baseName = fullPath.substring(fullPath.lastIndexOf('/') + 1);

      if (file.isDirectory())
      {
        html += "<li>[DIR] " + baseName + "</li>";
      }
      else
      {
        html += "<li><a href='/download?path=" + fullPath + "'>" + baseName + "</a>";
        html += " (" + String(file.size()) + " bytes)";
        html += " <a href='/delete?path=" + fullPath + "'>[Delete]</a></li>";
      }

      file.close();
      file = dir.openNextFile();
    }
    dir.close();
  }

  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}

void handleDownload()
{
  if (!server.hasArg("path"))
  {
    server.send(400, "text/plain", "Missing path");
    return;
  }

  String path = server.arg("path");
  File f = SD.open(path.c_str(), FILE_READ);

  if (!f)
  {
    server.send(404, "text/plain", "File not found");
    return;
  }

  String filename = path.substring(path.lastIndexOf('/') + 1);
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.streamFile(f, "application/octet-stream");
  f.close();
}

void handleDelete()
{
  if (!server.hasArg("path"))
  {
    server.send(400, "text/plain", "Missing path");
    return;
  }

  String path = server.arg("path");

  if (SD.remove(path.c_str()))
  {
    server.send(200, "text/html", "Deleted!<br><a href='/list'>Back</a>");
    loadDirectory(currentPath);  // Refresh browser display
    render();
  }
  else
  {
    server.send(500, "text/plain", "Delete failed");
  }
}

void handleUploadDone()
{
  server.send(200, "text/html", "Upload complete!<br><a href='/'>Home</a> | <a href='/list'>List</a>");
  loadDirectory(currentPath);  // Refresh browser display
  render();
}

void handleUploadStream()
{
  HTTPUpload& up = server.upload();

  if (up.status == UPLOAD_FILE_START)
  {
    String fname = sanitizeFilename(String(up.filename));
    uploadPath = uniquePath(currentPath, fname);
    uploadFile = SD.open(uploadPath.c_str(), FILE_WRITE);

    if (!uploadFile)
    {
      Serial.printf("Upload failed to open: %s\n", uploadPath.c_str());
    }
    else
    {
      Serial.printf("Upload started: %s\n", uploadPath.c_str());
    }
  }
  else if (up.status == UPLOAD_FILE_WRITE)
  {
    if (uploadFile)
    {
      uploadFile.write(up.buf, up.currentSize);
    }
  }
  else if (up.status == UPLOAD_FILE_END)
  {
    if (uploadFile)
    {
      uploadFile.close();
      Serial.printf("Upload done: %s (%u bytes)\n", uploadPath.c_str(), up.totalSize);
    }
  }
  else if (up.status == UPLOAD_FILE_ABORTED)
  {
    if (uploadFile) uploadFile.close();
    if (uploadPath.length()) SD.remove(uploadPath.c_str());
    Serial.println("Upload aborted");
  }
}

// ============ File Browser Functions ============

void clampScroll() 
{
  if (scrollOffset < 0) scrollOffset = 0;
  const int maxScroll = max(0, entryCount - ROWS_PER_PAGE);
  if (scrollOffset > maxScroll) scrollOffset = maxScroll;
}

void ensureVisible(int idx) 
{
  if (idx < scrollOffset) scrollOffset = idx;
  if (idx >= scrollOffset + ROWS_PER_PAGE) 
  {
    scrollOffset = idx - (ROWS_PER_PAGE - 1);
  }
  clampScroll();
}

int loadDirectory(const String& path) 
{
  entryCount = 0;
  selectedIndex = -1;
  scrollOffset = 0;

  File dir = SD.open(path.c_str());
  if (!dir || !dir.isDirectory()) 
  {
    Serial.printf("Failed to open directory: %s\n", path.c_str());
    return 0;
  }

  // Add ".." entry (except at root)
  if (path != "/" && entryCount < MAX_ENTRIES) 
  {
    strncpy(entries[entryCount].name, "..", MAX_NAME_LEN);
    entries[entryCount].name[MAX_NAME_LEN - 1] = '\0';
    entries[entryCount].isDir = true;
    entries[entryCount].size = 0;
    entryCount++;
  }

  // List all files/folders
  File file = dir.openNextFile();
  while (file && entryCount < MAX_ENTRIES) 
  {
    String fullName = String(file.name());
    int lastSlash = fullName.lastIndexOf('/');
    String baseName = (lastSlash >= 0) ? fullName.substring(lastSlash + 1) : fullName;

    // Skip hidden files
    if (!baseName.startsWith(".")) 
    {
      strncpy(entries[entryCount].name, baseName.c_str(), MAX_NAME_LEN);
      entries[entryCount].name[MAX_NAME_LEN - 1] = '\0';
      entries[entryCount].isDir = file.isDirectory();
      entries[entryCount].size = file.size();
      entryCount++;
    }

    file.close();
    file = dir.openNextFile();
  }

  dir.close();
  clampScroll();

  Serial.printf("Loaded %d entries from %s\n", entryCount, path.c_str());
  return entryCount;
}

String joinPath(const String& dir, const String& base)
{
  if (dir == "/") return "/" + base;
  return dir + "/" + base;
}

// ============ File Viewers ============

// PNG decoder callback functions
PNG png;
File pngFile;
uint16_t pngLineBuffer[MAX_IMAGE_WIDTH];

void* pngOpen(const char* filename, int32_t* size)
{
  pngFile = SD.open(filename, FILE_READ);
  if (!pngFile)
  {
    *size = 0;
    return nullptr;
  }
  *size = pngFile.size();
  return &pngFile;
}

void pngClose(void* handle)
{
  File* f = (File*)handle;
  if (f && *f) f->close();
}

int32_t pngRead(PNGFILE*, uint8_t* buffer, int32_t length)
{
  if (!pngFile) return 0;
  return pngFile.read(buffer, length);
}

int32_t pngSeek(PNGFILE*, int32_t position)
{
  if (!pngFile) return 0;
  return pngFile.seek(position) ? position : 0;
}

int pngDraw(PNGDRAW* pDraw)
{
  if ((int)pDraw->iWidth > MAX_IMAGE_WIDTH)
  {
    return 0;  // Buffer overflow protection
  }

  png.getLineAsRGB565(pDraw, pngLineBuffer, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);
  tft.pushImage(0, (int)pDraw->y, (int)pDraw->iWidth, 1, pngLineBuffer);
  return 1;
}

bool viewPNG(const String& filepath)
{
  int rc = png.open(filepath.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);

  if (rc == PNG_SUCCESS)
  {
    tft.fillScreen(TFT_BLACK);

    int16_t xpos = (SCREEN_WIDTH - png.getWidth()) / 2;
    int16_t ypos = (SCREEN_HEIGHT - png.getHeight()) / 2;

    if (xpos < 0) xpos = 0;
    if (ypos < 0) ypos = 0;

    tft.startWrite();
    rc = png.decode(NULL, 0);
    tft.endWrite();

    png.close();
    return (rc == PNG_SUCCESS);
  }

  return false;
}

void viewText(const String& filepath)
{
  File file = SD.open(filepath.c_str());

  if (!file)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.println("Failed to open file");
    delay(2000);
    return;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  int y = 2;
  int lineHeight = 12;  // Increased from 10 to prevent overlap
  String line = "";

  while (file.available() && y < SCREEN_HEIGHT - lineHeight)
  {
    char c = file.read();

    if (c == '\n' || c == '\r')
    {
      if (line.length() > 0)
      {
        tft.setCursor(2, y);
        tft.print(line);  // Changed from println to print
        y += lineHeight;
        line = "";
      }
      else
      {
        // Empty line (blank line between paragraphs)
        y += lineHeight;
      }
    }
    else if (c >= 32 && c < 127)  // Printable ASCII only
    {
      line += c;

      // Wrap long lines
      if (line.length() >= 53)  // ~320px / 6px per char
      {
        tft.setCursor(2, y);
        tft.print(line);  // Changed from println to print
        y += lineHeight;
        line = "";
      }
    }
  }

  // Print last line
  if (line.length() > 0 && y < SCREEN_HEIGHT - lineHeight)
  {
    tft.setCursor(2, y);
    tft.print(line);  // Changed from println to print
  }

  file.close();

  // Show "Tap to return" message
  tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(80, SCREEN_HEIGHT - 16);
  tft.print("Tap to return");
}

void viewFile(const String& filepath, FileType ftype)
{
  bool success = false;

  switch (ftype)
  {
    case TYPE_TEXT:
      viewText(filepath);
      success = true;
      break;

    case TYPE_PNG:
      success = viewPNG(filepath);
      if (!success)
      {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.setCursor(10, 10);
        tft.println("PNG decode failed");
      }
      break;

    default:
      return;  // Non-viewable file
  }

  // Wait for tap to return
  while (digitalRead(T_IRQ) == HIGH) delay(10);  // Wait for touch
  delay(200);
  while (digitalRead(T_IRQ) == LOW) delay(10);   // Wait for release
  delay(200);

  // Redraw file browser
  render();
}

void navigateIntoSelected()
{
  if (selectedIndex < 0 || selectedIndex >= entryCount) return;

  String name = String(entries[selectedIndex].name);

  // Handle folders
  if (entries[selectedIndex].isDir)
  {
    if (name == "..")
    {
      // Go up one level
      if (currentPath == "/") return;
      int lastSlash = currentPath.lastIndexOf('/');
      currentPath = (lastSlash <= 0) ? "/" : currentPath.substring(0, lastSlash);
    }
    else
    {
      // Enter directory
      currentPath = joinPath(currentPath, name);
    }

    loadDirectory(currentPath);
    render();
  }
  else
  {
    // Handle files - view if viewable
    FileType ftype = getFileType(entries[selectedIndex].name,
                                  entries[selectedIndex].size,
                                  entries[selectedIndex].isDir);

    if (isViewable(ftype))
    {
      String filepath = joinPath(currentPath, name);
      Serial.printf("Viewing file: %s (type=%d)\n", filepath.c_str(), ftype);
      viewFile(filepath, ftype);
    }
    else
    {
      // Show "cannot view" message
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_ORANGE);
      tft.setCursor(40, 100);

      if (ftype == TYPE_TOO_LARGE)
      {
        tft.println("File too large to view");
      }
      else
      {
        tft.println("Cannot view binary file");
      }

      tft.setTextColor(TFT_WHITE);
      tft.setCursor(80, 140);
      tft.println("Tap to return");

      delay(500);

      // Wait for tap
      while (digitalRead(T_IRQ) == HIGH) delay(10);
      delay(200);
      while (digitalRead(T_IRQ) == LOW) delay(10);
      delay(200);

      render();
    }
  }
}

int hitTestEntry(int16_t touchY) 
{
  if (touchY < LIST_TOP || touchY >= SCREEN_HEIGHT) return -1;

  const int row = (touchY - LIST_TOP) / ROW_HEIGHT;
  const int idx = scrollOffset + row;

  if (idx < 0 || idx >= entryCount) return -1;
  return idx;
}

// ============ Arduino Setup/Loop ============

void setup() 
{
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== SD Card File Browser ===");

  // Initialize TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextWrap(false);

  // Initialize Touch
  initTouch();
  Serial.println("Touch initialized (custom SPI)");

  // Initialize SD Card with higher frequency for better compatibility
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  // Try multiple frequencies for better compatibility
  bool sdOk = false;
  uint32_t frequencies[] = {25000000, 20000000, 10000000, 4000000, 1000000};

  for (int i = 0; i < 5 && !sdOk; i++)
  {
    Serial.printf("Trying SD at %d Hz...\n", frequencies[i]);
    sdOk = SD.begin(SD_CS, sdSPI, frequencies[i]);
    if (!sdOk) delay(100);
  }

  if (!sdOk)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("SD Card init failed!");
    tft.println("Check:");
    tft.println("- Card inserted?");
    tft.println("- FAT16/FAT32/exFAT?");
    tft.println("- Card not locked?");

    Serial.println("SD Card init failed!");
    Serial.println("Tried all frequencies. Check card.");
    while (true) delay(1000);
  }

  // Detect card type and size
  uint8_t cardType = SD.cardType();
  uint64_t cardSize = SD.cardSize() / (1024 * 1024); // MB

  Serial.println("SD Card initialized successfully!");
  Serial.printf("Card Type: ");
  switch (cardType)
  {
    case CARD_MMC:  Serial.println("MMC"); break;
    case CARD_SD:   Serial.println("SDSC"); break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("UNKNOWN"); break;
  }
  Serial.printf("Card Size: %llu MB\n", cardSize);
  Serial.printf("Total Space: %llu MB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used Space: %llu MB\n", SD.usedBytes() / (1024 * 1024));

  // Load root directory
  loadDirectory(currentPath);
  render();

  Serial.println("Ready! Tap to navigate.");

  // Initialize WiFi AP
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(AP_SSID, AP_PASS);

  if (apOk)
  {
    wifiIP = WiFi.softAPIP().toString();
    Serial.println("\n=== WiFi Hotspot Started ===");
    Serial.printf("SSID: %s\n", AP_SSID);
    Serial.printf("Password: %s\n", AP_PASS);
    Serial.printf("IP: http://%s\n", wifiIP.c_str());
    Serial.println("============================\n");

    // Setup web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/list", HTTP_GET, handleList);
    server.on("/download", HTTP_GET, handleDownload);
    server.on("/delete", HTTP_GET, handleDelete);
    server.on("/upload", HTTP_POST, handleUploadDone, handleUploadStream);

    server.begin();
    Serial.println("Web server started");
  }
  else
  {
    Serial.println("WiFi AP failed");
    wifiIP = "";
  }

  // Redraw to show WiFi status
  render();
}

void loop()
{
  // Handle web server
  server.handleClient();

  // Handle touch input
  int16_t x, y;

  if (getTouchPoint(x, y)) 
  {
    // Touch detected
    delay(100);  // Debounce

    const int idx = hitTestEntry(y);

    if (idx >= 0) 
    {
      if (selectedIndex == idx) 
      {
        // Double-tap on same entry - navigate into it
        navigateIntoSelected();
      } 
      else 
      {
        // First tap - select entry
        selectedIndex = idx;
        ensureVisible(selectedIndex);
        render();

        Serial.printf("Selected: %s %s\n",
          entries[idx].isDir ? "[DIR]" : "[FILE]",
          entries[idx].name);
      }
    }

    // Wait for release
    while (digitalRead(T_IRQ) == LOW) 
    {
      delay(10);
    }
    delay(100);  // Additional debounce after release
  }
}
