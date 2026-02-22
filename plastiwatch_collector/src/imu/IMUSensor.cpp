// =============================================================================
// IMUSensor.cpp — PlastiWatch: MPU6050 Driver Implementation
// =============================================================================

#include "IMUSensor.h"
#include "../config/Config.h"
#include "../core/Logger.h"

static const char* TAG = "IMUSensor";

// =============================================================================
// Constructor
// =============================================================================

IMUSensor::IMUSensor()
    : m_ready(false), m_accelScale(0.0f), m_gyroScale(0.0f)
{}

// =============================================================================
// begin()
// =============================================================================

bool IMUSensor::begin() {
    Wire.begin(IMU_I2C_SDA_PIN, IMU_I2C_SCL_PIN, IMU_I2C_CLOCK_HZ);

    // Verify device identity
    uint8_t whoAmI = readReg(REG_WHO_AM_I);
    if (whoAmI != 0x68) {
        Logger::errorf(TAG,
            "WHO_AM_I mismatch: expected 0x68, got 0x%02X. "
            "Check wiring and I2C address.", whoAmI);
        return false;
    }
    Logger::debugf(TAG, "WHO_AM_I = 0x%02X — device detected", whoAmI);

    // Wake the device (clear SLEEP bit)
    if (!writeReg(REG_PWR_MGMT_1, 0x00)) {
        Logger::error(TAG, "Failed to wake device (PWR_MGMT_1 write error)");
        return false;
    }
    delay(100);  // Allow oscillator to stabilise

    // Use PLL with X-axis gyroscope reference (more stable than internal osc)
    if (!writeReg(REG_PWR_MGMT_1, 0x01)) {
        Logger::error(TAG, "Failed to set clock source");
        return false;
    }

    // Enable all axes (default after wake, but explicit is better)
    if (!writeReg(REG_PWR_MGMT_2, 0x00)) {
        Logger::error(TAG, "Failed to enable all axes");
        return false;
    }

    // Configure Digital Low-Pass Filter
    if (!writeReg(REG_CONFIG, IMU_DLPF_CFG & 0x07)) {
        Logger::error(TAG, "Failed to set DLPF");
        return false;
    }

    // Set sample rate divider: rate = 1000 / (1 + SMPLRT_DIV) Hz
    uint8_t sampleRateDiv = (1000u / SAMPLE_RATE_HZ) - 1u;
    if (!writeReg(REG_SMPLRT_DIV, sampleRateDiv)) {
        Logger::error(TAG, "Failed to set sample rate divider");
        return false;
    }

    // Accelerometer full-scale range
    if (!writeReg(REG_ACCEL_CFG, accelFsRegValue())) {
        Logger::error(TAG, "Failed to configure accelerometer FS range");
        return false;
    }

    // Gyroscope full-scale range
    if (!writeReg(REG_GYRO_CFG, gyroFsRegValue())) {
        Logger::error(TAG, "Failed to configure gyroscope FS range");
        return false;
    }

    computeScaleFactors();

    m_ready = true;
    Logger::infof(TAG,
        "Initialised — accel ±%dg  gyro ±%ddps  sample rate %dHz  DLPF %d",
        IMU_ACCEL_FS_G, IMU_GYRO_FS_DPS, SAMPLE_RATE_HZ, IMU_DLPF_CFG);

    return true;
}

// =============================================================================
// readSample()
// =============================================================================

bool IMUSensor::readSample(IMUSample& out) {
    // Burst-read 14 bytes: ACCEL_XOUT_H … GYRO_ZOUT_L (TEMP skipped by index)
    uint8_t buf[14];
    if (!readBurst(REG_ACCEL_XOUT, buf, 14)) {
        Logger::warn(TAG, "Burst read failed");
        return false;
    }

    // Reassemble signed 16-bit big-endian values
    auto toInt16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
    };

    int16_t rawAx = toInt16(buf[0],  buf[1]);
    int16_t rawAy = toInt16(buf[2],  buf[3]);
    int16_t rawAz = toInt16(buf[4],  buf[5]);
    // buf[6..7] = temperature (unused)
    int16_t rawGx = toInt16(buf[8],  buf[9]);
    int16_t rawGy = toInt16(buf[10], buf[11]);
    int16_t rawGz = toInt16(buf[12], buf[13]);

    out.accX = static_cast<float>(rawAx) * m_accelScale;
    out.accY = static_cast<float>(rawAy) * m_accelScale;
    out.accZ = static_cast<float>(rawAz) * m_accelScale;
    out.gyrX = static_cast<float>(rawGx) * m_gyroScale;
    out.gyrY = static_cast<float>(rawGy) * m_gyroScale;
    out.gyrZ = static_cast<float>(rawGz) * m_gyroScale;

    return true;
}

