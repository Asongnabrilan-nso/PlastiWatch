/* LIVE INFERENCE SKETCH - ACCEL & GYRO ONLY */

// REPLACE THIS with your actual downloaded library header
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <motion-detection_inferencing.h>

Adafruit_MPU6050 mpu;

// This buffer works based on the number of axes you trained with
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

void setup() {
  Serial.begin(115200);
  Wire.begin(D4, D5);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1)
      delay(10);
  }

  // Ensure these match your training configuration
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Inference ready...");
}

void loop() {
  ei_printf("Sampling...\n");

  uint64_t next_tick = micros();

  // Loop to fill the buffer with data
  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
       i += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {

    sensors_event_t a, g, temp;
    // 'temp' is read here to prevent library errors, but ignored below
    mpu.getEvent(&a, &g, &temp);

    features[i + 0] = a.acceleration.x;
    features[i + 1] = a.acceleration.y;
    features[i + 2] = a.acceleration.z;

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME == 6) {
      features[i + 3] = g.gyro.x;
      features[i + 4] = g.gyro.y;
      features[i + 5] = g.gyro.z;
    }

    // Manage sampling timing accurately
    next_tick += (uint64_t)((1.0 / EI_CLASSIFIER_FREQUENCY) * 1000000);
    while (micros() < next_tick) { /* busy wait */
    }
  }

  // Create signal structure from the filled buffer
  signal_t signal;
  int err = numpy::signal_from_buffer(
      features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

  if (err != 0) {
    ei_printf("Error creating signal: %d\n", err);
    return;
  }

  // Run Inference
  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  if (res != EI_IMPULSE_OK) {
    ei_printf("Classifier error: %d\n", res);
    return;
  }

  // Print Predictions
  ei_printf("Predictions: ");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    ei_printf("%s: %.5f  ", result.classification[ix].label,
              result.classification[ix].value);
  }
  ei_printf("\n");

  delay(100); // Small delay between inferences
}