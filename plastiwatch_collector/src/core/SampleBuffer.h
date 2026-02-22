// =============================================================================
// SampleBuffer.h — PlastiWatch: Fixed-capacity IMU sample storage
//
// Stores one labeled data window of IMUSample structs.
// All 6 DOF (3-axis accel + 3-axis gyro) are retained per sample.
// =============================================================================
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../config/Config.h"

// -----------------------------------------------------------------------------
// IMUSample — one timestamped sensor reading
// -----------------------------------------------------------------------------
struct IMUSample {
    float accX;  ///< Acceleration X  [m/s²]
    float accY;  ///< Acceleration Y  [m/s²]
    float accZ;  ///< Acceleration Z  [m/s²]
    float gyrX;  ///< Angular rate X  [deg/s]
    float gyrY;  ///< Angular rate Y  [deg/s]
    float gyrZ;  ///< Angular rate Z  [deg/s]
};

// -----------------------------------------------------------------------------
// SampleBuffer — statically-allocated ring for one collection window
// -----------------------------------------------------------------------------
class SampleBuffer {
public:
    static constexpr size_t CAPACITY = MAX_SAMPLES;

    // -- Lifecycle ------------------------------------------------------------

    SampleBuffer() : m_count(0) {}

    /// Reset the buffer to empty without touching data memory.
    void clear() { m_count = 0; }

    // -- Mutation -------------------------------------------------------------

    /// Append a sample.
    /// @return true if the sample was stored; false if the buffer is full.
    bool push(const IMUSample& sample) {
        if (isFull()) return false;
        m_samples[m_count++] = sample;
        return true;
    }

    // -- Query ----------------------------------------------------------------

    /// Number of samples currently stored.
    size_t count() const { return m_count; }

    /// True when no more samples can be pushed.
    bool isFull() const { return m_count >= CAPACITY; }

    /// True when no samples have been stored since the last clear().
    bool isEmpty() const { return m_count == 0; }

    /// Fill ratio as a value 0.0–1.0.
    float fillRatio() const { return static_cast<float>(m_count) / CAPACITY; }

    /// Nominal interval between samples (ms), derived from compile-time config.
    float intervalMs() const { return static_cast<float>(SAMPLE_INTERVAL_MS); }

    // -- Access ---------------------------------------------------------------

    const IMUSample& operator[](size_t index) const { return m_samples[index]; }
    IMUSample&       operator[](size_t index)       { return m_samples[index]; }

    const IMUSample* data() const { return m_samples; }

private:
    IMUSample m_samples[CAPACITY];
    size_t    m_count;
};
