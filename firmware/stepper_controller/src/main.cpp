// TMC2209 Stepper Controller with Single-Wire UART
// Optimized for StepperOnline 17HS08-1004S motors
// ESP32-C6 XIAO with UART control for TMC2209

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <TMCStepper.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <FastLED.h>

// Pin definitions for ESP32-C6 XIAO (using D# pin names)
#define STEP_PIN D0          // D0 - Step pin to TMC2209 STEP
#define DIR_PIN D1           // D1 - Direction pin to TMC2209 DIR
#define ENABLE_PIN D4        // D4 - Enable pin to TMC2209 EN (active low)
#define UART_TX_PIN D5       // D5 - UART TX pin
#define UART_RX_PIN D6       // D6 - UART RX pin
#define LED_PIN D9           // D9 - Status LED

// **CRITICAL HARDWARE SETUP:**
// Connect BOTH ESP32 D5 AND D6 → 1kΩ resistor → TMC2209 PDN_UART pin
// TMC2209 single-wire UART requires both TX and RX connected to the same PDN_UART pin
// This allows bidirectional communication over the single UART line

// Motor specifications - StepperOnline 17HS08-1004S
const int MOTOR_STEPS_PER_REV = 200;     // 1.8° per step
const int MOTOR_CURRENT_MA = 600;        // 600mA (60% of 1.0A rating for safety)
const int MOTOR_MICROSTEPS = 1;          // Full steps for maximum speed

// TMC2209 Configuration
TMC2209Stepper driver(&Serial1, 0.11f, 0b00);  // Single-wire UART, 0.11Ω sense resistor

// Web server
WebServer server(80);

// Motion control
struct MotionControl {
    long position = 0;
    int speed = 2000;        // steps/sec
    bool enabled = false;
    bool moving = false;
} motion;

// LED status
CRGB statusLed[1];

// WiFi credentials
const char* ssid = "Rigging Electric";
const char* password = "academy123";
String deviceId = "stepper_controller_" + String(ESP.getEfuseMac(), HEX);

// Function declarations
void setupWiFi();
void setupOTA();
void setupTMC2209();
void handleWebRoot();
void handleWebAPI();
void handleMotorCommand(String action, JsonDocument& params);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("TMC2209 Stepper Controller Starting...");
    Serial.println("Hardware: ESP32-C6 XIAO + TMC2209 + StepperOnline 17HS08-1004S");
    Serial.println("CRITICAL: Ensure BOTH D5 & D6 → 1kΩ resistor → TMC2209 PDN_UART");
    
    // Initialize LED
    FastLED.addLeds<WS2812, LED_PIN, GRB>(statusLed, 1);
    statusLed[0] = CRGB::Red;  // Red = starting up
    FastLED.show();
    
    // Initialize pins
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    digitalWrite(STEP_PIN, LOW);
    digitalWrite(DIR_PIN, LOW);
    digitalWrite(ENABLE_PIN, HIGH);  // Disable motor initially
    
    // Setup UART for TMC2209 - both TX and RX connected to PDN_UART via 1kΩ resistor
    Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);  // RX=D6, TX=D5
    
    setupWiFi();
    setupTMC2209();
    setupOTA();
    
    // Web server routes
    server.on("/", handleWebRoot);
    server.on("/api", handleWebAPI);
    server.begin();
    
    // mDNS
    MDNS.begin(deviceId.c_str());
    MDNS.addService("http", "tcp", 80);
    
    statusLed[0] = CRGB::Green;  // Green = ready
    FastLED.show();
    
    Serial.println("Setup complete. Device ID: " + deviceId);
    Serial.println("Web interface: http://" + WiFi.localIP().toString());
}

void loop() {
    server.handleClient();
    ArduinoOTA.handle();
    
    // Update LED status
    if (WiFi.status() != WL_CONNECTED) {
        statusLed[0] = CRGB::Red;
    } else if (motion.moving) {
        statusLed[0] = CRGB::Blue;
    } else if (motion.enabled) {
        statusLed[0] = CRGB::Green;
    } else {
        statusLed[0] = CRGB::Yellow;
    }
    FastLED.show();
    
    delay(50);
}

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("WiFi Connected!");
        Serial.println("IP: " + WiFi.localIP().toString());
    } else {
        Serial.println();
        Serial.println("WiFi connection failed - continuing without network");
    }
}