// =============================================================================
// printConfig()
// =============================================================================

void IMUSensor::printConfig() const {
    Logger::info(TAG, "-- MPU6050 Configuration --");
    Logger::infof(TAG, "  SMPLRT_DIV  : 0x%02X", readReg(REG_SMPLRT_DIV));
    Logger::infof(TAG, "  CONFIG      : 0x%02X", readReg(REG_CONFIG));
    Logger::infof(TAG, "  GYRO_CONFIG : 0x%02X", readReg(REG_GYRO_CFG));
    Logger::infof(TAG, "  ACCEL_CONFIG: 0x%02X", readReg(REG_ACCEL_CFG));
    Logger::infof(TAG, "  PWR_MGMT_1  : 0x%02X", readReg(REG_PWR_MGMT_1));
}

// =============================================================================
// runSelfTest()
// =============================================================================

void IMUSensor::runSelfTest() {
    Logger::info(TAG, "Running self-test (10 samples)...");
    if (!m_ready) { Logger::warn(TAG, "Sensor not initialised"); return; }

    float minAcc = 1e9f, maxAcc = -1e9f, sumAcc = 0.0f;
    IMUSample s;
    for (int i = 0; i < 10; i++) {
        if (readSample(s)) {
            float mag = sqrtf(s.accX * s.accX + s.accY * s.accY + s.accZ * s.accZ);
            if (mag < minAcc) minAcc = mag;
            if (mag > maxAcc) maxAcc = mag;
            sumAcc += mag;
        }
        delay(SAMPLE_INTERVAL_MS);
    }
    Logger::infof(TAG,
        "Accel magnitude [m/s²] — min: %.3f  max: %.3f  avg: %.3f  (expect ~9.81)",
        minAcc, maxAcc, sumAcc / 10.0f);
}

// =============================================================================
// Private helpers
// =============================================================================

bool IMUSensor::writeReg(uint8_t reg, uint8_t value) const {
    Wire.beginTransmission(IMU_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

uint8_t IMUSensor::readReg(uint8_t reg) const {
    Wire.beginTransmission(IMU_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(static_cast<uint8_t>(IMU_I2C_ADDRESS), static_cast<uint8_t>(1));
    return Wire.available() ? Wire.read() : 0xFF;
}

bool IMUSensor::readBurst(uint8_t startReg, uint8_t* buf, uint8_t len) const {
    Wire.beginTransmission(IMU_I2C_ADDRESS);
    Wire.write(startReg);
    if (Wire.endTransmission(false) != 0) return false;

    uint8_t received = Wire.requestFrom(
        static_cast<uint8_t>(IMU_I2C_ADDRESS), len);
    if (received != len) return false;

    for (uint8_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

uint8_t IMUSensor::accelFsRegValue() const {
    // AFS_SEL bits [4:3] of ACCEL_CONFIG
    switch (IMU_ACCEL_FS_G) {
        case  4: return 0x08;
        case  8: return 0x10;
        case 16: return 0x18;
        default: return 0x00;  // ±2g
    }
}

uint8_t IMUSensor::gyroFsRegValue() const {
    // FS_SEL bits [4:3] of GYRO_CONFIG
    switch (IMU_GYRO_FS_DPS) {
        case  500: return 0x08;
        case 1000: return 0x10;
        case 2000: return 0x18;
        default:   return 0x00;  // ±250 dps
    }
}

void IMUSensor::computeScaleFactors() {
    // Accelerometer: sensitivity = 16384 / FS_G  LSB/g → convert to m/s²
    constexpr float GRAVITY = 9.80665f;
    float lsbPerG = 16384.0f / static_cast<float>(IMU_ACCEL_FS_G);
    m_accelScale  = GRAVITY / lsbPerG;

    // Gyroscope: sensitivity = 131 / (FS_DPS / 250)  LSB/(deg/s)
    float lsbPerDps = 131.0f / (static_cast<float>(IMU_GYRO_FS_DPS) / 250.0f);
    m_gyroScale     = 1.0f / lsbPerDps;
}
