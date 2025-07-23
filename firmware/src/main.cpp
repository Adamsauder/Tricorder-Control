/*
 * Tricorder Control Firmware - Minimal Version
 * ESP32-based prop controller with basic functionality
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <TFT_eSPI.h>

// Pin definitions for ESP32-2432S032C-I
#define LED_PIN 2          // NeoPixel data pin (external connection)
#define NUM_LEDS 12        // Number of NeoPixels in strip
#define TFT_BL 27          // TFT backlight pin

// Built-in RGB LED pins
#define RGB_LED_R 4        // Red channel
#define RGB_LED_G 16       // Green channel  
#define RGB_LED_B 17       // Blue channel

// Network configuration
const char* WIFI_SSID = "Rigging Electric";
const char* WIFI_PASSWORD = "academy123";
const int UDP_PORT = 8888;

// Device identification
String deviceId = "TRICORDER_001";
String firmwareVersion = "1.0.0";

// Hardware objects
CRGB leds[NUM_LEDS];
TFT_eSPI tft = TFT_eSPI();
WiFiUDP udp;

// State variables
bool wifiConnected = false;
bool videoPlaying = false;
String currentVideo = "";
CRGB currentColor = CRGB::Black;
uint8_t ledBrightness = 128;

// Function declarations
void handleUDPCommands();
void setLEDColor(int r, int g, int b);
void setLEDBrightness(int brightness);
void setBuiltinLED(int r, int g, int b);
void sendResponse(String commandId, String result);
void sendStatus(String commandId);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Tricorder Control System...");
  
  // Initialize LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  
  // Initialize built-in RGB LED pins
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  
  // Set built-in LED to blue during boot
  setBuiltinLED(0, 0, 255);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  
  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255); // Full brightness
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Tricorder Booting...");
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  tft.setCursor(10, 40);
  tft.println("Connecting to WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("WiFi Connected!");
    tft.setCursor(10, 40);
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    
    // Set built-in LED to green for WiFi connected
    setBuiltinLED(0, 255, 0);
    
    // Start UDP listener
    udp.begin(UDP_PORT);
    Serial.printf("UDP server started on port %d\n", UDP_PORT);
    
    // Start mDNS
    if (MDNS.begin(deviceId.c_str())) {
      Serial.println("mDNS responder started");
      MDNS.addService("tricorder", "udp", UDP_PORT);
    }
  } else {
    Serial.println("WiFi connection failed!");
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 10);
    tft.println("WiFi Failed!");
    
    // Set built-in LED to red for WiFi failed
    setBuiltinLED(255, 0, 0);
  }
  
  // Set initial LED state
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  
  Serial.println("Setup complete!");
}

void loop() {
  // Handle UDP commands
  handleUDPCommands();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    Serial.println("WiFi disconnected!");
  }
  
  delay(10);
}

void handleUDPCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    
    Serial.printf("Received UDP packet: %s\n", incomingPacket);
    
    // Parse JSON command
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, incomingPacket);
    
    if (!error) {
      String action = doc["action"];
      String commandId = doc["commandId"];
      
      if (action == "set_led_color") {
        int r = doc["parameters"]["r"];
        int g = doc["parameters"]["g"];
        int b = doc["parameters"]["b"];
        setLEDColor(r, g, b);
        sendResponse(commandId, "LED color set");
      }
      else if (action == "set_led_brightness") {
        int brightness = doc["parameters"]["brightness"];
        setLEDBrightness(brightness);
        sendResponse(commandId, "LED brightness set");
      }
      else if (action == "set_builtin_led") {
        int r = doc["parameters"]["r"];
        int g = doc["parameters"]["g"];
        int b = doc["parameters"]["b"];
        setBuiltinLED(r, g, b);
        sendResponse(commandId, "Built-in LED color set");
      }
      else if (action == "status") {
        sendStatus(commandId);
      }
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
  
  Serial.printf("Sent response: %s\n", response.c_str());
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
  
  String response;
  serializeJson(doc, response);
  
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
  
  Serial.printf("Sent status: %s\n", response.c_str());
}

void setBuiltinLED(int r, int g, int b) {
  // Note: These LEDs are typically inverted (LOW = ON, HIGH = OFF)
  // Adjust based on your board's behavior
  analogWrite(RGB_LED_R, 255 - r);  // Inverted PWM
  analogWrite(RGB_LED_G, 255 - g);  // Inverted PWM  
  analogWrite(RGB_LED_B, 255 - b);  // Inverted PWM
  
  Serial.printf("Built-in RGB LED set to R:%d G:%d B:%d\n", r, g, b);
}
