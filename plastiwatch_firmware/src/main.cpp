#include "config.h"
#include "managers/OTAManager.h"
#include "system_events.h"
#include <Arduino.h>

// Global Handles
QueueHandle_t sensorQueue = NULL;
QueueHandle_t uiQueue = NULL;
EventGroupHandle_t systemEventGroup = NULL;

// Task Handles
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t aiTaskHandle = NULL;
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t powerTaskHandle = NULL;

// Forward declarations of task functions
void sensorTask(void *pvParameters);
void aiTask(void *pvParameters);
void uiTask(void *pvParameters);
void powerTask(void *pvParameters);

OTAManager otaManager;

void setup() {
  Serial.begin(115200);
  // while (!Serial); // Don't wait for serial in production

  Serial.println("Plastiwatch Booting...");

  // Initialize Hardware
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_HAPTIC, OUTPUT);
  digitalWrite(PIN_HAPTIC, LOW);

  // Initialize OTA
  otaManager.init();

  // Create Queues & Event Groups
  sensorQueue = xQueueCreate(10, sizeof(SensorData));
  uiQueue = xQueueCreate(5, sizeof(UIEvent));
  systemEventGroup = xEventGroupCreate();

  // Create Tasks
  xTaskCreate(sensorTask, "Sensor", STACK_SIZE_SENSOR, NULL, PRIORITY_SENSOR,
              &sensorTaskHandle);
  xTaskCreate(aiTask, "AI", STACK_SIZE_AI, NULL, PRIORITY_AI, &aiTaskHandle);
  xTaskCreate(uiTask, "UI", STACK_SIZE_UI, NULL, PRIORITY_UI, &uiTaskHandle);
  xTaskCreate(powerTask, "Power", STACK_SIZE_POWER, NULL, PRIORITY_POWER,
              &powerTaskHandle);

  Serial.println("Tasks Created. Scheduler running.");
}

void loop() {
  // Main loop is empty, everything is in FreeRTOS tasks
  // ElegantOTA loop if needed
  otaManager.handle();
  vTaskDelay(10); // Yield
}
