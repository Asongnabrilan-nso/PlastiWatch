#include "drivers/DisplayDriver.h"
#include "drivers/HapticDriver.h"
#include "managers/InputManager.h"
#include "system_events.h"
#include <Arduino.h>

void uiTask(void *pvParameters) {
  DisplayDriver display;
  HapticDriver haptic;
  InputManager input;

  display.init();
  haptic.init();
  input.init();

  display.showLogo();
  vTaskDelay(pdMS_TO_TICKS(2000));

  ActivityClass currentActivity = ACTIVITY_IDLE;
  float currentBattery = 100.0;
  bool showingLogo = false;

  display.showActivity(currentActivity, currentBattery);

  UIEvent event;
  while (true) {
    input.update();

    // Check for events
    if (xQueueReceive(uiQueue, &event, 0)) {
      switch (event.type) {
      case UIEvent::UPDATE_ACTIVITY:
        currentActivity = event.data.activity;
        if (!showingLogo)
          display.showActivity(currentActivity, currentBattery);
        break;

      case UIEvent::UPDATE_BATTERY:
        currentBattery = event.data.batteryLevel;
        if (!showingLogo)
          display.showActivity(currentActivity, currentBattery);
        break;

      case UIEvent::BUTTON_SINGLE_CLICK:
        haptic.trigger();
        showingLogo = !showingLogo;
        if (showingLogo)
          display.showLogo();
        else
          display.showActivity(currentActivity, currentBattery);
        break;

      case UIEvent::BUTTON_DOUBLE_CLICK:
        haptic.trigger();
        showingLogo = false;
        display.showActivity(currentActivity, currentBattery);
        break;

      case UIEvent::BUTTON_LONG_PRESS:
        haptic.buzz(500);
        display.turnOff();
        xEventGroupSetBits(systemEventGroup, EVENT_BIT_SLEEP_REQUEST);
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // UI refresh rate / input polling
  }
}
