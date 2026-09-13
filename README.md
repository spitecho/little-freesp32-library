# Little FreESP32 Library — Setup Guide

This turns your Freenove ESP32-S3 board into a WiFi hotspot that anyone
can join to browse and read public-domain ebooks in their phone's
browser — no app, no login, no internet connection needed.

**Files in this project:**

* `free_library.ino` — the firmware you upload to the board
* `index.html` — the webpage visitors see (goes on the SD card)
* `free_library-no_captive_portal.ino` — Alternative firmware with no captive portal

\---

## Step 1: Install the Arduino IDE and ESP32 support

1. Download and install the Arduino IDE: https://www.arduino.cc/en/software
2. Open **File → Preferences**, and in "Additional Boards Manager URLs" paste:

```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

3. Open **Tools → Board → Boards Manager**, search "esp32", and install it
(by Espressif Systems).
4. Plug in your board with a USB cable, then go to **Tools → Board** and
select **"ESP32S3 Dev Module"**.
5. Under **Tools → Port**, pick the port that appeared when you plugged
in the board.

## Step 2: Prepare the SD card

Take the SD card out of the board (or use the included USB card reader)
and plug it into your computer.

1. **Format it as FAT32** if it isn't already (most 1GB cards from
Freenove come pre-formatted — you can skip this if files already work).
2. Create this folder structure on the card:

```
   /index.html          <- copy this from the "data" folder provided
   /epub.min.js          <- see Step 3 below
   /jszip.min.js          <- see Step 3 below
   /books/                <- create this empty folder, then add .epub files
   ```

3. Copy the `index.html` file (from the `data` folder here) to the
**root** of the SD card.

## Step 3: Download the two small JavaScript libraries (one-time)

The reader page needs two JavaScript files to actually parse and
display EPUB books. Because visitors connect *only* to your library's
WiFi (no general internet), these files must live on the SD card
rather than being loaded from a website each time.

Download these two files on your computer, then copy them to the SD
card's root folder (same place as `index.html`):

* epub.js: https://cdn.jsdelivr.net/npm/epubjs/dist/epub.min.js
→ save as `epub.min.js`
* JSZip: https://cdnjs.cloudflare.com/ajax/libs/jszip/3.10.1/jszip.min.js
→ save as `jszip.min.js`

(Right-click each link → "Save Link As…" in most browsers.)

## Step 4: Add your ebooks

Find public-domain EPUB files — [Project Gutenberg](https://www.gutenberg.org)
and [Standard Ebooks](https://standardebooks.org) are great sources — and
copy the `.epub` files into the `/books/` folder on the SD card.

Your SD card should now look like:

```
/index.html
/epub.min.js
/jszip.min.js
/books/
    alice-in-wonderland.epub
    pride-and-prejudice.epub
    ...
```

## Step 5: Upload the firmware

1. Put the SD card back in the board.
2. Open `free_library.ino` in the Arduino IDE.
3. (Optional) Near the top, change:

```cpp
   const char* AP_SSID     = "Little Free Library";
   const char* AP_PASSWORD = "";   // leave blank for an open network
   ```

4. Click **Upload** (the right-arrow icon). The IDE will compile and
flash the board over USB — this can take a minute or two the first time.
5. Open **Tools → Serial Monitor** (set baud rate to 115200) to watch
the board boot. You should see:

```
   SD card mounted OK.
   WiFi hotspot started: Little Free Library
   Visitors should open: http://192.168.4.1
   ```

## Step 6: Try it out

1. On your phone, join the WiFi network **"Little Free Library"**.
2. Open a browser and go to **http://192.168.4.1**
3. You should see your list of books, each with a **Read** button
(opens the browser-based reader) and a **Download** link (saves the
raw .epub file, e.g. for use in Kindle or another reader app).

\---

## Troubleshooting

* **"SD card mount FAILED" in Serial Monitor** — check the card is
fully inserted, and confirm it's formatted FAT32. If it still fails,
double-check the SD pin numbers (`SD_CLK`, `SD_CMD`, `SD_D0` near the
top of the sketch) against your specific board's manual — Freenove
has a few board revisions with slightly different wiring.
* **Page loads but no books show up** — make sure your `.epub` files
are directly inside `/books/`, not in a subfolder, and that the
filenames end in `.epub`.
* **"Read" button does nothing** — usually means `epub.min.js` or
`jszip.min.js` didn't make it onto the SD card correctly (Step 3).
Open the browser's developer console (if available) to check for a
script-loading error.
* **Only a few people can connect at once** — that's normal for a
basic ESP32 access point (expect it to comfortably handle a handful
of simultaneous readers), which is plenty for a Little Free Library.

## Optional next steps

* Add a printed QR code on the box pointing to the WiFi network name,
so passersby know what to join.
* Precompute nicer titles/authors by naming your files like
`Alice in Wonderland - Lewis Carroll.epub` — the page currently just
cleans up underscores/dashes in the filename for display.

