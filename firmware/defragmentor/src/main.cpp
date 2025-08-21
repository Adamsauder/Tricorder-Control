#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <driver/ledc.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "PropConfig.h"

// Pin definitions for ESP32-C3 XIAO
#define LED_PIN D2    // D2 (GPIO4) - LEDs
#define TRIGGER_PIN D3 // D3 (GPIO5) - Trigger input (changed from GPIO6)
#define SERVO_PIN D10  // D10 (GPIO10) - Servo signal (changed from GPIO18)
#define POWER_PIN D8   // D8 (GPIO8) - 5V boost enable

// LED configuration for RGBW SK6812 LEDs
#define NUM_LEDS 2
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

// Servo PWM configuration for ESP32-C3 native LEDC
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50 // 50Hz for servo
#define SERVO_RESOLUTION LEDC_TIMER_14_BIT

// System state
bool currentState = false;  // false=idle, true=active
int servoPosition = 0;      // Current servo position (0-180)
bool powerEnabled = false;

// Configuration system
PropConfig propConfig;
PropConfig::Config config;

// Device configuration variables - loaded from persistent storage
String deviceId = "DEFRAGMENTOR_001";
String deviceLabel = "Defragmentor 001";
String deviceType = "defragmentor";
int sacnUniverse = 1;
int sacnStartAddress = 1;
int numLEDs = NUM_LEDS;
int ledBrightness = 128;
int fixtureNumber = 1;

// WiFi settings - loaded from configuration
String wifiSSID = "Rigging Electric";
String wifiPassword = "academy123";

// Web server instance
AsyncWebServer server(80);

// Function declarations
void loadConfiguration();
void enableServoPower(bool enable);
void setupNativePWM();
void moveServoToPosition(int angle);
void setLEDPattern();
void initializeWiFi();
void setupWebServer();
void sendPeriodicStatus();
void handleGetConfig();

// Configuration management functions - need request parameter
void handleGetConfig(AsyncWebServerRequest *request);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("Simplified Defragmentor Control System Starting...");
  Serial.println("Hardware: ESP32-C3 XIAO with Native PWM Servo Control");
  
  // Initialize configuration system first
  if (!propConfig.begin()) {
    Serial.println("ERROR: Failed to initialize configuration storage!");
    return;
  }
  
  // Load configuration (with WiFi credentials)
  loadConfiguration();
  
  Serial.printf("Device: %s (%s)\n", deviceLabel.c_str(), deviceId.c_str());
  Serial.printf("Pin assignments: LEDs=D2(GPIO%d), Trigger=D3(GPIO%d), Servo=D10(GPIO%d), Power=D8(GPIO%d)\n", 
                LED_PIN, TRIGGER_PIN, SERVO_PIN, POWER_PIN);
  Serial.printf("Configuration: SACN Universe=%d, DMX Address=%d, Brightness=%d\n",
                sacnUniverse, sacnStartAddress, ledBrightness);
  Serial.printf("WiFi: %s / %s\n", wifiSSID.c_str(), wifiPassword.c_str());
  
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
  
  // Initialize RGBW LEDs
  Serial.println("Initializing RGBW NeoPixel LEDs...");
  strip.begin();
  strip.setBrightness(ledBrightness);
  strip.clear();
  strip.show();
  
  // Set initial LED state
  setLEDPattern();
  
  // Initialize WiFi
  if (wifiSSID.length() > 0) {
    initializeWiFi();
    // Setup web server with OTA after WiFi connection
    if (WiFi.status() == WL_CONNECTED) {
      setupWebServer();
    }
  } else {
    Serial.println("No WiFi credentials configured - running in standalone mode");
  }
  
  Serial.println("Testing Defragmentor systems...");
  
  // Test LED sequence
  for (int i = 0; i < 3; i++) {
    strip.setPixelColor(0, strip.Color(255, 0, 0, 0));  // Red
    strip.setPixelColor(1, strip.Color(0, 255, 0, 0));  // Green
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
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
    setLEDPattern(); // LEDs change FIRST for immediate feedback
    moveServoToPosition(180); // Then servo moves to active position
    setLEDPattern(); // LEDs change back after movement completes
  } else if (!triggerPressed && currentState) {
    // Trigger released, return to idle
    Serial.println("Trigger released - returning to idle");
    currentState = false;
    setLEDPattern(); // LEDs change FIRST for immediate feedback
    moveServoToPosition(0); // Then servo returns to idle position
    setLEDPattern(); // LEDs change back after movement completes
  }
  
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
  // Servo expects wider pulse range for full 180° movement
  // 0.5ms = 2.5% duty, 2.5ms = 12.5% duty at 50Hz
  // For 14-bit resolution: 0.5ms = 410, 1.5ms = 1229, 2.5ms = 2048
  uint32_t min_duty = 410;   // 0.5ms pulse width (wider range)
  uint32_t max_duty = 2048;  // 2.5ms pulse width (wider range)
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
    // Active state - both LEDs bright red when triggered
    strip.setPixelColor(0, strip.Color(255, 0, 0, 0));  // Bright red (R,G,B,W)
    strip.setPixelColor(1, strip.Color(255, 0, 0, 0));  // Bright red (R,G,B,W)
  } else {
    // Idle state - LED 0 teal, LED 1 pure white (using white channel)
    strip.setPixelColor(0, strip.Color(0, 128, 128, 0));  // Teal (R,G,B,W)
    strip.setPixelColor(1, strip.Color(0, 0, 0, 255));    // Pure white using W channel
  }
  strip.show();
  Serial.printf("LEDs set to %s pattern\n", currentState ? "ACTIVE (both red)" : "IDLE (teal/white)");
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

