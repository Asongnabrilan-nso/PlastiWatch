#include "drivers/IMUDriver.h"
#include "system_events.h"
#include <Arduino.h>

void sensorTask(void *pvParameters) {
  IMUDriver imu;
  if (!imu.init()) {
    Serial.println("IMU Init Failed!");
    vTaskDelete(NULL);
  }

  SensorData data;
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(16); // ~62.5Hz

  xLastWakeTime = xTaskGetTickCount();

  while (true) {
    if (imu.readData(data)) {
      xQueueSend(sensorQueue, &data, 0);
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
