/*
 * Polyinoculator Control Firmware
 * Seeed Studio XIAO ESP32-C3 based prop controller
 * Multi-strip WS2812B LEDs controlled via SACN (E1.31) protocol
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// Pin definitions for Seeed Studio XIAO ESP32-C3 - Multi-strip configuration
// Note: Using digital pin numbers (D10, D3, D4) - preserves USB-JTAG functionality
#define LED_PIN_1 D10      // Strip 1: 7 pixels on D10 (GPIO18 - USB-JTAG but can be overridden)
#define LED_PIN_2 D3       // Strip 2: 4 pixels on D3 (GPIO21)
#define LED_PIN_3 D4       // Strip 3: 4 pixels on D4 (GPIO6)
#define NUM_LEDS_1 7       // Strip 1 length
#define NUM_LEDS_2 4       // Strip 2 length
#define NUM_LEDS_3 4       // Strip 3 length
#define TOTAL_LEDS 15      // Total: 7 + 4 + 4 = 15 pixels
#define STATUS_LED_PIN 3   // Optional status LED pin

// Network configuration
const char* WIFI_SSID = "Rigging Electric";
const char* WIFI_PASSWORD = "academy123";
const int UDP_PORT = 8888;
// SACN will be controlled via UDP commands from server, not direct processing

// Device identification
String deviceId = "POLYINOCULATOR_001";
String firmwareVersion = "0.2";
int sacnUniverse = 1;  // SACN universe for this device (configurable)
int sacnStartAddress = 4;  // Starting DMX address (channels 4-48 for 15 LEDs)

// Hardware objects - Separate arrays for each strip
CRGB leds1[NUM_LEDS_1];  // Strip 1: D10 (GPIO18), 7 LEDs
CRGB leds2[NUM_LEDS_2];  // Strip 2: D3 (GPIO21), 4 LEDs
CRGB leds3[NUM_LEDS_3];  // Strip 3: D4 (GPIO6), 4 LEDs
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
void setAllLEDColor(int r, int g, int b);
void setStripColor(int stripNum, int r, int g, int b);
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
  Serial.printf("Multi-strip configuration: Strip1=%d LEDs, Strip2=%d LEDs, Strip3=%d LEDs\n", 
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("Pin assignments: D10=%d LEDs, D3=%d LEDs, D4=%d LEDs\n",
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  
  // Initialize LED strips - Multi-pin configuration with alternate color orders
  // Note: Some WS2812 strips use RGB instead of GRB color order
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds1, NUM_LEDS_1);  // Strip 1: D10 (GPIO18), 7 LEDs
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds2, NUM_LEDS_2);  // Strip 2: D3 (GPIO21), 4 LEDs
  FastLED.addLeds<WS2812B, LED_PIN_3, GRB>(leds3, NUM_LEDS_3);  // Strip 3: D4 (GPIO6), 4 LEDs
  
  // Try different color order if GRB doesn't work:
  // FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds2, NUM_LEDS_2);  // Alternative for Strip 2
  // FastLED.addLeds<WS2812, LED_PIN_3, RGB>(leds3, NUM_LEDS_3);  // Alternative for Strip 3
  
  FastLED.setBrightness(ledBrightness);
  
  // Clear all LEDs first
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
  FastLED.show();
  delay(500);
  
  // Test each strip individually for diagnostics
  Serial.println("Testing strips individually...");
  
  // Test Strip 1 - Red
  Serial.println("Testing Strip 1 (D10/GPIO18) - RED");
  fill_solid(leds1, NUM_LEDS_1, CRGB::Red);
  FastLED.show();
  delay(2000);
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  FastLED.show();
  delay(500);
  
  // Test Strip 2 - Green  
  Serial.println("Testing Strip 2 (D3/GPIO21) - GREEN");
  fill_solid(leds2, NUM_LEDS_2, CRGB::Green);
  FastLED.show();
  delay(2000);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  FastLED.show();
  delay(500);
  
  // Test Strip 3 - Blue
  Serial.println("Testing Strip 3 (D4/GPIO6) - BLUE");
  fill_solid(leds3, NUM_LEDS_3, CRGB::Blue);
  FastLED.show();
  delay(2000);
  fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
  FastLED.show();
  delay(1000);
  
  Serial.println("Strip testing complete. Check which colors appeared.");
  
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
    
    // Breathing effect during WiFi connection - all strips
    int brightness = (sin(attempts * 0.3) + 1) * 64;
    fill_solid(leds1, NUM_LEDS_1, CRGB(0, 0, brightness));
    fill_solid(leds2, NUM_LEDS_2, CRGB(0, 0, brightness));
    fill_solid(leds3, NUM_LEDS_3, CRGB(0, 0, brightness));
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
    
    // Success LED pattern - light up all strips sequentially
    for (int i = 0; i < NUM_LEDS_1; i++) {
      leds1[i] = CRGB::Green;
      FastLED.show();
      delay(50);
    }
    for (int i = 0; i < NUM_LEDS_2; i++) {
      leds2[i] = CRGB::Green;
      FastLED.show();
      delay(50);
    }
    for (int i = 0; i < NUM_LEDS_3; i++) {
      leds3[i] = CRGB::Green;
      FastLED.show();
      delay(50);
    }
    delay(500);
    fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
    fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
    fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
    FastLED.show();
    
    digitalWrite(STATUS_LED_PIN, HIGH); // Status LED on when connected
  } else {
    Serial.println("\nFailed to connect to WiFi");
    // Error LED pattern - flash all strips red
    for (int i = 0; i < 5; i++) {
      fill_solid(leds1, NUM_LEDS_1, CRGB::Red);
      fill_solid(leds2, NUM_LEDS_2, CRGB::Red);
      fill_solid(leds3, NUM_LEDS_3, CRGB::Red);
      FastLED.show();
      delay(200);
      fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
      fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
      fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
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
      fill_solid(leds1, NUM_LEDS_1, CRGB::Red);
      fill_solid(leds2, NUM_LEDS_2, CRGB::Red);
      fill_solid(leds3, NUM_LEDS_3, CRGB::Red);
      FastLED.show();
    } else {
      Serial.println("WiFi reconnected!");
      fill_solid(leds1, NUM_LEDS_1, CRGB::Green);
      fill_solid(leds2, NUM_LEDS_2, CRGB::Green);
      fill_solid(leds3, NUM_LEDS_3, CRGB::Green);
      FastLED.show();
      delay(500);
      fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
      fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
      fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
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
      response["numLeds"] = TOTAL_LEDS;
      response["numLeds1"] = NUM_LEDS_1;
      response["numLeds2"] = NUM_LEDS_2;
      response["numLeds3"] = NUM_LEDS_3;
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
      setAllLEDColor(r, g, b);
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
        for (int i = 0; i < ledsArray.size() && i < TOTAL_LEDS; i++) {
          if (ledsArray[i].is<JsonArray>() && ledsArray[i].size() >= 3) {
            int r = ledsArray[i][0];
            int g = ledsArray[i][1];
            int b = ledsArray[i][2];
            setIndividualLED(i, r, g, b); // Use our function that handles strip mapping
          }
        }
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

void setAllLEDColor(int r, int g, int b) {
  currentColor = CRGB(r, g, b);
  fill_solid(leds1, NUM_LEDS_1, currentColor);
  fill_solid(leds2, NUM_LEDS_2, currentColor);
  fill_solid(leds3, NUM_LEDS_3, currentColor);
  FastLED.show();
  Serial.printf("All LED strips set to R:%d G:%d B:%d\n", r, g, b);
}

void setStripColor(int stripNum, int r, int g, int b) {
  CRGB color = CRGB(r, g, b);
  switch(stripNum) {
    case 1:
      fill_solid(leds1, NUM_LEDS_1, color);
      break;
    case 2:
      fill_solid(leds2, NUM_LEDS_2, color);
      break;
    case 3:
      fill_solid(leds3, NUM_LEDS_3, color);
      break;
    default:
      Serial.printf("Invalid strip number: %d\n", stripNum);
      return;
  }
  FastLED.show();
  Serial.printf("Strip %d set to R:%d G:%d B:%d\n", stripNum, r, g, b);
}

void setLEDBrightness(int brightness) {
  ledBrightness = constrain(brightness, 0, 255);
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
  Serial.printf("LED brightness set to %d\n", ledBrightness);
}

void setIndividualLED(int ledIndex, int r, int g, int b) {
  // Map global LED index to strip and local index
  if (ledIndex >= 0 && ledIndex < TOTAL_LEDS) {
    CRGB color = CRGB(r, g, b);
    
    if (ledIndex < NUM_LEDS_1) {
      // Strip 1: LEDs 0-6
      leds1[ledIndex] = color;
    } else if (ledIndex < NUM_LEDS_1 + NUM_LEDS_2) {
      // Strip 2: LEDs 7-10
      leds2[ledIndex - NUM_LEDS_1] = color;
    } else {
      // Strip 3: LEDs 11-14
      leds3[ledIndex - NUM_LEDS_1 - NUM_LEDS_2] = color;
    }
    
    FastLED.show();
    Serial.printf("LED %d set to R:%d G:%d B:%d\n", ledIndex, r, g, b);
  }
}

void rainbow() {
  for (int j = 0; j < 256; j++) {
    // Strip 1: 7 LEDs
    for (int i = 0; i < NUM_LEDS_1; i++) {
      leds1[i] = CHSV(((i * 256 / NUM_LEDS_1) + j) & 255, 255, 255);
    }
    // Strip 2: 4 LEDs
    for (int i = 0; i < NUM_LEDS_2; i++) {
      leds2[i] = CHSV(((i * 256 / NUM_LEDS_2) + j + 85) & 255, 255, 255); // Offset hue
    }
    // Strip 3: 4 LEDs
    for (int i = 0; i < NUM_LEDS_3; i++) {
      leds3[i] = CHSV(((i * 256 / NUM_LEDS_3) + j + 170) & 255, 255, 255); // Different offset
    }
    FastLED.show();
    delay(10);
  }
}

void scannerEffect(int r, int g, int b, int delayMs) {
  CRGB color = CRGB(r, g, b);
  
  // Clear all strips
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
  
  // Scan through Strip 1 (7 LEDs)
  for (int i = 0; i < NUM_LEDS_1; i++) {
    fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
    leds1[i] = color;
    FastLED.show();
    delay(delayMs);
  }
  
  // Scan through Strip 2 (4 LEDs)
  for (int i = 0; i < NUM_LEDS_2; i++) {
    fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
    leds2[i] = color;
    FastLED.show();
    delay(delayMs);
  }
  
  // Scan through Strip 3 (4 LEDs)
  for (int i = 0; i < NUM_LEDS_3; i++) {
    fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
    leds3[i] = color;
    FastLED.show();
    delay(delayMs);
  }
  
  // Clear all again
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
  FastLED.show();
}

void pulseEffect(int r, int g, int b, int duration) {
  CRGB color = CRGB(r, g, b);
  unsigned long startTime = millis();
  
  while (millis() - startTime < duration) {
    float progress = (millis() - startTime) / (float)duration;
    float brightness = (sin(progress * 2 * PI) + 1) / 2; // 0 to 1
    
    CRGB dimmedColor = color;
    dimmedColor.nscale8(255 * brightness);
    
    fill_solid(leds1, NUM_LEDS_1, dimmedColor);
    fill_solid(leds2, NUM_LEDS_2, dimmedColor);
    fill_solid(leds3, NUM_LEDS_3, dimmedColor);
    FastLED.show();
    delay(20);
  }
  
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  fill_solid(leds3, NUM_LEDS_3, CRGB::Black);
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
  doc["numLeds"] = TOTAL_LEDS;
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
  doc["numLeds"] = TOTAL_LEDS;
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
