// =============================================================================
// DataCollector.cpp — PlastiWatch: Collection State Machine Implementation
// =============================================================================

#include "DataCollector.h"
#include "../config/Config.h"
#include "../core/Logger.h"
#include "../network/NetworkManager.h"
#include "../uploader/EdgeImpulseClient.h"

static const char* TAG = "Collector";

// =============================================================================
// Constructor
// =============================================================================

DataCollector::DataCollector(IMUSensor& imu, OLEDDisplay& display)
    : m_imu(imu)
    , m_display(display)
    , m_state(CollectorState::IDLE)
    , m_labelIndex(0)
    , m_sampleDeadlineMs(0)
    , m_collectionEndMs(0)
    , m_lastProgressMs(0)
    , m_lastIdleDisplayMs(0)
    , m_btnLastRaw(false)
    , m_btnDebounced(false)
    , m_btnLastChangeMs(0)
    , m_btnPressStartMs(0)
{}

// =============================================================================
// begin()
// =============================================================================

void DataCollector::begin() {
    // Enable the ESP32-C3 internal pull-up resistor so the pin sits at logic HIGH
    // when the button is open.  Pressing the button connects GPIO3 to GND (LOW).
    pinMode(LABEL_BTN_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED_PIN,  OUTPUT);
    setLed(false);

    printBanner();
    printStatus();

    // Show initial idle screen now that WiFi status is known
    m_display.showIdle(activeLabel(),
                       NetworkManager::isConnected(),
                       NetworkManager::localIP().c_str());
    m_lastIdleDisplayMs = millis();
}

// =============================================================================
// update() — called from loop()
// =============================================================================

void DataCollector::update() {
    pollButton();
    pollSerial();

    switch (m_state) {
        case CollectorState::IDLE:       handleIdle();       break;
        case CollectorState::COLLECTING: handleCollecting(); break;
        case CollectorState::UPLOADING:  handleUploading();  break;
        case CollectorState::ERROR:      /* wait for user input */ break;
    }
}

// =============================================================================
// State handlers
// =============================================================================

void DataCollector::handleIdle() {
    // Slow heartbeat LED: 50 ms on every 3 s to show we are alive
    static unsigned long lastHeartbeatMs = 0;
    static bool ledOn = false;

    unsigned long now = millis();
    if (!ledOn && now - lastHeartbeatMs > 3000) {
        setLed(true);
        ledOn = true;
        lastHeartbeatMs = now;
    } else if (ledOn && now - lastHeartbeatMs > 50) {
        setLed(false);
        ledOn = false;
    }

    // Refresh OLED every 5 s so WiFi status stays current
    if (now - m_lastIdleDisplayMs >= 5000) {
        m_lastIdleDisplayMs = now;
        m_display.showIdle(activeLabel(),
                           NetworkManager::isConnected(),
                           NetworkManager::localIP().c_str());
    }

    NetworkManager::ensureConnected();
}

void DataCollector::handleCollecting() {
    unsigned long now = millis();

    // -- Check if window has ended -------------------------------------------
    if (now >= m_collectionEndMs || m_buffer.isFull()) {
        Logger::infof(TAG, "Collection complete — %u samples captured",
            m_buffer.count());
        m_state = CollectorState::UPLOADING;
        setLed(false);
        return;
    }

    // -- Throttle to SAMPLE_RATE_HZ ------------------------------------------
    if (now < m_sampleDeadlineMs) return;
    m_sampleDeadlineMs = now + SAMPLE_INTERVAL_MS;

    // -- Read one sample ------------------------------------------------------
    IMUSample sample;
    if (!m_imu.readSample(sample)) {
        Logger::warn(TAG, "IMU read failed — skipping sample");
        return;
    }
    m_buffer.push(sample);

    // -- LED: fast blink during recording -------------------------------------
    setLed((m_buffer.count() / 5) % 2 == 0);  // toggle every 5 samples

    // -- Progress report every second ----------------------------------------
    if (now - m_lastProgressMs >= 1000) {
        m_lastProgressMs = now;
        uint32_t remaining = (m_collectionEndMs > now)
                             ? (m_collectionEndMs - now) / 1000
                             : 0;
        Logger::infof(TAG,
            "  [%s] %u samples — %u s remaining",
            activeLabel(), m_buffer.count(), remaining);
        m_display.showCollecting(activeLabel(),
                                 m_buffer.count(),
                                 SampleBuffer::CAPACITY,
                                 remaining);
    }
}

