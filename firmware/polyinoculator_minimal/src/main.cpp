/*
 * Polyinoculator Minimal - Ultra Minimal White LED with Potentiometer Control
 * ESP32-C3 XIAO based polyinoculator with 3x WS2812B LED strips - Standalone Operation
 * 
 * Features:
 * - Three WS2812B LED strips on pins D3, D4, D5 (configurable counts)
 * - Potentiometer brightness control on pin A1
 * - All LEDs display white light, dimmed by potentiometer
 * - No WiFi, no sACN, no network protocols
 * - Maximum battery optimization
 * - Ultra-simple standalone operation
 */

#include <Arduino.h>
#include <FastLED.h>

// Pin definitions for Seeed Studio XIAO ESP32-C3
#define LED_PIN_1 D3       // Strip 1: GPIO4
#define LED_PIN_2 D4       // Strip 2: GPIO5 (longer ribbon)
#define LED_PIN_3 D5       // Strip 3: GPIO6
#define POT_PIN A1         // A1 (GPIO1) - Potentiometer for brightness control

// LED configuration - match original Polyinoculator
#define MAX_LEDS_1 6       // Strip 1: 6 LEDs (small)
#define MAX_LEDS_2 14      // Strip 2: 14 LEDs (longer ribbon)
#define MAX_LEDS_3 6       // Strip 3: 6 LEDs (small)
#define TOTAL_LEDS 26      // Total LEDs across all strips

// LED arrays for each strip
CRGB leds1[MAX_LEDS_1];    // Strip 1
CRGB leds2[MAX_LEDS_2];    // Strip 2 (longer)
CRGB leds3[MAX_LEDS_3];    // Strip 3

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
void updateLEDs();

// Setup function - minimal initialization
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("🔬 Polyinoculator Minimal v1.0 - Ultra Minimal White LEDs + Pot Control");
  
  // Configure pins
  pinMode(POT_PIN, INPUT);
  analogReadResolution(12); // 12-bit ADC resolution (0-4095)
  
  // Initialize FastLED strips
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds1, MAX_LEDS_1);
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds2, MAX_LEDS_2);
  FastLED.addLeds<WS2812B, LED_PIN_3, GRB>(leds3, MAX_LEDS_3);
  
  // Clear all LEDs initially
  FastLED.clear();
  FastLED.show();
  
  Serial.println("✅ Polyinoculator ready - Pot-controlled 26x white LEDs");
  Serial.printf("📍 Strip 1: %d LEDs on D3, Strip 2: %d LEDs on D4, Strip 3: %d LEDs on D5\n", 
                MAX_LEDS_1, MAX_LEDS_2, MAX_LEDS_3);
  Serial.println("🎛️  Turn potentiometer on A1 to control brightness");
}

// Main loop - potentiometer control only
void loop() {
  unsigned long currentMillis = millis();
  
  // Read potentiometer and update LED brightness
  if (currentMillis - lastPotRead > POT_READ_INTERVAL) {
    readPotentiometer();
    updateLEDs();
    lastPotRead = currentMillis;
  }
  
  // Power-efficient delay
  delay(20); // Fast enough for smooth pot response
}

// Read potentiometer with smoothing
void readPotentiometer() {
  // Read potentiometer value from analog pin A1
  potValue = analogRead(POT_PIN); // 12-bit ADC (0-4095)
  
  // Apply exponential smoothing to prevent LED flickering
  float currentValue = (float)potValue / 4095.0; // Normalize to 0.0-1.0
  smoothedPot = (SMOOTHING_FACTOR * currentValue) + ((1.0 - SMOOTHING_FACTOR) * smoothedPot);
  
  // Convert to brightness multiplier
  brightnessMultiplier = smoothedPot;
}

// Update all LED strips based on potentiometer value
void updateLEDs() {
  // Calculate white brightness based on potentiometer
  uint8_t brightness = (uint8_t)(whiteBrightness * brightnessMultiplier);
  
  // Set all LEDs in Strip 1 to white
  for (int i = 0; i < MAX_LEDS_1; i++) {
    leds1[i] = CRGB(brightness, brightness, brightness);
  }
  
  // Set LEDs in Strip 2 to white (pixels 6-11 masked off)
  for (int i = 0; i < MAX_LEDS_2; i++) {
    if (i >= 6 && i <= 11) {
      // Keep pixels 6-11 off (masked)
      leds2[i] = CRGB(0, 0, 0);
    } else {
      // Control pixels 0-5 and 12-13 with potentiometer
      leds2[i] = CRGB(brightness, brightness, brightness);
    }
  }
  
  // Set all LEDs in Strip 3 to white
  for (int i = 0; i < MAX_LEDS_3; i++) {
    leds3[i] = CRGB(brightness, brightness, brightness);
  }
  
  // Update all strips
  FastLED.show();
  
  // Print status occasionally (every ~1 second at 20ms delay)
  static int printCounter = 0;
  if (++printCounter >= 50) {
    Serial.printf("🎛️  Pot: %.2f, LED Brightness: %d (%.1f%%), Total LEDs: %d\n", 
                  brightnessMultiplier, brightness, brightnessMultiplier * 100.0, TOTAL_LEDS);
    printCounter = 0;
  }
}