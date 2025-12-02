#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

// Data Structures
struct SensorData {
    float ax, ay, az;
    float gx, gy, gz;
};

enum ActivityClass {
    ACTIVITY_IDLE,
    ACTIVITY_WALKING,
    ACTIVITY_RUNNING,
    ACTIVITY_FALL
};

struct UIEvent {
    enum Type {
        UPDATE_ACTIVITY,
        UPDATE_BATTERY,
        BUTTON_SINGLE_CLICK,
        BUTTON_DOUBLE_CLICK,
        BUTTON_LONG_PRESS
    } type;
    
    union {
        ActivityClass activity;
        float batteryLevel;
    } data;
};

// Global Handles (defined in main.cpp)
extern QueueHandle_t sensorQueue;
extern QueueHandle_t uiQueue;
extern EventGroupHandle_t systemEventGroup;

// Event Group Bits
#define EVENT_BIT_FALL_DETECTED (1 << 0)
#define EVENT_BIT_SLEEP_REQUEST (1 << 1)
