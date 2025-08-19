/*
 * Comprehensive Servo Troubleshooting Sketch
 * Tests servo functionality with multiple approaches
 * 
 * Features:
 * - ESP32Servo library test
 * - Native ESP32 PWM test  
 * - Signal analysis and verification
 * - TPS61023 power control testing
 * - Multiple servo control methods
 * - Serial commands for interactive testing
 */

#include <ESP32Servo.h>
#include <driver/ledc.h>

// Pin definitions for Seeed Studio XIAO ESP32-C3
#define SERVO_PIN D5       // Servo motor on D5 (GPIO4)
#define POWER_ENABLE_PIN D9 // TPS61023 5V boost enable on D9 (GPIO8)

// PWM Configuration for native ESP32 control
#define SERVO_PWM_CHANNEL 0
#define SERVO_PWM_FREQ 50      // 50Hz for standard servos
#define SERVO_PWM_RESOLUTION 16 // 16-bit resolution
#define SERVO_MIN_PULSE 500    // 0.5ms minimum pulse
#define SERVO_MAX_PULSE 2500   // 2.5ms maximum pulse
#define SERVO_PERIOD 20000     // 20ms period (50Hz)

// Test variables
Servo testServo;
bool useESP32Servo = true;
bool powerEnabled = false;
int currentPosition = 90;
unsigned long lastMove = 0;
bool autoSweep = false;
int sweepDirection = 1;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== SERVO TROUBLESHOOTING TOOLKIT ===");
  Serial.println("ESP32-C3 XIAO Servo Control Test");
  Serial.println("Hardware: TPS61023 5V Boost + Servo Motor");
  Serial.println("=====================================\n");
  
  // Initialize power control pin
  pinMode(POWER_ENABLE_PIN, OUTPUT);
  digitalWrite(POWER_ENABLE_PIN, LOW);
  
  // Initialize servo pin
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);
  
  Serial.printf("Servo Pin: D5 (GPIO%d)\n", SERVO_PIN);
  Serial.printf("Power Pin: D9 (GPIO%d)\n", POWER_ENABLE_PIN);
  Serial.println();
  
  printCommands();
  
  // Start with power disabled
  Serial.println("Starting with power DISABLED for safety");
  Serial.println("Use 'p' command to enable power when ready");
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    handleSerialCommand();
  }
  
  // Auto sweep if enabled
  if (autoSweep && powerEnabled && (millis() - lastMove > 1000)) {
    currentPosition += (sweepDirection * 30);
    if (currentPosition >= 180) {
      currentPosition = 180;
      sweepDirection = -1;
    } else if (currentPosition <= 0) {
      currentPosition = 0;
      sweepDirection = 1;
    }
    
    moveServoToPosition(currentPosition);
    lastMove = millis();
  }
  
  delay(50);
}

