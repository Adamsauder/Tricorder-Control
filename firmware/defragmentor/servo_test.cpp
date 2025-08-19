/*
 * Simple Servo Test for ESP32-C3
 * This tests just the servo functionality without any other features
 */

#include <ESP32Servo.h>

#define SERVO_PIN 21  // D3 = GPIO21

Servo testServo;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32-C3 Servo Test Starting...");
  Serial.printf("Servo pin: GPIO%d (D3)\n", SERVO_PIN);
  
  // Allow allocation of all timers for servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Attach servo with explicit timing
  testServo.setPeriodHertz(50);      // Standard 50hz servo
  testServo.attach(SERVO_PIN, 500, 2500);  // 500-2500us pulse width
  
  Serial.println("Servo attached successfully");
  delay(500);
  
  // Test center position first
  Serial.println("Moving to center (90°)...");
  testServo.write(90);
  delay(2000);
}

void loop() {
  Serial.println("Testing servo sweep...");
  
  // Move to 0 degrees
  Serial.println("Moving to 0°");
  testServo.write(0);
  delay(2000);
  
  // Move to 90 degrees  
  Serial.println("Moving to 90°");
  testServo.write(90);
  delay(2000);
  
  // Move to 180 degrees
  Serial.println("Moving to 180°");
  testServo.write(180);
  delay(2000);
  
  // Test with microseconds
  Serial.println("Testing with microseconds...");
  
  Serial.println("500µs (0°)");
  testServo.writeMicroseconds(500);
  delay(2000);
  
  Serial.println("1500µs (90°)");
  testServo.writeMicroseconds(1500);
  delay(2000);
  
  Serial.println("2500µs (180°)");
  testServo.writeMicroseconds(2500);
  delay(2000);
  
  Serial.println("Cycle complete. Repeating in 3 seconds...");
  delay(3000);
}
