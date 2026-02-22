// =============================================================================
// OLEDDisplay.h — PlastiWatch: SSD1306 128×64 Display Driver
//
// Wraps the Adafruit SSD1306 library and exposes one screen-drawing method
// per application state.  All methods are no-ops if begin() failed, so the
// rest of the firmware works correctly even if no display is attached.
//
// The OLED shares the I2C bus with the MPU6050.  Wire.begin() must therefore
// be called (inside IMUSensor::begin()) before OLEDDisplay::begin().
//
// Screen layout reference (128 × 64 px, size-1 font = 6 × 8 px/char)
// -----------------------------------------------------------------------
//  IDLE                       COLLECTING
//  ─────────────────────────  ─────────────────────────
//  [IDLE          WiFi:OK  ]  [REC              walking]  ← inverted bar
//                             [████████████░░░░░░░░░░░]  ← progress bar
//   walking  (size 2, cntrd)   Samples: 350 / 500
//                              Time left: 2s
//   WiFi: 192.168.1.42        ─────────────────────────
//  ─────────────────────────   Short: stop & upload
//   Short: change label
//   Hold 2s: RECORD
//
//  UPLOADING                  RESULT / ERROR
//  ─────────────────────────  ─────────────────────────
//  [       UPLOADING        ]  [      UPLOAD OK        ]  ← inverted bar
//   Label:   walking            Label:   walking
//   Samples: 500                Samples: 500
//        Sending data to       ─────────────────────────
//        Edge Impulse...        Sent to Edge Impulse!
//                               Returning to IDLE...
// =============================================================================
#pragma once

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "../config/Config.h"

// =============================================================================
// OLEDDisplay
// =============================================================================
class OLEDDisplay {
public:
    OLEDDisplay();

    /// Initialise the SSD1306.  Must be called after Wire.begin().
    /// @return true on success; false if the device is unreachable.
    bool begin();

    /// True after a successful begin().
    bool isReady() const { return m_ready; }

    // -- Screens --------------------------------------------------------------

    /// Splash screen shown during boot initialisation.
    void showBoot();

    /// Calibration prompt — instructs user to place device on a flat surface.
    void showCalibrationReady();

    /// Calibration in progress — @p progress is 0.0 (start) to 1.0 (done).
    void showCalibrating(float progress);

    /// Calibration complete — shown briefly before transitioning to IDLE.
    void showCalibrationDone();

    /// Pre-recording countdown — shown for each second of RECORDING_START_DELAY_MS.
    /// @param label     Currently selected activity label.
    /// @param secsLeft  Seconds remaining before recording begins (≥ 1).
    void showRecordingCountdown(const char* label, uint32_t secsLeft);

    /// Idle state — waiting for user input.
    void showIdle(const char* label, bool wifiConnected, const char* ip);

    /// Active data collection in progress.
    void showCollecting(const char* label,
                        size_t      count,
                        size_t      total,
                        uint32_t    secsLeft);

    /// HTTP upload in progress (blocking call on the MCU).
    void showUploading(const char* label, size_t sampleCount);

    /// Upload result — shown for OLED_RESULT_DWELL_MS before returning to idle.
    void showUploadResult(bool        success,
                          const char* label,
                          size_t      sampleCount,
                          const char* errorMsg = nullptr);

    /// Fatal / recoverable error screen.
    void showError(const char* message);

    /// Blank the display.
    void clear();

private:
    Adafruit_SSD1306 m_display;
    bool             m_ready;

    /// Draw a filled progress bar with a 1-px white border.
    void drawProgressBar(int16_t x, int16_t y,
                         int16_t w, int16_t h,
                         float   ratio);

    /// Print text centered horizontally at the given y position.
    void printCentered(const char* text, int16_t y, uint8_t size);

    /// Draw an inverted (white-fill, black-text) header bar across the full
    /// width.  leftText is left-aligned; rightText is right-aligned.
    /// Pass nullptr for rightText to center leftText instead.
    void drawHeaderBar(const char* leftText, const char* rightText = nullptr);
};