void setupTMC2209() {
    driver.begin();
    delay(100);  // Give TMC2209 time to initialize
    
    // Configure for StepperOnline 17HS08-1004S motor
    driver.toff(4);                    // Enable driver
    driver.rms_current(MOTOR_CURRENT_MA);  // 600mA current
    driver.microsteps(MOTOR_MICROSTEPS);   // Full steps for maximum speed
    driver.pwm_autoscale(true);        // Enable automatic current scaling
    driver.en_spreadCycle(false);      // StealthChop for quiet operation
    
    delay(100);  // Allow settings to take effect
    
    // Test TMC2209 communication and verify settings
    uint16_t readMicrosteps = driver.microsteps();
    uint16_t readCurrent = driver.rms_current();
    
    Serial.println("=== TMC2209 Configuration ===");
    if (driver.test_connection()) {
        Serial.println("✓ TMC2209 communication successful");
    } else {
        Serial.println("✗ TMC2209 communication failed!");
        Serial.println("  Check UART wiring: D5(TX) & D6(RX) → 1kΩ resistor → PDN_UART");
        Serial.println("  Both D5 and D6 must be connected to the same PDN_UART pin!");
    }
    
    Serial.println("Settings Applied:");
    Serial.println("  Target Microsteps: " + String(MOTOR_MICROSTEPS));
    Serial.println("  Actual Microsteps: " + String(readMicrosteps));
    Serial.println("  Target Current: " + String(MOTOR_CURRENT_MA) + "mA");
    Serial.println("  Actual Current: " + String(readCurrent) + "mA");
    
    if (readMicrosteps != MOTOR_MICROSTEPS) {
        Serial.println("⚠ WARNING: Microstep setting failed!");
        Serial.println("  This indicates UART communication issues.");
        Serial.println("  Motor will run in default microstepping mode.");
    }
    
    if (readCurrent != MOTOR_CURRENT_MA) {
        Serial.println("⚠ WARNING: Current setting may not have applied correctly.");
    }
    
    Serial.println("=============================");
}

