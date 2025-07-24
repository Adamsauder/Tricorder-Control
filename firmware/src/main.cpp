/*
 * Tricorder Control Firmware - Enhanced with Video Playback
 * ESP32-based prop controller with video playback from SD card
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <JPEGDEC.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Pin definitions for ESP32-2432S032C-I
#define LED_PIN 2          // NeoPixel data pin (external connection)
#define NUM_LEDS 12        // Number of NeoPixels in strip
#define TFT_BL 27          // TFT backlight pin

// SD Card pins (typical SPI configuration for ESP32-2432S032C)
#define SD_CS 5            // SD card chip select
#define SD_MOSI 23         // SD card MOSI
#define SD_MISO 19         // SD card MISO
#define SD_SCLK 18         // SD card SCLK

// Built-in RGB LED pins
#define RGB_LED_R 4        // Red channel
#define RGB_LED_G 16       // Green channel  
#define RGB_LED_B 17       // Blue channel

// Video playback settings
#define FRAME_DELAY_MS 33  // ~30 FPS (1000ms / 30)
#define VIDEO_BUFFER_SIZE 12288  // 12KB buffer for larger JPEG frames

// Network configuration
const char* WIFI_SSID = "Rigging Electric";
const char* WIFI_PASSWORD = "academy123";
const int UDP_PORT = 8888;

// Device identification
String deviceId = ""; // Will be set based on MAC address
String firmwareVersion = "1.0.0";

// Hardware objects
CRGB leds[NUM_LEDS];
TFT_eSPI tft = TFT_eSPI();
WiFiUDP udp;
JPEGDEC jpeg;
WebServer otaServer(80);  // Web server for OTA updates

// Video playback objects
File videoFile;
uint8_t* videoBuffer;

// State variables
bool wifiConnected = false;
bool videoPlaying = false;
bool videoLooping = false;
bool sdCardInitialized = false;
String currentVideo = "";
String videoDirectory = "/videos";
CRGB currentColor = CRGB::Black;
uint8_t ledBrightness = 128;
unsigned long lastFrameTime = 0;
int currentFrame = 0;
int totalFrames = 1;
String frameFiles[30]; // Store up to 30 frame files
bool isAnimatedSequence = false;

// Video frame callback
int JPEGDraw(JPEGDRAW *pDraw) {
  // Draw the JPEG frame to the TFT display
  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  return 1;
}

// Function declarations
void handleUDPCommands();
void setLEDColor(int r, int g, int b);
void setLEDBrightness(int brightness);
void setBuiltinLED(int r, int g, int b);
void sendResponse(String commandId, String result);
void sendStatus(String commandId);
bool initializeSDCard();
bool playVideo(String filename, bool loop = false);
void stopVideo();
void updateVideoPlayback();
bool listVideos();
String getVideoList();
void showVideoFrame();
void setupOTA();
void setupOTAWebServer();
void showModernBootScreen();
void updateBootStatus(String component, String status, uint16_t color, bool checkmark);
void updateBootIP(String ipAddress);
void showBootComplete();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Tricorder Control System...");
  
  // Generate unique device ID based on MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  deviceId = "TRICORDER_" + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  deviceId.toUpperCase();
  Serial.printf("Device ID: %s\n", deviceId.c_str());
  
  // Allocate video buffer
  videoBuffer = (uint8_t*)malloc(VIDEO_BUFFER_SIZE);
  if (!videoBuffer) {
    Serial.println("Failed to allocate video buffer!");
  }
  
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
  tft.setRotation(0);  // 90° counter-clockwise from original rotation(1)
  
  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255); // Full brightness
  
  // Modern boot screen design
  showModernBootScreen();
  
  // Initialize WiFi
  updateBootStatus("WIFI", "Connecting...", TFT_YELLOW, false);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) { // 20 second timeout
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Update connecting animation every second
    if (attempts % 2 == 0) {
      String dots = "";
      int dotCount = (attempts/2) % 4;
      for (int i = 0; i < dotCount; i++) {
        dots += ".";
      }
      updateBootStatus("WIFI", "Connecting" + dots, TFT_YELLOW, false);
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    updateBootStatus("WIFI", "Connected", TFT_GREEN, true);
    updateBootIP(WiFi.localIP().toString());
    
    // Initialize UDP
    udp.begin(UDP_PORT);
    Serial.printf("UDP server listening on port %d\n", UDP_PORT);
    
    // Start mDNS
    if (MDNS.begin(deviceId.c_str())) {
      Serial.println("mDNS responder started");
      MDNS.addService("tricorder", "udp", UDP_PORT);
    }
    
    // Initialize OTA updates
    setupOTA();
    
    // Initialize OTA web server
    setupOTAWebServer();
    
    // Set built-in LED to green when connected
    setBuiltinLED(0, 255, 0);
  } else {
    Serial.println("\nFailed to connect to WiFi");
    updateBootStatus("WIFI", "Failed", TFT_RED, false);
    
    // Set built-in LED to red when failed
    setBuiltinLED(255, 0, 0);
  }
  
  // Initialize SD Card
  updateBootStatus("SD", "Initializing...", TFT_YELLOW, false);
  
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) {
    sdCardInitialized = true;
    Serial.println("SD card initialized successfully!");
    updateBootStatus("SD", "Ready", TFT_GREEN, true);
    
    // Create videos directory if it doesn't exist
    if (!SD.exists(videoDirectory)) {
      SD.mkdir(videoDirectory);
      Serial.println("Created /videos directory");
    }
    
    // List available videos
    listVideos();
  } else {
    Serial.println("SD card initialization failed!");
    updateBootStatus("SD", "Failed", TFT_RED, false);
  }
  
  // Show system ready
  updateBootStatus("SYS", "Ready", TFT_GREEN, true);
  showBootComplete();
  
  delay(2000); // Show status briefly
  
  // Set initial LED state
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  
  Serial.println("Setup complete!");
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle OTA web server
  otaServer.handleClient();
  
  // Handle UDP commands
  handleUDPCommands();
  
  // Update video playback
  if (videoPlaying) {
    updateVideoPlayback();
  }
  
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
      
      // Handle discovery command
      if (action == "discovery") {
        JsonDocument response;
        response["commandId"] = commandId;
        response["deviceId"] = deviceId;
        response["type"] = "tricorder";
        response["firmwareVersion"] = firmwareVersion;
        response["ipAddress"] = WiFi.localIP().toString();
        
        String responseStr;
        serializeJson(response, responseStr);
        
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write((const uint8_t*)responseStr.c_str(), responseStr.length());
        udp.endPacket();
        
        Serial.printf("Sent discovery response: %s\n", responseStr.c_str());
      }
      // Handle ping command
      else if (action == "ping") {
        JsonDocument response;
        response["commandId"] = commandId;
        response["deviceId"] = deviceId;
        response["result"] = "pong";
        response["timestamp"] = millis();
        
        String responseStr;
        serializeJson(response, responseStr);
        
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write((const uint8_t*)responseStr.c_str(), responseStr.length());
        udp.endPacket();
        
        Serial.printf("Sent ping response: %s\n", responseStr.c_str());
      }
      else if (action == "set_led_color") {
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
        unsigned long cmdStart = millis();
        int r = doc["parameters"]["r"];
        int g = doc["parameters"]["g"];
        int b = doc["parameters"]["b"];
        setBuiltinLED(r, g, b);
        unsigned long cmdEnd = millis();
        Serial.printf("⚡ LED command processed in %dms\n", cmdEnd - cmdStart);
        sendResponse(commandId, "Built-in LED color set");
      }
      else if (action == "play_video") {
        String filename = doc["parameters"]["filename"];
        bool loop = doc["parameters"]["loop"].as<bool>() || false;
        
        if (sdCardInitialized) {
          if (playVideo(filename, loop)) {
            sendResponse(commandId, "Video started: " + filename);
          } else {
            sendResponse(commandId, "Failed to start video: " + filename);
          }
        } else {
          sendResponse(commandId, "SD card not available");
        }
      }
      else if (action == "stop_video") {
        stopVideo();
        sendResponse(commandId, "Video stopped");
      }
      else if (action == "list_videos") {
        if (sdCardInitialized) {
          listVideos(); // Still print to serial for debugging
          String videoList = getVideoList();
          sendResponse(commandId, videoList);
        } else {
          sendResponse(commandId, "SD card not available");
        }
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
  doc["sdCardInitialized"] = sdCardInitialized;
  doc["videoPlaying"] = videoPlaying;
  doc["currentVideo"] = currentVideo;
  doc["videoLooping"] = videoLooping;
  doc["currentFrame"] = currentFrame;
  
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
  
  unsigned long startTime = millis();
  
  // Set all channels simultaneously for better responsiveness
  int red_val = 255 - r;    // Inverted PWM
  int green_val = 255 - g;  // Inverted PWM  
  int blue_val = 255 - b;   // Inverted PWM
  
  // Write all channels at once to minimize delay
  analogWrite(RGB_LED_R, red_val);
  analogWrite(RGB_LED_G, green_val);
  analogWrite(RGB_LED_B, blue_val);
  
  // Small delay to ensure PWM settles (especially important for blue channel)
  delayMicroseconds(100);
  
  unsigned long endTime = millis();
  Serial.printf("Built-in RGB LED set to R:%d G:%d B:%d (took %dms)\n", r, g, b, endTime - startTime);
}

void showModernBootScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // Draw header bar with gradient effect
  tft.fillRect(0, 0, 240, 35, TFT_NAVY);
  tft.fillRect(0, 0, 240, 2, TFT_BLUE);
  tft.fillRect(0, 33, 240, 2, TFT_DARKGREY);
  
  // Use larger built-in font for the title
  tft.setTextFont(4); // Use font 4 (larger, more modern looking)
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 8);
  tft.println("TRICORDER");
  
  // Add version/model info in smaller font
  tft.setTextFont(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(155, 10);
  tft.println("MK-VII");
  
  // Add ALPHA VERSION badge
  tft.setTextFont(1);
  tft.setTextColor(TFT_RED);
  tft.setCursor(155, 22);
  tft.println("ALPHA VERSION");
  
  // Draw device info section
  tft.setTextFont(2);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.setCursor(10, 45);
  tft.println("DEVICE INFO");
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 65);
  tft.print("ID: ");
  tft.setTextColor(TFT_CYAN);
  tft.println(deviceId);
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 75);
  tft.print("FW: ");
  tft.setTextColor(TFT_YELLOW);
  tft.println(firmwareVersion);
  
  // Draw status panel with modern styling
  tft.fillRect(5, 90, 230, 110, TFT_BLACK);
  tft.drawRect(5, 90, 230, 110, TFT_DARKGREY);
  tft.drawRect(6, 91, 228, 108, TFT_DARKGREY);
  
  // Modern section title
  tft.setTextFont(2);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.setCursor(15, 100);
  tft.println("SYSTEM STATUS");
  
  // Draw separator line with style
  tft.drawLine(15, 115, 225, 115, TFT_DARKGREY);
  tft.drawLine(15, 116, 225, 116, TFT_BLUE);
}

void updateBootStatus(String component, String status, uint16_t color, bool checkmark) {
  static int yPos = 125;
  
  if (component == "WIFI") yPos = 125;
  else if (component == "SD") yPos = 155;
  else if (component == "SYS") yPos = 175;
  
  // Clear the line (larger area to avoid overlaps)
  tft.fillRect(15, yPos - 2, 210, 18, TFT_BLACK);
  
  // Draw component name with larger font
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, yPos);
  tft.print(component);
  
  // Draw colon
  tft.print(":");
  
  // Draw status with regular font
  tft.setTextFont(1);
  tft.setTextColor(color);
  tft.setCursor(70, yPos + 3);
  tft.print(status);
  
  // Draw modern status indicators
  if (checkmark) {
    // Draw modern checkmark (filled circle with check)
    tft.fillCircle(200, yPos + 6, 8, TFT_DARKGREEN);
    tft.drawCircle(200, yPos + 6, 8, TFT_GREEN);
    // Draw checkmark lines
    tft.drawLine(196, yPos + 6, 198, yPos + 8, TFT_WHITE);
    tft.drawLine(198, yPos + 8, 204, yPos + 2, TFT_WHITE);
    tft.drawLine(196, yPos + 7, 198, yPos + 9, TFT_WHITE);
    tft.drawLine(198, yPos + 9, 204, yPos + 3, TFT_WHITE);
  } else if (color == TFT_RED) {
    // Draw modern X (filled circle with X)
    tft.fillCircle(200, yPos + 6, 8, TFT_MAROON);
    tft.drawCircle(200, yPos + 6, 8, TFT_RED);
    // Draw X lines
    tft.drawLine(196, yPos + 2, 204, yPos + 10, TFT_WHITE);
    tft.drawLine(204, yPos + 2, 196, yPos + 10, TFT_WHITE);
    tft.drawLine(196, yPos + 3, 204, yPos + 11, TFT_WHITE);
    tft.drawLine(204, yPos + 3, 196, yPos + 11, TFT_WHITE);
  } else if (color == TFT_YELLOW) {
    // Draw loading indicator (spinning circle)
    tft.drawCircle(200, yPos + 6, 6, TFT_YELLOW);
    tft.drawCircle(200, yPos + 6, 5, TFT_ORANGE);
  }
}

void updateBootIP(String ipAddress) {
  // Clear the IP line area (below WiFi status, with proper spacing)
  tft.fillRect(15, 140, 210, 12, TFT_BLACK);
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(25, 143);
  tft.print("IP: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(ipAddress);
}

void showBootComplete() {
  // Draw modern completion panel
  tft.fillRect(5, 210, 230, 50, TFT_DARKGREEN);
  tft.drawRect(5, 210, 230, 50, TFT_GREEN);
  tft.drawRect(6, 211, 228, 48, TFT_GREENYELLOW);
  
  // Add corner styling
  tft.drawLine(5, 210, 10, 210, TFT_GREENYELLOW);
  tft.drawLine(5, 210, 5, 215, TFT_GREENYELLOW);
  tft.drawLine(230, 210, 235, 210, TFT_GREENYELLOW);
  tft.drawLine(235, 210, 235, 215, TFT_GREENYELLOW);
  
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 225);
  tft.println("SYSTEM READY");
  
  tft.setTextFont(1);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.setCursor(15, 245);
  tft.println("Awaiting commands...");
  
  // Add alpha version badge in bottom right
  tft.setTextFont(1);
  tft.setTextColor(TFT_RED);
  tft.setCursor(170, 245);
  tft.println("ALPHA v0.1");
}

bool playVideo(String filename, bool loop) {
  if (!sdCardInitialized) {
    Serial.println("SD card not initialized");
    return false;
  }
  
  // Stop any currently playing video
  if (videoPlaying) {
    stopVideo();
  }
  
  // Reset animation state
  totalFrames = 0;
  isAnimatedSequence = false;
  
  // Check if this is a folder-based animation
  String folderPath = videoDirectory + "/" + filename;
  
  if (SD.exists(folderPath)) {
    // Check if it's a directory
    File testDir = SD.open(folderPath);
    if (testDir && testDir.isDirectory()) {
      testDir.close();
      
      // Load all JPEG files from the folder
      File animDir = SD.open(folderPath);
      if (!animDir) {
        Serial.printf("Failed to open animation folder: %s\n", folderPath.c_str());
        return false;
      }
      
      // Collect all frame files
      File file = animDir.openNextFile();
      while (file && totalFrames < 30) {
        if (!file.isDirectory()) {
          String frameFile = file.name();
          if (frameFile.endsWith(".jpg") || frameFile.endsWith(".jpeg") || 
              frameFile.endsWith(".JPG") || frameFile.endsWith(".JPEG")) {
            frameFiles[totalFrames] = folderPath + "/" + frameFile;
            totalFrames++;
            Serial.printf("Added frame %d: %s\n", totalFrames, frameFile.c_str());
          }
        }
        file = animDir.openNextFile();
      }
      animDir.close();
      
      // Sort frame files to ensure correct playback order
      if (totalFrames > 1) {
        for (int i = 0; i < totalFrames - 1; i++) {
          for (int j = i + 1; j < totalFrames; j++) {
            if (frameFiles[i] > frameFiles[j]) {
              String temp = frameFiles[i];
              frameFiles[i] = frameFiles[j];
              frameFiles[j] = temp;
            }
          }
        }
        Serial.println("Sorted frame files:");
        for (int i = 0; i < totalFrames; i++) {
          Serial.printf("  Frame %d: %s\n", i, frameFiles[i].c_str());
        }
      }
      
      Serial.printf("Animation loaded: %d frames total\n", totalFrames);
      
      if (totalFrames > 0) {
        isAnimatedSequence = true;
        Serial.printf("Loaded %d frames for animation: %s\n", totalFrames, filename.c_str());
        
        // Set video state for animation
        videoPlaying = true;
        videoLooping = loop;
        currentVideo = filename;
        currentFrame = 0;
        lastFrameTime = millis();
        
        // Clear screen
        tft.fillScreen(TFT_BLACK);
        
        return true;
      } else {
        Serial.printf("No JPEG files found in folder: %s\n", folderPath.c_str());
        return false;
      }
    }
  }
  
  // Fall back to single file mode
  String actualFile = "";
  
  // If filename already has extension, use it directly
  if (filename.endsWith(".jpg") || filename.endsWith(".jpeg") || 
      filename.endsWith(".JPG") || filename.endsWith(".JPEG")) {
    actualFile = filename;
  } else {
    // Search for files that match the base name
    File dir = SD.open(videoDirectory);
    if (!dir) {
      Serial.println("Failed to open videos directory");
      return false;
    }
    
    File file = dir.openNextFile();
    String bestMatch = "";
    
    while (file) {
      if (!file.isDirectory()) {
        String candidateFile = file.name();
        
        // Check if this file matches our search
        if ((candidateFile.endsWith(".jpg") || candidateFile.endsWith(".jpeg") || 
             candidateFile.endsWith(".JPG") || candidateFile.endsWith(".JPEG"))) {
          
          // Check if filename is contained in the candidate
          if (candidateFile.indexOf(filename) == 0) {
            // Prefer exact matches or first frame
            if (bestMatch == "" || 
                candidateFile == filename + ".jpg" || 
                candidateFile == filename + ".jpeg" ||
                candidateFile.indexOf("_001") > 0 ||
                candidateFile.indexOf("_frame_001") > 0) {
              bestMatch = candidateFile;
            }
          }
        }
      }
      file = dir.openNextFile();
    }
    
    dir.close();
    
    if (bestMatch != "") {
      actualFile = bestMatch;
    } else {
      // Try adding .jpg extension
      actualFile = filename + ".jpg";
    }
  }
  
  // Construct full path
  String fullPath = videoDirectory + "/" + actualFile;
  
  // Check if file exists
  if (!SD.exists(fullPath)) {
    Serial.printf("Video file not found: %s\n", fullPath.c_str());
    Serial.printf("Tried: %s\n", actualFile.c_str());
    return false;
  }
  
  // Single file mode - just store the path in frameFiles[0]
  frameFiles[0] = fullPath;
  totalFrames = 1;
  isAnimatedSequence = false;
  
  Serial.printf("Starting single image playback: %s -> %s (Loop: %s)\n", 
                filename.c_str(), actualFile.c_str(), loop ? "Yes" : "No");
  
  // Set video state
  videoPlaying = true;
  videoLooping = loop;
  currentVideo = filename;
  currentFrame = 0;
  lastFrameTime = millis();
  
  // Clear screen
  tft.fillScreen(TFT_BLACK);
  
  return true;
}

void stopVideo() {
  if (videoPlaying) {
    videoPlaying = false;
    videoLooping = false;
    currentFrame = 0;
    totalFrames = 0;
    isAnimatedSequence = false;
    
    // Clear frame files array
    for (int i = 0; i < 30; i++) {
      frameFiles[i] = "";
    }
    
    // Close video file if it was opened (legacy single file mode)
    if (videoFile) {
      videoFile.close();
    }
    
    // Clear screen and show status
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Video Stopped");
    
    Serial.printf("Video stopped: %s\n", currentVideo.c_str());
    currentVideo = "";
  }
}

void updateVideoPlayback() {
  if (!videoPlaying || totalFrames == 0) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // For single images, show once and handle looping
  if (!isAnimatedSequence) {
    if (currentFrame == 0) {
      showVideoFrame();
      currentFrame = 1; // Mark as shown
    }
    
    // For single images, only stop if not looping
    if (!videoLooping && currentFrame > 0) {
      // Single image shown, not looping - keep displaying
      return;
    }
    return;
  }
  
  // For animated sequences, check if it's time for the next frame
  if (currentTime - lastFrameTime >= FRAME_DELAY_MS) {
    Serial.printf("Frame timer triggered - currentFrame: %d, totalFrames: %d\n", currentFrame, totalFrames);
    showVideoFrame();
    lastFrameTime = currentTime;
    currentFrame++;
    Serial.printf("Advanced to frame %d\n", currentFrame);
    
    // Check if we've reached the end of the animation
    if (currentFrame >= totalFrames) {
      Serial.printf("Animation complete - currentFrame %d >= totalFrames %d\n", currentFrame, totalFrames);
      if (videoLooping) {
        // Restart animation
        currentFrame = 0;
        Serial.println("Looping animation...");
      } else {
        // Stop animation
        Serial.println("Stopping animation (not looping)");
        stopVideo();
      }
    }
  }
}

void showVideoFrame() {
  if (!videoPlaying || totalFrames == 0) {
    return;
  }
  
  // Validate frame index
  if (currentFrame < 0 || currentFrame >= totalFrames) {
    Serial.printf("ERROR: Invalid frame index %d (totalFrames: %d)\n", currentFrame, totalFrames);
    return;
  }
  
  // Get the current frame file path
  String currentFramePath = frameFiles[currentFrame];
  Serial.printf("Attempting to show frame %d: %s\n", currentFrame, currentFramePath.c_str());
  
  // Open and display the current frame
  File frameFile = SD.open(currentFramePath, FILE_READ);
  if (!frameFile) {
    Serial.printf("ERROR: Failed to open frame file: %s\n", currentFramePath.c_str());
    Serial.printf("This was frame %d of %d\n", currentFrame, totalFrames);
    
    // Skip this frame and try the next one
    currentFrame++;
    if (currentFrame >= totalFrames) {
      Serial.println("Reached end due to failed frame load, restarting...");
      if (videoLooping) {
        currentFrame = 0;
      } else {
        stopVideo();
      }
    }
    return;
  }
  
  // Read the entire JPEG file into buffer
  size_t fileSize = frameFile.size();
  if (fileSize > VIDEO_BUFFER_SIZE) {
    Serial.printf("Frame file too large: %d bytes (max %d)\n", fileSize, VIDEO_BUFFER_SIZE);
    frameFile.close();
    return;
  }
  
  size_t bytesRead = frameFile.read(videoBuffer, fileSize);
  frameFile.close();
  
  if (bytesRead > 0) {
    // Try to decode as JPEG
    if (jpeg.openRAM(videoBuffer, bytesRead, JPEGDraw)) {
      // Set scale to fit screen
      jpeg.setPixelType(RGB565_BIG_ENDIAN);
      
      // Decode and display
      if (jpeg.decode(0, 0, 0)) {
        Serial.printf("SUCCESS: Displayed frame %d/%d: %s\n", currentFrame + 1, totalFrames, currentFramePath.c_str());
      } else {
        Serial.printf("ERROR: JPEG decode failed for frame %d: %s\n", currentFrame, currentFramePath.c_str());
      }
      
      jpeg.close();
    } else {
      Serial.printf("ERROR: JPEG open failed for frame %d: %s\n", currentFrame, currentFramePath.c_str());
    }
  } else {
    Serial.printf("ERROR: No bytes read from frame %d: %s\n", currentFrame, currentFramePath.c_str());
  }
}

bool listVideos() {
  if (!sdCardInitialized) {
    Serial.println("SD card not initialized");
    return false;
  }
  
  File dir = SD.open(videoDirectory);
  if (!dir) {
    Serial.println("Failed to open videos directory");
    return false;
  }
  
  Serial.println("Available videos:");
  Serial.println("=================");
  
  File file = dir.openNextFile();
  int videoCount = 0;
  
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      size_t fileSize = file.size();
      
      // Check for supported video formats (using JPEG sequences for now)
      if (filename.endsWith(".jpg") || filename.endsWith(".jpeg") || 
          filename.endsWith(".JPG") || filename.endsWith(".JPEG")) {
        Serial.printf("  %s (%d bytes)\n", filename.c_str(), fileSize);
        videoCount++;
      }
    }
    file = dir.openNextFile();
  }
  
  dir.close();
  
  if (videoCount == 0) {
    Serial.println("  No videos found in /videos directory");
    Serial.println("  Supported formats: .jpg, .jpeg (JPEG sequences)");
  } else {
    Serial.printf("Found %d video files\n", videoCount);
  }
  
  return true;
}

String getVideoList() {
  String videoList = "";
  
  if (!sdCardInitialized) {
    return "SD card not initialized";
  }
  
  File dir = SD.open(videoDirectory);
  if (!dir) {
    return "Failed to open videos directory";
  }
  
  File item = dir.openNextFile();
  int folderCount = 0;
  int fileCount = 0;
  String folders[20]; // Store up to 20 folder names
  String uniqueFiles[20]; // Store up to 20 unique file names
  int uniqueFileCount = 0;
  
  while (item) {
    String itemName = item.name();
    
    if (item.isDirectory()) {
      // This is a folder - check if it contains JPEG files
      String folderPath = videoDirectory + "/" + itemName;
      File subDir = SD.open(folderPath);
      if (subDir) {
        File subFile = subDir.openNextFile();
        bool hasJpegs = false;
        while (subFile && !hasJpegs) {
          if (!subFile.isDirectory()) {
            String subFileName = subFile.name();
            if (subFileName.endsWith(".jpg") || subFileName.endsWith(".jpeg") || 
                subFileName.endsWith(".JPG") || subFileName.endsWith(".JPEG")) {
              hasJpegs = true;
            }
          }
          subFile = subDir.openNextFile();
        }
        subDir.close();
        
        if (hasJpegs && folderCount < 20) {
          folders[folderCount] = itemName;
          folderCount++;
        }
      }
    } else {
      // This is a file - check if it's a JPEG
      if (itemName.endsWith(".jpg") || itemName.endsWith(".jpeg") || 
          itemName.endsWith(".JPG") || itemName.endsWith(".JPEG")) {
        
        // Extract base name (remove frame numbers and extensions)
        String baseName = itemName;
        
        // Remove common frame suffixes like _001, _frame_001, etc.
        int framePos = baseName.indexOf("_frame_");
        if (framePos == -1) framePos = baseName.lastIndexOf("_");
        
        if (framePos > 0) {
          String suffix = baseName.substring(framePos + 1);
          // Check if suffix is numeric (frame number)
          bool isNumeric = true;
          for (int i = 0; i < suffix.length(); i++) {
            char c = suffix.charAt(i);
            if (c < '0' || c > '9') {
              if (c != '.' && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z')) {
                isNumeric = false;
                break;
              }
            }
          }
          if (isNumeric || suffix.startsWith("00") || suffix.startsWith("frame")) {
            baseName = baseName.substring(0, framePos);
          }
        }
        
        // Remove file extension
        int dotPos = baseName.lastIndexOf(".");
        if (dotPos > 0) {
          baseName = baseName.substring(0, dotPos);
        }
        
        // Check if this base name is already in our list
        bool found = false;
        for (int i = 0; i < uniqueFileCount; i++) {
          if (uniqueFiles[i] == baseName) {
            found = true;
            break;
          }
        }
        
        // Add to unique list if not found
        if (!found && uniqueFileCount < 20) {
          uniqueFiles[uniqueFileCount] = baseName;
          uniqueFileCount++;
        }
        
        fileCount++;
      }
    }
    item = dir.openNextFile();
  }
  
  dir.close();
  
  if (folderCount == 0 && uniqueFileCount == 0) {
    videoList = "No videos found. Create folders with JPEG sequences or place JPEG files in /videos";
  } else {
    videoList = "";
    
    if (folderCount > 0) {
      videoList += String(folderCount) + " animations: ";
      for (int i = 0; i < folderCount; i++) {
        if (i > 0) videoList += ", ";
        videoList += folders[i];
      }
    }
    
    if (uniqueFileCount > 0) {
      if (folderCount > 0) videoList += " | ";
      videoList += String(uniqueFileCount) + " images: ";
      for (int i = 0; i < uniqueFileCount; i++) {
        if (i > 0) videoList += ", ";
        videoList += uniqueFiles[i];
      }
    }
    
    if (fileCount > 0) {
      videoList += " (" + String(fileCount) + " total files)";
    }
  }
  
  return videoList;
}

// OTA Setup Functions
void setupOTA() {
  ArduinoOTA.setHostname(deviceId.c_str());
  ArduinoOTA.setPassword("tricorder123");  // Set OTA password
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    
    // Stop video playback and clear display
    videoPlaying = false;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("OTA Update");
    tft.setCursor(10, 40);
    tft.println("Starting...");
    
    // Set built-in LED to orange during update
    setBuiltinLED(255, 165, 0);
    
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    tft.setCursor(10, 70);
    tft.setTextColor(TFT_GREEN);
    tft.println("Complete!");
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int lastPercent = 0;
    unsigned int percent = (progress / (total / 100));
    
    if (percent != lastPercent && percent % 10 == 0) {
      tft.fillRect(10, 100, 300, 30, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.setTextColor(TFT_CYAN);
      tft.printf("Progress: %u%%\n", percent);
      lastPercent = percent;
    }
    
    Serial.printf("Progress: %u%%\r", percent);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.println("OTA Error!");
    
    setBuiltinLED(255, 0, 0);  // Red for error
    
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
      tft.setCursor(10, 40);
      tft.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
      tft.setCursor(10, 40);
      tft.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
      tft.setCursor(10, 40);
      tft.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
      tft.setCursor(10, 40);
      tft.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
      tft.setCursor(10, 40);
      tft.println("End Failed");
    }
    
    delay(5000);
    ESP.restart();
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void setupOTAWebServer() {
  // Handle firmware upload via web interface
  otaServer.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head><title>Tricorder OTA Update</title>";
    html += "<style>body{font-family:Arial;margin:40px;background:#f0f0f0;}";
    html += ".container{max-width:600px;margin:0 auto;background:white;padding:30px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;text-align:center;}";
    html += ".info{background:#e7f3ff;padding:15px;border-radius:5px;margin:20px 0;}";
    html += "input[type=file]{width:100%;padding:10px;margin:10px 0;border:2px dashed #ccc;border-radius:5px;}";
    html += "input[type=submit]{background:#007cba;color:white;padding:15px 30px;border:none;border-radius:5px;font-size:16px;cursor:pointer;width:100%;}";
    html += "input[type=submit]:hover{background:#005a87;}";
    html += ".status{margin:20px 0;padding:10px;border-radius:5px;display:none;}";
    html += ".success{background:#d4edda;color:#155724;border:1px solid #c3e6cb;}";
    html += ".error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>Tricorder Firmware Update</h1>";
    html += "<div class='info'>";
    html += "<strong>Device:</strong> " + deviceId + "<br>";
    html += "<strong>Current Firmware:</strong> " + firmwareVersion + "<br>";
    html += "<strong>IP Address:</strong> " + WiFi.localIP().toString() + "<br>";
    html += "<strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes";
    html += "</div>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<p>Select firmware file (.bin):</p>";
    html += "<input type='file' name='firmware' accept='.bin' required>";
    html += "<br><input type='submit' value='Upload Firmware'>";
    html += "</form>";
    html += "<div id='status' class='status'></div>";
    html += "</div></body></html>";
    
    otaServer.send(200, "text/html", html);
  });
  
  // Handle firmware upload
  otaServer.on("/update", HTTP_POST, []() {
    otaServer.sendHeader("Connection", "close");
    otaServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = otaServer.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      
      // Display update status on screen
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW);
      tft.setTextSize(2);
      tft.setCursor(10, 10);
      tft.println("Web OTA Update");
      tft.setCursor(10, 40);
      tft.println("Uploading...");
      
      setBuiltinLED(255, 165, 0);  // Orange during update
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      } else {
        // Update progress on screen
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 1000) {  // Update every second
          int progress = (Update.progress() * 100) / Update.size();
          tft.fillRect(10, 70, 300, 30, TFT_BLACK);
          tft.setCursor(10, 70);
          tft.setTextColor(TFT_CYAN);
          tft.printf("Progress: %d%%", progress);
          lastUpdate = millis();
        }
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        tft.setCursor(10, 100);
        tft.setTextColor(TFT_GREEN);
        tft.println("Success!");
        tft.setCursor(10, 130);
        tft.println("Rebooting...");
      } else {
        Update.printError(Serial);
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(10, 10);
        tft.println("Update Failed!");
        setBuiltinLED(255, 0, 0);  // Red for error
      }
    }
  });
  
  otaServer.begin();
  Serial.println("OTA Web Server started on port 80");
}
