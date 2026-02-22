// =============================================================================
// DataCollector.h — PlastiWatch: High-level Collection State Machine
//
// Orchestrates the full data collection cycle:
//   IDLE → COLLECTING → UPLOADING → IDLE
//
// Trigger modes
//   Hardware : Short press (< 2 s) — cycle the active label.
//              Long press  (≥ 2 s) — start recording (same as serial "start").
//              The button uses the ESP32-C3 internal pull-up resistor — no
//              external resistor needed.  Logic is active LOW (pressed = LOW).
//   Serial   : Commands entered in the serial monitor.
//
// Serial commands (send with newline):
//   label:<name>   — set active label  (e.g. "label:running")
//   start          — begin recording
//   stop           — stop recording early and upload
//   status         — print current state and label
//   selftest       — run IMU self-test
//   help           — list available commands
// =============================================================================
#pragma once

#include <Arduino.h>
#include "../core/SampleBuffer.h"
#include "../imu/IMUSensor.h"
#include "../display/OLEDDisplay.h"

// -----------------------------------------------------------------------------
// Collector state
// -----------------------------------------------------------------------------
enum class CollectorState : uint8_t {
    IDLE,
    COLLECTING,
    UPLOADING,
    ERROR
};

// -----------------------------------------------------------------------------
// DataCollector
// -----------------------------------------------------------------------------
class DataCollector {
public:
    DataCollector(IMUSensor& imu, OLEDDisplay& display);

    /// One-time setup.  Call from Arduino setup().
    void begin();

    /// Periodic update.  Call from Arduino loop() — returns immediately.
    void update();

    /// Current FSM state.
    CollectorState state() const { return m_state; }

    /// Currently selected activity label.
    const char* activeLabel() const { return ACTIVITY_LABELS[m_labelIndex]; }

private:
    // -- Dependencies ---------------------------------------------------------
    IMUSensor&   m_imu;
    OLEDDisplay& m_display;
    SampleBuffer m_buffer;

    // -- State ----------------------------------------------------------------
    CollectorState m_state;
    uint8_t        m_labelIndex;

    // -- Timing ---------------------------------------------------------------
    unsigned long m_sampleDeadlineMs;   ///< Next sample due timestamp
    unsigned long m_collectionEndMs;    ///< When to stop collecting
    unsigned long m_lastProgressMs;     ///< Last progress print timestamp
    unsigned long m_lastIdleDisplayMs;  ///< Last idle-screen refresh timestamp

    // -- Button debounce & long-press detection --------------------------------
    // Standard two-stage debouncer:
    //   m_btnLastRaw      — last raw digitalRead() value (may still be bouncing)
    //   m_btnDebounced    — stable pressed state (true once line is stable LOW)
    //   m_btnLastChangeMs — timestamp of the last raw-state change
    //   m_btnPressStartMs — when the button was stably pressed (falling edge)
    //                       used to distinguish short press (cycle label) from
    //                       long press ≥ BTN_LONG_PRESS_MS (start recording)
    bool          m_btnLastRaw;
    bool          m_btnDebounced;
    unsigned long m_btnLastChangeMs;
    unsigned long m_btnPressStartMs;

    // -- State handlers -------------------------------------------------------
    void handleIdle();
    void handleCollecting();
    void handleUploading();

    // -- Startup helpers ------------------------------------------------------
    /// Blocking wait for the user button to be pressed and released.
    /// Used during calibration prompt in begin().
    void waitForButtonPress();

    /// Collect IMU_CALIB_SAMPLES samples, compute mean bias offsets, and apply
    /// them to the IMU sensor via setOffsets().  Shows progress on the OLED.
    void runCalibration();

    // -- Input ----------------------------------------------------------------
    void pollButton();
    void pollSerial();
    void processSerialCommand(const String& cmd);

    // -- Actions --------------------------------------------------------------
    void startRecording();
    void stopAndUpload();
    void cycleLabel();

    // -- UI helpers -----------------------------------------------------------
    void printBanner() const;
    void printStatus() const;
    void setLed(bool on) const;
    void blinkLed(uint8_t times, uint16_t periodMs) const;
};
