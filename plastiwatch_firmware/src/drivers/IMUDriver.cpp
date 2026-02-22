#include "drivers/IMUDriver.h"

IMUDriver::IMUDriver() {}

bool IMUDriver::init() {
    if (!mpu.begin(I2C_ADDR_MPU6050)) {
        Serial.println("Failed to find MPU6050 chip");
        return false;
    }
    
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    
    return true;
}

bool IMUDriver::readData(SensorData& data) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    data.ax = a.acceleration.x;
    data.ay = a.acceleration.y;
    data.az = a.acceleration.z;
    
    data.gx = g.gyro.x;
    data.gy = g.gyro.y;
    data.gz = g.gyro.z;

    return true;
}
