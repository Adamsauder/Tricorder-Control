/*
 * Rainbow Display Firmware
 * 
 * ESP32-C3 XIAO Rainbow LED Controller
 * - 60 RGB pixels total (WS2812B)
 * - First 30 pixels on D3 (GPIO4)
 * - Second 30 pixels on D4 (GPIO5)
 * - Continuous rainbow pattern animation
 * 
 * Hardware: Seeed Studio XIAO ESP32-C3
 * 
 * Created: September 2025
 */

#include <Arduino.h>
#include <FastLED.h>

// Pin definitions for Seeed XIAO ESP32-C6 (using C3 toolchain for compatibility)
#define LED_PIN_1 D3     // D3 pin (GPIO4) - First 50 pixels
#define LED_PIN_2 D4     // D4 pin (GPIO5) - Second 57 pixels
#define SWITCH_PIN D10   // D10 pin - Mode switch (3.3V = white, disconnected = rainbow)
#define BRIGHTNESS_POT D0 // D0 pin (GPIO1) - Brightness control potentiometer

// LED configuration
#define NUM_LEDS_STRIP1 50
#define NUM_LEDS_STRIP2 57
#define TOTAL_LEDS 107
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Animation settings
#define MAX_BRIGHTNESS 127     // Maximum brightness (0-255) - 50% of full brightness
#define MIN_BRIGHTNESS 10      // Minimum brightness to avoid completely off
#define FRAMES_PER_SECOND 60   // Animation speed
#define RAINBOW_SPEED 3        // Rainbow rotation speed

// LED arrays
CRGB leds1[NUM_LEDS_STRIP1];  // Strip 1 (D3) - 50 pixels
CRGB leds2[NUM_LEDS_STRIP2];  // Strip 2 (D4) - 120 pixels

// Animation variables
uint8_t gHue = 0;  // Rotating base color

// Mode control variables
bool whiteMode = false;      // Current mode: false = rainbow, true = white
bool lastSwitchState = false; // Previous switch state for change detection

// Brightness control variables
uint8_t currentBrightness = MAX_BRIGHTNESS;  // Current LED brightness
uint16_t lastPotValue = 0;                   // Previous pot reading for smoothing
uint16_t smoothedPotValue = 0;               // Running average of pot readings
uint8_t smoothingCounter = 0;                // Counter for smoothing algorithm
#define SMOOTHING_SAMPLES 8                  // Number of samples for averaging
#define BRIGHTNESS_CHANGE_THRESHOLD 3        // Minimum change to update brightness

// Function declarations
void rainbow();
void solidWhite();
void checkModeSwitch();
void updateBrightness();
void printStatus();
void statusUpdate();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Rainbow Display Starting...");
  Serial.println("Hardware: Seeed XIAO ESP32-C6 (using C3 toolchain)");
  Serial.printf("Total LEDs: %d (50 + 57)\n", TOTAL_LEDS);
  Serial.println("Strip 1: D3 pin (GPIO4) - 50 LEDs");
  Serial.println("Strip 2: D4 pin (GPIO5) - 57 LEDs");
  Serial.println("Switch: D10 pin (GPIO21) - Connect to 3.3V for white mode");
  Serial.println("Brightness: D0 pin (GPIO1) - Potentiometer for brightness control");
  
  // Initialize mode switch pin
  pinMode(SWITCH_PIN, INPUT_PULLDOWN);  // Use internal pulldown resistor
  
  // Initialize brightness potentiometer pin
  pinMode(BRIGHTNESS_POT, INPUT);  // Analog input for potentiometer
  
  // Initialize smoothed potentiometer value
  smoothedPotValue = analogRead(BRIGHTNESS_POT);
  
  // Initialize FastLED for both strips
  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds1, NUM_LEDS_STRIP1);
  FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_STRIP2);
  
  // Set initial brightness
  FastLED.setBrightness(currentBrightness);
  
  // Clear all LEDs initially
  FastLED.clear();
  FastLED.show();
  
  Serial.println("Rainbow animation started!");
}

