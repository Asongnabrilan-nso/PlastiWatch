#include "system_events.h"
#include <Arduino.h>

// Placeholder for Edge Impulse
#include <motion-detection_inferencing.h>

void aiTask(void *pvParameters) {
  SensorData data;

  // Placeholder for classifier init
  // run_classifier_init();

  while (true) {
    if (xQueueReceive(sensorQueue, &data, portMAX_DELAY)) {
      // Buffer data for inference
      // ...

      // Run inference (simulated)
      // For now, just print or do nothing
      // If fall detected:
      // xEventGroupSetBits(systemEventGroup, EVENT_BIT_FALL_DETECTED);

      // If activity changes:
      // UIEvent event;
      // event.type = UIEvent::UPDATE_ACTIVITY;
      // event.data.activity = ACTIVITY_WALKING; // Example
      // xQueueSend(uiQueue, &event, 0);
    }
  }
}