// Setup web server with OTA functionality
void setupWebServer() {
  Serial.println("Setting up web server...");
  
  // Main status page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><title>Defragmentor Control</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
    html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "button{padding:10px 20px;margin:5px;border:none;border-radius:4px;cursor:pointer;font-size:16px}";
    html += ".btn-primary{background:#007bff;color:white}";
    html += ".btn-danger{background:#dc3545;color:white}";
    html += ".btn-success{background:#28a745;color:white}";
    html += ".btn-warning{background:#ffc107;color:black}";
    html += "input[type='range']{width:100%}</style></head><body>";
    html += "<div class='container'><h1>Defragmentor Control Panel</h1>";
    html += "<div><strong>Device:</strong> ESP32-C3 XIAO</div>";
    html += "<div><strong>Status:</strong> <span id='status'>Loading...</span></div>";
    html += "<div><strong>Servo Position:</strong> <span id='servo'>Loading...</span>&deg;</div>";
    html += "<div><strong>Power:</strong> <span id='power'>Loading...</span></div>";
    html += "<div><h3>Servo Control</h3>";
    html += "<input type='range' id='servoSlider' min='0' max='180' value='90' onchange='moveServo(this.value)'>";
    html += "<p>Position: <span id='servoValue'>90</span>&deg;</p></div>";
    html += "<div><h3>Quick Actions</h3>";
    html += "<button class='btn-success' onclick='activate()'>Activate</button>";
    html += "<button class='btn-danger' onclick='deactivate()'>Deactivate</button>";
    html += "<button class='btn-warning' onclick='togglePower()'>Toggle Power</button>";
    html += "<button class='btn-primary' onclick='refreshStatus()'>Refresh</button></div>";
    html += "<div style='margin-top:30px;padding:20px;background:#e9ecef;border-radius:4px'>";
    html += "<h3>Firmware Updates</h3>";
    html += "<p>Upload new firmware to update the defragmentor remotely.</p>";
    html += "<button class='btn-primary' onclick='window.open(\"/update\", \"_blank\")'>Open OTA Update</button></div></div>";
    html += "<script>";
    html += "function refreshStatus(){fetch('/status').then(response=>response.json()).then(data=>{";
    html += "document.getElementById('status').textContent=data.state?'ACTIVE':'IDLE';";
    html += "document.getElementById('servo').textContent=data.servo;";
    html += "document.getElementById('power').textContent=data.power?'ON':'OFF';";
    html += "document.getElementById('servoSlider').value=data.servo;";
    html += "document.getElementById('servoValue').textContent=data.servo;});}";
    html += "function activate(){fetch('/activate',{method:'POST'}).then(()=>refreshStatus());}";
    html += "function deactivate(){fetch('/deactivate',{method:'POST'}).then(()=>refreshStatus());}";
    html += "function togglePower(){fetch('/toggle-power',{method:'POST'}).then(()=>refreshStatus());}";
    html += "function moveServo(angle){document.getElementById('servoValue').textContent=angle;";
    html += "fetch('/servo/'+angle,{method:'POST'}).then(()=>refreshStatus());}";
    html += "setInterval(refreshStatus,5000);refreshStatus();";
    html += "</script></body></html>";
    request->send(200, "text/html", html);
  });

  // Configuration page
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><title>Defragmentor Configuration</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
    html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += ".form-group{margin:15px 0}.form-group label{display:block;margin-bottom:5px;font-weight:bold}";
    html += ".form-group input{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}";
    html += "button{padding:10px 20px;margin:5px;border:none;border-radius:4px;cursor:pointer;font-size:16px}";
    html += ".btn-primary{background:#007bff;color:white}.btn-success{background:#28a745;color:white}";
    html += ".btn-secondary{background:#6c757d;color:white}.section{margin:30px 0;padding:20px;border:1px solid #ddd;border-radius:8px}";
    html += ".section h3{margin-top:0;color:#007bff}</style></head><body>";
    html += "<div class='container'><h1>Defragmentor Configuration</h1>";
    
    // Device Settings Section
    html += "<div class='section'><h3>Device Settings</h3>";
    html += "<div class='form-group'><label>Device Label:</label>";
    html += "<input type='text' id='deviceLabel' value='" + deviceLabel + "'></div>";
    html += "<div class='form-group'><label>Fixture Number:</label>";
    html += "<input type='number' id='fixtureNumber' min='1' max='999' value='" + String(fixtureNumber) + "'></div>";
    html += "</div>";
    
    // SACN/DMX Settings Section
    html += "<div class='section'><h3>SACN/DMX Settings</h3>";
    html += "<div class='form-group'><label>SACN Universe (1-63999):</label>";
    html += "<input type='number' id='sacnUniverse' min='1' max='63999' value='" + String(sacnUniverse) + "'></div>";
    html += "<div class='form-group'><label>DMX Start Address (1-512):</label>";
    html += "<input type='number' id='dmxStartAddress' min='1' max='512' value='" + String(sacnStartAddress) + "'></div>";
    html += "</div>";
    
    // LED Settings Section
    html += "<div class='section'><h3>LED Settings</h3>";
    html += "<div class='form-group'><label>Brightness (0-255):</label>";
    html += "<input type='range' id='brightness' min='0' max='255' value='" + String(ledBrightness) + "' oninput='document.getElementById(\"brightnessValue\").textContent=this.value'>";
    html += "<span id='brightnessValue'>" + String(ledBrightness) + "</span></div>";
    html += "<div class='form-group'><label>Number of LEDs:</label>";
    html += "<input type='number' id='numLeds' min='1' max='100' value='" + String(numLEDs) + "' readonly style='background:#f8f9fa'></div>";
    html += "</div>";
    
    // WiFi Settings Section
    html += "<div class='section'><h3>WiFi Settings</h3>";
    html += "<div class='form-group'><label>WiFi SSID:</label>";
    html += "<input type='text' id='wifiSSID' value='" + wifiSSID + "'></div>";
    html += "<div class='form-group'><label>WiFi Password:</label>";
    html += "<input type='password' id='wifiPassword' value='" + wifiPassword + "'></div>";
    html += "</div>";
    
    // Device Info Section
    html += "<div class='section'><h3>Device Information</h3>";
    html += "<p><strong>Device ID:</strong> " + deviceId + "</p>";
    html += "<p><strong>Device Type:</strong> " + deviceType + "</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>Firmware Version:</strong> v1.0.0</p>";
    html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
    html += "</div>";
    
    // Action Buttons
    html += "<div style='text-align:center;margin:30px 0'>";
    html += "<button class='btn-success' onclick='saveConfiguration()'>Save Configuration</button>";
    html += "<button class='btn-secondary' onclick='window.location.href=\"/\"'>Back to Control Panel</button>";
    html += "<button class='btn-primary' onclick='window.location.reload()'>Refresh</button>";
    html += "<button class='btn-warning' onclick='rebootDevice()' style='background:#ffc107;color:#212529;margin-left:20px'>Reboot Device</button>";
    html += "</div>";
    
    html += "<div id='status' style='margin:20px 0;padding:10px;border-radius:4px;display:none'></div>";
    html += "</div>";
    
    // JavaScript for configuration management
    html += "<script>";
    html += "function saveConfiguration(){";
    html += "const config={";
    html += "deviceLabel:document.getElementById('deviceLabel').value,";
    html += "fixtureNumber:parseInt(document.getElementById('fixtureNumber').value),";
    html += "sacnUniverse:parseInt(document.getElementById('sacnUniverse').value),";
    html += "dmxStartAddress:parseInt(document.getElementById('dmxStartAddress').value),";
    html += "brightness:parseInt(document.getElementById('brightness').value),";
    html += "wifiSSID:document.getElementById('wifiSSID').value,";
    html += "wifiPassword:document.getElementById('wifiPassword').value};";
    html += "fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(config)})";
    html += ".then(response=>response.json()).then(data=>{";
    html += "const status=document.getElementById('status');";
    html += "if(data.status==='updated'){";
    html += "status.style.display='block';status.style.background='#d4edda';status.style.color='#155724';";
    html += "status.textContent='Configuration saved successfully! Changes will take effect after restart.';";
    html += "}else{";
    html += "status.style.display='block';status.style.background='#fff3cd';status.style.color='#856404';";
    html += "status.textContent='No changes were made to the configuration.';}";
    html += "}).catch(error=>{";
    html += "const status=document.getElementById('status');";
    html += "status.style.display='block';status.style.background='#f8d7da';status.style.color='#721c24';";
    html += "status.textContent='Error saving configuration: '+error.message;});}";
    html += "function rebootDevice(){";
    html += "if(confirm('Are you sure you want to reboot the device? This will restart the defragmentor.')){";
    html += "fetch('/reboot',{method:'POST'})";
    html += ".then(()=>{";
    html += "const status=document.getElementById('status');";
    html += "status.style.display='block';status.style.background='#d4edda';status.style.color='#155724';";
    html += "status.textContent='Device is rebooting... Please wait 10 seconds then refresh the page.';";
    html += "}).catch(error=>{";
    html += "console.error('Reboot error:',error);";
    html += "});}}";
    html += "</script></body></html>";
    
    request->send(200, "text/html", html);
  });
  
  // Status API endpoint
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["device"] = "Defragmentor";
    doc["state"] = currentState;
    doc["servo"] = servoPosition;
    doc["power"] = powerEnabled;
    doc["wifi"] = WiFi.status() == WL_CONNECTED;
    doc["ip"] = WiFi.localIP().toString();
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // Control endpoints
  server.on("/activate", HTTP_POST, [](AsyncWebServerRequest *request){
    currentState = true;
    moveServoToPosition(180);
    setLEDPattern();
    request->send(200, "text/plain", "Activated");
  });
  
  server.on("/deactivate", HTTP_POST, [](AsyncWebServerRequest *request){
    currentState = false;
    moveServoToPosition(0);
    setLEDPattern();
    request->send(200, "text/plain", "Deactivated");
  });
  
  server.on("/toggle-power", HTTP_POST, [](AsyncWebServerRequest *request){
    enableServoPower(!powerEnabled);
    request->send(200, "text/plain", powerEnabled ? "Power ON" : "Power OFF");
  });
  
  // Servo control endpoint
  server.on("^\\/servo\\/([0-9]+)$", HTTP_POST, [](AsyncWebServerRequest *request){
    String angleStr = request->pathArg(0);
    int angle = angleStr.toInt();
    angle = constrain(angle, 0, 180);
    moveServoToPosition(angle);
    request->send(200, "text/plain", "Servo moved to " + String(angle) + "°");
  });

  // OTA Update page
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><title>OTA Update</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
    html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += ".upload-area{border:2px dashed #ccc;padding:40px;text-align:center;margin:20px 0;border-radius:8px}";
    html += ".upload-area.dragover{border-color:#007bff;background:#f8f9fa}";
    html += "input[type='file']{margin:20px 0}";
    html += "button{padding:10px 20px;margin:5px;border:none;border-radius:4px;cursor:pointer;font-size:16px;background:#007bff;color:white}";
    html += ".progress{width:100%;height:20px;background:#f0f0f0;border-radius:10px;margin:20px 0;overflow:hidden}";
    html += ".progress-bar{height:100%;background:#28a745;width:0%;transition:width 0.3s}";
    html += ".hidden{display:none}</style></head><body>";
    html += "<div class='container'><h1>Firmware Update</h1>";
    html += "<p><strong>Device:</strong> ESP32-C3 XIAO Defragmentor</p>";
    html += "<p><strong>Current Version:</strong> v1.0.0</p>";
    html += "<div class='upload-area' id='uploadArea'>";
    html += "<p>Select a firmware file (.bin) to upload</p>";
    html += "<input type='file' id='fileInput' accept='.bin' style='display:none'>";
    html += "<button onclick='document.getElementById(\"fileInput\").click()'>Choose File</button>";
    html += "<p id='fileName'></p></div>";
    html += "<div class='progress hidden' id='progressContainer'>";
    html += "<div class='progress-bar' id='progressBar'></div></div>";
    html += "<p id='status'></p>";
    html += "<button onclick='uploadFirmware()' id='uploadBtn' disabled>Upload Firmware</button>";
    html += "<button onclick='window.location.href=\"/\"'>Back to Control Panel</button>";
    html += "</div><script>";
    html += "let selectedFile=null;";
    html += "document.getElementById('fileInput').addEventListener('change',function(e){";
    html += "selectedFile=e.target.files[0];";
    html += "if(selectedFile){";
    html += "document.getElementById('fileName').textContent='Selected: '+selectedFile.name+' ('+Math.round(selectedFile.size/1024)+'KB)';";
    html += "document.getElementById('uploadBtn').disabled=false;}});";
    html += "function uploadFirmware(){";
    html += "if(!selectedFile){alert('Please select a file first');return;}";
    html += "const formData=new FormData();formData.append('firmware',selectedFile);";
    html += "const xhr=new XMLHttpRequest();";
    html += "xhr.upload.addEventListener('progress',function(e){";
    html += "if(e.lengthComputable){";
    html += "const percentComplete=(e.loaded/e.total)*100;";
    html += "document.getElementById('progressContainer').classList.remove('hidden');";
    html += "document.getElementById('progressBar').style.width=percentComplete+'%';";
    html += "document.getElementById('status').textContent='Uploading: '+Math.round(percentComplete)+'%';}});";
    html += "xhr.addEventListener('load',function(){";
    html += "if(xhr.status===200){";
    html += "document.getElementById('status').textContent='Upload successful! Device is rebooting...';";
    html += "setTimeout(function(){window.location.href='/';},5000);}else{";
    html += "document.getElementById('status').textContent='Upload failed: '+xhr.responseText;}});";
    html += "xhr.addEventListener('error',function(){";
    html += "document.getElementById('status').textContent='Upload failed due to network error';});";
    html += "xhr.open('POST','/update');xhr.send(formData);}";
    html += "</script></body></html>";
    request->send(200, "text/html", html);
  });

  // OTA Update handler
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    bool shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
    if (shouldReboot) {
      delay(100);
      ESP.restart();
    }
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if (!index) {
      Serial.printf("OTA Update Start: %s\n", filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("OTA Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // Configuration API endpoints
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
    handleGetConfig(request);
  });
  
  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request){
    // This will be handled by the body handler
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    // Handle JSON body data
    String body = "";
    for (size_t i = 0; i < len; i++) {
      body += (char)data[i];
    }
    
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    bool configChanged = false;
    
    // Update configuration
    if (doc.containsKey("deviceLabel")) {
      String newLabel = doc["deviceLabel"].as<String>();
      if (newLabel != deviceLabel) {
        deviceLabel = newLabel;
        propConfig.setDeviceLabel(newLabel);
        configChanged = true;
      }
    }
    
    if (doc.containsKey("fixtureNumber")) {
      int newNumber = doc["fixtureNumber"];
      if (newNumber != fixtureNumber && newNumber >= 1 && newNumber <= 999) {
        fixtureNumber = newNumber;
        propConfig.setFixtureNumber(newNumber);
        configChanged = true;
      }
    }
    
    if (doc.containsKey("sacnUniverse")) {
      int newUniverse = doc["sacnUniverse"];
      if (newUniverse != sacnUniverse && newUniverse >= 1 && newUniverse <= 63999) {
        sacnUniverse = newUniverse;
        propConfig.setSACNUniverse(newUniverse);
        configChanged = true;
      }
    }
    
    if (doc.containsKey("dmxStartAddress")) {
      int newAddress = doc["dmxStartAddress"];
      if (newAddress != sacnStartAddress && newAddress >= 1 && newAddress <= 512) {
        sacnStartAddress = newAddress;
        propConfig.setDMXStartAddress(newAddress);
        configChanged = true;
      }
    }
    
    if (doc.containsKey("brightness")) {
      int newBrightness = doc["brightness"];
      if (newBrightness != ledBrightness && newBrightness >= 0 && newBrightness <= 255) {
        ledBrightness = newBrightness;
        propConfig.setBrightness(newBrightness);
        strip.setBrightness(ledBrightness);
        strip.show(); // Apply brightness change immediately
        configChanged = true;
      }
    }
    
    if (doc.containsKey("wifiSSID")) {
      String newSSID = doc["wifiSSID"].as<String>();
      if (newSSID != wifiSSID) {
        wifiSSID = newSSID;
        propConfig.setWiFiSSID(newSSID);
        configChanged = true;
      }
    }
    
    if (doc.containsKey("wifiPassword")) {
      String newPassword = doc["wifiPassword"].as<String>();
      if (newPassword != wifiPassword) {
        wifiPassword = newPassword;
        propConfig.setWiFiPassword(newPassword);
        configChanged = true;
      }
    }
    
    if (configChanged) {
      // Update the main config struct with all current values
      config.deviceLabel = deviceLabel;
      config.fixtureNumber = fixtureNumber;
      config.sacnUniverse = sacnUniverse;
      config.dmxStartAddress = sacnStartAddress;
      config.brightness = ledBrightness;
      config.wifiSSID = wifiSSID;
      config.wifiPassword = wifiPassword;
      
      // Save all changes to NVS
      if (propConfig.saveConfig(config)) {
        Serial.println("Configuration updated and saved to NVS");
      } else {
        Serial.println("Configuration updated but failed to save to NVS");
      }
      
      propConfig.printConfig();
      request->send(200, "application/json", "{\"status\":\"updated\"}");
    } else {
      request->send(200, "application/json", "{\"status\":\"no_changes\"}");
    }
  });

  // Reboot endpoint
  server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("Reboot requested via web interface");
    request->send(200, "application/json", "{\"status\":\"rebooting\",\"message\":\"Device will reboot in 2 seconds\"}");
    delay(100); // Allow response to be sent
    Serial.println("Rebooting device...");
    ESP.restart();
  });

  // Start server
  server.begin();
  
  Serial.println("Web server started!");
  Serial.printf("Control interface: http://%s/\n", WiFi.localIP().toString().c_str());
}

