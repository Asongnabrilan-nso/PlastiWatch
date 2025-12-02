#include "drivers/HapticDriver.h"

HapticDriver::HapticDriver() {}

void HapticDriver::init() {
  pinMode(PIN_HAPTIC, OUTPUT);
  digitalWrite(PIN_HAPTIC, LOW);
}

void HapticDriver::trigger() {
  digitalWrite(PIN_HAPTIC, HIGH);
  delay(50); // Short blocking delay, acceptable for haptic feedback usually
  digitalWrite(PIN_HAPTIC, LOW);
}

void HapticDriver::buzz(uint32_t durationMs) {
  digitalWrite(PIN_HAPTIC, HIGH);
  delay(durationMs);
  digitalWrite(PIN_HAPTIC, LOW);
}
