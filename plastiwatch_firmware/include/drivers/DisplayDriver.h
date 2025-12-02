#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "system_events.h"

class DisplayDriver {
public:
    DisplayDriver();
    bool init();
    void showLogo();
    void showActivity(ActivityClass activity, float batteryLevel);
    void turnOff();
    void turnOn();

private:
    Adafruit_SSD1306 display;
    void drawBattery(float level);
};
