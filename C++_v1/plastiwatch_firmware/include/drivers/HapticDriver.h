#pragma once

#include "config.h"
#include <Arduino.h>

class HapticDriver {
public:
  HapticDriver();
  void init();
  void trigger(); // Short pulse
  void buzz(uint32_t durationMs);

private:
  // Non-blocking implementation would be better with FreeRTOS timers,
  // but for now blocking is okay for short pulses or use a separate task/timer.
  // We'll use simple blocking for short triggers, or non-blocking if needed.
};