void DataCollector::handleUploading() {
    const size_t sampleCount = m_buffer.count();  // save before clear

    setLed(true);  // Solid LED while uploading
    m_display.showUploading(activeLabel(), sampleCount);

    Logger::infof(TAG, "Uploading to Edge Impulse (label: \"%s\")...",
        activeLabel());

    UploadResult result = EdgeImpulseClient::upload(m_buffer, activeLabel());
    const bool   ok     = (result == UploadResult::OK);

    if (ok) {
        Logger::info(TAG, "Upload complete");
        blinkLed(3, 200);
    } else {
        Logger::errorf(TAG, "Upload failed: %s",
            EdgeImpulseClient::resultStr(result));
        blinkLed(6, 100);
    }

    // Show result on screen for OLED_RESULT_DWELL_MS before returning to idle
    m_display.showUploadResult(ok, activeLabel(), sampleCount,
        ok ? nullptr : EdgeImpulseClient::resultStr(result));
    delay(OLED_RESULT_DWELL_MS);

    m_buffer.clear();
    m_state = CollectorState::IDLE;
    setLed(false);
    printStatus();

    // Immediately refresh idle screen (don't wait for the 5-second timer)
    m_display.showIdle(activeLabel(),
                       NetworkManager::isConnected(),
                       NetworkManager::localIP().c_str());
    m_lastIdleDisplayMs = millis();
}

// =============================================================================
// Input polling
// =============================================================================

void DataCollector::pollButton() {
    // Active-LOW: button pressed → GPIO reads LOW → rawPressed = true
    bool          rawPressed = (digitalRead(LABEL_BTN_PIN) == LOW);
    unsigned long now        = millis();

    // ── Stage 1: Debounce ────────────────────────────────────────────────────
    // Reset the stability timer whenever the raw reading changes.  Only once
    // the signal has been stable for BTN_DEBOUNCE_MS do we treat it as real.
    if (rawPressed != m_btnLastRaw) {
        m_btnLastChangeMs = now;
        m_btnLastRaw      = rawPressed;
    }
    if (now - m_btnLastChangeMs < BTN_DEBOUNCE_MS) return;  // still settling

    // ── Stage 2: Edge detection ───────────────────────────────────────────────
    // Fire on the rising edge (button released) so the action triggers after a
    // clean, confirmed press rather than on contact closure.

    // Rising edge: stable LOW → stable HIGH  (button released)
    if (!rawPressed && m_btnDebounced) {
        m_btnDebounced = false;
        if (m_state == CollectorState::IDLE) {
            if (now - m_btnPressStartMs >= BTN_LONG_PRESS_MS) {
                startRecording();   // long press (≥ 2 s)
            } else {
                cycleLabel();       // short press — cycle label
            }
        }
    }

    // Falling edge: stable HIGH → stable LOW  (button pressed)
    if (rawPressed && !m_btnDebounced) {
        m_btnDebounced    = true;
        m_btnPressStartMs = now;    // start timing the hold
    }
}

void DataCollector::pollSerial() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
        processSerialCommand(cmd);
    }
}

void DataCollector::processSerialCommand(const String& cmd) {
    Logger::debugf(TAG, "Serial command: \"%s\"", cmd.c_str());

    if (cmd.equalsIgnoreCase("start")) {
        if (m_state == CollectorState::IDLE) {
            startRecording();
        } else {
            Logger::warn(TAG, "Cannot start — not in IDLE state");
        }

    } else if (cmd.equalsIgnoreCase("stop")) {
        if (m_state == CollectorState::COLLECTING) {
            stopAndUpload();
        } else {
            Logger::warn(TAG, "Cannot stop — not recording");
        }

    } else if (cmd.startsWith("label:")) {
        String newLabel = cmd.substring(6);
        newLabel.trim();
        bool found = false;
        for (uint8_t i = 0; i < NUM_ACTIVITY_LABELS; i++) {
            if (newLabel.equalsIgnoreCase(ACTIVITY_LABELS[i])) {
                m_labelIndex = i;
                found = true;
                break;
            }
        }
        if (found) {
            Logger::infof(TAG, "Label set to \"%s\"", activeLabel());
        } else {
            Logger::warnf(TAG, "Unknown label \"%s\"", newLabel.c_str());
        }

    } else if (cmd.equalsIgnoreCase("status")) {
        printStatus();
        NetworkManager::printStatus();

    } else if (cmd.equalsIgnoreCase("selftest")) {
        m_imu.runSelfTest();

    } else if (cmd.equalsIgnoreCase("imuconfig")) {
        m_imu.printConfig();

    } else if (cmd.equalsIgnoreCase("help")) {
        Serial.println("\nAvailable commands:");
        Serial.println("  label:<name>  — set label (standing|walking|running|falling)");
        Serial.println("  start         — begin recording");
        Serial.println("  stop          — stop early and upload");
        Serial.println("  status        — show current state");
        Serial.println("  selftest      — run IMU self-test");
        Serial.println("  imuconfig     — print IMU register values");
        Serial.println("  help          — show this message\n");

    } else {
        Logger::warnf(TAG, "Unknown command \"%s\" — type \"help\"", cmd.c_str());
    }
}

