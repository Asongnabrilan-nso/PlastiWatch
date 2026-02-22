// =============================================================================
// Config.h — PlastiWatch Data Collector: Central Configuration
//
// Edit the values in this file to match your environment before flashing.
// All project-wide constants live here; no magic numbers elsewhere.
// =============================================================================
#pragma once

// -----------------------------------------------------------------------------
// WiFi
// -----------------------------------------------------------------------------
#define WIFI_SSID                   "Hardware Community 2.4"
#define WIFI_PASSWORD               "ko67!aM3uYU!BDK"
#define WIFI_CONNECT_TIMEOUT_MS     15000   ///< Max time to establish connection
#define WIFI_RECONNECT_INTERVAL_MS  5000    ///< Delay between reconnect attempts

// -----------------------------------------------------------------------------
// Edge Impulse — Data Ingestion API
// Get your API key from: Studio → Dashboard → Keys
// -----------------------------------------------------------------------------
#define EI_API_KEY          "ei_ff23c0d64c355b9d1b956fad0e3030b7979f02d36d5ebdb3be2ba55c029dbbad"
#define EI_DEVICE_NAME      "plastiwatch-001"   ///< Logical device identifier
#define EI_DEVICE_TYPE      "XIAO_ESP32C3"      ///< Device type shown in studio
#define EI_INGESTION_HOST   "ingestion.edgeimpulse.com"
#define EI_INGESTION_PATH   "/api/training/data"
#define EI_HTTP_TIMEOUT_MS  15000               ///< Upload request timeout

// -----------------------------------------------------------------------------
// MPU6050 IMU — Hardware
// Xiao ESP32C3 default I2C: SDA=GPIO6 (D4), SCL=GPIO7 (D5)
// -----------------------------------------------------------------------------
#define IMU_I2C_SDA_PIN     6           ///< I2C data pin
#define IMU_I2C_SCL_PIN     7           ///< I2C clock pin
#define IMU_I2C_CLOCK_HZ    400000      ///< 400 kHz fast-mode I2C
#define IMU_I2C_ADDRESS     0x68        ///< MPU6050 AD0=LOW → 0x68, AD0=HIGH → 0x69

/// Accelerometer full-scale range.  Valid: 2, 4, 8, 16 (g)
#define IMU_ACCEL_FS_G      2

/// Gyroscope full-scale range.  Valid: 250, 500, 1000, 2000 (deg/s)
#define IMU_GYRO_FS_DPS     250

/// Digital Low-Pass Filter cutoff — 0..6.
/// 3 → 44 Hz accel / 42 Hz gyro (good balance for activity recognition)
#define IMU_DLPF_CFG        3

// -----------------------------------------------------------------------------
// Data Collection
// -----------------------------------------------------------------------------
#define SAMPLE_RATE_HZ          100     ///< IMU sampling frequency
#define SAMPLE_INTERVAL_MS      (1000 / SAMPLE_RATE_HZ)
#define COLLECTION_DURATION_S   5       ///< Seconds per labeled window
#define MAX_SAMPLES             (SAMPLE_RATE_HZ * COLLECTION_DURATION_S)

/// Activity labels — must match Edge Impulse project labels exactly
#define NUM_ACTIVITY_LABELS     4
static const char* const ACTIVITY_LABELS[NUM_ACTIVITY_LABELS] = {
    "standing",
    "walking",
    "running",
    "falling"
};

// -----------------------------------------------------------------------------
// Hardware Pins (Xiao ESP32C3)
// -----------------------------------------------------------------------------

// External label-cycle button.
// Wiring: one lead → GPIO3 (D3), other lead → GND.
// The ESP32-C3 internal pull-up resistor (INPUT_PULLUP) holds the line HIGH
// when the button is open.  Pressing the button pulls the line LOW (active LOW).
// No external pull-up resistor required.
#define LABEL_BTN_PIN       3   ///< GPIO3 / D3  (active LOW, uses internal pull-up)
#define STATUS_LED_PIN      10  ///< Onboard LED (active LOW)

/// Minimum time (ms) the signal must be stable before a press is accepted.
/// 50 ms absorbs the mechanical contact bounce of most tactile buttons.
#define BTN_DEBOUNCE_MS     50
#define BTN_LONG_PRESS_MS   2000  ///< Hold ≥ 2 s to start recording

// -----------------------------------------------------------------------------
// OLED Display — SSD1306 128×64 (I2C, shared bus with MPU6050)
// Default I2C address: 0x3C (AD0/SA0 jumper open, factory default).
// Change to 0x3D if the solder jumper on the back of your board is bridged.
// -----------------------------------------------------------------------------
#define OLED_I2C_ADDRESS     0x3C   ///< SSD1306 I2C address
#define OLED_SCREEN_WIDTH    128    ///< Pixel width
#define OLED_SCREEN_HEIGHT   64     ///< Pixel height
#define OLED_RESET_PIN       -1     ///< No dedicated reset pin (share MCU reset)
#define OLED_RESULT_DWELL_MS 2500   ///< How long upload result stays on screen (ms)

// -----------------------------------------------------------------------------
// Serial
// -----------------------------------------------------------------------------
#define SERIAL_BAUD_RATE    115200

// -----------------------------------------------------------------------------
// JSON Payload Buffer
// Worst-case estimate: header (~400 B) + MAX_SAMPLES × ~72 B/sample
// -----------------------------------------------------------------------------
#define JSON_PAYLOAD_MAX_BYTES  (400 + (MAX_SAMPLES * 72))
