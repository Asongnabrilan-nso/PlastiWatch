#pragma once

#include <Arduino.h>

// Pin Definitions (Xiao ESP32C3)
#define PIN_BATTERY_ADC 2 // D0/A0
#define PIN_BUTTON 3      // D1/A1
#define PIN_HAPTIC 4      // D2/A2
#define PIN_I2C_SDA 6     // D4
#define PIN_I2C_SCL 7     // D5

// I2C Addresses
#define I2C_ADDR_MPU6050 0x68
#define I2C_ADDR_OLED 0x3C

// System Constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Task Priorities (Higher number = Higher priority)
#define PRIORITY_SENSOR 3
#define PRIORITY_AI 2
#define PRIORITY_UI 1
#define PRIORITY_POWER 1

// Task Stack Sizes
#define STACK_SIZE_SENSOR 4096
#define STACK_SIZE_AI 8192
#define STACK_SIZE_UI 4096
#define STACK_SIZE_POWER 2048

// Deep Sleep
#define BUTTON_HOLD_TIME_MS 3000

// Wi-Fi & OTA
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define OTA_PORT 80
