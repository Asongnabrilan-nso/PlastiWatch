// =============================================================================
// OLEDDisplay.cpp — PlastiWatch: SSD1306 Display Implementation
//
// Screen layouts (128 × 64 px).  Size-1 font = 6 × 8 px/char (21 cols max).
// Size-2 font = 12 × 16 px/char (10 cols max).
//
// All screens use an inverted 11-px header bar (white background, black text)
// that shows the current state name on the left and key status on the right.
// =============================================================================

#include "OLEDDisplay.h"
#include <Wire.h>
#include "../core/Logger.h"

static const char* TAG = "OLED";

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

OLEDDisplay::OLEDDisplay()
    : m_display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET_PIN)
    , m_ready(false)
{}

// ─────────────────────────────────────────────────────────────────────────────
// begin()
// ─────────────────────────────────────────────────────────────────────────────

bool OLEDDisplay::begin() {
    if (!m_display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
        Logger::error(TAG,
            "SSD1306 not found — check wiring and OLED_I2C_ADDRESS in Config.h");
        return false;
    }

    m_display.setTextColor(SSD1306_WHITE);
    m_display.cp437(true);   // Correct CP437 symbol mapping
    m_display.clearDisplay();
    m_ready = true;

    Logger::infof(TAG, "SSD1306 %dx%d initialised at 0x%02X",
        OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, OLED_I2C_ADDRESS);

    showBoot();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// clear()
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::clear() {
    if (!m_ready) return;
    m_display.clearDisplay();
    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

// Print text centered horizontally at y.
// Character width = 6 px × size.
void OLEDDisplay::printCentered(const char* text, int16_t y, uint8_t size) {
    int16_t x = ((int16_t)OLED_SCREEN_WIDTH
                 - (int16_t)(strlen(text) * 6u * (uint16_t)size)) / 2;
    if (x < 0) x = 0;
    m_display.setTextSize(size);
    m_display.setCursor(x, y);
    m_display.print(text);
}

// Draw an 11-px inverted header bar across the full display width.
//   leftText  — left-aligned inside the bar (2 px margin)
//   rightText — right-aligned inside the bar (2 px margin)
//               Pass nullptr to center leftText instead.
void OLEDDisplay::drawHeaderBar(const char* leftText, const char* rightText) {
    // White filled background
    m_display.fillRect(0, 0, OLED_SCREEN_WIDTH, 11, SSD1306_WHITE);

    m_display.setTextSize(1);
    m_display.setTextColor(SSD1306_BLACK);

    if (rightText && rightText[0]) {
        // Left-align leftText
        m_display.setCursor(2, 2);
        m_display.print(leftText);

        // Right-align rightText
        int16_t rx = (int16_t)OLED_SCREEN_WIDTH
                     - (int16_t)(strlen(rightText) * 6) - 2;
        if (rx < 60) rx = 60;   // clamp so it never overlaps leftText
        m_display.setCursor(rx, 2);
        m_display.print(rightText);
    } else {
        // Center leftText
        int16_t cx = ((int16_t)OLED_SCREEN_WIDTH
                      - (int16_t)(strlen(leftText) * 6)) / 2;
        if (cx < 0) cx = 0;
        m_display.setCursor(cx, 2);
        m_display.print(leftText);
    }

    // Restore normal (white-on-black) text colour for the rest of the screen
    m_display.setTextColor(SSD1306_WHITE);
}

// ─────────────────────────────────────────────────────────────────────────────
// showBoot()
//
//   y= 0  size 2  "Plasti"   (centered)
//   y=18  size 2  "Watch"    (centered)
//   y=35  ─── separator line ───
//   y=39  size 1  "IMU Data Collector"  (centered)
//   y=49  size 1  "v1.0  |  ESP32C3"   (centered)
//   y=56  size 1  "Initialising..."    (centered)
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::showBoot() {
    if (!m_ready) return;
    m_display.clearDisplay();

    // "Plasti" — 6 chars × 12 px = 72 px wide → center x = (128–72)/2 = 28
    m_display.setTextSize(2);
    m_display.setCursor(28, 0);
    m_display.print("Plasti");

    // "Watch" — 5 chars × 12 px = 60 px wide → center x = (128–60)/2 = 34
    m_display.setCursor(34, 18);
    m_display.print("Watch");

    // Thin separator centred in the lower half
    m_display.drawFastHLine(16, 35, 96, SSD1306_WHITE);

    // Subtitle and status (size 1, centred)
    printCentered("IMU Data Collector", 39, 1);
    printCentered("v1.0  |  ESP32C3",   49, 1);
    printCentered("Initialising...",    56, 1);

    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// showIdle()
//
//   y= 0  [INVERTED BAR]  "IDLE"  |  "WiFi:OK" or "OFFLINE"
//   y=14  size 2  label text, centered
//   y=33  size 1  IP address or "No WiFi  (Config.h)"
//   y=44  ─── separator ───
//   y=47  size 1  "Short: change label"
//   y=56  size 1  "Hold 2s: RECORD"
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::showIdle(const char* label, bool wifiConnected, const char* ip) {
    if (!m_ready) return;
    m_display.clearDisplay();

    // Header bar — state name left, WiFi status right
    drawHeaderBar("IDLE", wifiConnected ? "WiFi:OK" : "OFFLINE");

    // Active label in large font, centered
    printCentered(label, 14, 2);

    // WiFi / IP line
    m_display.setTextSize(1);
    m_display.setCursor(0, 33);
    if (wifiConnected) {
        m_display.printf("WiFi: %s", ip[0] ? ip : "getting IP...");
    } else {
        m_display.print("No WiFi  (Config.h)");
    }

    // Separator + button/command hints
    m_display.drawFastHLine(0, 44, 128, SSD1306_WHITE);
    m_display.setCursor(0, 47);
    m_display.print("Btn: next label");
    m_display.setCursor(0, 56);
    m_display.print("Serial: 'start' cmd");

    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// showCollecting()
//
//   y= 0  [INVERTED BAR]  "REC"  |  label
//   y=13  progress bar (128 × 10 px)
//   y=26  size 1  "Samples: 350 / 500"
//   y=36  size 1  "Time left: 3s"
//   y=47  ─── separator ───
//   y=50  size 1  "Short: stop & upload"
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::showCollecting(const char* label,
                                 size_t      count,
                                 size_t      total,
                                 uint32_t    secsLeft) {
    if (!m_ready) return;
    m_display.clearDisplay();

    // Header bar — recording tag left, active label right
    drawHeaderBar("REC", label);

    // Progress bar
    float ratio = (total > 0)
        ? static_cast<float>(count) / static_cast<float>(total)
        : 0.0f;
    drawProgressBar(0, 13, 128, 10, ratio);

    // Sample count and countdown
    m_display.setTextSize(1);
    m_display.setCursor(0, 26);
    m_display.printf("Samples: %u / %u", (unsigned)count, (unsigned)total);

    m_display.setCursor(0, 36);
    m_display.printf("Time left: %us", (unsigned)secsLeft);

    // Separator + hint
    m_display.drawFastHLine(0, 47, 128, SSD1306_WHITE);
    m_display.setCursor(0, 50);
    m_display.print("Short: stop & upload");

    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// showUploading()
//
//   y= 0  [INVERTED BAR]  "UPLOADING"  (centered)
//   y=16  size 1  "Label:   <label>"
//   y=26  size 1  "Samples: <n>"
//   y=40  size 1  "Sending data to"    (centered)
//   y=50  size 1  "Edge Impulse..."    (centered)
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::showUploading(const char* label, size_t sampleCount) {
    if (!m_ready) return;
    m_display.clearDisplay();

    // Centered header bar
    drawHeaderBar("UPLOADING");

    m_display.setTextSize(1);

    m_display.setCursor(0, 16);
    m_display.printf("Label:   %s", label);

    m_display.setCursor(0, 26);
    m_display.printf("Samples: %u", (unsigned)sampleCount);

    printCentered("Sending data to", 40, 1);
    printCentered("Edge Impulse...", 50, 1);

    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// showUploadResult()
//
//   SUCCESS                          FAILED
//   ─────────────────────────        ─────────────────────────
//   [INVERTED]  "UPLOAD OK"          [INVERTED]  "UPLOAD FAILED"
//   Label:   <label>                 Label:   <label>
//   Samples: <n>                     Samples: <n>
//   ── separator ──                  ── separator ──
//   Sent to Edge Impulse!            <error message>
//   Returning to IDLE...             See serial monitor
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::showUploadResult(bool        success,
                                   const char* label,
                                   size_t      sampleCount,
                                   const char* errorMsg) {
    if (!m_ready) return;
    m_display.clearDisplay();

    // Header bar — centered state text
    drawHeaderBar(success ? "UPLOAD OK" : "UPLOAD FAILED");

    m_display.setTextSize(1);

    m_display.setCursor(0, 16);
    m_display.printf("Label:   %s", label);

    m_display.setCursor(0, 26);
    m_display.printf("Samples: %u", (unsigned)sampleCount);

    m_display.drawFastHLine(0, 37, 128, SSD1306_WHITE);

    if (success) {
        printCentered("Sent to Edge Impulse!", 42, 1);
        printCentered("Returning to IDLE...",  52, 1);
    } else {
        // Show up to 21 chars of the error message
        m_display.setCursor(0, 42);
        if (errorMsg) {
            char buf[22] = {};
            strncpy(buf, errorMsg, 21);
            m_display.print(buf);
        } else {
            m_display.print("Upload error");
        }
        printCentered("See serial monitor", 52, 1);
    }

    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// showError()
//
//   y= 0  [INVERTED BAR]  "!! SYSTEM ERROR !!"  (centered)
//   y=15  ─── separator ───
//   y=19  size 1  message line 1  (word-wrapped at 21 chars, up to 3 lines)
//   y=29  size 1  message line 2
//   y=39  size 1  message line 3
//   y=50  ─── separator ───
//   y=54  size 1  "Check wiring + reset"
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::showError(const char* message) {
    if (!m_ready) return;
    m_display.clearDisplay();

    drawHeaderBar("!! SYSTEM ERROR !!");

    m_display.drawFastHLine(0, 14, 128, SSD1306_WHITE);

    // Word-wrap: break at word boundaries up to 21 chars per line
    m_display.setTextSize(1);
    const char* p = message;
    for (uint8_t line = 0; line < 3 && *p; ++line) {
        const char* end       = p;
        const char* lastSpace = nullptr;
        uint8_t     n         = 0;

        while (*end && n < 21) {
            if (*end == ' ') lastSpace = end;
            end++;
            n++;
        }

        // If we hit the limit mid-word and a space exists, break there
        if (*end && lastSpace && n >= 21) {
            end = lastSpace;
        }

        char buf[22] = {};
        uint8_t len = static_cast<uint8_t>(end - p);
        if (len > 21) len = 21;
        memcpy(buf, p, len);

        m_display.setCursor(0, 19 + line * 10);
        m_display.print(buf);

        p = end;
        if (*p == ' ') ++p;   // skip the break space
    }

    m_display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
    printCentered("Check wiring + reset", 54, 1);

    m_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// drawProgressBar()  (private)
// ─────────────────────────────────────────────────────────────────────────────

void OLEDDisplay::drawProgressBar(int16_t x, int16_t y,
                                  int16_t w, int16_t h,
                                  float   ratio) {
    m_display.drawRect(x, y, w, h, SSD1306_WHITE);

    int16_t fillW = static_cast<int16_t>(static_cast<float>(w - 2) * ratio);
    if (fillW > 0) {
        m_display.fillRect(x + 1, y + 1, fillW, h - 2, SSD1306_WHITE);
    }
}
