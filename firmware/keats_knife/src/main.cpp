/*
 * Keats Knife - Ultra Minimal ESP32-C3 White LED   // Read potentiometer and update LED brightness
  if (currentMillis - lastPotRead > POT_READ_INTERVAL) {
    potValue = readPotentiometer();
    updateLED();
    lastPotRead = currentMillis;
  }
  
  // Power-efficient delay
  delay(20); // Fast enough for smooth pot responsePower-efficient delay
  delay(20); // Fast enough for smooth pot response Features:
 * - Single white LED on analog pin A0
 * - Potentiometer brightness control on analog pin A1 (POT_PIN)
 * - No WiFi, no Bluetooth, no sACN, no network
 * - Maximum battery optimization
 * - Ultra-simple standalone operation
 * 
 * Hardware: Seeed Studio XIAO ESP32C3 + White LED on A0 + Potentiometer on A1
 */

#include <Arduino.h>

// Hardware configuration
#define LED_PIN A0      // Analog pin for LED control (white LED)
#define POT_PIN A1      // Potentiometer for brightness control

// Potentiometer control
float potValue = 0.0;
float smoothedPot = 0.0;
const float SMOOTHING_FACTOR = 0.1;  // Exponential smoothing
unsigned long lastPotRead = 0;
const unsigned long POT_READ_INTERVAL = 50; // Read pot every 50ms

// Function declarations
float readPotentiometer();
void updateLED();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("🔪 Keats Knife v2.0 - Ultra Minimal White LED + Pot Control");
  
  // Configure LED pin for analog output
  pinMode(LED_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  
  // Set PWM to maximum resolution for better analog output
  analogWriteResolution(12); // 12-bit resolution (0-4095)
  analogReadResolution(12);  // 12-bit ADC resolution (0-4095)
  
  // Start with LED off, will be controlled by potentiometer
  analogWrite(LED_PIN, 0);
  
  Serial.println("✅ Keats Knife ready - Pot-controlled white LED");
  Serial.println("🎛️  Turn potentiometer to control brightness");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Keep LED at full brightness (no breathing effect)
  // Use 12-bit PWM value for maximum 3.3V output
  // Read potentiometer and update LED brightness
  if (currentMillis - lastPotRead > POT_READ_INTERVAL) {
    potValue = readPotentiometer();
    updateLED();
    lastPotRead = currentMillis;
    
    // Print status occasionally
    Serial.printf("� Brightness: %d, 🔋 Battery: %.2fV\n", 
                  currentBrightness, batteryVoltage);
  }
  
  // Deep sleep management for ultimate power saving
  if (deepSleepEnabled && (currentMillis - lastActivity > DEEP_SLEEP_TIMEOUT)) {
    enterDeepSleep();
  }
  
  // Ultra power-efficient delay
  delay(100); // Even slower since no animation needed
}

float readPotentiometer() {
  // Read potentiometer value from analog pin A1
  uint16_t rawValue = analogRead(POT_PIN); // 12-bit ADC (0-4095)
  
  // Apply exponential smoothing to prevent LED flickering
  float currentValue = (float)rawValue / 4095.0; // Normalize to 0.0-1.0
  smoothedPot = (SMOOTHING_FACTOR * currentValue) + ((1.0 - SMOOTHING_FACTOR) * smoothedPot);
  
  return smoothedPot;
}

void updateLED() {
  // Convert potentiometer value (0.0-1.0) to PWM value (0-4095)
  uint16_t pwmValue = (uint16_t)(potValue * 4095);
  
  // Set white LED brightness based on potentiometer
  analogWrite(LED_PIN, pwmValue);
  
  // Print status occasionally (every ~1 second at 20ms delay)
  static int printCounter = 0;
  if (++printCounter >= 50) {
    Serial.printf("🎛️  Pot: %.2f, LED PWM: %d (%.1f%%)\n", 
                  potValue, pwmValue, potValue * 100.0);
    printCounter = 0;
  }
}