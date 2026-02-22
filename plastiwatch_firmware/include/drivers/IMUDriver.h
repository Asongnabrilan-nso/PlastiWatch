#pragma once

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "config.h"
#include "system_events.h"

class IMUDriver {
public:
    IMUDriver();
    bool init();
    bool readData(SensorData& data);

private:
    Adafruit_MPU6050 mpu;
};