void loop() {
  // Check for mode switch changes
  checkModeSwitch();
  
  // Update brightness from potentiometer
  updateBrightness();
  
  // Run appropriate animation based on mode
  if (whiteMode) {
    solidWhite();
  } else {
    rainbow();
  }
  
  // Update LEDs
  FastLED.show();
  
  // Control animation speed
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  
  // Rotate the rainbow
  EVERY_N_MILLISECONDS(20) {
    gHue += RAINBOW_SPEED;
  }
  
  // Print status updates
  statusUpdate();
}

void rainbow() {
  // Fill both strips with rainbow pattern
  // Each LED gets a color based on its position in the total sequence
  
  // Strip 1: LEDs 0-49
  for (int i = 0; i < NUM_LEDS_STRIP1; i++) {
    uint8_t hue1 = gHue + (i * 256 / TOTAL_LEDS);
    leds1[i] = CHSV(hue1, 255, 255);
  }
  
  // Strip 2: LEDs 50-106 (continue the pattern)
  for (int i = 0; i < NUM_LEDS_STRIP2; i++) {
    uint8_t hue2 = gHue + ((i + NUM_LEDS_STRIP1) * 256 / TOTAL_LEDS);
    leds2[i] = CHSV(hue2, 255, 255);
  }
}

void solidWhite() {
  // Set all LEDs to solid white
  for (int i = 0; i < NUM_LEDS_STRIP1; i++) {
    leds1[i] = CRGB::White;
  }
  
  for (int i = 0; i < NUM_LEDS_STRIP2; i++) {
    leds2[i] = CRGB::White;
  }
}

void checkModeSwitch() {
  // Read switch state (HIGH when connected to 3.3V)
  bool currentSwitchState = digitalRead(SWITCH_PIN);
  
  // Check for state change
  if (currentSwitchState != lastSwitchState) {
    // Switch state changed
    whiteMode = currentSwitchState;
    
    Serial.print("Mode switched to: ");
    Serial.println(whiteMode ? "WHITE" : "RAINBOW");
    
    // Clear LEDs when switching modes for smooth transition
    FastLED.clear();
    
    lastSwitchState = currentSwitchState;
  }
}

void updateBrightness() {
  // Read potentiometer value (0-4095 on ESP32-C6)
  uint16_t rawPotValue = analogRead(BRIGHTNESS_POT);
  
  // Implement exponential moving average for smoothing
  // Formula: smoothed = (alpha * new_value) + ((1-alpha) * old_value)
  // Using alpha = 1/8 for gentle smoothing
  smoothedPotValue = ((rawPotValue) + (7 * smoothedPotValue)) / 8;
  
  // Map smoothed potentiometer value to brightness range
  uint8_t newBrightness = map(smoothedPotValue, 0, 4095, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  
  // Only update if change is significant enough to avoid jitter
  if (abs(newBrightness - currentBrightness) >= BRIGHTNESS_CHANGE_THRESHOLD) {
    currentBrightness = newBrightness;
    
    // Update FastLED brightness
    FastLED.setBrightness(currentBrightness);
    
    // Debug output (only when brightness changes significantly)
    Serial.printf("Brightness: %d (raw: %d, smoothed: %d)\n", 
                  currentBrightness, rawPotValue, smoothedPotValue);
  }
}

void printStatus() {
  // Print system information
  Serial.println("\n=== Rainbow Display Status ===");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Animation speed: %d FPS\n", FRAMES_PER_SECOND);
  Serial.printf("Brightness: %d/255\n", currentBrightness);
  Serial.printf("Rainbow hue: %d\n", gHue);
  Serial.printf("Mode: %s\n", whiteMode ? "WHITE" : "RAINBOW");
  Serial.println("===============================");
}

// Print status every 10 seconds
void statusUpdate() {
  EVERY_N_SECONDS(10) {
    printStatus();
  }
}