// Configuration management functions
void loadConfiguration() {
  if (propConfig.loadConfig(config)) {
    deviceId = config.deviceLabel.substring(0, config.deviceLabel.indexOf('_')) + "_" + String(random(1000, 9999));
    deviceLabel = config.deviceLabel;
    sacnUniverse = config.sacnUniverse;
    sacnStartAddress = config.dmxStartAddress;
    numLEDs = config.numLeds;
    fixtureNumber = config.fixtureNumber;
    ledBrightness = config.brightness;
    wifiSSID = config.wifiSSID;
    wifiPassword = config.wifiPassword;
    
    if (config.firstBoot) {
      Serial.println("First boot detected - using defaults");
      propConfig.setFirstBoot(false);
    }
  } else {
    Serial.println("Failed to load config - using defaults");
    // Set defaults for defragmentor
    deviceId = "DEFRAGMENTOR_" + String(random(1000, 9999));
    deviceLabel = "Defragmentor " + String(random(100, 999));
    sacnUniverse = 1;
    sacnStartAddress = 1;
    numLEDs = NUM_LEDS;
    fixtureNumber = 1;
    ledBrightness = 128;
    wifiSSID = "Rigging Electric";
    wifiPassword = "academy123";
    
    // Save defaults
    config.deviceLabel = deviceLabel;
    config.sacnUniverse = sacnUniverse;
    config.dmxStartAddress = sacnStartAddress;
    config.numLeds = numLEDs;
    config.brightness = ledBrightness;
    config.wifiSSID = wifiSSID;
    config.wifiPassword = wifiPassword;
    config.deviceType = "defragmentor";
    config.fixtureNumber = fixtureNumber;
    config.firstBoot = false;
    propConfig.saveConfig(config);
  }
  
  Serial.println("Configuration loaded:");
  Serial.printf("LED brightness will be set to: %d\n", ledBrightness);
  propConfig.printConfig();
}

void handleGetConfig(AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["deviceId"] = deviceId;
  doc["deviceLabel"] = deviceLabel;
  doc["deviceType"] = "defragmentor";
  doc["firmwareVersion"] = "v1.0.0";
  doc["sacnUniverse"] = sacnUniverse;
  doc["dmxStartAddress"] = sacnStartAddress;
  doc["numLeds"] = numLEDs;
  doc["brightness"] = ledBrightness;
  doc["wifiSSID"] = wifiSSID;
  doc["fixtureNumber"] = fixtureNumber;
  doc["online"] = true;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["uptime"] = millis();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["currentState"] = currentState;
  doc["servoPosition"] = servoPosition;
  doc["powerEnabled"] = powerEnabled;
  
  String response;
  serializeJson(doc, response);
  
  AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
  res->addHeader("Access-Control-Allow-Origin", "*");
  request->send(res);
}

// Send periodic status updates
void sendPeriodicStatus() {
  Serial.printf("Status: State=%s, Servo=%d°, Power=%s, WiFi=%s\n", 
                currentState ? "ACTIVE" : "IDLE", 
                servoPosition,
                powerEnabled ? "ON" : "OFF",
                WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
}