void setupOTA() {
    ArduinoOTA.setHostname(deviceId.c_str());
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void handleWebRoot() {
    String html = "<!DOCTYPE html><html><head><title>TMC2209 Stepper Controller</title>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f5f5f5} ";
    html += ".container{background:white;padding:20px;border-radius:8px;max-width:600px;margin:auto} ";
    html += "button{padding:12px 16px;margin:5px;border:none;border-radius:4px;cursor:pointer;font-size:14px} ";
    html += ".btn-move{background:#007bff;color:white} .btn-enable{background:#28a745;color:white} ";
    html += ".btn-disable{background:#6c757d;color:white} .btn-stop{background:#dc3545;color:white} ";
    html += ".status{background:#e9ecef;padding:15px;border-radius:4px;margin:15px 0} ";
    html += ".warning{background:#fff3cd;border:1px solid #ffeaa7;padding:10px;border-radius:4px;margin:10px 0} ";
    html += "h1{color:#333;margin-bottom:10px} h3{color:#555;margin:20px 0 10px 0} ";
    html += ".grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:10px 0}</style>";
    
    // Auto-refresh meta tag
    html += "<meta http-equiv='refresh' content='5'>";
    html += "</head><body><div class='container'>";
    
    html += "<h1>🔧 TMC2209 Stepper Controller</h1>";
    html += "<div><strong>Device:</strong> " + deviceId + "</div>";
    
    // Status section with current readings
    html += "<div class='status'>";
    html += "<h3>📊 Current Status</h3>";
    html += "<div><strong>Position:</strong> " + String(motion.position) + " steps</div>";
    html += "<div><strong>Speed:</strong> " + String(motion.speed) + " steps/sec</div>";
    html += "<div><strong>Motor Enabled:</strong> " + String(motion.enabled ? "✅ YES" : "❌ NO") + "</div>";
    html += "<div><strong>Currently Moving:</strong> " + String(motion.moving ? "🔄 YES" : "⏹ NO") + "</div>";
    
    // TMC2209 diagnostics
    uint16_t currentMicrosteps = driver.microsteps();
    uint16_t currentRMS = driver.rms_current();
    html += "<div><strong>Microsteps:</strong> " + String(currentMicrosteps) + 
            (currentMicrosteps == 1 ? " ✅ (Full Steps)" : " ⚠ (Not Full Steps)") + "</div>";
    html += "<div><strong>Motor Current:</strong> " + String(currentRMS) + "mA</div>";
    html += "</div>";
    
    // Warning if not in full step mode
    if (currentMicrosteps != 1) {
        html += "<div class='warning'>";
        html += "⚠ <strong>Warning:</strong> Motor is not in full-step mode (showing " + String(currentMicrosteps) + " microsteps). ";
        html += "This indicates UART communication issues. Check that BOTH D5 & D6 are connected via 1kΩ resistor to TMC2209 PDN_UART.";
        html += "</div>";
    }
    
    html += "<h3>🏃 Quick Moves (Current Mode: " + String(currentMicrosteps) + " microsteps)</h3>";
    html += "<div class='grid'>";
    html += "<button class='btn-move' onclick=\"window.location.href='/api?action=move_relative&steps=-200'\">⬅ 1 Rev</button>";
    html += "<button class='btn-move' onclick=\"window.location.href='/api?action=move_relative&steps=-50'\">⬅ 1/4 Rev</button>";
    html += "<button class='btn-move' onclick=\"window.location.href='/api?action=move_relative&steps=-10'\">⬅ 10 Steps</button>";
    html += "<button class='btn-move' onclick=\"window.location.href='/api?action=move_relative&steps=10'\">10 Steps ➡</button>";
    html += "<button class='btn-move' onclick=\"window.location.href='/api?action=move_relative&steps=50'\">1/4 Rev ➡</button>";
    html += "<button class='btn-move' onclick=\"window.location.href='/api?action=move_relative&steps=200'\">1 Rev ➡</button>";
    html += "</div>";
    
    html += "<h3>⚙ Motor Control</h3>";
    html += "<button class='btn-enable' onclick=\"window.location.href='/api?action=enable'\">🔌 Enable Motor</button> ";
    html += "<button class='btn-disable' onclick=\"window.location.href='/api?action=disable'\">⏹ Disable Motor</button> ";
    html += "<button class='btn-stop' onclick=\"window.location.href='/api?action=stop'\">🛑 Emergency Stop</button><br><br>";
    
    html += "<div style='text-align:center;margin:20px 0'>";
    html += "<button onclick='window.location.reload()' style='background:#17a2b8;color:white;padding:10px 20px'>🔄 Refresh Status</button>";
    html += "</div>";
    
    html += "<div style='font-size:12px;color:#666;text-align:center;margin-top:20px'>";
    html += "Page auto-refreshes every 5 seconds | Hardware: ESP32-C6 + TMC2209 + StepperOnline 17HS08-1004S";
    html += "</div>";
    
    html += "</div></body></html>";
    server.send(200, "text/html", html);
}

void handleWebAPI() {
    if (server.method() == HTTP_GET && !server.hasArg("action")) {
        // Status request only (no action parameter)
        JsonDocument doc;
        doc["deviceId"] = deviceId;
        doc["position"] = motion.position;
        doc["speed"] = motion.speed;
        doc["enabled"] = motion.enabled;
        doc["moving"] = motion.moving;
        doc["microsteps"] = driver.microsteps();
        doc["current"] = driver.rms_current();
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
        
    } else if (server.method() == HTTP_POST) {
        // Command request
        JsonDocument doc;
        deserializeJson(doc, server.arg("plain"));
        
        String action = doc["action"];
        handleMotorCommand(action, doc);
        
        server.send(200, "application/json", "{\"success\":true}");
        
    } else if (server.method() == HTTP_GET && server.hasArg("action")) {
        // Handle GET with parameters (simple commands from web buttons)
        String action = server.arg("action");
        JsonDocument params;
        
        if (server.hasArg("steps")) {
            params["steps"] = server.arg("steps").toInt();
        }
        
        handleMotorCommand(action, params);
        
        // Redirect back to main page with cache-busting
        server.sendHeader("Location", "/?t=" + String(millis()));
        server.sendHeader("Cache-Control", "no-cache");
        server.send(302, "text/html", "");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleMotorCommand(String action, JsonDocument& params) {
    Serial.println("Command: " + action);
    
    if (action == "enable") {
        motion.enabled = true;
        digitalWrite(ENABLE_PIN, LOW);  // Active low
        Serial.println("Motor enabled");
        
    } else if (action == "disable") {
        motion.enabled = false;
        motion.moving = false;
        digitalWrite(ENABLE_PIN, HIGH);  // Active low
        Serial.println("Motor disabled");
        
    } else if (action == "stop") {
        motion.moving = false;
        Serial.println("Motor stopped");
        
    } else if (action == "move_relative") {
        if (!motion.enabled) {
            Serial.println("Motor not enabled");
            return;
        }
        
        if (params["steps"].is<int>()) {
            int steps = params["steps"];
            Serial.println("Moving " + String(steps) + " steps");
            
            motion.moving = true;
            
            // Set direction
            digitalWrite(DIR_PIN, steps >= 0 ? HIGH : LOW);
            
            // Calculate timing for speed control
            int delayMicros = 1000000 / (2 * motion.speed);  // Microseconds per step edge
            
            // Execute steps
            for (int i = 0; i < abs(steps); i++) {
                if (!motion.moving) break;  // Allow stop
                
                digitalWrite(STEP_PIN, HIGH);
                delayMicroseconds(delayMicros);
                digitalWrite(STEP_PIN, LOW);
                delayMicroseconds(delayMicros);
                
                motion.position += (steps >= 0) ? 1 : -1;
                
                // Handle web requests during long moves
                if (i % 10 == 0) {
                    server.handleClient();
                }
            }
            
            motion.moving = false;
            Serial.println("Move complete. Position: " + String(motion.position));
        }
        
    } else if (action == "set_speed") {
        if (params["speed"].is<int>()) {
            motion.speed = params["speed"];
            Serial.println("Speed set to: " + String(motion.speed));
        }
        
    } else if (action == "home") {
        Serial.println("Homing to position 0");
        // Simple home - just reset position counter
        motion.position = 0;
        
    } else {
        Serial.println("Unknown command: " + action);
    }
}