void handleSerialCommand() {
  String command = Serial.readString();
  command.trim();
  command.toLowerCase();
  
  if (command == "help" || command == "h") {
    printCommands();
  }
  else if (command == "p") {
    togglePower();
  }
  else if (command == "lib") {
    switchToESP32Servo();
  }
  else if (command == "pwm") {
    switchToNativePWM();
  }
  else if (command == "test") {
    runServoTest();
  }
  else if (command == "sweep") {
    toggleAutoSweep();
  }
  else if (command == "0") {
    moveServoToPosition(0);
  }
  else if (command == "90") {
    moveServoToPosition(90);
  }
  else if (command == "180") {
    moveServoToPosition(180);
  }
  else if (command.startsWith("move ")) {
    int angle = command.substring(5).toInt();
    moveServoToPosition(angle);
  }
  else if (command == "signal") {
    analyzeSignal();
  }
  else if (command == "power") {
    checkPowerStatus();
  }
  else if (command == "pins") {
    checkPinStatus();
  }
  else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

void printCommands() {
  Serial.println("Available Commands:");
  Serial.println("==================");
  Serial.println("help     - Show this help");
  Serial.println("p        - Toggle power supply (TPS61023)");
  Serial.println("lib      - Switch to ESP32Servo library");
  Serial.println("pwm      - Switch to native PWM control");
  Serial.println("test     - Run comprehensive servo test");
  Serial.println("sweep    - Toggle auto sweep mode");
  Serial.println("0        - Move servo to 0 degrees");
  Serial.println("90       - Move servo to 90 degrees");
  Serial.println("180      - Move servo to 180 degrees");
  Serial.println("move X   - Move servo to X degrees (0-180)");
  Serial.println("signal   - Analyze PWM signal");
  Serial.println("power    - Check power supply status");
  Serial.println("pins     - Check pin status");
  Serial.println();
}

void togglePower() {
  powerEnabled = !powerEnabled;
  digitalWrite(POWER_ENABLE_PIN, powerEnabled ? HIGH : LOW);
  
  Serial.printf("Power supply %s\n", powerEnabled ? "ENABLED" : "DISABLED");
  Serial.printf("TPS61023 Enable Pin (D9): %s\n", powerEnabled ? "HIGH" : "LOW");
  
  if (powerEnabled) {
    Serial.println("⚡ 5V boost converter should now be active");
    Serial.println("⚠️  WARNING: Servo will now have power!");
    delay(100); // Allow power to stabilize
  } else {
    Serial.println("🔌 Power supply disabled - servo safe to handle");
  }
}

void switchToESP32Servo() {
  if (!useESP32Servo) {
    // Detach native PWM first
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
  }
  
  useESP32Servo = true;
  testServo.attach(SERVO_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  
  Serial.println("📚 Switched to ESP32Servo library");
  Serial.printf("Attached to pin D5 (GPIO%d) with pulse range %d-%d μs\n", 
                SERVO_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

void switchToNativePWM() {
  if (useESP32Servo) {
    testServo.detach();
  }
  
  useESP32Servo = false;
  
  // Configure LEDC timer
  ledc_timer_config_t timer_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_16_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = SERVO_PWM_FREQ,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer_config);
  
  // Configure LEDC channel
  ledc_channel_config_t channel_config = {
    .gpio_num = SERVO_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&channel_config);
  
  Serial.println("⚙️  Switched to native ESP32 PWM control");
  Serial.printf("PWM: 50Hz, 16-bit resolution on GPIO%d\n", SERVO_PIN);
}

void moveServoToPosition(int angle) {
  if (!powerEnabled) {
    Serial.println("❌ Cannot move servo - power supply disabled!");
    Serial.println("Use 'p' command to enable power first");
    return;
  }
  
  angle = constrain(angle, 0, 180);
  currentPosition = angle;
  
  if (useESP32Servo) {
    // ESP32Servo library method
    int pulseWidth = map(angle, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    
    Serial.printf("📐 ESP32Servo: Moving to %d° (pulse: %d μs)\n", angle, pulseWidth);
    
    testServo.writeMicroseconds(pulseWidth);
    // Also try standard write as backup
    testServo.write(angle);
    
  } else {
    // Native PWM method
    int pulseWidth = map(angle, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    
    // Calculate duty cycle (16-bit resolution)
    // Duty = (pulse_width / period) * max_duty
    uint32_t duty = (pulseWidth * 65535) / SERVO_PERIOD;
    
    Serial.printf("⚙️  Native PWM: Moving to %d° (pulse: %d μs, duty: %d)\n", 
                   angle, pulseWidth, duty);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  }
  
  Serial.printf("✅ Servo command sent for %d degrees\n", angle);
}

void runServoTest() {
  if (!powerEnabled) {
    Serial.println("❌ Cannot run test - power supply disabled!");
    return;
  }
  
  Serial.println("\n🧪 RUNNING COMPREHENSIVE SERVO TEST");
  Serial.println("====================================");
  
  // Test both control methods
  Serial.println("\n1. Testing ESP32Servo Library:");
  switchToESP32Servo();
  testServoPositions();
  
  delay(2000);
  
  Serial.println("\n2. Testing Native PWM Control:");
  switchToNativePWM();
  testServoPositions();
  
  Serial.println("\n✅ Test complete!");
}

void testServoPositions() {
  int testAngles[] = {0, 45, 90, 135, 180, 90};
  int numTests = sizeof(testAngles) / sizeof(testAngles[0]);
  
  for (int i = 0; i < numTests; i++) {
    Serial.printf("Test %d: Moving to %d degrees...\n", i+1, testAngles[i]);
    moveServoToPosition(testAngles[i]);
    delay(1500);
  }
}

void toggleAutoSweep() {
  autoSweep = !autoSweep;
  
  if (autoSweep) {
    if (!powerEnabled) {
      Serial.println("❌ Cannot start sweep - power supply disabled!");
      autoSweep = false;
      return;
    }
    Serial.println("🔄 Auto sweep ENABLED - servo will move continuously");
    Serial.println("Use 'sweep' again to stop");
    currentPosition = 90;
    sweepDirection = 1;
    lastMove = millis();
  } else {
    Serial.println("⏹️  Auto sweep DISABLED");
  }
}

void analyzeSignal() {
  Serial.println("\n📊 PWM SIGNAL ANALYSIS");
  Serial.println("======================");
  
  if (useESP32Servo) {
    Serial.println("Mode: ESP32Servo Library");
    Serial.printf("Pin: D5 (GPIO%d)\n", SERVO_PIN);
    Serial.printf("Expected frequency: 50Hz (20ms period)\n");
    Serial.printf("Current angle: %d degrees\n", currentPosition);
    
    int expectedPulse = map(currentPosition, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    Serial.printf("Expected pulse width: %d μs\n", expectedPulse);
    
  } else {
    Serial.println("Mode: Native PWM");
    Serial.printf("Pin: D5 (GPIO%d)\n", SERVO_PIN);
    Serial.printf("PWM frequency: %d Hz\n", SERVO_PWM_FREQ);
    Serial.printf("Resolution: %d bits\n", SERVO_PWM_RESOLUTION);
    
    uint32_t currentDuty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    int pulseWidth = (currentDuty * SERVO_PERIOD) / 65535;
    
    Serial.printf("Current duty cycle: %d / 65535\n", currentDuty);
    Serial.printf("Calculated pulse width: %d μs\n", pulseWidth);
  }
  
  Serial.println("\n📏 Expected Signal Characteristics:");
  Serial.println("  • Frequency: 50Hz (20ms period)");
  Serial.println("  • Pulse width range: 500-2500 μs");
  Serial.println("  • 0°   = 500 μs pulse");
  Serial.println("  • 90°  = 1500 μs pulse");
  Serial.println("  • 180° = 2500 μs pulse");
  Serial.println("\n💡 Use an oscilloscope or logic analyzer to verify");
}

void checkPowerStatus() {
  Serial.println("\n⚡ POWER SYSTEM STATUS");
  Serial.println("=====================");
  
  bool enablePinState = digitalRead(POWER_ENABLE_PIN);
  
  Serial.printf("TPS61023 Enable Pin (D9/GPIO%d): %s\n", 
                POWER_ENABLE_PIN, enablePinState ? "HIGH" : "LOW");
  Serial.printf("Power Control Variable: %s\n", powerEnabled ? "ENABLED" : "DISABLED");
  Serial.printf("Expected 5V Output: %s\n", enablePinState ? "ACTIVE" : "OFF");
  
  if (enablePinState != powerEnabled) {
    Serial.println("⚠️  WARNING: Pin state doesn't match control variable!");
  }
  
  Serial.println("\n🔧 Power Supply Notes:");
  Serial.println("  • TPS61023 boosts 3.3V to 5V when enabled");
  Serial.println("  • Enable pin HIGH = 5V output active");
  Serial.println("  • Enable pin LOW = No 5V output");
  Serial.println("  • Servo requires 5V for proper operation");
}

void checkPinStatus() {
  Serial.println("\n📌 PIN STATUS CHECK");
  Serial.println("==================");
  
  Serial.printf("Servo Pin D5 (GPIO%d): ", SERVO_PIN);
  pinMode(SERVO_PIN, INPUT);
  bool servoState = digitalRead(SERVO_PIN);
  Serial.println(servoState ? "HIGH" : "LOW");
  
  Serial.printf("Power Pin D9 (GPIO%d): ", POWER_ENABLE_PIN);
  bool powerState = digitalRead(POWER_ENABLE_PIN);
  Serial.println(powerState ? "HIGH" : "LOW");
  
  // Restore servo pin as output
  pinMode(SERVO_PIN, OUTPUT);
  
  Serial.println("\n🔧 Pin Configuration:");
  Serial.println("  • D5 (GPIO4) = Servo PWM output");
  Serial.println("  • D9 (GPIO8) = TPS61023 enable output");
  
  Serial.println("\n📋 Troubleshooting Checklist:");
  Serial.println("  □ Power supply enabled (D9 HIGH)");
  Serial.println("  □ 5V present at servo red wire");
  Serial.println("  □ Ground connected (servo black/brown wire)");
  Serial.println("  □ Signal connected to D5 (servo white/yellow wire)");
  Serial.println("  □ Servo responds to manual pulse test");
}