// =============================================================================
// Actions
// =============================================================================

void DataCollector::startRecording() {
    if (!m_imu.isReady()) {
        Logger::error(TAG, "IMU not ready — cannot record");
        m_state = CollectorState::ERROR;
        return;
    }
    if (!NetworkManager::isConnected()) {
        Logger::warn(TAG, "WiFi not connected — attempting connection");
        if (!NetworkManager::connect()) {
            Logger::error(TAG, "Cannot upload without WiFi — aborting");
            return;
        }
    }

    m_buffer.clear();
    m_state            = CollectorState::COLLECTING;
    m_sampleDeadlineMs = millis();
    m_collectionEndMs  = millis() + (COLLECTION_DURATION_S * 1000UL);
    m_lastProgressMs   = millis();

    Logger::infof(TAG,
        "Recording started — label: \"%s\"  duration: %d s  rate: %d Hz",
        activeLabel(), COLLECTION_DURATION_S, SAMPLE_RATE_HZ);

    // Show collecting screen immediately so the user sees feedback
    m_display.showCollecting(activeLabel(), 0, SampleBuffer::CAPACITY,
                             COLLECTION_DURATION_S);
    blinkLed(2, 150);
}

void DataCollector::stopAndUpload() {
    Logger::infof(TAG, "Recording stopped early — %u samples", m_buffer.count());
    m_collectionEndMs = millis();  // Force handleCollecting to transition
}

void DataCollector::cycleLabel() {
    m_labelIndex = (m_labelIndex + 1) % NUM_ACTIVITY_LABELS;
    Logger::infof(TAG, "Active label: \"%s\"", activeLabel());

    // Refresh idle screen immediately so the new label is visible
    m_display.showIdle(activeLabel(),
                       NetworkManager::isConnected(),
                       NetworkManager::localIP().c_str());
    m_lastIdleDisplayMs = millis();
    blinkLed(1, 100);
}

// =============================================================================
// UI helpers
// =============================================================================

void DataCollector::printBanner() const {
    Logger::separator('=');
    Logger::info(TAG, "  PlastiWatch — IMU Data Collector for Edge Impulse");
    Logger::infof(TAG, "  Sample rate : %d Hz", SAMPLE_RATE_HZ);
    Logger::infof(TAG, "  Window      : %d s (%d samples max)",
        COLLECTION_DURATION_S, MAX_SAMPLES);
    Logger::infof(TAG, "  Device      : %s", EI_DEVICE_NAME);
    Logger::separator('=');
    Serial.println("\nSerial commands: type \"help\" for a full list.");
    Serial.println("Button (GPIO3): short press = cycle label | hold 2 s = start recording");
    Serial.println("Recording    : also startable via serial command  start\n");
}

void DataCollector::printStatus() const {
    const char* stateStr = "UNKNOWN";
    switch (m_state) {
        case CollectorState::IDLE:       stateStr = "IDLE";       break;
        case CollectorState::COLLECTING: stateStr = "COLLECTING"; break;
        case CollectorState::UPLOADING:  stateStr = "UPLOADING";  break;
        case CollectorState::ERROR:      stateStr = "ERROR";      break;
    }
    Logger::infof(TAG, "State: %-12s  Label: \"%s\"  WiFi: %s",
        stateStr, activeLabel(),
        NetworkManager::isConnected() ? "connected" : "disconnected");
}

void DataCollector::setLed(bool on) const {
    // Xiao ESP32C3 LED is active LOW
    digitalWrite(STATUS_LED_PIN, on ? LOW : HIGH);
}

void DataCollector::blinkLed(uint8_t times, uint16_t periodMs) const {
    for (uint8_t i = 0; i < times; i++) {
        setLed(true);
        delay(periodMs / 2);
        setLed(false);
        delay(periodMs / 2);
    }
}
