#include <Wire.h>

#define I2C_SDA 6   // XIAO ESP32C3 Expansion Board SDA pin
#define I2C_SCL 7   // XIAO ESP32C3 Expansion Board SCL pin

void setup() {
  Serial.begin(115200);
  // Initialize I2C with correct pins
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("\nI2C Scanner starting...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("Scan complete\n");

  delay(2000);  // wait 2 seconds before next scan
}
