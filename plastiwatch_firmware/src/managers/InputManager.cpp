#include "managers/InputManager.h"

InputManager::InputManager() {
  lastState = HIGH; // Pullup
  lastDebounceTime = 0;
  buttonPressTime = 0;
  buttonPressed = false;
  waitingForDoubleClick = false;
  lastClickTime = 0;
}

void InputManager::init() { pinMode(PIN_BUTTON, INPUT_PULLUP); }

void InputManager::update() {
  bool currentState = digitalRead(PIN_BUTTON);
  uint32_t now = millis();

  // Debounce
  if (currentState != lastState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    // State stabilized
    if (currentState == LOW && !buttonPressed) {
      // Button Down
      buttonPressed = true;
      buttonPressTime = now;
    } else if (currentState == HIGH && buttonPressed) {
      // Button Up
      buttonPressed = false;
      uint32_t pressDuration = now - buttonPressTime;

      if (pressDuration >= LONG_PRESS_TIME) {
        // Long Press
        UIEvent event;
        event.type = UIEvent::BUTTON_LONG_PRESS;
        xQueueSend(uiQueue, &event, 0);
        waitingForDoubleClick = false;
      } else {
        // Short Press - Check for Double Click
        if (waitingForDoubleClick) {
          // Double Click
          UIEvent event;
          event.type = UIEvent::BUTTON_DOUBLE_CLICK;
          xQueueSend(uiQueue, &event, 0);
          waitingForDoubleClick = false;
        } else {
          // First Click, wait to see if second comes
          waitingForDoubleClick = true;
          lastClickTime = now;
        }
      }
    }
  }

  // Timeout for Double Click
  if (waitingForDoubleClick && (now - lastClickTime > DOUBLE_CLICK_TIME)) {
    // Single Click confirmed
    UIEvent event;
    event.type = UIEvent::BUTTON_SINGLE_CLICK;
    xQueueSend(uiQueue, &event, 0);
    waitingForDoubleClick = false;
  }

  lastState = currentState;
}
