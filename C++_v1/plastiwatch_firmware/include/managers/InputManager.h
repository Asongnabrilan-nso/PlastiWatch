#pragma once

#include "config.h"
#include "system_events.h"
#include <Arduino.h>

class InputManager {
public:
  InputManager();
  void init();
  void update(); // Call this periodically (e.g. every 10ms)

private:
  bool lastState;
  uint32_t lastDebounceTime;
  uint32_t buttonPressTime;
  bool buttonPressed;
  bool waitingForDoubleClick;
  uint32_t lastClickTime;

  const uint32_t DEBOUNCE_DELAY = 50;
  const uint32_t LONG_PRESS_TIME = 3000;
  const uint32_t DOUBLE_CLICK_TIME = 400;
};
