#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <driver/ledc.h>

// Pin definitions for ESP32-C3 XIAO
#define LED_PIN 4     // D5 (GPIO4) - LEDs
#define TRIGGER_PIN 6 // D4 (GPIO6) - Trigger input
#define SERVO_PIN 18  // D10 (GPIO18) - Servo signal 
#define POWER_PIN 8   // D9 (GPIO8) - 5V boost enable

// LED configuration
#define NUM_LEDS 2
CRGB leds[NUM_LEDS];

// Servo PWM configuration for ESP32-C3 native LEDC
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50 // 50Hz for servo
#define SERVO_RESOLUTION LEDC_TIMER_14_BIT

// System state
bool currentState = false;  // false=idle, true=active
int servoPosition = 0;      // Current servo position (0-180)
bool powerEnabled = false;

// WiFi settings
String wifiSSID = "Rigging Electric";
String wifiPassword = "academy123";

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("Simplified Defragmentor Control System Starting...");
  Serial.println("Hardware: ESP32-C3 XIAO with Native PWM Servo Control");
  Serial.printf("Pin assignments: LEDs=D5(GPIO%d), Trigger=D4(GPIO%d), Servo=D10(GPIO%d), Power=D9(GPIO%d)\n", 
                LED_PIN, TRIGGER_PIN, SERVO_PIN, POWER_PIN);
  
  // Configure power control pin
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW); // Start with power disabled
  
  // Configure trigger pin
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  
  // Initialize servo power and PWM
  Serial.println("Enabling servo power supply...");
  enableServoPower(true);
  setupNativePWM();
  
  // Test servo movement
  Serial.println("Testing servo positions...");
  moveServoToPosition(90);   // Center position
  delay(1000);
  moveServoToPosition(0);    // 0 degrees
  delay(1000);
  moveServoToPosition(180);  // 180 degrees  
  delay(1000);
  moveServoToPosition(0);    // Back to idle position
  
  // Initialize LEDs
  Serial.println("Initializing LEDs...");
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();
  
  // Set initial LED state
  setLEDPattern();
  
  // Initialize WiFi
  if (wifiSSID.length() > 0) {
    initializeWiFi();
  } else {
    Serial.println("No WiFi credentials configured - running in standalone mode");
  }
  
  Serial.println("Testing Defragmentor systems...");
  
  // Test LED sequence
  for (int i = 0; i < 3; i++) {
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Green;
    FastLED.show();
    delay(200);
    leds[0] = CRGB::Black;
    leds[1] = CRGB::Black;
    FastLED.show();
    delay(200);
  }
  
  Serial.println("Defragmentor initialization complete");
  Serial.println("Ready for trigger input and network commands");
  
  setLEDPattern();
}

void loop() {
  // Handle trigger input
  bool triggerPressed = digitalRead(TRIGGER_PIN) == LOW; // Assuming active low
  
  if (triggerPressed && !currentState) {
    // Trigger pressed, activate sequence
    Serial.println("Trigger activated - starting defragmentation sequence");
    currentState = true;
    moveServoToPosition(180); // Move to active position
    setLEDPattern();
  } else if (!triggerPressed && currentState) {
    // Trigger released, return to idle
    Serial.println("Trigger released - returning to idle");
    currentState = false;
    moveServoToPosition(0); // Return to idle position
    setLEDPattern();
  }
  
  // Update LED animations
  FastLED.show();
  
  // Send periodic status (every 5 seconds)
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate > 5000) {
    sendPeriodicStatus();
    lastStatusUpdate = millis();
  }
  
  delay(50); // Small delay for responsiveness
}

// Enable/disable servo power supply
void enableServoPower(bool enable) {
  digitalWrite(POWER_PIN, enable ? HIGH : LOW);
  powerEnabled = enable;
  Serial.printf("Servo power: %s\n", enable ? "ENABLED" : "DISABLED");
  if (enable) {
    delay(100); // Allow power to stabilize
  }
}

// Initialize native ESP32 PWM for servo control
void setupNativePWM() {
  Serial.println("Configuring native ESP32 LEDC PWM for servo...");
  
  // Configure timer
  ledc_timer_config_t timer_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = SERVO_RESOLUTION,
    .timer_num = SERVO_TIMER,
    .freq_hz = SERVO_FREQ,
    .clk_cfg = LEDC_AUTO_CLK
  };
  
  esp_err_t timer_result = ledc_timer_config(&timer_config);
  Serial.printf("Timer config result: %s\n", esp_err_to_name(timer_result));
  
  // Configure channel
  ledc_channel_config_t channel_config = {
    .gpio_num = SERVO_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = SERVO_CHANNEL,
    .timer_sel = SERVO_TIMER,
    .duty = 0,
    .hpoint = 0
  };
  
  esp_err_t channel_result = ledc_channel_config(&channel_config);
  Serial.printf("Channel config result: %s\n", esp_err_to_name(channel_result));
  
  Serial.println("Native PWM setup complete");
}

// Move servo to specific angle (0-180 degrees)
void moveServoToPosition(int angle) {
  if (!powerEnabled) {
    Serial.println("WARNING: Cannot move servo - power is disabled");
    return;
  }
  
  // Constrain angle
  angle = constrain(angle, 0, 180);
  
  // Calculate duty cycle for servo position
  // Servo expects 1-2ms pulse width, 50Hz frequency
  // 1ms = 5% duty, 2ms = 10% duty at 50Hz
  // For 14-bit resolution: 1ms = 819, 1.5ms = 1229, 2ms = 1638
  uint32_t min_duty = 819;   // 1ms pulse width
  uint32_t max_duty = 1638;  // 2ms pulse width
  uint32_t duty = map(angle, 0, 180, min_duty, max_duty);
  
  Serial.printf("Moving servo to %d° (duty: %lu)\n", angle, duty);
  
  esp_err_t result = ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty);
  if (result != ESP_OK) {
    Serial.printf("Error setting duty: %s\n", esp_err_to_name(result));
    return;
  }
  
  result = ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
  if (result != ESP_OK) {
    Serial.printf("Error updating duty: %s\n", esp_err_to_name(result));
    return;
  }
  
  servoPosition = angle;
  delay(500); // Allow servo to move
}

// Set LED pattern based on current state
void setLEDPattern() {
  if (currentState) {
    // Active state - bright colors
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Blue;
  } else {
    // Idle state - dim colors
    leds[0] = CRGB(32, 0, 0);    // Dim red
    leds[1] = CRGB(0, 0, 32);    // Dim blue
  }
  FastLED.show();
  Serial.printf("LEDs set to %s pattern\n", currentState ? "ACTIVE" : "IDLE");
}

// Initialize WiFi connection
void initializeWiFi() {
  Serial.printf("Connecting to WiFi network: %s\n", wifiSSID.c_str());
  
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("WiFi connected! IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed - running in standalone mode");
  }
}

// Send periodic status updates
void sendPeriodicStatus() {
  Serial.printf("Status: State=%s, Servo=%d°, Power=%s, WiFi=%s\n", 
                currentState ? "ACTIVE" : "IDLE", 
                servoPosition,
                powerEnabled ? "ON" : "OFF",
                WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
}
