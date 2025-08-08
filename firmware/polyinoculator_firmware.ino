/*
 * Polyinoculator Control Firmware
 * Seeed Studio XIAO ESP32-C3 based prop controller
 * 12x WS2812B LEDs controlled via SACN (E1.31) protocol
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// Pin definitions for Seeed Studio XIAO ESP32-C3
#define LED_PIN 10         // NeoPixel data pin - D10/GPIO10 (recommended for XIAO C3)
#define NUM_LEDS 12        // Number of WS2812B pixels
#define STATUS_LED_PIN 3   // Optional status LED pin

// Network configuration
const char* WIFI_SSID = "Rigging Electric";
const char* WIFI_PASSWORD = "academy123";
const int UDP_PORT = 8888;
// SACN will be controlled via UDP commands from server, not direct processing

// Device identification
String deviceId = "POLYINOCULATOR_001";
String firmwareVersion = "0.1";
int sacnUniverse = 1;  // SACN universe for this device (configurable)
int sacnStartAddress = 4;  // Starting DMX address (channels 4-39 for 12 LEDs)

// Hardware objects
CRGB leds[NUM_LEDS];
WiFiUDP udp;

// State variables
bool wifiConnected = false;
CRGB currentColor = CRGB::Black;
uint8_t ledBrightness = 128;
bool sacnEnabled = true;

// Timing variables
unsigned long lastStatusSend = 0;
const unsigned long STATUS_INTERVAL = 10000; // Send status every 10 seconds

// Function declarations
void handleUDPCommands();
void setLEDColor(int r, int g, int b);
void setLEDBrightness(int brightness);
void setIndividualLED(int ledIndex, int r, int g, int b);
void rainbow();
void scannerEffect(int r, int g, int b, int delayMs = 100);
void pulseEffect(int r, int g, int b, int duration = 2000);
void sendResponse(String commandId, String result);
void sendStatus(String commandId);
void sendPeriodicStatus();
void processNetworkCommand(String jsonCommand);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Polyinoculator Control System...");
  
  // Initialize LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  
  // Optional status LED (if available)
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Startup LED effect
  rainbow();
  
  // Initialize WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) { // 20 second timeout
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Breathing effect during WiFi connection
    int brightness = (sin(attempts * 0.3) + 1) * 64;
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, brightness));
    FastLED.show();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    
    // Initialize UDP for control commands
    udp.begin(UDP_PORT);
    Serial.printf("UDP server listening on port %d\n", UDP_PORT);
    
    // Start mDNS
    if (MDNS.begin(deviceId.c_str())) {
      Serial.println("mDNS responder started");
      MDNS.addService("polyinoculator", "udp", UDP_PORT);
    }
    
    // Success LED pattern
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Green;
      FastLED.show();
      delay(50);
    }
    delay(500);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    
    digitalWrite(STATUS_LED_PIN, HIGH); // Status LED on when connected
  } else {
    Serial.println("\nFailed to connect to WiFi");
    // Error LED pattern
    for (int i = 0; i < 5; i++) {
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      FastLED.show();
      delay(200);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(200);
    }
  }
  
  Serial.println("Setup complete!");
}

void loop() {
  // Handle UDP control commands
  handleUDPCommands();
  
  // Send periodic status
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > STATUS_INTERVAL) {
    sendPeriodicStatus();
    lastStatus = millis();
  }
  
  // Check WiFi connection
  static bool lastWifiStatus = false;
  bool currentWifiStatus = (WiFi.status() == WL_CONNECTED);
  
  if (currentWifiStatus != lastWifiStatus) {
    wifiConnected = currentWifiStatus;
    digitalWrite(STATUS_LED_PIN, currentWifiStatus ? HIGH : LOW);
    
    if (!currentWifiStatus) {
      Serial.println("WiFi disconnected!");
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      FastLED.show();
    } else {
      Serial.println("WiFi reconnected!");
      fill_solid(leds, NUM_LEDS, CRGB::Green);
      FastLED.show();
      delay(500);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
    }
    lastWifiStatus = currentWifiStatus;
  }
  
  delay(1); // Small delay to prevent watchdog issues
}

void handleUDPCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    
    processNetworkCommand(String(incomingPacket));
  }
}

void processNetworkCommand(String jsonCommand) {
  Serial.printf("Received JSON: %s\n", jsonCommand.c_str());
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonCommand);
  
  if (!error) {
    String action = doc["action"];
    String commandId = doc["commandId"];
    
    Serial.printf("Parsed action='%s', commandId='%s'\n", action.c_str(), commandId.c_str());
    
    if (action == "discovery") {
      JsonDocument response;
      response["commandId"] = commandId;
      response["deviceId"] = deviceId;
      response["type"] = "polyinoculator";
      response["firmwareVersion"] = firmwareVersion;
      response["ipAddress"] = WiFi.localIP().toString();
      response["numLeds"] = NUM_LEDS;
      response["sacnUniverse"] = sacnUniverse;
      
      String responseStr;
      serializeJson(response, responseStr);
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.print(responseStr);
      udp.endPacket();
    }
    else if (action == "set_led_color") {
      int r = doc["r"];
      int g = doc["g"];
      int b = doc["b"];
      setLEDColor(r, g, b);
      sendResponse(commandId, "LED color set");
    }
    else if (action == "set_brightness") {
      int brightness = doc["brightness"];
      setLEDBrightness(brightness);
      sendResponse(commandId, "Brightness set");
    }
    else if (action == "set_individual_led") {
      int ledIndex = doc["ledIndex"];
      int r = doc["r"];
      int g = doc["g"];
      int b = doc["b"];
      setIndividualLED(ledIndex, r, g, b);
      sendResponse(commandId, "Individual LED set");
    }
    else if (action == "set_leds_array") {
      // Command for setting multiple LEDs at once (used by SACN)
      if (doc.containsKey("leds") && doc["leds"].is<JsonArray>()) {
        JsonArray ledsArray = doc["leds"];
        for (int i = 0; i < ledsArray.size() && i < NUM_LEDS; i++) {
          if (ledsArray[i].is<JsonArray>() && ledsArray[i].size() >= 3) {
            int r = ledsArray[i][0];
            int g = ledsArray[i][1];
            int b = ledsArray[i][2];
            leds[i] = CRGB(r, g, b);
          }
        }
        FastLED.show();
        sendResponse(commandId, "LED array set");
      } else {
        sendResponse(commandId, "Invalid LED array format");
      }
    }
    else if (action == "rainbow") {
      rainbow();
      sendResponse(commandId, "Rainbow effect activated");
    }
    else if (action == "scanner") {
      int r = doc.containsKey("r") ? doc["r"] : 255;
      int g = doc.containsKey("g") ? doc["g"] : 0;
      int b = doc.containsKey("b") ? doc["b"] : 0;
      scannerEffect(r, g, b);
      sendResponse(commandId, "Scanner effect activated");
    }
    else if (action == "pulse") {
      int r = doc.containsKey("r") ? doc["r"] : 255;
      int g = doc.containsKey("g") ? doc["g"] : 255;
      int b = doc.containsKey("b") ? doc["b"] : 255;
      pulseEffect(r, g, b);
      sendResponse(commandId, "Pulse effect activated");
    }
    else if (action == "toggle_sacn") {
      sacnEnabled = !sacnEnabled;
      sendResponse(commandId, sacnEnabled ? "SACN enabled" : "SACN disabled");
    }
    else if (action == "status") {
      sendStatus(commandId);
    }
  }
}

void setLEDColor(int r, int g, int b) {
  currentColor = CRGB(r, g, b);
  fill_solid(leds, NUM_LEDS, currentColor);
  FastLED.show();
  Serial.printf("LED color set to R:%d G:%d B:%d\n", r, g, b);
}

void setLEDBrightness(int brightness) {
  ledBrightness = constrain(brightness, 0, 255);
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
  Serial.printf("LED brightness set to %d\n", ledBrightness);
}

void setIndividualLED(int ledIndex, int r, int g, int b) {
  if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
    leds[ledIndex] = CRGB(r, g, b);
    FastLED.show();
    Serial.printf("LED %d set to R:%d G:%d B:%d\n", ledIndex, r, g, b);
  }
}

void rainbow() {
  for (int j = 0; j < 256; j++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(((i * 256 / NUM_LEDS) + j) & 255, 255, 255);
    }
    FastLED.show();
    delay(10);
  }
}

void scannerEffect(int r, int g, int b, int delayMs) {
  CRGB color = CRGB(r, g, b);
  
  // Scan left to right
  for (int i = 0; i < NUM_LEDS; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[i] = color;
    FastLED.show();
    delay(delayMs);
  }
  
  // Scan right to left
  for (int i = NUM_LEDS - 2; i >= 1; i--) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[i] = color;
    FastLED.show();
    delay(delayMs);
  }
}

void pulseEffect(int r, int g, int b, int duration) {
  CRGB color = CRGB(r, g, b);
  unsigned long startTime = millis();
  
  while (millis() - startTime < duration) {
    float progress = (millis() - startTime) / (float)duration;
    float brightness = (sin(progress * 2 * PI) + 1) / 2; // 0 to 1
    
    CRGB dimmedColor = color;
    dimmedColor.nscale8(255 * brightness);
    
    fill_solid(leds, NUM_LEDS, dimmedColor);
    FastLED.show();
    delay(20);
  }
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void sendResponse(String commandId, String result) {
  JsonDocument doc;
  doc["commandId"] = commandId;
  doc["result"] = result;
  doc["timestamp"] = millis();
  doc["deviceId"] = deviceId;
  
  String response;
  serializeJson(doc, response);
  
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
}

void sendStatus(String commandId) {
  JsonDocument doc;
  doc["commandId"] = commandId;
  doc["deviceId"] = deviceId;
  doc["firmwareVersion"] = firmwareVersion;
  doc["wifiConnected"] = wifiConnected;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["numLeds"] = NUM_LEDS;
  doc["brightness"] = ledBrightness;
  doc["sacnEnabled"] = sacnEnabled;
  doc["sacnUniverse"] = sacnUniverse;
  
  String response;
  serializeJson(doc, response);
  
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
}

void sendPeriodicStatus() {
  JsonDocument doc;
  doc["deviceId"] = deviceId;
  doc["type"] = "polyinoculator";
  doc["firmwareVersion"] = firmwareVersion;
  doc["wifiConnected"] = wifiConnected;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["numLeds"] = NUM_LEDS;
  doc["brightness"] = ledBrightness;
  doc["sacnEnabled"] = sacnEnabled;
  doc["sacnUniverse"] = sacnUniverse;
  doc["timestamp"] = millis();
  
  String statusMsg;
  serializeJson(doc, statusMsg);
  
  // Broadcast to server
  IPAddress localIP = WiFi.localIP();
  IPAddress serverIP(localIP[0], localIP[1], localIP[2], 24);
  
  udp.beginPacket(serverIP, UDP_PORT);
  udp.write((const uint8_t*)statusMsg.c_str(), statusMsg.length());
  udp.endPacket();
}
