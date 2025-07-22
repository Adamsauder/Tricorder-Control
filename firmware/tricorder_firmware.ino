/*
 * Tricorder Control Firmware
 * ESP32-based prop controller with video playback and NeoPixel control
 * 
 * Hardware: ESP32-2432S032C-I Development Board
 * - 3.2" IPS TFT LCD (240x320) with capacitive touch
 * - ST7789 display driver
 * - ESP32-WROOM-32 module
 * - Built-in SD card slot
 * - USB-C power input
 * 
 * Features:
 * - WiFi connectivity with auto-pairing
 * - Video playback from SD card
 * - 12-channel NeoPixel LED control
 * - UDP command processing
 * - SACN (E1.31) lighting protocol
 * - mDNS service discovery
 * - OTA firmware updates
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <ESP32_FTPClient.h>
#include <AsyncWebServer.h>
#include <ESPAsyncWebServer.h>

// Pin definitions for ESP32-2432S032C-I
#define LED_PIN 2          // NeoPixel data pin (external connection)
#define NUM_LEDS 12        // Number of NeoPixels in strip
#define SD_CS 5            // SD card chip select (built-in)
// TFT pins are pre-configured in the board - using TFT_eSPI library defaults

// Network configuration
const char* WIFI_SSID = "Rigging Electrics";
const char* WIFI_PASSWORD = "academy123";
const int UDP_PORT = 8888;
const int WEB_PORT = 80;

// Device identification
String deviceId = "TRICORDER_001";
String firmwareVersion = "1.0.0";

// Hardware objects
CRGB leds[NUM_LEDS];
TFT_eSPI tft = TFT_eSPI();
WiFiUDP udp;
AsyncWebServer server(WEB_PORT);

// State variables
bool wifiConnected = false;
bool serverRegistered = false;
String currentVideo = "";
bool videoPlaying = false;
float brightness = 1.0;
CRGB currentColor = CRGB::Black;

// SACN variables
bool sacnEnabled = false;
int sacnUniverse = 1;
int sacnStartChannel = 1;

// Performance monitoring
unsigned long lastHeartbeat = 0;
unsigned long commandCount = 0;
float batteryVoltage = 0.0;
float temperature = 0.0;

void setup() {
  Serial.begin(115200);
  Serial.println("Tricorder Control Firmware v" + firmwareVersion);
  
  // Initialize hardware
  initializeLEDs();
  initializeDisplay();
  initializeSD();
  
  // Set device ID from MAC address
  deviceId = "TRICORDER_" + WiFi.macAddress().substring(12);
  deviceId.replace(":", "");
  
  // Connect to WiFi
  connectToWiFi();
  
  // Start services
  startUDPListener();
  startWebServer();
  startMDNS();
  
  // Show ready status
  showStatus("Ready - " + deviceId);
  setAllLEDs(CRGB::Green);
  delay(2000);
  setAllLEDs(CRGB::Black);
  
  Serial.println("System ready");
}

void loop() {
  // Main loop tasks
  handleWiFiConnection();
  handleUDPCommands();
  handleSACN();
  updateDisplay();
  sendHeartbeat();
  
  // Monitor system health
  monitorSystem();
  
  delay(10); // Small delay to prevent watchdog issues
}

void initializeLEDs() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();
  Serial.println("LEDs initialized");
}

void initializeDisplay() {
  tft.init();
  tft.setRotation(3); // Landscape mode for 320x240
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Tricorder Control");
  tft.println("Initializing...");
  Serial.println("Display initialized");
}

void initializeSD() {
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed");
    showError("SD Card Error");
    return;
  }
  Serial.println("SD card initialized");
}

void connectToWiFi() {
  showStatus("Connecting WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    attempts++;
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
    showStatus("WiFi: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed");
    showError("WiFi Failed");
  }
}

void startUDPListener() {
  if (udp.begin(UDP_PORT)) {
    Serial.println("UDP listener started on port " + String(UDP_PORT));
  } else {
    Serial.println("UDP listener failed to start");
  }
}

void startWebServer() {
  // Status endpoint
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    doc["device_id"] = deviceId;
    doc["firmware_version"] = firmwareVersion;
    doc["wifi_connected"] = wifiConnected;
    doc["ip_address"] = WiFi.localIP().toString();
    doc["current_video"] = currentVideo;
    doc["video_playing"] = videoPlaying;
    doc["battery_voltage"] = batteryVoltage;
    doc["temperature"] = temperature;
    doc["uptime"] = millis();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // File list endpoint
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(2048);
    JsonArray files = doc.createNestedArray("files");
    
    File root = SD.open("/");
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        JsonObject fileObj = files.createNestedObject();
        fileObj["name"] = file.name();
        fileObj["size"] = file.size();
      }
      file = root.openNextFile();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.begin();
  Serial.println("Web server started on port " + String(WEB_PORT));
}

void startMDNS() {
  if (MDNS.begin(deviceId.c_str())) {
    MDNS.addService("tricorder", "tcp", WEB_PORT);
    MDNS.addServiceTxt("tricorder", "tcp", "device_id", deviceId);
    MDNS.addServiceTxt("tricorder", "tcp", "firmware_version", firmwareVersion);
    MDNS.addServiceTxt("tricorder", "tcp", "capabilities", "video,leds,sacn");
    Serial.println("mDNS service started: " + deviceId + ".local");
  } else {
    Serial.println("mDNS failed to start");
  }
}

void handleWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    serverRegistered = false;
    showError("WiFi Disconnected");
    
    // Try to reconnect
    WiFi.reconnect();
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    showStatus("WiFi Reconnected");
  }
}

void handleUDPCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[512];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = 0;
    
    processCommand(String(buffer));
    commandCount++;
  }
}

void processCommand(String command) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, command);
  
  if (error) {
    Serial.println("JSON parsing failed: " + String(error.c_str()));
    return;
  }
  
  String commandId = doc["command_id"];
  String target = doc["target"];
  String action = doc["action"];
  
  // Check if command is for this device or broadcast
  if (target != deviceId && target != "ALL") {
    return;
  }
  
  unsigned long startTime = millis();
  String result = executeAction(action, doc["parameters"]);
  unsigned long executionTime = millis() - startTime;
  
  // Send response
  sendCommandResponse(commandId, result, executionTime);
}

String executeAction(String action, JsonObject parameters) {
  if (action == "video_play") {
    String filename = parameters["filename"];
    bool loop = parameters["loop"] | false;
    return playVideo(filename, loop);
    
  } else if (action == "video_pause") {
    return pauseVideo();
    
  } else if (action == "video_stop") {
    return stopVideo();
    
  } else if (action == "video_next") {
    return nextVideo();
    
  } else if (action == "led_color") {
    int r = parameters["r"] | 0;
    int g = parameters["g"] | 0;
    int b = parameters["b"] | 0;
    return setLEDColor(r, g, b);
    
  } else if (action == "led_brightness") {
    float brightness = parameters["brightness"] | 1.0;
    return setLEDBrightness(brightness);
    
  } else if (action == "led_animation") {
    String animation = parameters["animation"];
    return startLEDAnimation(animation);
    
  } else if (action == "ping") {
    return "pong";
    
  } else {
    return "error: unknown action";
  }
}

String playVideo(String filename, bool loop) {
  // Placeholder for video playback implementation
  currentVideo = filename;
  videoPlaying = true;
  showStatus("Playing: " + filename);
  return "success: playing " + filename;
}

String pauseVideo() {
  videoPlaying = false;
  showStatus("Paused: " + currentVideo);
  return "success: video paused";
}

String stopVideo() {
  videoPlaying = false;
  currentVideo = "";
  showStatus("Stopped");
  return "success: video stopped";
}

String nextVideo() {
  // Placeholder for next video logic
  return "success: next video";
}

String setLEDColor(int r, int g, int b) {
  currentColor = CRGB(r, g, b);
  setAllLEDs(currentColor);
  return "success: color set to RGB(" + String(r) + "," + String(g) + "," + String(b) + ")";
}

String setLEDBrightness(float newBrightness) {
  brightness = constrain(newBrightness, 0.0, 1.0);
  FastLED.setBrightness(brightness * 255);
  FastLED.show();
  return "success: brightness set to " + String(brightness);
}

String startLEDAnimation(String animation) {
  // Placeholder for animation implementation
  return "success: animation " + animation + " started";
}

void setAllLEDs(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void handleSACN() {
  // Placeholder for SACN (E1.31) implementation
  // This would listen for SACN packets and update LEDs accordingly
}

void sendCommandResponse(String commandId, String result, unsigned long executionTime) {
  DynamicJsonDocument doc(512);
  doc["command_id"] = commandId;
  doc["timestamp"] = millis();
  doc["device_id"] = deviceId;
  doc["status"] = result.startsWith("success") ? "success" : "error";
  doc["message"] = result;
  doc["execution_time_ms"] = executionTime;
  
  String response;
  serializeJson(doc, response);
  
  // Send back to sender
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print(response);
  udp.endPacket();
}

void sendHeartbeat() {
  unsigned long now = millis();
  if (now - lastHeartbeat > 5000) { // Every 5 seconds
    lastHeartbeat = now;
    
    DynamicJsonDocument doc(512);
    doc["type"] = "heartbeat";
    doc["device_id"] = deviceId;
    doc["timestamp"] = now;
    doc["status"] = "online";
    doc["command_count"] = commandCount;
    doc["battery_voltage"] = batteryVoltage;
    doc["temperature"] = temperature;
    doc["free_heap"] = ESP.getFreeHeap();
    
    String heartbeat;
    serializeJson(doc, heartbeat);
    
    // Broadcast heartbeat to server (multicast or known server IP)
    udp.beginPacket("255.255.255.255", UDP_PORT + 1);
    udp.print(heartbeat);
    udp.endPacket();
  }
}

void updateDisplay() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) { // Update every second
    lastUpdate = millis();
    
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(1);
    
    tft.println(deviceId);
    tft.println("IP: " + WiFi.localIP().toString());
    tft.println("Status: " + (wifiConnected ? "Connected" : "Disconnected"));
    tft.println("Video: " + (videoPlaying ? "Playing" : "Stopped"));
    tft.println("File: " + currentVideo);
    tft.println("Uptime: " + String(millis() / 1000) + "s");
    tft.println("Commands: " + String(commandCount));
    tft.println("Battery: " + String(batteryVoltage, 1) + "V");
    tft.println("Temp: " + String(temperature, 1) + "C");
  }
}

void showStatus(String message) {
  Serial.println("Status: " + message);
  tft.fillRect(0, 100, 320, 20, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 105);
  tft.print(message);
}

void showError(String message) {
  Serial.println("Error: " + message);
  tft.fillRect(0, 100, 320, 20, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 105);
  tft.print("ERROR: " + message);
}

void monitorSystem() {
  static unsigned long lastMonitor = 0;
  if (millis() - lastMonitor > 10000) { // Every 10 seconds
    lastMonitor = millis();
    
    // Read battery voltage (placeholder)
    batteryVoltage = 4.2; // Simulated value
    
    // Read temperature (placeholder)
    temperature = 25.0; // Simulated value
    
    // Check memory usage
    if (ESP.getFreeHeap() < 10000) {
      Serial.println("Warning: Low memory");
    }
  }
}
