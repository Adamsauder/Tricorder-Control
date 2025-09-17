/*
 * Pin Holder Firmware - Standalone RGBW LED Controller
 * ESP32-C3 XIAO based pin holder with single RGBW LED and web interface
 * Features: RGBW sliders, trim pot intensity control, NVS storage, WiFi hotspot fallback
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// Pin definitions for ESP32-C3 XIAO
#define LED_PIN D3    // D3 (GPIO5) - Single RGBW LED
#define NUM_LEDS 1    // Single LED

// LED configuration for RGBW LED
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

// Network settings - HOTSPOT ONLY MODE
const char* hotspot_ssid = "PinHolder_001";
const char* hotspot_password = "prop2024";

// Device configuration
String deviceId;
String deviceLabel = "Pin Holder 001";
String firmwareVersion = "Pin Holder v1.0 RGBW";

// RGBW color values (0-255)
uint8_t redValue = 0;
uint8_t greenValue = 0;
uint8_t blueValue = 0;
uint8_t whiteValue = 0;

// NVS storage
Preferences preferences;

// Web server
AsyncWebServer server(80);

// Function declarations
void startHotspot();
void setupWebServer();
void updateLED();
void saveColorsToNVS();
void loadColorsFromNVS();
String getDeviceId();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Starting Pin Holder (Hotspot Mode)...");
    
    // Initialize LED strip
    strip.begin();
    strip.clear();
    strip.show();
    
    // Generate unique device ID
    deviceId = getDeviceId();
    Serial.println("Device ID: " + deviceId);
    
    // Load saved colors from NVS
    loadColorsFromNVS();
    
    // Start WiFi hotspot immediately
    startHotspot();
    
    // Setup web server
    setupWebServer();
    
    // Start mDNS
    if (MDNS.begin("pinholder")) {
        Serial.println("mDNS responder started");
        MDNS.addService("http", "tcp", 80);
    }
    
    Serial.println("Pin Holder ready!");
    Serial.print("Web interface: http://");
    Serial.println(WiFi.softAPIP());
    
    // Initial LED update
    updateLED();
}

void loop() {
    // Simple loop - no WiFi management needed in hotspot mode
    delay(100);
}

void startHotspot() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hotspot_ssid, hotspot_password);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Hotspot started. IP address: ");
    Serial.println(IP);
    Serial.println("SSID: " + String(hotspot_ssid));
    Serial.println("Password: " + String(hotspot_password));
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
    <title>Pin Holder RGBW Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; }
        .controls { display: grid; gap: 15px; }
        .slider-group { background: #f8f9fa; padding: 15px; border-radius: 5px; }
        .slider-label { font-weight: bold; margin-bottom: 5px; }
        .slider-container { display: flex; align-items: center; gap: 10px; }
        .slider { flex: 1; height: 6px; border-radius: 3px; background: #ddd; -webkit-appearance: none; }
        .slider::-webkit-slider-thumb { appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #007bff; cursor: pointer; }
        .slider::-moz-range-thumb { width: 20px; height: 20px; border-radius: 50%; background: #007bff; cursor: pointer; border: none; }
        .slider-value { min-width: 40px; text-align: center; font-weight: bold; }
        .color-preview { width: 100px; height: 50px; border: 2px solid #333; border-radius: 5px; margin: 10px auto; }
        .intensity-display { text-align: center; font-size: 18px; font-weight: bold; color: #007bff; }
        .btn { padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 14px; margin: 5px; }
        .btn-primary { background: #007bff; color: white; }
        .btn-success { background: #28a745; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        #redSlider { background: linear-gradient(to right, #000, #f00); }
        #greenSlider { background: linear-gradient(to right, #000, #0f0); }
        #blueSlider { background: linear-gradient(to right, #000, #00f); }
        #whiteSlider { background: linear-gradient(to right, #000, #fff); }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📍 Pin Holder RGBW Control</h1>
            <p>Standalone LED Controller</p>
        </div>
        
        <div class="status">
            <strong>Device:</strong> <span id="deviceId">)html" + deviceId + R"html(</span><br>
            <strong>IP Address:</strong> <span id="ipAddress">)html" + WiFi.softAPIP().toString() + R"html(</span>
        </div>
        
        <div class="controls">
            <div class="slider-group">
                <div class="slider-label">Red Channel</div>
                <div class="slider-container">
                    <input type="range" id="redSlider" class="slider" min="0" max="255" value="0" oninput="updateColor()">
                    <span id="redValue" class="slider-value">0</span>
                </div>
            </div>
            
            <div class="slider-group">
                <div class="slider-label">Green Channel</div>
                <div class="slider-container">
                    <input type="range" id="greenSlider" class="slider" min="0" max="255" value="0" oninput="updateColor()">
                    <span id="greenValue" class="slider-value">0</span>
                </div>
            </div>
            
            <div class="slider-group">
                <div class="slider-label">Blue Channel</div>
                <div class="slider-container">
                    <input type="range" id="blueSlider" class="slider" min="0" max="255" value="0" oninput="updateColor()">
                    <span id="blueValue" class="slider-value">0</span>
                </div>
            </div>
            
            <div class="slider-group">
                <div class="slider-label">White Channel</div>
                <div class="slider-container">
                    <input type="range" id="whiteSlider" class="slider" min="0" max="255" value="0" oninput="updateColor()">
                    <span id="whiteValue" class="slider-value">0</span>
                </div>
            </div>
            
            <div class="color-preview" id="colorPreview"></div>
            
            <div style="display: flex; gap: 10px; justify-content: center;">
                <button class="btn btn-success" onclick="setPreset(255, 0, 0, 0)">🔴 Red</button>
                <button class="btn btn-success" onclick="setPreset(0, 255, 0, 0)">🟢 Green</button>
                <button class="btn btn-success" onclick="setPreset(0, 0, 255, 0)">🔵 Blue</button>
                <button class="btn btn-success" onclick="setPreset(0, 0, 0, 255)">⚪ White</button>
                <button class="btn btn-danger" onclick="setPreset(0, 0, 0, 0)">⚫ Off</button>
            </div>
        </div>
    </div>
    
    <script>
        function updateColor() {
            const r = document.getElementById('redSlider').value;
            const g = document.getElementById('greenSlider').value;
            const b = document.getElementById('blueSlider').value;
            const w = document.getElementById('whiteSlider').value;
            
            // Update value displays
            document.getElementById('redValue').textContent = r;
            document.getElementById('greenValue').textContent = g;
            document.getElementById('blueValue').textContent = b;
            document.getElementById('whiteValue').textContent = w;
            
            // Update color preview (RGB only, white channel not visible in CSS)
            document.getElementById('colorPreview').style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
            
            // Send to device
            fetch('/rgbw', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ r: parseInt(r), g: parseInt(g), b: parseInt(b), w: parseInt(w) })
            });
        }
        
        function setPreset(r, g, b, w) {
            document.getElementById('redSlider').value = r;
            document.getElementById('greenSlider').value = g;
            document.getElementById('blueSlider').value = b;
            document.getElementById('whiteSlider').value = w;
            updateColor();
        }
        
        function loadColors() {
            fetch('/colors')
            .then(response => response.json())
            .then(data => {
                document.getElementById('redSlider').value = data.r || 0;
                document.getElementById('greenSlider').value = data.g || 0;
                document.getElementById('blueSlider').value = data.b || 0;
                document.getElementById('whiteSlider').value = data.w || 0;
                updateColor();
            });
        }
        
        // Load initial colors
        loadColors();
    </script>
</body>
</html>
)html";
        request->send(200, "text/html", html);
    });
    
    // API endpoint for RGBW color setting
    server.on("/rgbw", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "RGBW color set");
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            JsonDocument doc;
            if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
                if (doc["r"].is<int>() && doc["g"].is<int>() && doc["b"].is<int>() && doc["w"].is<int>()) {
                    redValue = doc["r"];
                    greenValue = doc["g"];
                    blueValue = doc["b"];
                    whiteValue = doc["w"];
                    
                    Serial.printf("RGBW set to: R=%d, G=%d, B=%d, W=%d\n", redValue, greenValue, blueValue, whiteValue);
                    
                    // Save colors to NVS and update LED
                    saveColorsToNVS();
                    updateLED();
                    
                    request->send(200, "text/plain", "RGBW color set");
                    return;
                }
            }
            request->send(400, "text/plain", "Invalid JSON");
        }
    });
    
    // API endpoint for getting current colors
    server.on("/colors", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["r"] = redValue;
        doc["g"] = greenValue;
        doc["b"] = blueValue;
        doc["w"] = whiteValue;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Status endpoint
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["device_id"] = deviceId;
        doc["device_label"] = deviceLabel;
        doc["firmware_version"] = firmwareVersion;
        doc["ip_address"] = WiFi.softAPIP().toString();
        doc["r"] = redValue;
        doc["g"] = greenValue;
        doc["b"] = blueValue;
        doc["w"] = whiteValue;
        doc["uptime"] = millis() / 1000;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.begin();
    Serial.println("Pin Holder web server started on port 80");
}

void updateLED() {
    // Direct RGBW LED control - no intensity scaling
    strip.setPixelColor(0, strip.Color(redValue, greenValue, blueValue, whiteValue));
    strip.show();
    
    Serial.printf("LED updated: RGBW(%d,%d,%d,%d)\n", redValue, greenValue, blueValue, whiteValue);
}

void saveColorsToNVS() {
    preferences.begin("pinHolder", false);
    preferences.putUChar("red", redValue);
    preferences.putUChar("green", greenValue);
    preferences.putUChar("blue", blueValue);
    preferences.putUChar("white", whiteValue);
    preferences.end();
    
    Serial.printf("Colors saved to NVS: RGBW(%d,%d,%d,%d)\n", redValue, greenValue, blueValue, whiteValue);
}

void loadColorsFromNVS() {
    preferences.begin("pinHolder", true);
    redValue = preferences.getUChar("red", 0);
    greenValue = preferences.getUChar("green", 0);
    blueValue = preferences.getUChar("blue", 0);
    whiteValue = preferences.getUChar("white", 0);
    preferences.end();
    
    Serial.printf("Colors loaded from NVS: RGBW(%d,%d,%d,%d)\n", redValue, greenValue, blueValue, whiteValue);
}

String getDeviceId() {
    // Generate unique device ID based on MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    char deviceIdStr[32];
    sprintf(deviceIdStr, "PINHOLDER_%02X%02X%02X", mac[3], mac[4], mac[5]);
    
    return String(deviceIdStr);
}
