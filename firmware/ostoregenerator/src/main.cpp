/*
 * Ostoregenerator Firmware - sACN Compatible Single LED Device
 * ESP32-C3 XIAO based ostoregenerator with single WS2812B LED control and web configuration
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Update.h>
#include "PropConfig.h"

// Pin definitions for ESP32-C3 XIAO
#define LED_PIN D3    // D3 (GPIO5) - Single WS2812B LED
#define NUM_LEDS 1    // Single LED

// LED configuration for WS2812B RGB LED
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Network settings
#define UDP_PORT 8888      // Port for UDP status broadcasts (matches server)
#define SACN_PORT 5568     // sACN E1.31 standard port
#define SACN_MULTICAST_BASE "239.255.0.0"  // sACN multicast base address

// sACN E1.31 Constants - Correct E1.31 packet identifier
static const uint8_t ACN_PACKET_IDENTIFIER[12] = {0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00}; // "ASC-E1.17"
#define E131_PACKET_SIZE 638
#define E131_PREAMBLE_OFFSET 4  // E1.31 packets start with 4-byte preamble
#define E131_DATA_OFFSET 126
#define E131_UNIVERSE_OFFSET 113

// System state
bool deviceActive = false;  // false=idle, true=active
uint8_t currentR = 0, currentG = 0, currentB = 0;  // Current LED color
unsigned long lastActivity = 0;
unsigned long lastStatusBroadcast = 0;

// Configuration system
PropConfig propConfig;
PropConfig::Config config;

// Device configuration variables - loaded from persistent storage
String deviceId;  // Will be generated uniquely from MAC address
String deviceLabel = "IV Injector 001";
String deviceType = "iv_injector";
String firmwareVersion = "Ostoregenerator v1.0";
int sacnUniverse = 1;
int sacnStartAddress = 1;
int ledBrightness = 128;
int fixtureNumber = 4;

// WiFi settings - loaded from configuration
String wifiSSID = "Rigging Electric";
String wifiPassword = "academy123";

// Network instances
WiFiUDP udp;       // Main UDP socket for status broadcasts
WiFiUDP sacnUdp;   // Separate UDP socket for sACN
AsyncWebServer server(80);

// Response tracking for command responses
IPAddress currentSenderIP;
uint16_t currentSenderPort;

// sACN State Variables
bool sacnEnabled = true;
unsigned long lastSacnPacket = 0;
uint8_t lastSacnData[512] = {0};  // Store last received DMX data
bool sacnActive = false;  // True when receiving sACN data
uint8_t sacnSequence = 0;  // Track sACN sequence numbers
bool sacnPriority = false;  // True when sACN should override UDP LED commands

// Function declarations
void loadConfiguration();
void initializeWiFi();
void setupWebServer();
void sendPeriodicStatus();
void handleUDPCommands();
void handleSACNData();
void initializeSACN();
void processSACNPacket(uint8_t* buffer, size_t length);
void setLEDFromSACN();
void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
void setLEDPattern(String pattern);
void indicateActivity();
void handleGetConfig(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);
void handleOTAUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void handleOTAUpdate(String firmwareUrl, String commandId);
void sendResponse(String commandId, String result);

// Generate unique device ID based on ESP32 MAC address
String generateUniqueDeviceId() {
  // Get the ESP32 chip ID (full 48-bit MAC address)
  uint64_t chipid = ESP.getEfuseMac();
  
  // Debug output
  Serial.printf("Full chip ID: 0x%012llX\n", chipid);
  
  // Use multiple parts of the MAC address for better uniqueness
  uint32_t low32 = (uint32_t)chipid;           // Lower 32 bits
  uint32_t high16 = (uint32_t)(chipid >> 32);  // Upper 16 bits
  
  // Combine different parts and apply additional mixing
  uint32_t mixed1 = low32 ^ (high16 << 16);    // XOR with shifted high bits
  uint32_t mixed2 = (low32 >> 8) ^ (high16 << 8); // Different shift pattern
  uint16_t uniquePart = (uint16_t)(mixed1 ^ mixed2); // Final XOR
  
  Serial.printf("Low32: 0x%08X, High16: 0x%04X, Mixed: 0x%04X\n", low32, high16, uniquePart);
  
  char uniqueId[16];
  snprintf(uniqueId, sizeof(uniqueId), "OSTEO%04X", uniquePart);
  
  Serial.printf("Generated device ID: %s\n", uniqueId);
  
  return String(uniqueId);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("IV Injector Control System Starting...");
  Serial.println("Hardware: ESP32-C3 XIAO with Single WS2812B LED");
  Serial.printf("Firmware: %s\n", firmwareVersion.c_str());
  
  // Initialize configuration system first
  if (!propConfig.begin()) {
    Serial.println("ERROR: Failed to initialize configuration storage!");
    return;
  }
  
  // Generate unique device ID based on MAC address
  deviceId = generateUniqueDeviceId();
  Serial.printf("Generated unique device ID: %s\n", deviceId.c_str());
  
  // Load configuration (with WiFi credentials)
  loadConfiguration();
  
  Serial.printf("Device: %s (%s)\n", deviceLabel.c_str(), deviceId.c_str());
  Serial.printf("Pin assignments: LED=D3(GPIO%d)\n", LED_PIN);
  Serial.printf("Configuration: sACN Universe=%d, DMX Address=%d, Brightness=%d\n",
                sacnUniverse, sacnStartAddress, ledBrightness);
  Serial.printf("WiFi: %s\n", wifiSSID.c_str());
  
  // Initialize single WS2812B LED
  Serial.println("Initializing WS2812B LED...");
  strip.begin();
  strip.setBrightness(ledBrightness);
  strip.clear();
  strip.show();
  
  Serial.println("LED initialization complete - proceeding to WiFi setup");
  
  // Initialize WiFi
  if (wifiSSID.length() > 0) {
    initializeWiFi();
    
    // Setup web server and sACN after WiFi connection
    if (WiFi.status() == WL_CONNECTED) {
      setupWebServer();
      initializeSACN();
    }
  } else {
    Serial.println("No WiFi credentials configured - running in standalone mode");
  }
  
  Serial.println("IV Injector initialization complete");
  Serial.println("Ready for sACN and UDP commands");
  
  // Idle state - dim green
  setLEDColor(0, 64, 0);  // Dim green for idle state
  lastActivity = millis();
}

void loop() {
  // Handle network communication
  if (WiFi.status() == WL_CONNECTED) {
    handleUDPCommands();
    handleSACNData();
    
    // Send periodic status broadcasts
    if (millis() - lastStatusBroadcast > 5000) {
      sendPeriodicStatus();
      lastStatusBroadcast = millis();
    }
  }
  
  // Auto-return to idle after 30 seconds of no activity
  if (deviceActive && (millis() - lastActivity > 30000)) {
    Serial.println("Auto-return to idle state");
    deviceActive = false;
    setLEDColor(0, 64, 0);  // Dim green for idle
  }
  
  delay(10); // Small delay for stability
}

void loadConfiguration() {
  // Load persistent configuration
  if (propConfig.loadConfig(config)) {
    // Only use stored device ID if it's not the old hardcoded format
    if (!config.deviceLabel.isEmpty() && !config.deviceLabel.startsWith("IV_INJECTOR_")) {
      deviceId = config.deviceLabel;
    }
    // Use the stored device ID as the label, or the new unique ID if regenerated
    deviceLabel = deviceId;
    deviceType = config.deviceType;
    sacnUniverse = config.sacnUniverse;
    sacnStartAddress = config.dmxStartAddress;
    ledBrightness = config.brightness;
    wifiSSID = config.wifiSSID;
    wifiPassword = config.wifiPassword;
    fixtureNumber = config.fixtureNumber;
    
    Serial.println("Configuration loaded from storage");
    
    // If we regenerated the device ID (old format detected), save the new one
    if (config.deviceLabel != deviceId) {
      Serial.println("Device ID was regenerated, saving new configuration...");
      config.deviceLabel = deviceId;
      propConfig.saveConfig(config);
    }
  } else {
    Serial.println("Using default configuration");
    // Save defaults
    config.deviceLabel = deviceId;
    config.deviceType = deviceType;
    config.sacnUniverse = sacnUniverse;
    config.dmxStartAddress = sacnStartAddress;
    config.numLeds = NUM_LEDS;
    config.brightness = ledBrightness;
    config.wifiSSID = wifiSSID;
    config.wifiPassword = wifiPassword;
    config.fixtureNumber = fixtureNumber;
    config.firstBoot = false;
    propConfig.saveConfig(config);
  }
  
  // Update LED brightness
  strip.setBrightness(ledBrightness);
}

void initializeWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", wifiSSID.c_str());
  
  WiFi.mode(WIFI_STA);
  
  // Check if using static IP configuration
  if (!propConfig.getUseDHCP()) {
    IPAddress staticIP, gateway, subnet, dns;
    staticIP.fromString(propConfig.getStaticIP());
    gateway.fromString(propConfig.getStaticGateway());
    subnet.fromString(propConfig.getStaticSubnet());
    dns.fromString(propConfig.getStaticDNS());
    
    Serial.printf("Using static IP configuration: %s\n", staticIP.toString().c_str());
    Serial.printf("Gateway: %s, Subnet: %s, DNS: %s\n", 
                  gateway.toString().c_str(), 
                  subnet.toString().c_str(), 
                  dns.toString().c_str());
    
    if (!WiFi.config(staticIP, gateway, subnet, dns)) {
      Serial.println("Warning: Failed to configure static IP, falling back to DHCP");
    }
  } else {
    Serial.println("Using DHCP for IP configuration");
  }
  
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Blink LED during connection attempts
    strip.setPixelColor(0, strip.Color(0, 0, 255));  // Blue
    strip.show();
    delay(100);
    strip.clear();
    strip.show();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    // Setup mDNS
    String hostname = deviceId;
    hostname.toLowerCase();
    hostname.replace("_", "-");
    if (MDNS.begin(hostname.c_str())) {
      Serial.printf("mDNS responder started: %s.local\n", hostname.c_str());
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("iv-injector", "udp", UDP_PORT);
    }
    
    // Initialize UDP for status broadcasts
    if (udp.begin(UDP_PORT)) {
      Serial.printf("UDP listener started on port %d\n", UDP_PORT);
    }
    
    // Green flash for successful connection
    for (int i = 0; i < 2; i++) {
      strip.setPixelColor(0, strip.Color(0, 255, 0));  // Green
      strip.show();
      delay(100);
      strip.clear();
      strip.show();
      delay(100);
    }
  } else {
    Serial.println("\nWiFi connection failed!");
    // Red flash for failed connection
    for (int i = 0; i < 5; i++) {
      strip.setPixelColor(0, strip.Color(255, 0, 0));  // Red
      strip.show();
      delay(200);
      strip.clear();
      strip.show();
      delay(200);
    }
  }
}

void setupWebServer() {
  // CORS headers for all responses
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  // Serve main web interface
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"html(
<!DOCTYPE html>
<html>
<head>
    <title>IV Injector Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; }
        .controls { display: grid; gap: 10px; }
        .btn { padding: 15px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .btn-primary { background: #007bff; color: white; }
        .btn-success { background: #28a745; color: white; }
        .btn-warning { background: #ffc107; color: black; }
        .btn-danger { background: #dc3545; color: white; }
        .config { background: #f8f9fa; padding: 15px; border-radius: 5px; margin: 10px 0; }
        input, select { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #ddd; border-radius: 3px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🩸 IV Injector Control</h1>
            <p>ESP32-C3 Single LED Controller</p>
        </div>
        
        <div class="status" id="status">
            <strong>Status:</strong> <span id="statusText">Loading...</span><br>
            <strong>LED Color:</strong> <span id="ledColor">Unknown</span><br>
            <strong>IP Address:</strong> <span id="ipAddress">)html" + WiFi.localIP().toString() + R"html(</span>
        </div>
        
        <div class="controls">
            <button class="btn btn-success" onclick="setColor(255, 0, 0)">🔴 Red (Active)</button>
            <button class="btn btn-success" onclick="setColor(0, 255, 0)">🟢 Green (Ready)</button>
            <button class="btn btn-success" onclick="setColor(0, 0, 255)">🔵 Blue (Standby)</button>
            <button class="btn btn-success" onclick="setColor(255, 255, 0)">🟡 Yellow (Warning)</button>
            <button class="btn btn-success" onclick="setColor(255, 255, 255)">⚪ White (Bright)</button>
            <button class="btn btn-warning" onclick="setColor(0, 0, 0)">⚫ Off</button>
        </div>
        
        <div class="config">
            <h3>Configuration</h3>
            <label>Device Label:</label>
            <input type="text" id="deviceLabel" value="IV_INJECTOR_001">
            
            <label>sACN Universe:</label>
            <input type="number" id="sacnUniverse" value="1" min="1" max="63999">
            
            <label>DMX Start Address:</label>
            <input type="number" id="dmxAddress" value="1" min="1" max="512">
            
            <label>LED Brightness:</label>
            <input type="range" id="brightness" min="0" max="255" value="128" oninput="document.getElementById('brightnessValue').textContent=this.value">
            <span id="brightnessValue">128</span>
            
            <button class="btn btn-primary" onclick="saveConfig()">💾 Save Configuration</button>
            <button class="btn btn-danger" onclick="factoryReset()">🔄 Factory Reset</button>
        </div>
    </div>
    
    <script>
        function setColor(r, g, b) {
            fetch('/led', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ r: r, g: g, b: b })
            }).then(() => updateStatus());
        }
        
        function saveConfig() {
            const config = {
                deviceLabel: document.getElementById('deviceLabel').value,
                sacnUniverse: parseInt(document.getElementById('sacnUniverse').value),
                dmxStartAddress: parseInt(document.getElementById('dmxAddress').value),
                brightness: parseInt(document.getElementById('brightness').value)
            };
            
            fetch('/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            }).then(() => alert('Configuration saved!'));
        }
        
        function factoryReset() {
            if (confirm('Are you sure you want to reset to factory defaults?')) {
                fetch('/factory-reset', { method: 'POST' })
                .then(() => alert('Factory reset complete! Device will restart.'));
            }
        }
        
        function updateStatus() {
            fetch('/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById('statusText').textContent = data.active ? 'Active' : 'Idle';
                document.getElementById('ledColor').textContent = `RGB(${data.r}, ${data.g}, ${data.b})`;
            });
        }
        
        // Load initial configuration
        fetch('/config')
        .then(response => response.json())
        .then(config => {
            document.getElementById('deviceLabel').value = config.deviceLabel || 'IV_INJECTOR_001';
            document.getElementById('sacnUniverse').value = config.sacnUniverse || 1;
            document.getElementById('dmxAddress').value = config.dmxStartAddress || 1;
            document.getElementById('brightness').value = config.brightness || 128;
            document.getElementById('brightnessValue').textContent = config.brightness || 128;
        });
        
        // Update status periodically
        updateStatus();
        setInterval(updateStatus, 5000);
    </script>
</body>
</html>
)html";
    request->send(200, "text/html", html);
  });
  
  // API endpoints
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["device_id"] = deviceId;
    doc["device_type"] = deviceType;
    doc["active"] = deviceActive;
    doc["r"] = currentR;
    doc["g"] = currentG;
    doc["b"] = currentB;
    doc["brightness"] = ledBrightness;
    doc["sacn_universe"] = sacnUniverse;
    doc["dmx_address"] = sacnStartAddress;
    doc["ip_address"] = WiFi.localIP().toString();
    doc["wifi_ssid"] = wifiSSID;
    doc["firmware"] = firmwareVersion;
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("r") && request->hasParam("g") && request->hasParam("b")) {
      uint8_t r = request->getParam("r")->value().toInt();
      uint8_t g = request->getParam("g")->value().toInt();
      uint8_t b = request->getParam("b")->value().toInt();
      setLEDColor(r, g, b);
      request->send(200, "text/plain", "LED color set");
    } else {
      request->send(400, "text/plain", "Missing RGB parameters");
    }
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Handle JSON body
    if (index == 0) {
      JsonDocument doc;
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
        if (doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
          uint8_t r = doc["r"];
          uint8_t g = doc["g"];
          uint8_t b = doc["b"];
          setLEDColor(r, g, b);
          request->send(200, "text/plain", "LED color set");
          return;
        }
      }
      request->send(400, "text/plain", "Invalid JSON");
    }
  });
  
  server.on("/config", HTTP_GET, handleGetConfig);
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Configuration updated");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Handle JSON body for configuration updates
    if (index == 0) {
      JsonDocument doc;
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
        // Convert JSON to string and pass to PropConfig
        String jsonStr;
        serializeJson(doc, jsonStr);
        
        if (propConfig.fromJSON(jsonStr)) {
          // Check if network configuration changed
          bool networkChanged = doc.containsKey("useDHCP") || 
                               doc.containsKey("staticIP") || 
                               doc.containsKey("staticGateway") || 
                               doc.containsKey("staticSubnet") || 
                               doc.containsKey("staticDNS");
          
          if (networkChanged) {
            Serial.println("Network configuration changed - restarting to apply changes...");
            delay(1000);
            ESP.restart();
          }
          
          request->send(200, "text/plain", "Configuration updated successfully");
          return;
        }
      }
      request->send(400, "text/plain", "Invalid JSON or failed to save configuration");
    }
  });
  server.on("/factory-reset", HTTP_POST, handleFactoryReset);
  
  // Network configuration endpoint
  server.on("/network", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Network configuration updated - restarting...");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Handle network configuration changes
    if (index == 0) {
      JsonDocument doc;
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
        bool updated = false;
        
        if (doc.containsKey("useDHCP")) {
          propConfig.setUseDHCP(doc["useDHCP"]);
          updated = true;
        }
        
        if (doc.containsKey("staticIP")) {
          String ipStr = doc["staticIP"].as<String>();
          propConfig.setStaticIP(ipStr);
          updated = true;
        }
        
        if (doc.containsKey("staticGateway")) {
          String gwStr = doc["staticGateway"].as<String>();
          propConfig.setStaticGateway(gwStr);
          updated = true;
        }
        
        if (doc.containsKey("staticSubnet")) {
          String subnetStr = doc["staticSubnet"].as<String>();
          propConfig.setStaticSubnet(subnetStr);
          updated = true;
        }
        
        if (doc.containsKey("staticDNS")) {
          String dnsStr = doc["staticDNS"].as<String>();
          propConfig.setStaticDNS(dnsStr);
          updated = true;
        }
        
        if (updated) {
          Serial.println("Network configuration updated - restarting to apply changes...");
          request->send(200, "text/plain", "Network configuration updated - restarting...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(400, "text/plain", "No valid network configuration provided");
        }
        return;
      }
      request->send(400, "text/plain", "Invalid JSON");
    }
  });
  
  // Test endpoint for LED debugging
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("=== LED TEST ENDPOINT CALLED ===");
    
    // Force clear sACN priority for testing
    sacnPriority = false;
    sacnActive = false;
    
    // Test sequence
    Serial.println("Testing RED...");
    setLEDColor(255, 0, 0);
    delay(1000);
    
    Serial.println("Testing GREEN...");
    setLEDColor(0, 255, 0);
    delay(1000);
    
    Serial.println("Testing BLUE...");
    setLEDColor(0, 0, 255);
    delay(1000);
    
    Serial.println("Testing WHITE...");
    setLEDColor(255, 255, 255);
    delay(1000);
    
    Serial.println("Back to idle...");
    setLEDColor(0, 64, 0);  // Dim green
    
    request->send(200, "text/plain", "LED test sequence complete!");
  });
  
  // OTA update endpoint
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    request->send(response);
    ESP.restart();
  }, [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    // Handle OTA upload using lambda wrapper
    handleOTAUpdate(request, filename, index, data, len, final);
  });
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void handleUDPCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    // Store sender info for responses
    currentSenderIP = udp.remoteIP();
    currentSenderPort = udp.remotePort();
    
    char buffer[1024];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';
    
    Serial.printf("=== UDP PACKET RECEIVED ===\n");
    Serial.printf("Packet size: %d bytes\n", packetSize);
    Serial.printf("Raw data: %s\n", buffer);
    
    // Parse JSON command
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buffer);
    if (error == DeserializationError::Ok) {
      // Support both "command" (legacy) and "action" (new OTA format)
      String command = doc["command"];
      String action = doc["action"];
      String commandId = doc["commandId"];
      String target = doc["device_id"];
      
      // DEBUG: Show raw extracted values
      Serial.printf("🔍 Raw extracted values:\n");
      Serial.printf("   command: '%s' (length: %d)\n", command.c_str(), command.length());
      Serial.printf("   action: '%s' (length: %d)\n", action.c_str(), action.length());
      Serial.printf("   target: '%s'\n", target.c_str());
      
      // Use action if command is empty or null (new format)
      if ((command.length() == 0 || command == "null") && action.length() > 0) {
        Serial.printf("🔄 Using action '%s' as command\n", action.c_str());
        command = action;
      }
      
      Serial.printf("Parsed command/action: '%s', target: '%s'\n", command.c_str(), target.c_str());
      Serial.printf("Our device ID: '%s'\n", deviceId.c_str());
      
      // Check if command is for this device
      if (target == deviceId || target == "ALL" || target.startsWith("IV_INJECTOR")) {
        Serial.printf("✅ Command matches our device!\n");
        Serial.printf("Processing UDP command: %s\n", command.c_str());
        
        if (command == "led_color") {
          uint8_t r = doc["r"];
          uint8_t g = doc["g"];
          uint8_t b = doc["b"];
          Serial.printf("LED color command: RGB(%d, %d, %d)\n", r, g, b);
          setLEDColor(r, g, b);
          indicateActivity();
        }
        else if (command == "led_pattern") {
          String pattern = doc["pattern"];
          Serial.printf("LED pattern command: %s\n", pattern.c_str());
          setLEDPattern(pattern);
          indicateActivity();
        }
        else if (command == "status") {
          Serial.println("Status request received");
          sendPeriodicStatus();
        }
        else if (command == "ota_update") {
          String firmwareUrl = "";
          if (doc["parameters"].is<JsonObject>() && doc["parameters"]["firmware_url"].is<String>()) {
            firmwareUrl = doc["parameters"]["firmware_url"].as<String>();
          }
          
          if (firmwareUrl.length() > 0) {
            Serial.printf("🔄 Starting OTA update from: %s\n", firmwareUrl.c_str());
            if (commandId.length() > 0) {
              sendResponse(commandId, "OTA update started");
            }
            
            // Perform OTA update in background to avoid blocking the response
            handleOTAUpdate(firmwareUrl, commandId);
          } else {
            Serial.println("❌ OTA update failed: No firmware URL provided");
            if (commandId.length() > 0) {
              sendResponse(commandId, "OTA update failed: No firmware URL provided");
            }
          }
        }
        else {
          Serial.printf("⚠️  Unknown command: %s\n", command.c_str());
        }
        
        lastActivity = millis();
      } else {
        Serial.printf("❌ Command not for us (target: '%s')\n", target.c_str());
      }
    } else {
      Serial.printf("❌ JSON parse error: %s\n", error.c_str());
      Serial.printf("Raw data was: %s\n", buffer);
    }
    Serial.println("=== UDP PROCESSING COMPLETE ===");
  }
}

void initializeSACN() {
  if (!sacnEnabled) return;
  
  // Join sACN multicast group for our universe
  IPAddress multicastIP;
  multicastIP.fromString(String(SACN_MULTICAST_BASE));
  multicastIP[3] = sacnUniverse & 0xFF;  // Set last octet to universe number
  
  if (sacnUdp.beginMulticast(multicastIP, SACN_PORT)) {
    Serial.printf("sACN listener started on universe %d (IP: %s, Port: %d)\n", 
                  sacnUniverse, multicastIP.toString().c_str(), SACN_PORT);
  } else {
    Serial.println("Failed to start sACN listener");
    sacnEnabled = false;
  }
}

void handleSACNData() {
  if (!sacnEnabled) return;
  
  int packetSize = sacnUdp.parsePacket();
  if (packetSize > 0) {
    Serial.printf("sACN packet received: %d bytes\n", packetSize);
    
    if (packetSize >= E131_PACKET_SIZE) {
      uint8_t buffer[E131_PACKET_SIZE];
      int len = sacnUdp.read(buffer, E131_PACKET_SIZE);
      
      if (len >= E131_PACKET_SIZE) {
        processSACNPacket(buffer, len);
      }
    } else {
      // Read and discard smaller packets
      uint8_t tempBuffer[packetSize];
      sacnUdp.read(tempBuffer, packetSize);
      Serial.printf("Discarded small sACN packet: %d bytes\n", packetSize);
    }
  }
}

void processSACNPacket(uint8_t* buffer, size_t length) {
  Serial.printf("🔍 Processing packet: %d bytes\n", length);
  
  // Print first 16 bytes for debugging
  Serial.print("📦 Header bytes: ");
  for (int i = 0; i < 16 && i < length; i++) {
    Serial.printf("%02X ", buffer[i]);
  }
  Serial.println();
  
  // Print expected identifier starting after preamble
  Serial.print("🎯 Expected at offset 4: ");
  for (int i = 0; i < 12; i++) {
    Serial.printf("%02X ", ACN_PACKET_IDENTIFIER[i]);
  }
  Serial.println();
  
  // Verify sACN packet header - skip 4-byte preamble
  if (memcmp(buffer + E131_PREAMBLE_OFFSET, ACN_PACKET_IDENTIFIER, 12) != 0) {
    Serial.println("❌ Invalid sACN packet header");
    return; // Not a valid sACN packet
  }
  
  Serial.println("✅ Valid sACN packet header!");
  
  // Extract universe number
  uint16_t universe = (buffer[E131_UNIVERSE_OFFSET] << 8) | buffer[E131_UNIVERSE_OFFSET + 1];
  Serial.printf("sACN packet for universe %d (we want %d)\n", universe, sacnUniverse);
  
  if (universe != sacnUniverse) {
    return; // Not our universe
  }
  
  // Extract sequence number
  uint8_t sequence = buffer[111];
  
  // Simple sequence checking (ignoring wrap-around for now)
  if (sequence != 0 && sacnSequence != 0 && sequence <= sacnSequence) {
    Serial.printf("Old/duplicate packet: seq %d <= %d\n", sequence, sacnSequence);
    return; // Old or duplicate packet
  }
  sacnSequence = sequence;
  
  Serial.printf("Processing sACN universe %d, sequence %d\n", universe, sequence);
  
  // Copy DMX data
  memcpy(lastSacnData, &buffer[E131_DATA_OFFSET], 512);
  
  // Update status
  lastSacnPacket = millis();
  sacnActive = true;
  sacnPriority = true;  // sACN takes priority over UDP commands
  
  // Update LED from sACN data
  setLEDFromSACN();
  
  Serial.printf("sACN packet received - Universe: %d, Sequence: %d\n", universe, sequence);
}

void setLEDFromSACN() {
  if (!sacnActive || sacnStartAddress < 1 || sacnStartAddress > 509) {
    return;
  }
  
  // Single LED uses 3 channels: R, G, B
  uint8_t r = lastSacnData[sacnStartAddress - 1];     // DMX is 1-based, array is 0-based
  uint8_t g = lastSacnData[sacnStartAddress];
  uint8_t b = lastSacnData[sacnStartAddress + 1];
  
  Serial.printf("=== setLEDFromSACN: RGB(%d, %d, %d) ===\n", r, g, b);
  
  // Direct LED update from sACN - bypass priority check
  currentR = r;
  currentG = g;
  currentB = b;
  
  Serial.printf("sACN setting LED to RGB(%d, %d, %d)...\n", r, g, b);
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  Serial.println("✅ sACN LED update complete!");
  
  lastActivity = millis();
  
  // Check for sACN timeout (no data for 2.5 seconds)
  if (millis() - lastSacnPacket > 2500) {
    sacnActive = false;
    sacnPriority = false;
    Serial.println("sACN timeout - returning to UDP control");
  }
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  Serial.printf("=== setLEDColor called: RGB(%d, %d, %d) ===\n", r, g, b);
  Serial.printf("sACN Priority: %s, sACN Active: %s\n", 
                sacnPriority ? "TRUE" : "FALSE", 
                sacnActive ? "TRUE" : "FALSE");
  
  // Don't update if sACN has priority and this is not from sACN
  if (sacnPriority && sacnActive) {
    Serial.println("⚠️  LED update blocked by sACN priority!");
    return;
  }
  
  currentR = r;
  currentG = g;
  currentB = b;
  
  Serial.printf("Setting LED to RGB(%d, %d, %d)...\n", r, g, b);
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  Serial.println("✅ LED update complete!");
  
  lastActivity = millis();
}

void setLEDPattern(String pattern) {
  if (pattern == "off") {
    setLEDColor(0, 0, 0);
  }
  else if (pattern == "red") {
    setLEDColor(255, 0, 0);
  }
  else if (pattern == "green") {
    setLEDColor(0, 255, 0);
  }
  else if (pattern == "blue") {
    setLEDColor(0, 0, 255);
  }
  else if (pattern == "white") {
    setLEDColor(255, 255, 255);
  }
  else if (pattern == "yellow") {
    setLEDColor(255, 255, 0);
  }
  else if (pattern == "active") {
    setLEDColor(255, 0, 0);  // Red for active
    deviceActive = true;
  }
  else if (pattern == "idle") {
    setLEDColor(0, 64, 0);   // Dim green for idle
    deviceActive = false;
  }
}

void indicateActivity() {
  // Brief flash to indicate command received
  strip.setPixelColor(0, strip.Color(255, 255, 255));  // White flash
  strip.show();
  delay(50);
  strip.setPixelColor(0, strip.Color(currentR, currentG, currentB));  // Back to current color
  strip.show();
}

void sendPeriodicStatus() {
  JsonDocument doc;
  doc["device_id"] = deviceId;
  doc["device_type"] = deviceType;
  doc["firmware_version"] = firmwareVersion;
  doc["ip_address"] = WiFi.localIP().toString();
  doc["online"] = true;
  doc["active"] = deviceActive;
  doc["led_color"] = String(currentR) + "," + String(currentG) + "," + String(currentB);
  doc["brightness"] = ledBrightness;
  doc["sacn_universe"] = sacnUniverse;
  doc["dmx_address"] = sacnStartAddress;
  doc["sacn_active"] = sacnActive;
  doc["uptime"] = millis() / 1000;
  doc["wifi_ssid"] = wifiSSID;
  doc["last_activity"] = (millis() - lastActivity) / 1000;
  
  String status;
  serializeJson(doc, status);
  
  // Broadcast status
  udp.beginPacket("255.255.255.255", UDP_PORT);
  udp.print(status);
  udp.endPacket();
  
  Serial.printf("Status broadcast sent: %s\n", status.c_str());
}

void handleGetConfig(AsyncWebServerRequest *request) {
  String json = propConfig.toJSON();
  request->send(200, "application/json", json);
}

void handleFactoryReset(AsyncWebServerRequest *request) {
  Serial.println("Factory reset requested");
  
  if (propConfig.factoryReset()) {
    request->send(200, "text/plain", "Factory reset complete - restarting...");
    delay(1000);
    ESP.restart();
  } else {
    request->send(500, "text/plain", "Factory reset failed");
  }
}

void handleOTAUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
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
}

void handleOTAUpdate(String firmwareUrl, String commandId) {
  Serial.printf("🔄 Starting remote OTA update from: %s\n", firmwareUrl.c_str());
  
  HTTPClient http;
  http.begin(firmwareUrl);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);
    
    if (canBegin) {
      WiFiClient * client = http.getStreamPtr();
      size_t written = Update.writeStream(*client);
      
      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
      }
      
      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          if (commandId.length() > 0) {
            sendResponse(commandId, "OTA update completed successfully - rebooting");
          }
          delay(1000);
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
          if (commandId.length() > 0) {
            sendResponse(commandId, "OTA update failed - update not finished");
          }
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        if (commandId.length() > 0) {
          sendResponse(commandId, "OTA update failed - Error #: " + String(Update.getError()));
        }
      }
    } else {
      Serial.println("Not enough space to begin OTA");
      if (commandId.length() > 0) {
        sendResponse(commandId, "OTA update failed - not enough space");
      }
    }
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    if (commandId.length() > 0) {
      sendResponse(commandId, "OTA update failed - HTTP GET failed: " + String(httpCode));
    }
  }
  
  http.end();
}

void sendResponse(String commandId, String result) {
  JsonDocument doc;
  doc["commandId"] = commandId;
  doc["result"] = result;
  doc["timestamp"] = millis();
  doc["deviceId"] = deviceId;
  
  String response;
  serializeJson(doc, response);
  
  udp.beginPacket(currentSenderIP, currentSenderPort);
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
  
  Serial.printf("Sent response to %s:%d - %s\n", currentSenderIP.toString().c_str(), currentSenderPort, response.c_str());
}
