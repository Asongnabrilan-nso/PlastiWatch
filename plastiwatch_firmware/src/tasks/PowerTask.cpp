#include "config.h"
#include "system_events.h"
#include <Arduino.h>

void powerTask(void *pvParameters) {
  const TickType_t xFrequency = pdMS_TO_TICKS(10000); // Check every 10s
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (true) {
    // Check for Sleep Request
    EventBits_t bits = xEventGroupGetBits(systemEventGroup);
    if (bits & EVENT_BIT_SLEEP_REQUEST) {
      Serial.println("Entering Deep Sleep...");

      // Configure Wakeup
      // Wake up on Button Low (assuming pullup)
      esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_BUTTON,
                                        ESP_GPIO_WAKEUP_GPIO_LOW);

      esp_deep_sleep_start();
    }

    // Read Battery
    uint32_t raw = analogRead(PIN_BATTERY_ADC);
    // Convert to voltage (assuming standard Xiao divider, check docs if needed)
    // Xiao ESP32C3: Voltage divider is usually 100k/100k or similar.
    // Let's assume raw mapping for now or simple percentage.
    // 3.7V LiPo: 4.2V = 100%, 3.3V = 0%.
    // ADC is 12-bit (0-4095). 3.3V ref.
    // If divider is 1/2, max input is 1.65V at 3.3V? No, usually divider scales
    // to < 3.3V. Let's assume raw value for now and just map 0-4095 to 0-100%
    // loosely.
    float voltage = (raw / 4095.0) * 3.3 * 2; // Assuming 1/2 divider
    float level = (voltage - 3.3) / (4.2 - 3.3) * 100.0;
    if (level > 100)
      level = 100;
    if (level < 0)
      level = 0;

    UIEvent event;
    event.type = UIEvent::UPDATE_BATTERY;
    event.data.batteryLevel = level;
    xQueueSend(uiQueue, &event, 0);

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
