/**
 * Headless Data Logger for Edge Impulse
 * Board: Seeed XIAO ESP32C3 + Expansion Base
 * Sensor: MPU6050
 * Output: CSV files on SD Card
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- Configuration ---
const int SD_CS_PIN = D2;        // Chip Select for Expansion Base
const int SAMPLE_RATE_HZ = 62.5; // Edge Impulse often defaults to 62.5Hz, but 50-100Hz is fine
const unsigned long INTERVAL_MS = 1000 / SAMPLE_RATE_HZ;

// --- Objects ---
Adafruit_MPU6050 mpu;
File dataFile;
String fileName;

// --- State Variables ---
unsigned long lastSampleTime = 0;
bool isError = false;

// --- Helper: Blink LED to indicate error ---
void signalError() {
  isError = true;
  while (true) {
    digitalWrite(10, LOW);  // On
    delay(100);
    digitalWrite(10, HIGH); // Off
    delay(100);
  }
}

// --- Helper: Find next available filename ---
// Looks for DATA_00.csv, DATA_01.csv, etc.
String getNextFileName() {
  int index = 0;
  String name;
  while (true) {
    // Format filename to fit 8.3 convention just to be safe with older FAT
    if (index < 10) name = "/DATA_0" + String(index) + ".csv";
    else name = "/DATA_" + String(index) + ".csv";
    
    if (!SD.exists(name)) {
      return name;
    }
    index++;
    if (index > 99) signalError(); // Too many files, clean your SD card
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // LED Off (Active LOW usually)

  // 1. Initialize SD Card
  // Note: ESP32C3 SPI pins are remappable, but standard library handles default usually.
  // If fails, ensure your board is selected correctly in IDE.
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed");
    signalError();
  }

  // 2. Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 Chip not found");
    signalError();
  }

  // 3. Sensor Settings (Tweak based on expected motion intensity)
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G); // Walking/Running usually fits in 4G
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // 4. Create File and Write Header
  fileName = getNextFileName();
  Serial.print("Logging to: "); Serial.println(fileName);
  
  dataFile = SD.open(fileName, FILE_WRITE);
  if (!dataFile) {
    Serial.println("Failed to open file for writing");
    signalError();
  }

  // Edge Impulse CSV Header
  dataFile.println("timestamp,accX,accY,accZ,gyroX,gyroY,gyroZ");
  dataFile.flush(); // Ensure header is written
  
  // Visual cue that we are ready: Long 2s LED ON
  digitalWrite(10, LOW); 
  delay(2000);
  digitalWrite(10, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking sampling loop
  if (currentMillis - lastSampleTime >= INTERVAL_MS) {
    lastSampleTime = currentMillis;

    // Get new sensor events
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Prepare CSV String
    // Format: timestamp, ax, ay, az, gx, gy, gz
    // Units: m/s^2 and rad/s (Adafruit standard)
    String dataString = String(currentMillis) + "," + 
                        String(a.acceleration.x) + "," + 
                        String(a.acceleration.y) + "," + 
                        String(a.acceleration.z) + "," + 
                        String(g.gyro.x) + "," + 
                        String(g.gyro.y) + "," + 
                        String(g.gyro.z);

    // Write to SD
    // Note: We open/close or flush periodically. 
    // Keeping it open is faster but risks data loss on power cut.
    // For a run, we assume battery holds. We flush every write or every N writes.
    if (dataFile) {
      dataFile.println(dataString);
      
      // Flush occasionally to save data (e.g., every write is safer but slower)
      // Removing this flush will save battery and speed, but risks losing the whole file if power cuts.
      dataFile.flush(); 
      
      // Optional: Blink LED briefly every sample (can be annoying) 
      // or toggle LED state to show aliveness.
      // digitalWrite(10, !digitalRead(10)); 
    } else {
      signalError();
    }
  }
}