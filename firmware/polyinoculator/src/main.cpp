/*
 * Enhanced Polyinoculator Control Firmware
 * Seeed Studio XIAO ESP32-C3 based prop controller
 * Multi-strip WS2812B LED controller with sACN (E1.31) protocol support
 * Enhanced with OTA updates, persistent configuration, and server integration
 * 
 * Features:
 * - Three LED strips on pins D3, D4 (longer), D5
 * - sACN E1.31 protocol support with priority handling
 * - OTA firmware updates via web interface
 * - Persistent configuration storage
 * - Battery monitoring
 * - Web server for configuration
 * - UDP communication with prop control server
 * - Save current colors as default startup colors
 * - Set sACN address by group functionality
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include "PolyinoculatorConfig.h"

// Pin definitions for Seeed Studio XIAO ESP32-C3 - Updated pin configuration
#define LED_PIN_1 D3       // Strip 1: 30 pixels on D3 (GPIO4)
#define LED_PIN_2 D4       // Strip 2: 60 pixels on D4 (GPIO5) - LONGER RIBBON
#define LED_PIN_3 D5       // Strip 3: 30 pixels on D5 (GPIO6)
#define STATUS_LED_PIN 21  // Built-in LED pin for status
#define BATTERY_ADC_PIN A0 // Battery voltage monitoring

// LED configuration - configurable via settings
#define MAX_LEDS_1 6       // Maximum LEDs for Strip 1 (small)
#define MAX_LEDS_2 14      // Maximum LEDs for Strip 2 (longer ribbon)
#define MAX_LEDS_3 6       // Maximum LEDs for Strip 3 (small)
#define TOTAL_MAX_LEDS 26  // Total maximum LEDs

// Configuration and state management
PolyinoculatorConfig polyConfig;
PolyinoculatorConfigData config;

// Network configuration
const int UDP_PORT = 8888;
const int WEB_PORT = 80;
const int SACN_PORT = 5568;     // sACN E1.31 standard port

// sACN E1.31 Constants
#define ACN_PACKET_IDENTIFIER "ASC-E1.17\0\0\0"
#define E131_PACKET_SIZE 638
#define E131_DATA_OFFSET 126
#define E131_UNIVERSE_OFFSET 113
#define SACN_MULTICAST_BASE "239.255.0.0"  // sACN multicast base address

// Device configuration - loaded from storage
String deviceId;
String deviceLabel;
String firmwareVersion = "Enhanced Polyinoculator v2.1 OTA";
int sacnUniverse;
int sacnStartAddress;
int fixtureNumber;
String wifiSSID;
String wifiPassword;

// Hardware objects - Separate arrays for each strip with maximum sizes
CRGB leds1[MAX_LEDS_1];  // Strip 1: D3 (GPIO4), up to 6 LEDs
CRGB leds2[MAX_LEDS_2];  // Strip 2: D4 (GPIO5), up to 14 LEDs - LONGER RIBBON
CRGB leds3[MAX_LEDS_3];  // Strip 3: D5 (GPIO6), up to 6 LEDs
WiFiUDP udp;
WiFiUDP sacnUdp;
WebServer webServer(80);

// State variables
bool wifiConnected = false;
CRGB currentColor = CRGB::Black;
uint8_t ledBrightness = 255;
bool sacnEnabled = true;
int actualLeds1, actualLeds2, actualLeds3;  // Actual LED counts from config

// sACN State Variables
unsigned long lastSacnPacket = 0;
uint8_t lastSacnData[512] = {0};  // Store last received DMX data
bool sacnActive = false;  // True when receiving sACN data
uint8_t sacnSequence = 0;  // Track sACN sequence numbers
bool sacnPriority = false;  // True when sACN should override UDP LED commands

// Battery monitoring
float batteryVoltage = 0.0;
int batteryPercentage = 0;
String batteryStatus = "Unknown";
unsigned long lastBatteryRead = 0;
const unsigned long BATTERY_READ_INTERVAL = 5000; // Read battery every 5 seconds

// Timing variables
unsigned long lastStatusSend = 0;
const unsigned long STATUS_INTERVAL = 10000; // Send status every 10 seconds
unsigned long lastSacnTimeout = 0;
const unsigned long SACN_TIMEOUT = 3000; // 3 seconds without sACN = timeout

// Task handles for background operations
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t ledTaskHandle = nullptr;
TaskHandle_t sacnTaskHandle = nullptr;

// Server discovery
String serverIP = "";
int serverPort = 8888;
unsigned long lastServerDiscovery = 0;
const unsigned long SERVER_DISCOVERY_INTERVAL = 30000; // Discover server every 30 seconds

// Function declarations
void setupWiFi();
void handleUDPPacket();
void sendStatus();
void setupWebServer();
void handleSacnPacket();
String getMulticastAddress(int universe);
void initializeSACN();
void updateLEDs();
void loadConfiguration();
void saveConfiguration();
void serverDiscovery();
void networkTask(void* parameter);
void ledTask(void* parameter);
void sacnTask(void* parameter);
void setBuiltinLED(uint8_t r, uint8_t g, uint8_t b);
void handleOTAUpdate(String firmwareUrl, String commandId);
void performOTAUpdate(String firmwareUrl, String commandId);
void handleRemoteFileUpload(String filename, String fileUrl, String commandId);
void sendResponse(String commandId, String message);
void loadDefaultColors();
void saveCurrentAsDefault();
void updateBatteryStatus();
String getBatteryStatusString(float voltage);
int calculateBatteryPercentage(float voltage);
void handleSetSacnAddressByGroup(int groupNumber);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("🚀 Enhanced Polyinoculator Starting...");
  Serial.printf("Firmware: %s\n", firmwareVersion.c_str());
  
  // Initialize configuration
  loadConfiguration();
  
  // Set up status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  setBuiltinLED(255, 0, 0); // Red during startup
  
  // Initialize LED strips with actual configured lengths
  actualLeds1 = polyConfig.getStripLength(1);
  actualLeds2 = polyConfig.getStripLength(2);
  actualLeds3 = polyConfig.getStripLength(3);
  
  Serial.printf("LED Strip Configuration:\n");
  Serial.printf("  Strip 1 (D3): %d LEDs\n", actualLeds1);
  Serial.printf("  Strip 2 (D4): %d LEDs (longer ribbon)\n", actualLeds2);
  Serial.printf("  Strip 3 (D5): %d LEDs\n", actualLeds3);
  
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds1, actualLeds1);
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds2, actualLeds2);
  FastLED.addLeds<WS2812B, LED_PIN_3, GRB>(leds3, actualLeds3);
  FastLED.setBrightness(polyConfig.getBrightness());
  
  // Load default colors if enabled
  if (polyConfig.useDefaultColors()) {
    loadDefaultColors();
  } else {
    // Turn off all LEDs
    fill_solid(leds1, actualLeds1, CRGB::Black);
    fill_solid(leds2, actualLeds2, CRGB::Black);
    fill_solid(leds3, actualLeds3, CRGB::Black);
  }
  FastLED.show();
  
  // Setup WiFi
  setupWiFi();
  
  // Setup UDP for device communication
  udp.begin(polyConfig.getUdpPort());
  Serial.printf("📡 UDP server started on port %d\n", polyConfig.getUdpPort());
  
  // Setup sACN UDP with multicast
  if (polyConfig.isSacnEnabled()) {
    initializeSACN();
  }
  
  // Setup web server
  setupWebServer();
  
  // Start background tasks
  xTaskCreatePinnedToCore(networkTask, "NetworkTask", 4096, NULL, 1, &networkTaskHandle, 0);
  xTaskCreatePinnedToCore(ledTask, "LEDTask", 2048, NULL, 2, &ledTaskHandle, 1);
  if (polyConfig.isSacnEnabled()) {
    xTaskCreatePinnedToCore(sacnTask, "SacnTask", 3072, NULL, 3, &sacnTaskHandle, 0);
  }
  
  setBuiltinLED(0, 255, 0); // Green when ready
  Serial.println("✅ Enhanced Polyinoculator ready!");
  Serial.printf("🌐 Device: %s (%s)\n", deviceLabel.c_str(), deviceId.c_str());
  Serial.printf("📡 IP Address: %s\n", WiFi.localIP().toString().c_str());
}

// Calculate multicast address for sACN universe
String getMulticastAddress(int universe) {
  // sACN uses multicast addresses 239.255.0.1 through 239.255.255.255
  // Universe 1 = 239.255.0.1, Universe 2 = 239.255.0.2, etc.
  int subnet = (universe >> 8) & 0xFF;
  int host = universe & 0xFF;
  
  if (subnet == 0) {
    subnet = 0;
    host = universe;
  }
  
  return String("239.255.") + String(subnet) + "." + String(host);
}

void initializeSACN() {
  if (!polyConfig.isSacnEnabled()) {
    Serial.println("🎭 sACN disabled in configuration");
    sacnEnabled = false;
    return;
  }
  
  // Get sACN configuration
  sacnUniverse = polyConfig.getSacnUniverse();
  sacnStartAddress = polyConfig.getDmxAddress();
  
  Serial.printf("🎭 Initializing sACN: Universe %d, Address %d\n", sacnUniverse, sacnStartAddress);
  
  // Calculate multicast address for our universe
  String multicastAddr = getMulticastAddress(sacnUniverse);
  IPAddress multicastIP;
  if (!multicastIP.fromString(multicastAddr)) {
    Serial.printf("❌ Invalid multicast address: %s\n", multicastAddr.c_str());
    sacnEnabled = false;
    return;
  }
  
  // Start sACN UDP socket with multicast
  if (sacnUdp.beginMulticast(multicastIP, SACN_PORT)) {
    Serial.printf("✅ sACN receiver started: %s:%d\n", multicastAddr.c_str(), SACN_PORT);
    sacnEnabled = true;
  } else {
    Serial.println("❌ Failed to start sACN receiver");
    sacnEnabled = false;
  }
}

void loop() {
  // Main loop is minimal - most work is done in background tasks
  delay(100);
  
  // Update battery status periodically
  if (millis() - lastBatteryRead > BATTERY_READ_INTERVAL) {
    updateBatteryStatus();
    lastBatteryRead = millis();
  }
}

void loadConfiguration() {
  Serial.println("📝 Loading configuration...");
  
  if (!polyConfig.load()) {
    Serial.println("⚠️ Using default configuration");
  }
  
  config = polyConfig.getConfig();
  
  // Set device variables from config
  deviceId = String(config.propId);
  deviceLabel = String(config.deviceLabel);
  sacnUniverse = config.sacnUniverse;
  sacnStartAddress = config.dmxAddress;
  fixtureNumber = config.fixtureNumber;
  wifiSSID = String(config.wifiSSID);
  wifiPassword = String(config.wifiPassword);
  sacnEnabled = config.sacnEnabled;
  ledBrightness = config.brightness;
  
  Serial.printf("✅ Configuration loaded - Device: %s (%s)\n", 
                deviceLabel.c_str(), deviceId.c_str());
}

void saveConfiguration() {
  Serial.println("💾 Saving configuration...");
  if (polyConfig.save()) {
    Serial.println("✅ Configuration saved");
  } else {
    Serial.println("❌ Failed to save configuration");
  }
}

void setupWiFi() {
  Serial.printf("🌐 Connecting to WiFi: %s\n", wifiSSID.c_str());
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    setBuiltinLED(255, 255, 0); // Yellow during connection
    delay(100);
    setBuiltinLED(0, 0, 0);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\n✅ WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    setBuiltinLED(0, 255, 0); // Green for connected
    
    // Setup mDNS
    if (MDNS.begin(config.hostname)) {
      Serial.printf("📡 mDNS started: %s.local\n", config.hostname);
    }
  } else {
    Serial.println("\n❌ WiFi connection failed");
    setBuiltinLED(255, 0, 0); // Red for failed
  }
}

void setupWebServer() {
  // Root page
  webServer.on("/", HTTP_GET, []() {
    String html = "<h1>Enhanced Polyinoculator</h1>";
    html += "<p>Device: " + deviceLabel + " (" + deviceId + ")</p>";
    html += "<p>Firmware: " + firmwareVersion + "</p>";
    html += "<p>Strips: " + String(actualLeds1) + " + " + String(actualLeds2) + " + " + String(actualLeds3) + " LEDs</p>";
    html += "<p>WiFi: " + WiFi.localIP().toString() + "</p>";
    html += "<p>sACN: Universe " + String(sacnUniverse) + ", Address " + String(sacnStartAddress) + "</p>";
    webServer.send(200, "text/html", html);
  });
  
  // Status endpoint
  webServer.on("/status", HTTP_GET, []() {
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = deviceId;
    doc["deviceLabel"] = deviceLabel;
    doc["firmwareVersion"] = firmwareVersion;
    doc["wifiConnected"] = wifiConnected;
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["sacnUniverse"] = sacnUniverse;
    doc["sacnAddress"] = sacnStartAddress;
    doc["strip1LEDs"] = actualLeds1;
    doc["strip2LEDs"] = actualLeds2;
    doc["strip3LEDs"] = actualLeds3;
    doc["batteryVoltage"] = batteryVoltage;
    doc["batteryPercentage"] = batteryPercentage;
    doc["batteryStatus"] = batteryStatus;
    
    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
  });
  
  webServer.begin();
  Serial.printf("🌐 Web server started on port %d\n", WEB_PORT);
}

void networkTask(void* parameter) {
  while (true) {
    if (wifiConnected) {
      // Handle UDP packets
      handleUDPPacket();
      
      // Send periodic status
      if (millis() - lastStatusSend > STATUS_INTERVAL) {
        sendStatus();
        lastStatusSend = millis();
      }
      
      // Server discovery
      if (millis() - lastServerDiscovery > SERVER_DISCOVERY_INTERVAL) {
        serverDiscovery();
        lastServerDiscovery = millis();
      }
      
      // Handle web server
      webServer.handleClient();
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void ledTask(void* parameter) {
  while (true) {
    updateLEDs();
    vTaskDelay(20 / portTICK_PERIOD_MS); // 50Hz update rate
  }
}

void sacnTask(void* parameter) {
  while (true) {
    if (wifiConnected && sacnEnabled) {
      handleSacnPacket();
      
      // Check for sACN timeout
      if (sacnActive && (millis() - lastSacnPacket > SACN_TIMEOUT)) {
        Serial.println("⏰ sACN timeout - returning to UDP control");
        sacnActive = false;
        sacnPriority = false;
      }
    }
    
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void handleUDPPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[512];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';
    
    // Parse JSON command
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, buffer);
    
    if (error) {
      Serial.printf("❌ JSON parsing failed: %s\n", error.c_str());
      return;
    }
    
    String action = doc["action"];
    String commandId = doc["commandId"] | "";
    
    Serial.printf("📨 Received command: %s\n", action.c_str());
    
    if (action == "set_led_color") {
      if (!sacnPriority) { // Only accept UDP commands when not in sACN priority mode
        uint8_t r = doc["red"] | 0;
        uint8_t g = doc["green"] | 0;
        uint8_t b = doc["blue"] | 0;
        int strip = doc["strip"] | 0; // 0 = all strips, 1-3 = specific strip
        
        CRGB color = CRGB(r, g, b);
        
        if (strip == 0 || strip == 1) fill_solid(leds1, actualLeds1, color);
        if (strip == 0 || strip == 2) fill_solid(leds2, actualLeds2, color);
        if (strip == 0 || strip == 3) fill_solid(leds3, actualLeds3, color);
        
        sendResponse(commandId, "LED color set");
      }
    }
    else if (action == "set_brightness") {
      uint8_t brightness = doc["brightness"] | 255;
      FastLED.setBrightness(brightness);
      polyConfig.setBrightness(brightness);
      sendResponse(commandId, "Brightness set to " + String(brightness));
    }
    else if (action == "save_current_as_default") {
      saveCurrentAsDefault();
      sendResponse(commandId, "Current LED colors saved as default");
    }
    else if (action == "set_sacn_address_by_group") {
      int groupNumber = doc["group"] | 1;
      handleSetSacnAddressByGroup(groupNumber);
      sendResponse(commandId, "sACN address set for group " + String(groupNumber));
    }
    else if (action == "ota_update") {
      String firmwareUrl = "";
      if (doc["parameters"].is<JsonObject>() && doc["parameters"]["firmware_url"].is<String>()) {
        firmwareUrl = doc["parameters"]["firmware_url"].as<String>();
      }
      
      if (firmwareUrl.length() > 0) {
        Serial.printf("🔄 Starting OTA update from: %s\n", firmwareUrl.c_str());
        sendResponse(commandId, "OTA update started");
        handleOTAUpdate(firmwareUrl, commandId);
      } else {
        Serial.println("❌ OTA update failed: No firmware URL provided");
        sendResponse(commandId, "OTA update failed: No firmware URL provided");
      }
    }
    else if (action == "server_discovery_response") {
      serverIP = doc["server_ip"] | "";
      serverPort = doc["server_port"] | 8888;
      Serial.printf("🔍 Server discovered: %s:%d\n", serverIP.c_str(), serverPort);
    }
  }
}

void handleSacnPacket() {
  int packetSize = sacnUdp.parsePacket();
  if (packetSize >= E131_PACKET_SIZE) {
    uint8_t buffer[E131_PACKET_SIZE];
    sacnUdp.read(buffer, E131_PACKET_SIZE);
    
    // Verify E1.31 packet
    if (memcmp(buffer + 4, ACN_PACKET_IDENTIFIER, 12) == 0) {
      uint16_t universe = (buffer[E131_UNIVERSE_OFFSET] << 8) | buffer[E131_UNIVERSE_OFFSET + 1];
      
      if (universe == sacnUniverse) {
        uint8_t sequence = buffer[111];
        uint8_t* dmxData = &buffer[E131_DATA_OFFSET];
        
        // Update sACN state
        lastSacnPacket = millis();
        sacnActive = true;
        sacnPriority = true;
        sacnSequence = sequence;
        
        // Copy DMX data
        memcpy(lastSacnData, dmxData, 512);
        
        // Map DMX channels to LED strips
        int baseAddress = sacnStartAddress - 1; // DMX is 1-based, array is 0-based
        
        // Strip 1 (RGB per LED)
        for (int i = 0; i < actualLeds1 && (baseAddress + i * 3 + 2) < 512; i++) {
          leds1[i] = CRGB(
            dmxData[baseAddress + i * 3],     // Red
            dmxData[baseAddress + i * 3 + 1], // Green
            dmxData[baseAddress + i * 3 + 2]  // Blue
          );
        }
        
        // Strip 2 (RGB per LED) - starts after Strip 1
        int strip2Base = baseAddress + (actualLeds1 * 3);
        for (int i = 0; i < actualLeds2 && (strip2Base + i * 3 + 2) < 512; i++) {
          leds2[i] = CRGB(
            dmxData[strip2Base + i * 3],     // Red
            dmxData[strip2Base + i * 3 + 1], // Green
            dmxData[strip2Base + i * 3 + 2]  // Blue
          );
        }
        
        // Strip 3 (RGB per LED) - starts after Strip 2
        int strip3Base = strip2Base + (actualLeds2 * 3);
        for (int i = 0; i < actualLeds3 && (strip3Base + i * 3 + 2) < 512; i++) {
          leds3[i] = CRGB(
            dmxData[strip3Base + i * 3],     // Red
            dmxData[strip3Base + i * 3 + 1], // Green
            dmxData[strip3Base + i * 3 + 2]  // Blue
          );
        }
      }
    }
  }
}

void updateLEDs() {
  FastLED.show();
}

void sendStatus() {
  if (!wifiConnected || serverIP.length() == 0) return;
  
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["type"] = "polyinoculator";
  doc["deviceLabel"] = deviceLabel;
  doc["fixtureNumber"] = fixtureNumber;
  doc["firmwareVersion"] = firmwareVersion;
  doc["wifiConnected"] = wifiConnected;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["sacnUniverse"] = sacnUniverse;
  doc["sacnAddress"] = sacnStartAddress;
  doc["sacnActive"] = sacnActive;
  doc["strip1LEDs"] = actualLeds1;
  doc["strip2LEDs"] = actualLeds2;
  doc["strip3LEDs"] = actualLeds3;
  doc["totalLEDs"] = actualLeds1 + actualLeds2 + actualLeds3;
  doc["batteryVoltage"] = batteryVoltage;
  doc["batteryPercentage"] = batteryPercentage;
  doc["batteryStatus"] = batteryStatus;
  doc["timestamp"] = millis();
  
  String status;
  serializeJson(doc, status);
  
  udp.beginPacket(serverIP.c_str(), serverPort);
  udp.write((const uint8_t*)status.c_str(), status.length());
  udp.endPacket();
}

void serverDiscovery() {
  DynamicJsonDocument doc(256);
  doc["deviceId"] = deviceId;
  doc["action"] = "server_discovery";
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  
  // Broadcast discovery message
  udp.beginPacket("255.255.255.255", UDP_PORT);
  udp.write((const uint8_t*)message.c_str(), message.length());
  udp.endPacket();
}

void sendResponse(String commandId, String message) {
  if (!wifiConnected || serverIP.length() == 0 || commandId.length() == 0) return;
  
  DynamicJsonDocument doc(512);
  doc["deviceId"] = deviceId;
  doc["commandId"] = commandId;
  doc["response"] = message;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  udp.beginPacket(serverIP.c_str(), serverPort);
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
}

void setBuiltinLED(uint8_t r, uint8_t g, uint8_t b) {
  // Simple status LED control (assuming RGB LED on pin 21)
  // This is a simplified implementation - adjust based on your hardware
  if (r > 0) digitalWrite(STATUS_LED_PIN, HIGH);
  else digitalWrite(STATUS_LED_PIN, LOW);
}

void loadDefaultColors() {
  const PolyinoculatorConfigData& cfg = polyConfig.getConfig();
  
  // Load Strip 1 default colors
  for (int i = 0; i < actualLeds1; i++) {
    leds1[i] = CRGB(cfg.strip1DefaultColorR[i], cfg.strip1DefaultColorG[i], cfg.strip1DefaultColorB[i]);
  }
  
  // Load Strip 2 default colors
  for (int i = 0; i < actualLeds2; i++) {
    leds2[i] = CRGB(cfg.strip2DefaultColorR[i], cfg.strip2DefaultColorG[i], cfg.strip2DefaultColorB[i]);
  }
  
  // Load Strip 3 default colors
  for (int i = 0; i < actualLeds3; i++) {
    leds3[i] = CRGB(cfg.strip3DefaultColorR[i], cfg.strip3DefaultColorG[i], cfg.strip3DefaultColorB[i]);
  }
  
  Serial.println("✅ Default colors loaded");
}

void saveCurrentAsDefault() {
  // Save current Strip 1 colors
  uint8_t r1[MAX_LEDS_1], g1[MAX_LEDS_1], b1[MAX_LEDS_1];
  for (int i = 0; i < actualLeds1; i++) {
    r1[i] = leds1[i].r;
    g1[i] = leds1[i].g;
    b1[i] = leds1[i].b;
  }
  polyConfig.setDefaultColors(1, r1, g1, b1, actualLeds1);
  
  // Save current Strip 2 colors
  uint8_t r2[MAX_LEDS_2], g2[MAX_LEDS_2], b2[MAX_LEDS_2];
  for (int i = 0; i < actualLeds2; i++) {
    r2[i] = leds2[i].r;
    g2[i] = leds2[i].g;
    b2[i] = leds2[i].b;
  }
  polyConfig.setDefaultColors(2, r2, g2, b2, actualLeds2);
  
  // Save current Strip 3 colors
  uint8_t r3[MAX_LEDS_3], g3[MAX_LEDS_3], b3[MAX_LEDS_3];
  for (int i = 0; i < actualLeds3; i++) {
    r3[i] = leds3[i].r;
    g3[i] = leds3[i].g;
    b3[i] = leds3[i].b;
  }
  polyConfig.setDefaultColors(3, r3, g3, b3, actualLeds3);
  
  polyConfig.setUseDefaultColors(true);
  polyConfig.save();
  
  Serial.println("💾 Current LED colors saved as default startup colors");
}

void handleSetSacnAddressByGroup(int groupNumber) {
  // Set sACN address based on group number
  // Each polyinoculator needs (actualLeds1 + actualLeds2 + actualLeds3) * 3 channels
  int totalChannels = (actualLeds1 + actualLeds2 + actualLeds3) * 3;
  int newAddress = ((groupNumber - 1) * totalChannels) + 1;
  
  polyConfig.setDmxAddress(newAddress);
  polyConfig.save();
  
  sacnStartAddress = newAddress;
  
  Serial.printf("🎭 sACN address set to %d for group %d (needs %d channels)\n", 
                newAddress, groupNumber, totalChannels);
}

void updateBatteryStatus() {
  if (!polyConfig.isBatteryMonitoringEnabled()) return;
  
  // Read battery voltage from ADC
  int adcValue = analogRead(BATTERY_ADC_PIN);
  batteryVoltage = (adcValue / 4095.0) * 3.3 * 2.0; // Assuming voltage divider
  batteryVoltage *= polyConfig.getBatteryCalibration(); // Apply calibration
  
  batteryPercentage = calculateBatteryPercentage(batteryVoltage);
  batteryStatus = getBatteryStatusString(batteryVoltage);
}

String getBatteryStatusString(float voltage) {
  if (voltage > 4.0) return "High";
  else if (voltage > 3.7) return "Good";
  else if (voltage > 3.4) return "Low";
  else if (voltage > 3.0) return "Very Low";
  else return "Critical";
}

int calculateBatteryPercentage(float voltage) {
  // Simple linear mapping - adjust based on your battery type
  if (voltage >= 4.2) return 100;
  if (voltage <= 3.0) return 0;
  return (int)((voltage - 3.0) / (4.2 - 3.0) * 100);
}

void handleOTAUpdate(String firmwareUrl, String commandId) {
  Serial.printf("🔄 Handling OTA update from URL: %s\n", firmwareUrl.c_str());
  
  // Set status LEDs to indicate OTA in progress
  setBuiltinLED(0, 0, 255); // Blue for OTA
  
  // Perform the OTA update
  performOTAUpdate(firmwareUrl, commandId);
}

void performOTAUpdate(String firmwareUrl, String commandId) {
  Serial.printf("📡 Starting OTA update from: %s\n", firmwareUrl.c_str());
  
  // Configure HTTPUpdate
  httpUpdate.setLedPin(STATUS_LED_PIN, LOW);
  httpUpdate.rebootOnUpdate(false); // We'll handle reboot ourselves
  
  // Set up progress callback
  httpUpdate.onProgress([](int cur, int total) {
    if (total > 0) {
      int progress = (cur * 100) / total;
      Serial.printf("📊 OTA Progress: %d%% (%d/%d bytes)\n", progress, cur, total);
    }
  });
  
  // Perform the update
  WiFiClient client;
  t_httpUpdate_return result = httpUpdate.update(client, firmwareUrl);
  
  switch (result) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Update failed: %s\n", httpUpdate.getLastErrorString().c_str());
      setBuiltinLED(255, 0, 0); // Red for error
      break;
      
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("⚠️ No updates available");
      setBuiltinLED(255, 255, 0); // Yellow for no update
      break;
      
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Update successful! Rebooting...");
      setBuiltinLED(0, 255, 0); // Green for success
      
      delay(2000); // Give time to see the message
      
      // Gracefully stop tasks before reboot
      if (networkTaskHandle) vTaskDelete(networkTaskHandle);
      if (ledTaskHandle) vTaskDelete(ledTaskHandle);
      if (sacnTaskHandle) vTaskDelete(sacnTaskHandle);
      
      ESP.restart();
      break;
  }
}
