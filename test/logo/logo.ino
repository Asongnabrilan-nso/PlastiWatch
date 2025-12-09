#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- DISPLAY CONFIGURATION ---
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // I2C address for standard 128x64 display (common for 0.96 inch)

// Initialize the display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ======================================================================
// --- LOGO C-ARRAY DATA (128x64 pixels, 1024 bytes total) ---
// This is the full PlastiBytes/PlastiWatch logo data.
// ======================================================================
const unsigned char g_logo_array[] PROGMEM = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x8f, 0x07, 0x07, 0x07, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe3, 0xf1, 0xf8, 0x3c, 0x08, 0x00, 0x00, 0x00, 0x00, 0xc0, 
	0xf1, 0x1f, 0x1f, 0x3f, 0x7f, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 
	0xcf, 0xcf, 0xc7, 0xc7, 0xc7, 0xc3, 0xe3, 0xf6, 0xfc, 0xf8, 0xf0, 0x00, 0x01, 0x07, 0xff, 0xff, 
	0xff, 0xfe, 0xfc, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x38, 0x38, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f
};
// ======================================================================

void setup() {
  // 1. Initialize the display
  // If the initialization fails (returns 0), the program will halt here.
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // You can use Serial.begin() and Serial.println() here for debugging if needed
    for (;;)
      ;  // Loop forever if initialization fails
  }

  // 2. Clear the buffer (to make sure it's fully black first)
  display.clearDisplay();

  // 3. Draw the bitmap
  // The logo is 128x64 pixels, so we draw it at (0, 0) to cover the whole screen.
  display.drawBitmap(
    0,
    0,
    g_logo_array,
    SCREEN_WIDTH, SCREEN_HEIGHT,
    SSD1306_WHITE);

  // 4. Push the buffer content to the physical display
  display.display();
}

void loop() {
  // No code needed in loop() because the logo is static and drawn only once in setup()
}