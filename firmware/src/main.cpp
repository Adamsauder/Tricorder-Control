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
#define FRAME_DELAY_MS 66  // ~15 FPS (1000ms / 15)
#define VIDEO_BUFFER_SIZE 8192

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
JPEGDEC jpeg;

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
void showVideoFrame();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Tricorder Control System...");
  
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
  
  // Initialize SD Card
  tft.setCursor(10, 40);
  tft.println("Initializing SD...");
  
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) {
    sdCardInitialized = true;
    Serial.println("SD card initialized successfully!");
    tft.setCursor(10, 70);
    tft.println("SD Card: OK");
    
    // Create videos directory if it doesn't exist
    if (!SD.exists(videoDirectory)) {
      SD.mkdir(videoDirectory);
      Serial.println("Created /videos directory");
    }
    
    // List available videos
    listVideos();
  } else {
    Serial.println("SD card initialization failed!");
    tft.setCursor(10, 70);
    tft.setTextColor(TFT_RED);
    tft.println("SD Card: FAIL");
    tft.setTextColor(TFT_WHITE);
  }
  
  delay(1000); // Show status briefly
  
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
          listVideos();
          sendResponse(commandId, "Video list printed to serial");
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
  analogWrite(RGB_LED_R, 255 - r);  // Inverted PWM
  analogWrite(RGB_LED_G, 255 - g);  // Inverted PWM  
  analogWrite(RGB_LED_B, 255 - b);  // Inverted PWM
  
  Serial.printf("Built-in RGB LED set to R:%d G:%d B:%d\n", r, g, b);
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
  
  // Construct full path
  String fullPath = videoDirectory + "/" + filename;
  
  // Check if file exists
  if (!SD.exists(fullPath)) {
    Serial.printf("Video file not found: %s\n", fullPath.c_str());
    return false;
  }
  
  // Open the video file
  videoFile = SD.open(fullPath, FILE_READ);
  if (!videoFile) {
    Serial.printf("Failed to open video file: %s\n", fullPath.c_str());
    return false;
  }
  
  Serial.printf("Starting video playback: %s (Loop: %s)\n", filename.c_str(), loop ? "Yes" : "No");
  
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
  if (!videoPlaying || !videoFile) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Check if it's time for the next frame
  if (currentTime - lastFrameTime >= FRAME_DELAY_MS) {
    showVideoFrame();
    lastFrameTime = currentTime;
    currentFrame++;
  }
}

void showVideoFrame() {
  if (!videoFile || !videoFile.available()) {
    // End of file reached
    if (videoLooping) {
      // Restart video
      videoFile.seek(0);
      currentFrame = 0;
      Serial.println("Looping video...");
    } else {
      // Stop video
      stopVideo();
    }
    return;
  }
  
  // Read frame data
  size_t bytesRead = videoFile.read(videoBuffer, VIDEO_BUFFER_SIZE);
  
  if (bytesRead > 0) {
    // Try to decode as JPEG
    if (jpeg.openRAM(videoBuffer, bytesRead, JPEGDraw)) {
      // Set scale to fit screen
      jpeg.setPixelType(RGB565_BIG_ENDIAN);
      
      // Decode and display
      if (jpeg.decode(0, 0, 0)) {
        // Frame displayed successfully
      } else {
        Serial.println("JPEG decode failed");
      }
      
      jpeg.close();
    } else {
      Serial.println("JPEG open failed");
    }
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
