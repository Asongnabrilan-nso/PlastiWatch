#include "drivers/DisplayDriver.h"

DisplayDriver::DisplayDriver() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

bool DisplayDriver::init() {
    // Address 0x3C for 128x32, 0x3D for 128x64 usually, but config says 0x3C
    if(!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR_OLED)) {
        Serial.println(F("SSD1306 allocation failed"));
        return false;
    }
    display.clearDisplay();
    display.display();
    return true;
}

void DisplayDriver::showLogo() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println(F("PLASTI"));
    display.setCursor(10, 40);
    display.println(F("WATCH"));
    display.display();
}

void DisplayDriver::showActivity(ActivityClass activity, float batteryLevel) {
    display.clearDisplay();
    
    // Draw Battery
    drawBattery(batteryLevel);

    // Draw Activity
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);

    switch(activity) {
        case ACTIVITY_IDLE:
            display.println(F("IDLE"));
            break;
        case ACTIVITY_WALKING:
            display.println(F("WALKING"));
            break;
        case ACTIVITY_RUNNING:
            display.println(F("RUNNING"));
            break;
        case ACTIVITY_FALL:
            display.println(F("FALL!"));
            break;
    }
    display.display();
}

void DisplayDriver::drawBattery(float level) {
    // Simple battery icon top right
    display.drawRect(110, 0, 18, 10, SSD1306_WHITE);
    display.fillRect(112, 2, 14 * (level / 100.0), 6, SSD1306_WHITE);
    display.fillRect(128, 3, 2, 4, SSD1306_WHITE);
}

void DisplayDriver::turnOff() {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void DisplayDriver::turnOn() {
    display.ssd1306_command(SSD1306_DISPLAYON);
}
