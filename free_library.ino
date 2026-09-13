/*
  ===========================================================
   ESP32-S3 Little Free Library
  ===========================================================
  Turns the board into a WiFi hotspot that serves public-domain
  ebooks (EPUB files) from an SD card, with a browser-based reader.
  No app or download required — visitors just join the WiFi and
  open a web page.

  BEFORE YOU UPLOAD THIS:
  Set up your SD card first! See the README that came with this
  file for the full walkthrough. In short, the SD card needs:

    /index.html         <- the reader webpage (provided separately)
    /epub.min.js         <- epub.js library (you download this once)
    /jszip.min.js         <- jszip library (you download this once)
    /books/               <- put your .epub files in this folder

  WIFI:
  Change AP_SSID / AP_PASSWORD below if you like, then upload this
  sketch. Connect a phone or laptop to that WiFi network and open
  http://192.168.4.1 in a browser.
  ===========================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD_MMC.h>
#include <FS.h>

// ---------- SETTINGS YOU CAN CHANGE ----------
const char* AP_SSID     = "Little Free Library";
const char* AP_PASSWORD = "";   // "" = open network (no password). Or set something like "readfreely"
// ----------------------------------------------

// The ESP32's own address once it's running as an access point.
// This is the standard default — no need to change it.
IPAddress AP_IP(192, 168, 4, 1);

// A captive portal works by answering EVERY DNS lookup with our own
// address, so no matter what website the phone/laptop tries to load
// first, it lands on our page — which is what makes the "Sign in to
// network" popup appear automatically.
DNSServer dnsServer;
const byte DNS_PORT = 53;

// SD card pins on the Freenove ESP32-S3 board (SDMMC 1-bit mode).
// If your card fails to mount, double check these against your
// board's manual — some Freenove revisions differ slightly.
#define SD_CLK 39
#define SD_CMD 38
#define SD_D0  40

WebServer server(80);

// Works out what "Content-Type" header to send based on file extension.
// This is what tells the browser "this is a webpage" vs "this is a book".
String contentTypeFor(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".epub")) return "application/epub+zip";
  return "application/octet-stream";
}

// Streams any file straight from the SD card to the browser.
// This is the "file server" part — the ESP32 never needs to
// understand what's inside the EPUB, it just sends the bytes.
bool streamFile(const String& path, bool forceDownload = false) {
  if (!SD_MMC.exists(path)) return false;
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;

  if (forceDownload) {
    int slash = path.lastIndexOf('/');
    String filename = path.substring(slash + 1);
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  }
  server.streamFile(f, contentTypeFor(path));
  f.close();
  return true;
}

// GET /  ->  the reader webpage
void handleRoot() {
  if (!streamFile("/index.html")) {
    server.send(500, "text/plain", "index.html not found on SD card. See README.");
  }
}

// GET /books  ->  JSON list of available ebooks, e.g.
// [{"name":"alice.epub","size":123456}]
// The webpage calls this to build the list of books to show.
void handleBookList() {
  File dir = SD_MMC.open("/books");
  if (!dir || !dir.isDirectory()) {
    server.send(200, "application/json", "[]");
    return;
  }
  String json = "[";
  bool first = true;
  File entry = dir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    if (!entry.isDirectory() && name.endsWith(".epub")) {
      if (!first) json += ",";
      first = false;
      json += "{\"name\":\"" + name + "\",\"size\":" + String(entry.size()) + "}";
    }
    entry = dir.openNextFile();
  }
  json += "]";
  server.send(200, "application/json", json);
}

// Catches every other request:
//   /books/<file>.epub            -> streamed for the in-browser reader
//   /books/<file>.epub?download=1 -> streamed as a download
//   /epub.min.js, /jszip.min.js   -> served as plain static files
//   anything else (e.g. phones/laptops probing for a captive portal,
//   like /generate_204, /hotspot-detect.html, /ncsi.txt) -> redirected
//   to our home page, which is what triggers the "Sign in to network"
//   popup on most phones and laptops.
void handleNotFound() {
  String uri = server.uri();
  bool download = server.hasArg("download");
  if (streamFile(uri, download)) return;

  String home = "http://" + AP_IP.toString() + "/";
  server.sendHeader("Location", home, true);
  server.send(302, "text/plain", "Redirecting to " + home);
}

void setup() {
  Serial.begin(115200);

  // --- SD card ---
  SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0);
  if (!SD_MMC.begin("/sdcard", true)) {   // true = 1-bit mode
    Serial.println("SD card mount FAILED. Check the card is inserted and formatted FAT32.");
  } else {
    Serial.println("SD card mounted OK.");
  }

  // --- WiFi Access Point ---
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  if (strlen(AP_PASSWORD) > 0) {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
  } else {
    WiFi.softAP(AP_SSID);   // open network, no password
  }
  Serial.print("WiFi hotspot started: ");
  Serial.println(AP_SSID);
  Serial.print("Visitors should open: http://");
  Serial.println(WiFi.softAPIP());   // normally 192.168.4.1

  // --- Captive portal DNS ---
  // "*" means: whatever address anyone asks for, tell them it's us.
  dnsServer.start(DNS_PORT, "*", AP_IP);

  // --- Web server routes ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/books", HTTP_GET, handleBookList);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
