#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Pin and pixel configuration
#define LED_PIN 5        // Pin D3 on Seeed XIAO ESP32C3 
#define NUM_PIXELS 100   // Number of pixels in the strip
#define BRIGHTNESS 128   // 50% brightness (0-255)

// Create NeoPixel object
Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Initialize the NeoPixel strip
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // Initialize all pixels to 'off'

  // Set all pixels to 50% white (all 3 channels at 50%)
  for(int i = 0; i < NUM_PIXELS; i++) {
    // 127 is approximately 50% of 255
    strip.setPixelColor(i, strip.Color(127, 127, 127));
  }
  
  // Show the colors
  strip.show();
}

void loop() {
  // Nothing to do in loop - just maintain the static pattern
  delay(1000);
}
