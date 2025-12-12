#include "system_events.h"
#include <Arduino.h>
#include <motion-detection_inferencing.h>

void aiTask(void *pvParameters) {
  SensorData data;

  // Buffer for raw features
  // EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE: Total number of float values in a
  // window
  float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
  size_t feature_ix = 0;

  Serial.println("AI Task Started");

  while (true) {
    // Wait for data from SensorTask
    if (xQueueReceive(sensorQueue, &data, portMAX_DELAY)) {

      // Fill the buffer
      // Note: model_metadata.h defines EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME as 3
      // (Acc only) If your model uses Gyro, you'll need to update this and the
      // model
      features[feature_ix + 0] = data.ax;
      features[feature_ix + 1] = data.ay;
      features[feature_ix + 2] = data.az;
      // features[feature_ix + 3] = data.gx; // Uncomment if model needs 6 axes
      // features[feature_ix + 4] = data.gy;
      // features[feature_ix + 5] = data.gz;

      feature_ix += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;

      // Check if buffer is full
      if (feature_ix >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {

        // Create signal from buffer
        signal_t signal;
        int err = numpy::signal_from_buffer(
            features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0) {
          ei_printf("Failed to create signal from buffer (%d)\n", err);
          feature_ix = 0; // Reset
          continue;
        }

        // Run classifier
        ei_impulse_result_t result = {0};
        EI_IMPULSE_ERROR res =
            run_classifier(&signal, &result, false /* debug */);

        if (res != EI_IMPULSE_OK) {
          ei_printf("ERR: Failed to run classifier (%d)\n", res);
          feature_ix = 0; // Reset
          continue;
        }

        // Print Predictions & logic to update UI
        ei_printf("Predictions: ");
        int best_label_ix = -1;
        float best_val = 0.0f;

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          ei_printf("%s: %.5f  ", result.classification[ix].label,
                    result.classification[ix].value);
          if (result.classification[ix].value > best_val) {
            best_val = result.classification[ix].value;
            best_label_ix = ix;
          }
        }
        ei_printf("\n");

        if (best_label_ix >= 0 && best_val > 0.7f) {
          ActivityClass activity = ACTIVITY_IDLE;
          String label = result.classification[best_label_ix].label;

          if (label == "idle")
            activity = ACTIVITY_IDLE;
          else if (label == "updown")
            activity = ACTIVITY_UPDOWN;
          else if (label == "wave")
            activity = ACTIVITY_WAVE;
          else if (label == "snake")
            activity = ACTIVITY_SNAKE;

          // Send to UI Queue
          UIEvent event;
          event.type = UIEvent::UPDATE_ACTIVITY;
          event.data.activity = activity;
          xQueueSend(uiQueue, &event, 0);
        }

        // Reset buffer index for next window
        feature_ix = 0;
      }
    }
  }
}
