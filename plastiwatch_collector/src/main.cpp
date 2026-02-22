// =============================================================================
// main.cpp — PlastiWatch Data Collector
//
// Entry point for the PlatformIO / Arduino application.
//
// Boot sequence
//   1. Serial initialised
//   2. I2C bus started (shared by OLED + IMU)
//   3. OLED boot splash displayed
//   4. IMU (MPU6050) detected and configured
//   5. WiFi connection established
//   6. DataCollector state machine enters IDLE (idle screen shown)
//   7. Loop: update() handles all button/serial input and state transitions
//
// See src/config/Config.h to customise WiFi credentials, Edge Impulse API key,
// sample rate, and hardware pin assignments before flashing.
// =============================================================================

#include <Arduino.h>

#include "config/Config.h"
#include "core/Logger.h"
#include "imu/IMUSensor.h"
#include "network/NetworkManager.h"
#include "display/OLEDDisplay.h"
#include "collector/DataCollector.h"

static const char* TAG = "Main";

// ---------------------------------------------------------------------------
// Module-level instances
// ---------------------------------------------------------------------------
static IMUSensor     imu;
static OLEDDisplay   display;
static DataCollector collector(imu, display);

// =============================================================================
// setup()
// =============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1500);  // Allow USB serial to enumerate on host

    // -- Logging --------------------------------------------------------------
#ifdef PLASTIWATCH_DEBUG
    Logger::setLevel(LogLevel::DEBUG);
#else
    Logger::setLevel(LogLevel::INFO);
#endif

    Logger::banner("PlastiWatch — IMU Data Collector  v1.0");

    // -- OLED -----------------------------------------------------------------
    // Wire is not yet started; OLEDDisplay::begin() uses the same Wire instance
    // that IMUSensor::begin() will configure.  We start the bus here first so
    // the display can show the boot splash immediately.
    Wire.begin(IMU_I2C_SDA_PIN, IMU_I2C_SCL_PIN, IMU_I2C_CLOCK_HZ);
    if (!display.begin()) {
        Logger::warn(TAG,
            "OLED not detected — continuing without display. "
            "Check wiring and OLED_I2C_ADDRESS in Config.h.");
    }

    // -- IMU ------------------------------------------------------------------
    Logger::info(TAG, "Initialising IMU...");
    if (!imu.begin()) {
        Logger::error(TAG,
            "IMU initialisation failed. "
            "Check SDA/SCL wiring and I2C address in Config.h. "
            "Halting.");
        // Flash LED rapidly to indicate fatal error
        pinMode(STATUS_LED_PIN, OUTPUT);
        while (true) {
            digitalWrite(STATUS_LED_PIN, LOW);  delay(100);
            digitalWrite(STATUS_LED_PIN, HIGH); delay(100);
        }
    }

    // -- WiFi -----------------------------------------------------------------
    Logger::info(TAG, "Connecting to WiFi...");
    if (!NetworkManager::connect()) {
        Logger::warn(TAG,
            "WiFi connection failed — operating in offline mode. "
            "Data collection will work, but uploads will fail. "
            "Check WIFI_SSID / WIFI_PASSWORD in Config.h.");
    }

    // -- Collector ------------------------------------------------------------
    collector.begin();

    Logger::info(TAG, "Setup complete — ready.");
}

// =============================================================================
// loop()
// =============================================================================

void loop() {
    collector.update();
}
