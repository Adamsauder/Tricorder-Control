/*
 * Polyinoculator Control Firmware
 * Seeed Studio XIAO ESP32-C3 based prop controller
 * Multi-strip WS2812B L  Serial.printf("  Serial.printf("Device: %s (%s)\n", deviceLabel.c_str(), deviceId.c_str());
  Serial.printf("Multi-strip configuration: Strip1=%d LEDs, Strip2=%d LEDs, Strip3=%d LEDs\n", 
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("Pin assignments: D10=%d LEDs, D3=%d LEDs, D4=%d LEDs\n",
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("WiFi: %s / %s\n", wifiSSID.c_str(), wifiPassword.c_str());: %s (%  // Test Strip 1 - Red
  Se  // Test Strip 3 - Blue
  Serial.println("Testing Strip 3 (D4/GPIO6) - BLUE");al.println("Testing Strip 1 (D10/GPIO18) - RED");
  fill_solid(leds1, NUM_LEDS_1, CRGB::Red);
  FastLED.show();
  delay(2000);
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  FastLED.show();
  delay(500);
  
  // Test Strip 2 - Green  
  Serial.println("Testing Strip 2 (D3/GPIO21) - GREEN");
  fill_solid(leds2, NUM_LEDS_2, CRGB::Green);eLabel.c_str(), deviceId.c_str());
  Serial.printf("Multi-strip configuration: Strip1=%d LEDs, Strip2=%d LEDs, Strip3=%d LEDs\n", 
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("Pin assignments: D10=%d LEDs, D3=%d LEDs, D4=%d LEDs\n",
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("WiFi: %s / %s\n", wifiSSID.c_str(), wifiPassword.c_str());ntrolled via SACN (E1.31) protocol
 * Enhanced with persistent configuration storage
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <WebServer.h>
#include "PropConfig.h"

// Pin definitions for Seeed Studio XIAO ESP32-C3 - Multi-strip configuration
// Note: Using digital pin numbers (D10, D3, D4) - preserves USB-JTAG functionality
#define LED_PIN_1 D10      // Strip 1: 14 pixels on D10 (GPIO18 - USB-JTAG but can be overridden)
#define LED_PIN_2 D3       // Strip 2: 8 pixels on D3 (GPIO21)
#define LED_PIN_3 D4       // Strip 3: 8 pixels on D4 (GPIO6)
#define NUM_LEDS_1 14      // Strip 1 length (red strip)
#define NUM_LEDS_2 8       // Strip 2 length (green strip)
#define NUM_LEDS_3 8       // Strip 3 length
#define TOTAL_LEDS 30      // Total: 14 + 8 + 8 = 30 pixels
#define STATUS_LED_PIN 3   // Optional status LED pin

// Network configuration - loaded from persistent storage
PropConfig propConfig;
PropConfig::Config config;

// Network defaults (overridden by stored config)
const int UDP_PORT = 8888;
const int WEB_PORT = 80;
const int SACN_PORT = 5568;     // sACN E1.31 standard port

// sACN E1.31 Constants
#define ACN_PACKET_IDENTIFIER "ASC-E1.17\0\0\0"
#define E131_PACKET_SIZE 638
#define E131_DATA_OFFSET 126
#define E131_UNIVERSE_OFFSET 113

// Hardware configuration - now configurable
String deviceId;
String deviceLabel;
String firmwareVersion = "Enhanced Polyinoculator v2.0";
int sacnUniverse;
int sacnStartAddress;
int totalLEDs;
int fixtureNumber;
String wifiSSID;
String wifiPassword;

// Hardware objects - Separate arrays for each strip
CRGB leds1[NUM_LEDS_1];  // Strip 1: GPIO5, 7 LEDs
CRGB leds2[NUM_LEDS_2];  // Strip 2: GPIO6, 4 LEDs
CRGB leds3[NUM_LEDS_3];  // Strip 3: GPIO7, 4 LEDs
WiFiUDP udp;
WebServer webServer(80);

// State variables
bool wifiConnected = false;
CRGB currentColor = CRGB::Black;
uint8_t ledBrightness = 128;
bool sacnEnabled = true;
WiFiUDP sacnUdp;  // Separate UDP socket for sACN

// sACN State Variables
unsigned long lastSacnPacket = 0;
uint8_t lastSacnData[512] = {0};  // Store last received DMX data
bool sacnActive = false;  // True when receiving sACN data
uint8_t sacnSequence = 0;  // Track sACN sequence numbers
bool sacnPriority = false;  // True when sACN should override UDP LED commands

// Timing variables
unsigned long lastStatusSend = 0;
const unsigned long STATUS_INTERVAL = 10000; // Send status every 10 seconds

// Function declarations
void loadConfiguration();
void setupWebServer();
void handleGetConfig();
void handleSetConfig();
void handleFactoryReset();
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

// sACN E1.31 Functions
void initializeSACN();
void handleSACNPackets();
bool processSACNPacket(uint8_t* packet, size_t length);
void updateLEDsFromDMX(uint8_t* dmxData);
void setSACNPriority(bool enabled);
String getMulticastAddress(int universe);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Enhanced Polyinoculator Control System...");
  
  // Initialize configuration system first
  if (!propConfig.begin()) {
    Serial.println("ERROR: Failed to initialize configuration storage!");
    return;
  }
  
  // Load configuration (with WiFi credentials)
  loadConfiguration();
  
  Serial.printf("Device: %s (%s)\n", deviceLabel.c_str(), deviceId.c_str());
  Serial.printf("Multi-strip configuration: Strip1=%d LEDs, Strip2=%d LEDs, Strip3=%d LEDs\n", 
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("Pin assignments: D10=%d LEDs, D3=%d LEDs, D4=%d LEDs\n",
                NUM_LEDS_1, NUM_LEDS_2, NUM_LEDS_3);
  Serial.printf("WiFi: %s / %s\n", wifiSSID.c_str(), wifiPassword.c_str());
  
  // Initialize LED strips - Multi-pin configuration with alternate color orders
  // Note: Some WS2812 strips use RGB instead of GRB color order
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds1, NUM_LEDS_1);  // Strip 1: D10 (GPIO18), 14 LEDs
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds2, NUM_LEDS_2);  // Strip 2: D3 (GPIO21), 8 LEDs
  FastLED.addLeds<WS2812B, LED_PIN_3, GRB>(leds3, NUM_LEDS_3);  // Strip 3: D4 (GPIO6), 8 LEDs
  
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
  Serial.println("Testing Strip 1 (D10/GPIO18) - RED - 14 LEDs");
  fill_solid(leds1, NUM_LEDS_1, CRGB::Red);
  FastLED.show();
  delay(2000);
  fill_solid(leds1, NUM_LEDS_1, CRGB::Black);
  FastLED.show();
  delay(500);
  
  // Test Strip 2 - Green  
  Serial.println("Testing Strip 2 (D3/GPIO21) - GREEN - 8 LEDs");
  fill_solid(leds2, NUM_LEDS_2, CRGB::Green);
  FastLED.show();
  delay(2000);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  FastLED.show();
  delay(500);
  
  // Test Strip 3 - Blue
  Serial.println("Testing Strip 3 (D4/GPIO6) - BLUE - 8 LEDs");
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
  
  // Initialize WiFi using stored credentials
  Serial.printf("Connecting to WiFi: %s\n", wifiSSID.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
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
    
    // Setup web server for configuration API
    setupWebServer();
    
    // Initialize UDP for control commands
    udp.begin(UDP_PORT);
    Serial.printf("UDP server listening on port %d\n", UDP_PORT);
    
    // Initialize sACN receiver
    initializeSACN();
    
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

// ============================================================================
// sACN E1.31 Implementation
// ============================================================================

// Initialize sACN receiver
void initializeSACN() {
  if (!sacnEnabled) {
    Serial.println("sACN disabled in configuration");
    return;
  }
  
  Serial.printf("Initializing sACN: Universe %d, Address %d\n", sacnUniverse, sacnStartAddress);
  
  // Calculate multicast address for our universe
  String multicastAddr = getMulticastAddress(sacnUniverse);
  IPAddress multicastIP;
  if (!multicastIP.fromString(multicastAddr)) {
    Serial.printf("❌ Invalid multicast address: %s\n", multicastAddr.c_str());
    return;
  }
  
  // Start sACN UDP socket
  if (sacnUdp.beginMulticast(multicastIP, SACN_PORT)) {
    Serial.printf("✅ sACN receiver started: %s:%d\n", multicastAddr.c_str(), SACN_PORT);
  } else {
    Serial.println("❌ Failed to start sACN receiver");
    sacnEnabled = false;
  }
}

// Handle incoming sACN packets
void handleSACNPackets() {
  if (!sacnEnabled || !wifiConnected) return;
  
  int packetSize = sacnUdp.parsePacket();
  if (packetSize > 0) {
    uint8_t packet[E131_PACKET_SIZE];
    int bytesRead = sacnUdp.read(packet, min(packetSize, E131_PACKET_SIZE));
    
    if (bytesRead > 0) {
      if (processSACNPacket(packet, bytesRead)) {
        lastSacnPacket = millis();
        sacnActive = true;
        sacnPriority = true;  // Enable sACN priority when receiving data
      }
    }
  }
  
  // Disable sACN priority if no packets received for 30 seconds (consistent with tricorder)
  if (sacnActive && (millis() - lastSacnPacket > 30000)) {
    sacnActive = false;
    sacnPriority = false;
    Serial.println("sACN timeout (30s) - switching to UDP control");
  }
}

// Process received sACN E1.31 packet
bool processSACNPacket(uint8_t* packet, size_t length) {
  // Validate minimum packet size
  if (length < E131_DATA_OFFSET) {
    return false;
  }
  
  // Check ACN packet identifier
  if (memcmp(packet + 4, ACN_PACKET_IDENTIFIER, 12) != 0) {
    return false;
  }
  
  // Extract universe (bytes 113-114, big endian)
  uint16_t universe = (packet[E131_UNIVERSE_OFFSET] << 8) | packet[E131_UNIVERSE_OFFSET + 1];
  
  // Check if this packet is for our universe
  if (universe != sacnUniverse) {
    return false;
  }
  
  // Extract sequence number for duplicate detection
  uint8_t sequence = packet[111];
  
  // Simple sequence checking (handles wrap-around)
  if (sequence != sacnSequence + 1 && sequence != 0) {
    // Packet out of order or duplicate - still process but note it
    // In production, you might want more sophisticated duplicate detection
  }
  sacnSequence = sequence;
  
  // Extract DMX data (starts at byte 126)
  uint8_t* dmxData = packet + E131_DATA_OFFSET;
  size_t dmxLength = length - E131_DATA_OFFSET;
  
  // Copy DMX data and update LEDs
  if (dmxLength >= sacnStartAddress + (TOTAL_LEDS * 3)) {  // RGB = 3 channels per LED
    memcpy(lastSacnData, dmxData, min(dmxLength, (size_t)512));
    updateLEDsFromDMX(dmxData);
    return true;
  }
  
  return false;
}

// Update LEDs based on DMX data
void updateLEDsFromDMX(uint8_t* dmxData) {
  if (!sacnEnabled || !sacnPriority) return;
  
  // Calculate starting index in DMX data (DMX is 1-based, arrays are 0-based)
  int dmxIndex = sacnStartAddress - 1;
  
  // Update LEDs for each strip (RGB: 3 channels per LED)
  // Strip 1: 7 LEDs
  for (int i = 0; i < NUM_LEDS_1; i++) {
    int r = dmxData[dmxIndex + (i * 3) + 0];
    int g = dmxData[dmxIndex + (i * 3) + 1];
    int b = dmxData[dmxIndex + (i * 3) + 2];
    leds1[i] = CRGB(r, g, b);
  }
  dmxIndex += NUM_LEDS_1 * 3;
  
  // Strip 2: 4 LEDs
  for (int i = 0; i < NUM_LEDS_2; i++) {
    int r = dmxData[dmxIndex + (i * 3) + 0];
    int g = dmxData[dmxIndex + (i * 3) + 1];
    int b = dmxData[dmxIndex + (i * 3) + 2];
    leds2[i] = CRGB(r, g, b);
  }
  dmxIndex += NUM_LEDS_2 * 3;
  
  // Strip 3: 4 LEDs
  for (int i = 0; i < NUM_LEDS_3; i++) {
    int r = dmxData[dmxIndex + (i * 3) + 0];
    int g = dmxData[dmxIndex + (i * 3) + 1];
    int b = dmxData[dmxIndex + (i * 3) + 2];
    leds3[i] = CRGB(r, g, b);
  }
  
  // Apply brightness and show
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

// Set sACN priority mode
void setSACNPriority(bool enabled) {
  sacnPriority = enabled;
  if (enabled) {
    Serial.println("sACN priority enabled - ignoring UDP LED commands");
  } else {
    Serial.println("sACN priority disabled - accepting UDP LED commands");
  }
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

void loop() {
  // Handle web server requests
  if (wifiConnected) {
    webServer.handleClient();
    
    // Handle sACN packets (high priority for lighting)
    handleSACNPackets();
  }
  
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
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP LED command - sACN active");
    return;
  }
  
  currentColor = CRGB(r, g, b);
  fill_solid(leds1, NUM_LEDS_1, currentColor);
  fill_solid(leds2, NUM_LEDS_2, currentColor);
  fill_solid(leds3, NUM_LEDS_3, currentColor);
  FastLED.show();
  Serial.printf("All LED strips set to R:%d G:%d B:%d\n", r, g, b);
}

void setStripColor(int stripNum, int r, int g, int b) {
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP strip color command - sACN active");
    return;
  }
  
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
  doc["fixtureNumber"] = fixtureNumber;
  
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
  doc["deviceLabel"] = deviceLabel;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["numLeds"] = totalLEDs;
  doc["brightness"] = ledBrightness;
  doc["sacnEnabled"] = sacnEnabled;
  doc["sacnUniverse"] = sacnUniverse;
  doc["dmxStartAddress"] = sacnStartAddress;
  doc["fixtureNumber"] = fixtureNumber;
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

// Configuration management functions
void loadConfiguration() {
  if (propConfig.loadConfig(config)) {
    deviceId = config.deviceLabel.substring(0, config.deviceLabel.indexOf('_')) + "_" + String(random(1000, 9999));
    deviceLabel = config.deviceLabel;
    sacnUniverse = config.sacnUniverse;
    sacnStartAddress = config.dmxStartAddress;
    totalLEDs = config.numLeds;
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
    // Set defaults
    deviceId = "POLYINOCULATOR_" + String(random(1000, 9999));
    deviceLabel = "Polyinoculator " + String(random(100, 999));
    sacnUniverse = 1;
    sacnStartAddress = 1;
    totalLEDs = TOTAL_LEDS;
    fixtureNumber = 2;  // Default to 2 for polyinoculators
    ledBrightness = 128;
    wifiSSID = "Rigging Electric";
    wifiPassword = "academy123";
    
    // Save defaults
    config.deviceLabel = deviceLabel;
    config.sacnUniverse = sacnUniverse;
    config.dmxStartAddress = sacnStartAddress;
    config.numLeds = totalLEDs;
    config.brightness = ledBrightness;
    config.wifiSSID = wifiSSID;
    config.wifiPassword = wifiPassword;
    config.deviceType = "polyinoculator";
    config.fixtureNumber = fixtureNumber;
    config.firstBoot = false;
    propConfig.saveConfig(config);
  }
  
  Serial.println("Configuration loaded:");
  propConfig.printConfig();
}

void setupWebServer() {
  // Configuration API endpoints
  webServer.on("/api/config", HTTP_GET, handleGetConfig);
  webServer.on("/api/config", HTTP_POST, handleSetConfig);
  webServer.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  
  // CORS headers for web interface
  webServer.enableCORS(true);
  
  webServer.begin();
  Serial.printf("Web server started on port %d\n", WEB_PORT);
}

void handleGetConfig() {
  JsonDocument doc;
  doc["deviceId"] = deviceId;
  doc["deviceLabel"] = deviceLabel;
  doc["deviceType"] = "polyinoculator";
  doc["firmwareVersion"] = firmwareVersion;
  doc["sacnUniverse"] = sacnUniverse;
  doc["dmxStartAddress"] = sacnStartAddress;
  doc["numLeds"] = totalLEDs;
  doc["brightness"] = ledBrightness;
  doc["wifiSSID"] = wifiSSID;
  doc["online"] = true;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["uptime"] = millis();
  doc["freeHeap"] = ESP.getFreeHeap();
  
  String response;
  serializeJson(doc, response);
  webServer.send(200, "application/json", response);
}

void handleSetConfig() {
  if (!webServer.hasArg("plain")) {
    webServer.send(400, "application/json", "{\"error\":\"No JSON data\"}");
    return;
  }
  
  String body = webServer.arg("plain");
  JsonDocument doc;
  
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
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
      FastLED.setBrightness(ledBrightness);
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
    Serial.println("Configuration updated via API");
    propConfig.printConfig();
    webServer.send(200, "application/json", "{\"status\":\"updated\"}");
  } else {
    webServer.send(200, "application/json", "{\"status\":\"no_changes\"}");
  }
}

void handleFactoryReset() {
  Serial.println("Factory reset requested via API");
  
  if (propConfig.factoryReset()) {
    webServer.send(200, "application/json", "{\"status\":\"reset_scheduled\"}");
    delay(1000);
    ESP.restart();
  } else {
    webServer.send(500, "application/json", "{\"error\":\"reset_failed\"}");
  }
}
