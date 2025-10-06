/*
 * Ostoregenerator Firmware - Ultra Minimal White LED with Potentiometer Control
 * ESP32-C3 XIAO based ostoregenerator with single WS2812B LED - Standalone Operation
 * 
 * Features:
 * - 100x WS2812B white LEDs on pin D3
 * - Potentiometer brightness control on pin A0
 * - No WiFi, no sACN, no network protocols
 * - Maximum battery optimization
 * - Ultra-simple standalone operation
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Pin definitions for ESP32-C3 XIAO
#define LED_PIN D3    // D3 (GPIO5) - 100x WS2812B LEDs
#define POT_PIN A0    // A0 (GPIO0) - Potentiometer for brightness control
#define NUM_LEDS 100  // 100 LEDs in the strip

// LED configuration for WS2812B RGB LED
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Potentiometer control variables
int potValue = 0;                    // Raw potentiometer reading (0-4095)
float brightnessMultiplier = 1.0;    // Brightness multiplier from pot (0.0-1.0)
float smoothedPot = 0.0;             // Exponentially smoothed potentiometer value
unsigned long lastPotRead = 0;       // Last time we read the potentiometer
const unsigned long POT_READ_INTERVAL = 50;  // Read pot every 50ms for smooth control
const float SMOOTHING_FACTOR = 0.1;  // Exponential smoothing factor

// LED state
uint8_t whiteBrightness = 204;       // 80% white intensity (204/255 = 0.8)

// Function declarations
void readPotentiometer();
void updateLED();

// Setup function - minimal initialization
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("🔄 Ostoregenerator v2.0 - Ultra Minimal White LED + Pot Control");
  
  // Configure pins
  pinMode(POT_PIN, INPUT);
  analogReadResolution(12); // 12-bit ADC resolution (0-4095)
  
  // Initialize WS2812B LED strip
  strip.begin();
  strip.clear();
  strip.show();
  
  Serial.println("✅ Ostoregenerator ready - Pot-controlled 100x white LEDs");
  Serial.println("🎛️  Turn potentiometer to control brightness");
}

// Main loop - potentiometer control only
void loop() {
  unsigned long currentMillis = millis();
  
  // Read potentiometer and update LED brightness
  if (currentMillis - lastPotRead > POT_READ_INTERVAL) {
    readPotentiometer();
    updateLED();
    lastPotRead = currentMillis;
  }
  
  // Power-efficient delay
  delay(20); // Fast enough for smooth pot response
}

// Read potentiometer with smoothing
void readPotentiometer() {
  // Read potentiometer value from analog pin A0
  potValue = analogRead(POT_PIN); // 12-bit ADC (0-4095)
  
  // Apply exponential smoothing to prevent LED flickering
  float currentValue = (float)potValue / 4095.0; // Normalize to 0.0-1.0
  smoothedPot = (SMOOTHING_FACTOR * currentValue) + ((1.0 - SMOOTHING_FACTOR) * smoothedPot);
  
  // Convert to brightness multiplier
  brightnessMultiplier = smoothedPot;
}

// Update LED based on potentiometer value
void updateLED() {
  // Calculate white brightness based on potentiometer
  uint8_t brightness = (uint8_t)(whiteBrightness * brightnessMultiplier);
  
  // Set all 100 LEDs to white (R=G=B for white color)
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(brightness, brightness, brightness));
  }
  strip.show();
  
  // Print status occasionally (every ~1 second at 20ms delay)
  static int printCounter = 0;
  if (++printCounter >= 50) {
    Serial.printf("🎛️  Pot: %.2f, LED Brightness: %d (%.1f%%)\n", 
                  brightnessMultiplier, brightness, brightnessMultiplier * 100.0);
    printCounter = 0;
  }
}