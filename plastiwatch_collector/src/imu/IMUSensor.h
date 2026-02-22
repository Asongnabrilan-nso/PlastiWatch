// =============================================================================
// IMUSensor.h — PlastiWatch: MPU6050 Driver Interface
//
// Implements MPU6050 initialisation and raw data reading directly over Wire,
// avoiding heavy third-party library dependencies.
// =============================================================================
#pragma once

#include <Wire.h>
#include "../core/SampleBuffer.h"

// -----------------------------------------------------------------------------
// IMUSensor
// -----------------------------------------------------------------------------
class IMUSensor {
public:
    // -- Lifecycle ------------------------------------------------------------

    IMUSensor();

    /// Initialise I2C bus and configure the MPU6050.
    /// Must be called once in setup() after Serial.begin().
    /// @return true on success; false if the device is unreachable.
    bool begin();

    /// Check whether the sensor was successfully initialised.
    bool isReady() const { return m_ready; }

    // -- Data Acquisition -----------------------------------------------------

    /// Read one sample from the sensor into @p out.
    /// @return true if the read succeeded; false on I2C error.
    bool readSample(IMUSample& out);

    // -- Calibration ----------------------------------------------------------

    /// Apply per-axis bias offsets computed during the startup calibration.
    /// Offsets are subtracted from every subsequent readSample() call.
    /// @param ax/ay/az  Accelerometer bias [m/s²]  (accZ offset keeps gravity)
    /// @param gx/gy/gz  Gyroscope zero-rate bias [deg/s]
    void setOffsets(float ax, float ay, float az,
                    float gx, float gy, float gz);

    // -- Diagnostics ----------------------------------------------------------

    /// Read and print all relevant register values to Serial (INFO level).
    void printConfig() const;

    /// Perform a simple self-test: read 10 samples and log min/max/avg.
    void runSelfTest();

private:
    bool     m_ready;

    // -- MPU6050 register map -------------------------------------------------
    static constexpr uint8_t REG_SMPLRT_DIV  = 0x19;
    static constexpr uint8_t REG_CONFIG      = 0x1A;
    static constexpr uint8_t REG_GYRO_CFG    = 0x1B;
    static constexpr uint8_t REG_ACCEL_CFG   = 0x1C;
    static constexpr uint8_t REG_ACCEL_XOUT  = 0x3B;  ///< First of 14 data bytes
    static constexpr uint8_t REG_PWR_MGMT_1  = 0x6B;
    static constexpr uint8_t REG_PWR_MGMT_2  = 0x6C;
    static constexpr uint8_t REG_WHO_AM_I    = 0x75;

    // -- Scale factors (derived from Config.h FS settings) --------------------
    float m_accelScale;   ///< LSB → m/s²
    float m_gyroScale;    ///< LSB → deg/s

    // -- Calibration offsets (set via setOffsets(), default = 0) --------------
    float m_accelOffsets[3];  ///< Per-axis accel bias [m/s²]
    float m_gyroOffsets[3];   ///< Per-axis gyro zero-rate bias [deg/s]

    // -- Low-level Wire helpers -----------------------------------------------
    bool     writeReg(uint8_t reg, uint8_t value) const;
    uint8_t  readReg(uint8_t reg) const;
    bool     readBurst(uint8_t startReg, uint8_t* buf, uint8_t len) const;

    // -- Configuration helpers ------------------------------------------------
    uint8_t accelFsRegValue() const;
    uint8_t gyroFsRegValue()  const;
    void    computeScaleFactors();
};
