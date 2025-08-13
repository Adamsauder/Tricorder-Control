/*
 * Enhanced Tricorder Firmware - With Persistent Configuration and Web Interface
 * ESP32-based tricorder with video playback, LED control, and web configuration
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <JPEGDEC.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "TricorderConfig.h"

// Pin definitions for ESP32-2432S032C-I
#define LED_PIN 21         // NeoPixel data pin (external connection) - IO21
#define NUM_LEDS 3         // Number of NeoPixels in strip (3 front LEDs)
#define TFT_BL 27          // TFT backlight pin
#define LED_POWER_EN 22    // LED strip power enable pin (DC-DC converter)

// LED Type Configuration - Change this to match your hardware
// Uncomment ONE of the following lines:
#define LED_TYPE_RGB       // 3-channel RGB LEDs (WS2812B, WS2811, etc.)
// #define LED_TYPE_RGBW   // 4-channel RGBW LEDs (SK6812, WS2814, etc.)

// LED Strip Configuration
#ifdef LED_TYPE_RGB
  #define CHANNELS_PER_LED 3  // RGB = 3 channels
  #define LED_CHIPSET WS2812B
  #define LED_COLOR_ORDER GRB
#elif defined(LED_TYPE_RGBW)
  #define CHANNELS_PER_LED 4  // RGBW = 4 channels  
  #define LED_CHIPSET SK6812
  #define LED_COLOR_ORDER GRBW
#else
  #error "Must define either LED_TYPE_RGB or LED_TYPE_RGBW"
#endif

// SD Card pins (typical SPI configuration for ESP32-2432S032C)
#define SD_CS 5            // SD card chip select
#define SD_MOSI 23         // SD card MOSI
#define SD_MISO 19         // SD card MISO
#define SD_SCLK 18         // SD card SCLK

// Built-in RGB LED pins
#define RGB_LED_R 4        // Red channel
#define RGB_LED_G 16       // Green channel  
#define RGB_LED_B 17       // Blue channel

// Battery monitoring
#define BATTERY_PIN 39     // ADC pin for battery voltage (ADC1_CH3) - GPIO39 (was GPIO34)
#define BATTERY_VOLTAGE_DIVIDER 82.0  // Actual measured voltage divider ratio (4.1V battery → 0.05V ADC)
#define BATTERY_MAX_VOLTAGE 4.2      // Maximum battery voltage (for 100%)
#define BATTERY_MIN_VOLTAGE 3.0      // Minimum battery voltage (for 0%)

// Hardware reset pins and settings
#define RESET_PIN 12          // Primary reset pin (short to ground during boot)
#define RESET_PIN_2 13        // Secondary reset pin (alternative)
#define BOOT_BUTTON_PIN 0     // Boot button for runtime reset (IO0)
#define RESET_HOLD_TIME 3000  // Time to hold reset pin during boot (3 seconds)
#define BOOT_HOLD_TIME 5000   // Time to hold boot button during runtime (5 seconds)
#define RESET_BLINK_COUNT 6   // Number of LED blinks to indicate reset mode

// Video playback settings
#define FRAME_DELAY_MS 33  // ~30 FPS (1000ms / 30)
#define VIDEO_BUFFER_SIZE 65536  // 64KB buffer - reduced from 128KB due to ESP32 memory constraints

// Network settings
#define UDP_PORT 5000      // Port for UDP status broadcasts

// Enhanced Configuration System
TricorderConfig tricorderConfig;

// Global configuration variables (loaded from tricorderConfig)
String deviceId;
String firmwareVersion = "Enhanced Tricorder v2.0";

// Forward declaration of LED array (defined later)
extern CRGB leds[NUM_LEDS];

// Helper functions for LED color handling
void setLEDColor(int index, int r, int g, int b, int w = 0) {
  if (index < 0 || index >= NUM_LEDS) return;
  
  #ifdef LED_TYPE_RGB
    // RGB mode: ignore white channel
    leds[index] = CRGB(r, g, b);
  #elif defined(LED_TYPE_RGBW)
    // RGBW mode: FastLED handles RGBW with special syntax
    leds[index] = CRGB(r, g, b);
    // Note: For true RGBW support, you may need to use FastLED's raw buffer access
    // This is a simplified version that works with most RGBW strips
  #endif
}

void setAllLEDs(int r, int g, int b, int w = 0) {
  #ifdef LED_TYPE_RGB
    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  #elif defined(LED_TYPE_RGBW)
    for (int i = 0; i < NUM_LEDS; i++) {
      setLEDColor(i, r, g, b, w);
    }
  #endif
}

// Hardware objects
CRGB leds[NUM_LEDS];
TFT_eSPI tft = TFT_eSPI();
WiFiUDP udp;
WebServer webServer(80);
JPEGDEC jpeg;

// Dual-Core Task Handles
TaskHandle_t networkTaskHandle = NULL;
TaskHandle_t videoTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;

// Inter-core communication queues
QueueHandle_t ledCommandQueue = NULL;
QueueHandle_t networkCommandQueue = NULL;
QueueHandle_t videoCommandQueue = NULL;

// LED Command Structure
struct LEDCommand {
  enum Type { SET_COLOR, SET_BRIGHTNESS, SET_INDIVIDUAL, SCANNER_EFFECT, PULSE_EFFECT };
  Type type;
  int r, g, b, w;  // Added white channel for RGBW support
  int brightness;
  int ledIndex;
  int delayMs;
  int duration;
};

// Network Command Structure  
struct NetworkCommand {
  String data;
  IPAddress remoteIP;
  uint16_t remotePort;
};

// Video Command Structure
struct VideoCommand {
  enum Type { PLAY_VIDEO, DISPLAY_IMAGE, STOP_VIDEO };
  Type type;
  char filename[64];  // Fixed-size char array instead of String
  bool loop;
};

// AsyncWebServer otaServer(80);  // Web server for OTA updates - commented out for now

// Video playback objects
File videoFile;
uint8_t* videoBuffer;
size_t videoBufferSize = 0;  // Actual allocated buffer size

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

// Boot button reset monitoring (runtime)
bool bootButtonPressed = false;
unsigned long bootButtonPressStart = 0;
bool resetInProgress = false;
String frameFiles[30]; // Store up to 30 frame files
bool isAnimatedSequence = false;

// Timing variables
unsigned long lastStatusSend = 0;
const unsigned long STATUS_INTERVAL = 10000; // Send status every 10 seconds

// Video frame callback
int JPEGDraw(JPEGDRAW *pDraw) {
  // Draw the JPEG frame to the TFT display
  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  return 1;
}

// Function declarations
void handleUDPCommands();
void setLEDColorCommand(int r, int g, int b, int w);
void setLEDBrightness(int brightness);
void setIndividualLED(int ledIndex, int r, int g, int b);
void scannerEffect(int r, int g, int b, int delayMs = 100);
void pulseEffect(int r, int g, int b, int duration = 2000);
void setBuiltinLED(int r, int g, int b);
void sendResponse(String commandId, String result);
void sendStatus(String commandId);
void sendPeriodicStatus();
bool initializeSDCard();
bool playVideo(String filename, bool loop = false);
void stopVideo();
void updateVideoPlayback();
bool listVideos();
String getVideoList();
void showVideoFrame();
bool displayStaticImage(String filename);
bool displayBootImage(String filename);
void displayInitializationScreen();
void updateBootScreenWithStatus();
void displaySystemStatus();

// Battery monitoring functions
float readBatteryVoltage();
int getBatteryPercentage();
String getBatteryStatus();
void initializeBatteryMonitoring();

// Hardware reset functions
bool checkHardwareReset();
void performHardwareReset();
void blinkResetIndicator();
bool checkResetPinShorted();
void checkBootButtonReset();

// Dual-core task functions
void ledTask(void *pvParameters);
void networkTask(void *pvParameters);
void videoTask(void *pvParameters);
void processNetworkCommand(String jsonCommand);

// Enhanced web server functions
void setupWebServer();
void handleRoot();
void handleConfigPage();
void handleGetConfig();
void handleSetConfig();
void handleGetStatus();
void handleFactoryReset();
void handleRestart();
void handleGetVideos();
void handleFileUpload();
void handleNotFound();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Prop Control System...");
  
  // Initialize reset pins first (avoid using GPIO0 which interferes with bootloader)
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN_2, INPUT_PULLUP);
  
  // Initialize boot button for runtime reset monitoring (safe to use after boot)
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  
  // Check for hardware reset request BEFORE any other initialization
  if (checkHardwareReset()) {
    Serial.println("Hardware reset detected - performing factory reset");
    performHardwareReset();
    return; // Device will restart after reset
  }
  
  // Rapid reboot reset removed to prevent boot loops
  
  // Allocate video buffer with fallback sizes
  Serial.printf("Free heap before buffer allocation: %d bytes\n", ESP.getFreeHeap());
  
  // Try different buffer sizes in order of preference
  size_t bufferSizes[] = {VIDEO_BUFFER_SIZE, 32768, 16384, 8192}; // 64KB, 32KB, 16KB, 8KB
  size_t actualBufferSize = 0;
  
  for (int i = 0; i < 4; i++) {
    videoBuffer = (uint8_t*)malloc(bufferSizes[i]);
    if (videoBuffer) {
      actualBufferSize = bufferSizes[i];
      videoBufferSize = actualBufferSize;  // Store globally
      Serial.printf("Successfully allocated %d bytes for video buffer\n", actualBufferSize);
      break;
    } else {
      Serial.printf("Failed to allocate %d bytes, trying smaller size...\n", bufferSizes[i]);
    }
  }
  
  if (!videoBuffer) {
    Serial.println("CRITICAL: Failed to allocate any video buffer!");
    // Try one more time with a very small buffer
    videoBuffer = (uint8_t*)malloc(4096); // 4KB emergency buffer
    if (videoBuffer) {
      actualBufferSize = 4096;
      videoBufferSize = actualBufferSize;  // Store globally
      Serial.println("Emergency 4KB buffer allocated");
    } else {
      Serial.println("FATAL: Cannot allocate even 4KB buffer - system may be unstable");
      videoBufferSize = 0;
    }
  }
  
  Serial.printf("Final buffer size: %d bytes\n", actualBufferSize);
  Serial.printf("Free heap after buffer allocation: %d bytes\n", ESP.getFreeHeap());
  
  // Enable LED strip power (DC-DC converter)
  pinMode(LED_POWER_EN, OUTPUT);
  digitalWrite(LED_POWER_EN, HIGH);  // Enable power to LED strip
  Serial.println("LED power enabled (pin 22)");
  delay(100); // Allow power to stabilize
  
  // Initialize LEDs
  FastLED.addLeds<LED_CHIPSET, LED_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  
  // Initialize built-in RGB LED pins
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  
  // Initialize battery monitoring
  initializeBatteryMonitoring();
  
  // Set built-in LED to blue during boot
  setBuiltinLED(0, 0, 255);
  
  // Create inter-core communication queues BEFORE creating tasks
  ledCommandQueue = xQueueCreate(10, sizeof(LEDCommand));
  networkCommandQueue = xQueueCreate(20, sizeof(NetworkCommand));
  videoCommandQueue = xQueueCreate(5, sizeof(VideoCommand));
  
  if (!ledCommandQueue || !networkCommandQueue || !videoCommandQueue) {
    Serial.println("FATAL: Failed to create communication queues!");
    while (true) {
      setBuiltinLED(255, 0, 0); // Red error indication
      delay(1000);
    }
  }
  
  Serial.println("Communication queues created successfully");
  
  // Initialize display
  tft.init();
  tft.setRotation(0);  // 90° counter-clockwise from original rotation(1)
  
  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255); // Full brightness
  
  // Load and display boot background image
  bool bootImageLoaded = false;
  if (SD.begin(SD_CS)) {
    bootImageLoaded = displayBootImage("/boot.jpg");
    if (!bootImageLoaded) {
      // Try alternative locations
      bootImageLoaded = displayBootImage("/videos/boot.jpg");
    }
  }
  
  if (!bootImageLoaded) {
    // Fallback to black background if boot image not found
    tft.fillScreen(TFT_BLACK);
  }
  
  // Display initialization screen
  displayInitializationScreen();
  
  // Clear any existing boot count preferences to prevent boot loops
  Preferences prefs;
  if (prefs.begin("boot_count", false)) {
    prefs.clear();
    prefs.end();
    Serial.println("Cleared boot count preferences");
  }
  
  // Create dual-core tasks
  Serial.println("Creating dual-core tasks...");
  Serial.printf("Setup running on Core: %d\n", xPortGetCoreID());
  
  // Create LED task on Core 1 (high priority for real-time response)
  xTaskCreatePinnedToCore(
    ledTask,           // Task function
    "LED_Task",        // Task name
    4096,              // Stack size
    NULL,              // Parameters
    3,                 // Priority (high)
    &ledTaskHandle,    // Task handle
    1                  // Core 1 (APP_CPU)
  );
  
  // Create Network task on Core 0 (background processing)
  xTaskCreatePinnedToCore(
    networkTask,       // Task function
    "Network_Task",    // Task name
    8192,              // Stack size (larger for JSON processing)
    NULL,              // Parameters
    2,                 // Priority (medium)
    &networkTaskHandle, // Task handle
    0                  // Core 0 (PRO_CPU)
  );
  
  // Create Video task on Core 0 (background processing)
  xTaskCreatePinnedToCore(
    videoTask,         // Task function
    "Video_Task",      // Task name
    8192,              // Stack size (larger for video processing)
    NULL,              // Parameters
    1,                 // Priority (low)
    &videoTaskHandle,  // Task handle
    0                  // Core 0 (PRO_CPU)
  );
  
  // Wait a moment for tasks to initialize
  delay(500);
  
  if (ledTaskHandle && networkTaskHandle && videoTaskHandle) {
    Serial.println("✓ All dual-core tasks created successfully!");
    setBuiltinLED(0, 255, 0); // Green success indication
    
    // Startup LED effect via task
    LEDCommand startupEffect;
    startupEffect.type = LEDCommand::SCANNER_EFFECT;
    startupEffect.r = 0;
    startupEffect.g = 255;
    startupEffect.b = 0;
    startupEffect.delayMs = 150;
    xQueueSend(ledCommandQueue, &startupEffect, 0);
  } else {
    Serial.println("✗ Failed to create some tasks!");
    setBuiltinLED(255, 255, 0); // Yellow warning indication
  }
  
  // Display initialization screen
  displayInitializationScreen();
  
  // Initialize Enhanced Configuration System
  Serial.println("Initializing configuration system...");
  if (!tricorderConfig.begin()) {
    Serial.println("Failed to initialize configuration - using defaults");
    setBuiltinLED(255, 255, 0); // Yellow warning
  } else {
    Serial.println("Configuration system initialized successfully");
    
    // Initialize global variables from configuration
    deviceId = String(tricorderConfig.getPropId());
    
    // Apply configuration settings
    ledBrightness = tricorderConfig.getBrightness();
    FastLED.setBrightness(ledBrightness);
    
    // Update display brightness
    uint8_t displayBrightness = tricorderConfig.getDisplayBrightness();
    ledcWrite(0, displayBrightness);
    
    Serial.printf("Loaded configuration: %s (%s)\n", 
                  tricorderConfig.getDeviceLabel(), 
                  tricorderConfig.getPropId());
  }
  
  // Initialize WiFi using configuration
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  
  // Set hostname from configuration
  WiFi.setHostname(tricorderConfig.getHostname());
  
  // Use WiFi credentials from configuration
  WiFi.begin(tricorderConfig.getWiFiSSID(), tricorderConfig.getWiFiPassword());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) { // 20 second timeout
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    
    // Initialize UDP using configuration
    uint16_t udpPort = tricorderConfig.getUdpPort();
    udp.begin(udpPort);
    Serial.printf("UDP server listening on port %d\n", udpPort);
    
    // Initialize Web Server
    setupWebServer();
    webServer.begin();
    Serial.printf("Web server started on port %d\n", tricorderConfig.getWebPort());
    
    // Start mDNS
    String hostname = tricorderConfig.getHostname();
    if (MDNS.begin(hostname.c_str())) {
      Serial.println("mDNS responder started");
      MDNS.addService("tricorder", "udp", udpPort);
      MDNS.addService("http", "tcp", tricorderConfig.getWebPort());
    }
    
    // Set built-in LED to green when connected
    setBuiltinLED(0, 255, 0);
    
    // WiFi connection successful
  } else {
    Serial.println("\nFailed to connect to WiFi - Starting Access Point for configuration");
    
    // Start Access Point mode for configuration
    WiFi.mode(WIFI_AP);
    
    // Create AP with device-specific name and default password
    String apName = "Tricorder-" + String(deviceId);
    const char* apPassword = "tricorder123"; // Default password for emergency access
    
    Serial.printf("Starting Access Point: %s\n", apName.c_str());
    Serial.printf("Default password: %s\n", apPassword);
    
    if (WiFi.softAP(apName.c_str(), apPassword)) {
      Serial.println("Access Point started successfully!");
      Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
      Serial.println("Connect to this AP to configure WiFi settings");
      Serial.println("Default web interface: http://192.168.4.1");
      
      // Initialize Web Server even in AP mode
      setupWebServer();
      webServer.begin();
      Serial.printf("Web server started on port %d (AP mode)\n", tricorderConfig.getWebPort());
      
      wifiConnected = false; // Mark as not connected to station
    } else {
      Serial.println("Failed to start Access Point!");
    }
    
    // Set built-in LED to orange when in AP mode (red + green)
    setBuiltinLED(255, 128, 0);
  }
  
  // Initialize SD Card
  Serial.println("Initializing SD card...");
  if (SD.begin(SD_CS)) {
    sdCardInitialized = true;
    Serial.println("SD card initialized successfully!");
    
    // Create videos directory if it doesn't exist
    if (!SD.exists(videoDirectory)) {
      SD.mkdir(videoDirectory);
      Serial.println("Created /videos directory");
    }
    
    // List available videos
    listVideos();
  } else {
    Serial.println("SD card initialization failed!");
  }
  
  // Update the boot screen with final status instead of separate screen
  updateBootScreenWithStatus();
  
  Serial.println("Setup complete!");
}

void loop() {
  // Main loop now handles web server and system monitoring
  // Most work is done by dedicated tasks on both cores
  
  // Check for boot button reset (runtime monitoring)
  checkBootButtonReset();
  
  // Handle web server requests
  if (wifiConnected) {
    webServer.handleClient();
  }
  
  // Check WiFi connection and notify network task if status changes
  static bool lastWifiStatus = false;
  bool currentWifiStatus = (WiFi.status() == WL_CONNECTED);
  
  if (currentWifiStatus != lastWifiStatus) {
    wifiConnected = currentWifiStatus;
    if (!currentWifiStatus) {
      Serial.println("WiFi disconnected!");
      setBuiltinLED(255, 0, 0); // Red for disconnected
    } else {
      Serial.println("WiFi reconnected!");
      setBuiltinLED(0, 255, 0); // Green for connected
    }
    lastWifiStatus = currentWifiStatus;
  }
  
  // Monitor system health
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 30000) { // Every 30 seconds
    Serial.printf("System Health - Free Heap: %d bytes, Core: %d\n", 
                  ESP.getFreeHeap(), xPortGetCoreID());
    
    // Check if tasks are still running
    if (ledTaskHandle == NULL || networkTaskHandle == NULL || videoTaskHandle == NULL) {
      Serial.println("WARNING: One or more tasks have crashed!");
      setBuiltinLED(255, 255, 0); // Yellow for warning
    }
    
    lastHealthCheck = millis();
  }
  
  // Very short delay to prevent watchdog issues
  delay(10);
}

// Initialization Screen Function
void displayInitializationScreen() {
  // Just display the boot image without any text overlay
  // The text will be added later by updateBootScreenWithStatus()
  // This prevents text overlap issues
}

void updateBootScreenWithStatus() {
  // Update the boot screen with status info using the existing black center area
  // Don't clear anything - just write text in the designated black space
  
  // Set text properties for the LCARS center display area
  tft.setTextSize(1);
  int textX = 50;      // Left margin for center box
  int textY = 70;      // Top of center box area  
  int lineHeight = 14; // Line spacing
  int currentLine = 0;
  
  // Header - Device info
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(textX, textY + (currentLine * lineHeight));
  tft.printf("%s", tricorderConfig.getDeviceLabel());
  currentLine += 1;
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(textX, textY + (currentLine * lineHeight));
  tft.printf("ID: %s", deviceId.c_str());
  currentLine += 2;
  
  // Network status
  if (WiFi.getMode() == WIFI_STA && wifiConnected) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.println("WiFi: CONNECTED");
    currentLine += 1;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.printf("IP: %s", WiFi.localIP().toString().c_str());
    currentLine += 1;
    
  } else if (WiFi.getMode() == WIFI_AP) {
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.println("WiFi: ACCESS POINT");
    currentLine += 1;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.printf("Tricorder-%s", deviceId.c_str());
    currentLine += 1;
    
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.println("Pass: tricorder123");
    currentLine += 1;
    
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.println("WiFi: DISCONNECTED");
    currentLine += 1;
  }
  
  // SD Card status
  currentLine += 1; 
  if (sdCardInitialized) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.println("SD Card: OK");
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(textX, textY + (currentLine * lineHeight));
    tft.println("SD Card: FAILED");
  }
  currentLine += 2;
  
  // Reset instructions
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(textX, textY + (currentLine * lineHeight));
  tft.println("Reset: Hold BOOT 5s");
  currentLine += 2;
  
  // Ready indicator
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(textX, textY + (currentLine * lineHeight));
  tft.println("SYSTEM READY");
}

// LED Task - Runs on Core 1 for real-time LED control
void ledTask(void *pvParameters) {
  Serial.printf("LED Task starting on Core: %d\n", xPortGetCoreID());
  
  // Re-initialize FastLED on this core to ensure proper multi-core operation
  FastLED.addLeds<LED_CHIPSET, LED_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  Serial.println("FastLED re-initialized on LED task core");
  
  LEDCommand command;
  
  while (true) {
    // Wait for LED commands from other tasks/cores
    if (xQueueReceive(ledCommandQueue, &command, portMAX_DELAY) == pdPASS) {
      Serial.printf("LED Task received command type: %d\n", command.type);
      switch (command.type) {
        case LEDCommand::SET_COLOR:
          Serial.printf("Setting LED color to R:%d G:%d B:%d W:%d\n", command.r, command.g, command.b, command.w);
          currentColor = CRGB(command.r, command.g, command.b);
          setAllLEDs(command.r, command.g, command.b, command.w);
          FastLED.show();
          Serial.println("LED color updated and displayed");
          break;
          
        case LEDCommand::SET_BRIGHTNESS:
          ledBrightness = constrain(command.brightness, 0, 255);
          FastLED.setBrightness(ledBrightness);
          FastLED.show();
          break;
          
        case LEDCommand::SET_INDIVIDUAL:
          if (command.ledIndex >= 0 && command.ledIndex < NUM_LEDS) {
            setLEDColor(command.ledIndex, command.r, command.g, command.b, command.w);
            FastLED.show();
          }
          break;
          
        case LEDCommand::SCANNER_EFFECT:
          {
            // Scan left to right
            for (int i = 0; i < NUM_LEDS; i++) {
              setAllLEDs(0, 0, 0, 0);  // Clear all LEDs
              setLEDColor(i, command.r, command.g, command.b, command.w);
              FastLED.show();
              delay(command.delayMs);
            }
            // Scan right to left
            for (int i = NUM_LEDS - 2; i >= 1; i--) {
              setAllLEDs(0, 0, 0, 0);  // Clear all LEDs
              setLEDColor(i, command.r, command.g, command.b, command.w);
              FastLED.show();
              delay(command.delayMs);
            }
          }
          break;
          
        case LEDCommand::PULSE_EFFECT:
          {
            CRGB color = CRGB(command.r, command.g, command.b);
            unsigned long startTime = millis();
            
            while (millis() - startTime < command.duration) {
              float progress = (millis() - startTime) / (float)command.duration;
              float brightness = (sin(progress * 2 * PI) + 1) / 2; // 0 to 1
              
              CRGB dimmedColor = color;
              dimmedColor.nscale8(255 * brightness);
              
              fill_solid(leds, NUM_LEDS, dimmedColor);
              FastLED.show();
              delay(20);
            }
          }
          break;
      }
    }
    
    // Small yield to prevent watchdog issues
    taskYIELD();
  }
}

// Network Task - Runs on Core 0 for UDP/WiFi handling
void networkTask(void *pvParameters) {
  Serial.printf("Network Task starting on Core: %d\n", xPortGetCoreID());
  
  // WiFi is already initialized in setup(), just wait for connection
  while (!wifiConnected) {
    delay(100);
  }
  
  Serial.println("Network task: WiFi connected, starting UDP handling");
  
  // Initialize SD Card
  if (SD.begin(SD_CS)) {
    sdCardInitialized = true;
    Serial.println("SD card initialized successfully!");
    
    // Create videos directory if it doesn't exist
    if (!SD.exists(videoDirectory)) {
      SD.mkdir(videoDirectory);
      Serial.println("Created /videos directory");
    }
    
    // List available videos
    listVideos();
  } else {
    Serial.println("SD card initialization failed!");
  }
  
  // Main network loop
  unsigned long lastStatusSend = 0;
  
  while (true) {
    // Handle UDP commands if connected
    if (wifiConnected) {
      int packetSize = udp.parsePacket();
      if (packetSize) {
        char incomingPacket[255];
        int len = udp.read(incomingPacket, 255);
        if (len > 0) {
          incomingPacket[len] = 0;
        }
        
        // Process the command (simplified version)
        processNetworkCommand(String(incomingPacket));
      }
      
      // Send periodic status to server (every 10 seconds)
      unsigned long currentTime = millis();
      if (currentTime - lastStatusSend > STATUS_INTERVAL) {
        sendPeriodicStatus();
        lastStatusSend = currentTime;
      }
    }
    
    // Small delay to prevent overwhelming the network
    delay(5);
  }
}

// Video Task - Runs on Core 0 for video processing
void videoTask(void *pvParameters) {
  Serial.printf("Video Task starting on Core: %d\n", xPortGetCoreID());
  
  VideoCommand command;
  
  while (true) {
    // Wait for video commands
    if (xQueueReceive(videoCommandQueue, &command, 100) == pdPASS) {
      Serial.printf("Video Task received command type: %d, filename: %s\n", command.type, command.filename);
      switch (command.type) {
        case VideoCommand::PLAY_VIDEO:
          Serial.printf("Video Task: Starting video playback: %s\n", command.filename);
          playVideo(String(command.filename), command.loop);
          break;
          
        case VideoCommand::DISPLAY_IMAGE:
          {
            Serial.printf("Video Task: Displaying image: %s\n", command.filename);
            bool result = displayStaticImage(String(command.filename));
            Serial.printf("Video Task: Image display result: %s\n", result ? "SUCCESS" : "FAILED");
          }
          break;
          
        case VideoCommand::STOP_VIDEO:
          Serial.println("Video Task: Stopping video");
          stopVideo();
          break;
      }
    }
    
    // Update video playback if playing
    if (videoPlaying) {
      updateVideoPlayback();
    }
    
    // Small delay to prevent overwhelming the video system
    delay(10);
  }
}

// Simplified network command processor for network task
void processNetworkCommand(String jsonCommand) {
  Serial.printf("Network Task: Received JSON: %s\n", jsonCommand.c_str());
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonCommand);
  
  if (!error) {
    String action = doc["action"];
    String commandId = doc["commandId"];
    
    Serial.printf("Network Task: Parsed action='%s', commandId='%s'\n", action.c_str(), commandId.c_str());
    
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
      udp.print(responseStr);
      udp.endPacket();
    }
    // Handle LED commands by sending to LED task
    else if (action == "set_led_color") {
      LEDCommand ledCmd;
      ledCmd.type = LEDCommand::SET_COLOR;
      ledCmd.r = doc["r"];
      ledCmd.g = doc["g"];
      ledCmd.b = doc["b"];
      ledCmd.w = doc.containsKey("w") ? doc["w"] : 0;  // White channel optional for RGB compatibility
      
      Serial.printf("Network task sending LED command R:%d G:%d B:%d W:%d\n", ledCmd.r, ledCmd.g, ledCmd.b, ledCmd.w);
      BaseType_t result = xQueueSend(ledCommandQueue, &ledCmd, 0);
      if (result == pdPASS) {
        Serial.println("LED command successfully queued");
      } else {
        Serial.println("Failed to queue LED command - queue may be full");
      }
      
      sendResponse(commandId, "LED color set");
    }
    else if (action == "set_builtin_led") {
      int r = doc["r"];
      int g = doc["g"];
      int b = doc["b"];
      setBuiltinLED(r, g, b);
      
      sendResponse(commandId, "Built-in LED color set");
    }
    // Handle video commands by sending to video task
    else if (action == "play_video") {
      String filename;
      bool loop = false;
      
      // Check if filename is in parameters object (new format) or directly (legacy)
      if (doc.containsKey("parameters")) {
        if (doc["parameters"].containsKey("filename")) {
          filename = doc["parameters"]["filename"].as<String>();
        }
        if (doc["parameters"].containsKey("loop")) {
          loop = doc["parameters"]["loop"];
        }
      } else {
        // Legacy format
        if (doc.containsKey("filename")) {
          filename = doc["filename"].as<String>();
        }
        if (doc.containsKey("loop")) {
          loop = doc["loop"];
        }
      }
      
      VideoCommand vidCmd;
      vidCmd.type = VideoCommand::PLAY_VIDEO;
      strncpy(vidCmd.filename, filename.c_str(), sizeof(vidCmd.filename) - 1);
      vidCmd.filename[sizeof(vidCmd.filename) - 1] = '\0';  // Ensure null termination
      vidCmd.loop = loop;
      BaseType_t result = xQueueSend(videoCommandQueue, &vidCmd, pdMS_TO_TICKS(1000));
      
      if (result == pdPASS) {
        sendResponse(commandId, "Video playback started");
      } else {
        sendResponse(commandId, "Failed to queue video command");
      }
    }
    else if (action == "display_image") {
      String filename;
      
      // Check if filename is in parameters object (new format) or directly (legacy)
      if (doc.containsKey("parameters") && doc["parameters"].containsKey("filename")) {
        filename = doc["parameters"]["filename"].as<String>();
      } else if (doc.containsKey("filename")) {
        filename = doc["filename"].as<String>();
      } else {
        filename = ""; // Empty filename
      }
      
      Serial.printf("Network Task: display_image command, filename JSON value: '%s'\n", filename.c_str());
      
      VideoCommand vidCmd;
      vidCmd.type = VideoCommand::DISPLAY_IMAGE;
      strncpy(vidCmd.filename, filename.c_str(), sizeof(vidCmd.filename) - 1);
      vidCmd.filename[sizeof(vidCmd.filename) - 1] = '\0';  // Ensure null termination
      
      Serial.printf("Network Task: Queuing display command with filename: '%s'\n", vidCmd.filename);
      BaseType_t result = xQueueSend(videoCommandQueue, &vidCmd, pdMS_TO_TICKS(1000));
      
      if (result == pdPASS) {
        sendResponse(commandId, "Image command queued");
      } else {
        sendResponse(commandId, "Failed to queue image command");
      }
    }
    else if (action == "status") {
      sendStatus(commandId);
    }
    else if (action == "get_battery") {
      JsonDocument batteryDoc;
      batteryDoc["commandId"] = commandId;
      batteryDoc["deviceId"] = deviceId;
      batteryDoc["batteryVoltage"] = readBatteryVoltage();
      batteryDoc["batteryPercentage"] = getBatteryPercentage();
      batteryDoc["batteryStatus"] = getBatteryStatus();
      
      String response;
      serializeJson(batteryDoc, response);
      
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.write((const uint8_t*)response.c_str(), response.length());
      udp.endPacket();
    }
    else if (action == "debug_adc") {
      // Debug ADC reading with detailed information
      JsonDocument debugDoc;
      debugDoc["commandId"] = commandId;
      debugDoc["deviceId"] = deviceId;
      
      // Test all common ADC pins
      analogSetAttenuation(ADC_11db);  // 0-3.3V range
      analogReadResolution(12);        // 12-bit resolution
      
      int testPins[] = {34, 35, 36, 39, 32, 33};
      JsonArray adcReadings = debugDoc.createNestedArray("adcReadings");
      
      for (int i = 0; i < 6; i++) {
        int pin = testPins[i];
        int rawReading = analogRead(pin);
        float voltage = (rawReading / 4095.0) * 3.3;
        
        JsonObject reading = adcReadings.createNestedObject();
        reading["pin"] = pin;
        reading["rawValue"] = rawReading;
        reading["voltage"] = voltage;
        reading["isPrimaryPin"] = (pin == BATTERY_PIN);
      }
      
      // Primary pin detailed reading
      int primaryRaw = analogRead(BATTERY_PIN);
      float primaryVoltage = (primaryRaw / 4095.0) * 3.3;
      float calculatedBattery = primaryVoltage * BATTERY_VOLTAGE_DIVIDER;
      
      debugDoc["primaryPin"] = BATTERY_PIN;
      debugDoc["primaryRawADC"] = primaryRaw;
      debugDoc["primaryVoltageADC"] = primaryVoltage;
      debugDoc["voltageDivider"] = BATTERY_VOLTAGE_DIVIDER;
      debugDoc["calculatedBatteryVoltage"] = calculatedBattery;
      debugDoc["adcResolution"] = 12;
      debugDoc["adcAttenuation"] = "11dB (0-3.3V)";
      
      String response;
      serializeJson(debugDoc, response);
      
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.write((const uint8_t*)response.c_str(), response.length());
      udp.endPacket();
    }
  }
}

void handleUDPCommands() {
  // This function is now simplified since network handling is done by networkTask
  // Keep it for legacy compatibility but it does minimal work
}

void setLEDColorCommand(int r, int g, int b, int w = 0) {
  // Send command to LED task for thread-safe execution
  LEDCommand cmd;
  cmd.type = LEDCommand::SET_COLOR;
  cmd.r = r;
  cmd.g = g;
  cmd.b = b;
  cmd.w = w;
  
  if (ledCommandQueue) {
    xQueueSend(ledCommandQueue, &cmd, 0);
  }
}

void setLEDBrightness(int brightness) {
  // Send command to LED task for thread-safe execution
  LEDCommand cmd;
  cmd.type = LEDCommand::SET_BRIGHTNESS;
  cmd.brightness = brightness;
  
  if (ledCommandQueue) {
    xQueueSend(ledCommandQueue, &cmd, 0);
  }
}

// Set individual LED color (0-2 for the 3 front LEDs)
void setIndividualLED(int ledIndex, int r, int g, int b) {
  // Send command to LED task for thread-safe execution
  LEDCommand cmd;
  cmd.type = LEDCommand::SET_INDIVIDUAL;
  cmd.ledIndex = ledIndex;
  cmd.r = r;
  cmd.g = g;
  cmd.b = b;
  
  if (ledCommandQueue) {
    xQueueSend(ledCommandQueue, &cmd, 0);
  }
}

// Create a scanner/Kitt effect across the 3 LEDs
void scannerEffect(int r, int g, int b, int delayMs) {
  // Send command to LED task for thread-safe execution
  LEDCommand cmd;
  cmd.type = LEDCommand::SCANNER_EFFECT;
  cmd.r = r;
  cmd.g = g;
  cmd.b = b;
  cmd.delayMs = delayMs;
  
  if (ledCommandQueue) {
    xQueueSend(ledCommandQueue, &cmd, 0);
  }
}

// Pulse all LEDs (breathing effect)
void pulseEffect(int r, int g, int b, int duration) {
  // Send command to LED task for thread-safe execution
  LEDCommand cmd;
  cmd.type = LEDCommand::PULSE_EFFECT;
  cmd.r = r;
  cmd.g = g;
  cmd.b = b;
  cmd.duration = duration;
  
  if (ledCommandQueue) {
    xQueueSend(ledCommandQueue, &cmd, 0);
  }
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
  doc["sdCardInitialized"] = sdCardInitialized;
  doc["videoPlaying"] = videoPlaying;
  doc["currentVideo"] = currentVideo;
  doc["videoLooping"] = videoLooping;
  doc["currentFrame"] = currentFrame;
  
  // Battery information
  doc["batteryVoltage"] = readBatteryVoltage();
  doc["batteryPercentage"] = getBatteryPercentage();
  doc["batteryStatus"] = getBatteryStatus();
  
  String response;
  serializeJson(doc, response);
  
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
  
  Serial.printf("Sent status: %s\n", response.c_str());
}

void sendPeriodicStatus() {
  // Send periodic status to server (broadcast to server IP)
  JsonDocument doc;
  doc["deviceId"] = deviceId;
  doc["type"] = "tricorder";
  doc["deviceLabel"] = tricorderConfig.getDeviceLabel();
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
  doc["timestamp"] = millis();
  
  // Include battery information in periodic status
  doc["batteryVoltage"] = readBatteryVoltage();
  doc["batteryPercentage"] = getBatteryPercentage();
  doc["batteryStatus"] = getBatteryStatus();
  
  String statusMsg;
  serializeJson(doc, statusMsg);
  
  // Broadcast to server subnet (try a few common server IPs)
  IPAddress localIP = WiFi.localIP();
  IPAddress serverIP(localIP[0], localIP[1], localIP[2], 24); // Usually .24 for server
  
  udp.beginPacket(serverIP, UDP_PORT);
  udp.write((const uint8_t*)statusMsg.c_str(), statusMsg.length());
  udp.endPacket();
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
  if (fileSize > videoBufferSize) {
    Serial.printf("Frame file too large: %d bytes (max %d)\n", fileSize, videoBufferSize);
    frameFile.close();
    return;
  }
  
  size_t bytesRead = frameFile.read(videoBuffer, fileSize);
  frameFile.close();
  
  if (bytesRead > 0) {
    // Try to decode as JPEG with optimized settings
    if (jpeg.openRAM(videoBuffer, bytesRead, JPEGDraw)) {
      // Configure JPEG decoder for best quality
      jpeg.setPixelType(RGB565_BIG_ENDIAN);
      
      // Get image dimensions
      int width = jpeg.getWidth();
      int height = jpeg.getHeight();
      
      // Calculate centering for smaller images
      int xOffset = (width < 240) ? (240 - width) / 2 : 0;
      int yOffset = (height < 320) ? (320 - height) / 2 : 0;
      
      // Clear screen only when ready to display new frame
      if (currentFrame == 0 || isAnimatedSequence) {
        tft.fillScreen(TFT_BLACK);
      }
      
      // Decode and display
      if (jpeg.decode(xOffset, yOffset, 0)) {
        Serial.printf("SUCCESS: Displayed frame %d/%d: %s (%dx%d)\n", currentFrame + 1, totalFrames, currentFramePath.c_str(), width, height);
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

bool displayStaticImage(String filename) {
  if (!sdCardInitialized) {
    Serial.println("SD card not initialized");
    return false;
  }
  
  // Stop any current video playback
  stopVideo();
  
  // Try to find the exact filename first in root directory
  String fullPath = "/" + filename;
  
  // Check if file exists with the exact name in root
  if (!SD.exists(fullPath)) {
    Serial.printf("File not found with exact name: %s\n", fullPath.c_str());
    
    // Try in videos directory
    fullPath = videoDirectory + "/" + filename;
    if (!SD.exists(fullPath)) {
      Serial.printf("File not found in videos directory: %s\n", fullPath.c_str());
      
      // Try with common JPEG extensions in both directories
      String extensions[] = {".jpg", ".JPG", ".jpeg", ".JPEG"};
      bool found = false;
      
      // First try root directory with extensions
      for (int i = 0; i < 4; i++) {
        String testPath = "/" + filename + extensions[i];
        Serial.printf("Trying root: %s\n", testPath.c_str());
        if (SD.exists(testPath)) {
          fullPath = testPath;
          found = true;
          Serial.printf("Found file in root: %s\n", fullPath.c_str());
          break;
        }
        
        // Also try removing extension if it already has one
        int dotPos = filename.lastIndexOf(".");
        if (dotPos > 0) {
          String baseName = filename.substring(0, dotPos);
          testPath = "/" + baseName + extensions[i];
          Serial.printf("Trying root (base): %s\n", testPath.c_str());
          if (SD.exists(testPath)) {
            fullPath = testPath;
            found = true;
            Serial.printf("Found file in root (base): %s\n", fullPath.c_str());
            break;
          }
        }
      }
      
      // If not found in root, try videos directory with extensions
      if (!found) {
        for (int i = 0; i < 4; i++) {
          String testPath = videoDirectory + "/" + filename + extensions[i];
          Serial.printf("Trying videos: %s\n", testPath.c_str());
          if (SD.exists(testPath)) {
            fullPath = testPath;
            found = true;
            Serial.printf("Found file in videos: %s\n", fullPath.c_str());
            break;
          }
          
          // Also try removing extension if it already has one
          int dotPos = filename.lastIndexOf(".");
          if (dotPos > 0) {
            String baseName = filename.substring(0, dotPos);
            testPath = videoDirectory + "/" + baseName + extensions[i];
            Serial.printf("Trying videos (base): %s\n", testPath.c_str());
            if (SD.exists(testPath)) {
              fullPath = testPath;
              found = true;
              Serial.printf("Found file in videos (base): %s\n", fullPath.c_str());
              break;
            }
          }
        }
      }
      
      if (!found) {
        Serial.printf("JPEG image file not found: %s\n", filename.c_str());
        return false;
      }
    } else {
      Serial.printf("Found file in videos directory: %s\n", fullPath.c_str());
    }
  } else {
    Serial.printf("Found file with exact name in root: %s\n", fullPath.c_str());
  }
  
  Serial.printf("Displaying static image: %s\n", fullPath.c_str());
  
  // Open the file
  File imageFile = SD.open(fullPath, FILE_READ);
  if (!imageFile) {
    Serial.printf("Failed to open image file: %s\n", fullPath.c_str());
    return false;
  }
  
  // Get file size first
  size_t fileSize = imageFile.size();
  Serial.printf("File opened successfully, size: %d bytes\n", fileSize);
  
  if (fileSize == 0) {
    Serial.println("ERROR: File is empty (0 bytes)");
    imageFile.close();
    return false;
  }
  
  if (fileSize > videoBufferSize) {
    Serial.printf("Image file too large: %d bytes (max %d)\n", fileSize, videoBufferSize);
    imageFile.close();
    return false;
  }
  
  // Try to read the file with enhanced diagnostics
  Serial.printf("Attempting to read %d bytes from file...\n", fileSize);
  
  // Check if file is at the beginning
  Serial.printf("File position before read: %d\n", imageFile.position());
  
  // Try a small test read first
  uint8_t testByte;
  size_t testRead = imageFile.read(&testByte, 1);
  Serial.printf("Test read of 1 byte returned: %d bytes (value: 0x%02X)\n", testRead, testRead > 0 ? testByte : 0);
  
  // Reset file position
  imageFile.seek(0);
  Serial.printf("File position after seek(0): %d\n", imageFile.position());
  
  // Check if buffer is valid
  if (!videoBuffer) {
    Serial.println("ERROR: Video buffer is NULL!");
    imageFile.close();
    return false;
  }
  
  // Try reading in smaller chunks to diagnose
  size_t bytesRead = 0;
  size_t chunkSize = min((size_t)1024, fileSize); // Read in 1KB chunks or file size if smaller
  size_t totalBytesToRead = fileSize;
  
  Serial.printf("Reading file in chunks of %d bytes...\n", chunkSize);
  
  while (bytesRead < totalBytesToRead) {
    size_t remainingBytes = totalBytesToRead - bytesRead;
    size_t currentChunkSize = min(chunkSize, remainingBytes);
    
    Serial.printf("Attempting to read chunk: %d bytes at offset %d\n", currentChunkSize, bytesRead);
    size_t chunkBytesRead = imageFile.read(videoBuffer + bytesRead, currentChunkSize);
    Serial.printf("Chunk read result: %d bytes\n", chunkBytesRead);
    
    if (chunkBytesRead == 0) {
      Serial.printf("Read failed at offset %d - SD card or file corruption?\n", bytesRead);
      break;
    }
    
    bytesRead += chunkBytesRead;
    
    if (chunkBytesRead < currentChunkSize) {
      Serial.printf("Partial chunk read: got %d, expected %d\n", chunkBytesRead, currentChunkSize);
      break;
    }
  }
  
  imageFile.close();
  
  Serial.printf("Final read result: %d bytes from file (expected %d)\n", bytesRead, fileSize);
  
  if (bytesRead == 0) {
    Serial.println("ERROR: No bytes read from image file");
    Serial.println("Possible causes:");
    Serial.println("  1. SD card hardware failure");
    Serial.println("  2. File system corruption");
    Serial.println("  3. Insufficient power to SD card");
    Serial.println("  4. Bad SD card connection");
    return false;
  }
  
  if (bytesRead != fileSize) {
    Serial.printf("WARNING: Partial read - got %d bytes, expected %d bytes\n", bytesRead, fileSize);
  }
  
  // Decode JPEG with optimized settings
  Serial.printf("Attempting to decode JPEG: %s (%d bytes)\n", fullPath.c_str(), fileSize);
  if (jpeg.openRAM(videoBuffer, bytesRead, JPEGDraw)) {
    // Configure JPEG decoder for best quality
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    
    // Get image dimensions
    int width = jpeg.getWidth();
    int height = jpeg.getHeight();
    Serial.printf("JPEG dimensions: %dx%d\n", width, height);
    
    // Calculate centering for smaller images
    int xOffset = (width < 240) ? (240 - width) / 2 : 0;
    int yOffset = (height < 320) ? (320 - height) / 2 : 0;
    
    // Clear the screen only when we're ready to display the new image
    tft.fillScreen(TFT_BLACK);
    
    Serial.println("JPEG opened successfully, attempting decode...");
    if (jpeg.decode(xOffset, yOffset, 0)) {
      Serial.println("JPEG decoded successfully");
      jpeg.close();
      return true;
    } else {
      Serial.println("JPEG decode failed");
    }
    jpeg.close();
  } else {
    Serial.println("JPEG open failed");
  }
  
  return false;
}

bool displayBootImage(String filename) {
  // Initialize SPI and SD card for boot image
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card not available for boot image");
    return false;
  }
  
  // Check if file exists
  if (!SD.exists(filename)) {
    Serial.printf("Boot image not found: %s\n", filename.c_str());
    return false;
  }
  
  Serial.printf("Loading boot image: %s\n", filename.c_str());
  
  // Open the file
  File imageFile = SD.open(filename, FILE_READ);
  if (!imageFile) {
    Serial.printf("Failed to open boot image: %s\n", filename.c_str());
    return false;
  }
  
  // Get file size
  size_t fileSize = imageFile.size();
  Serial.printf("Boot image size: %d bytes\n", fileSize);
  
  if (fileSize == 0) {
    Serial.println("Boot image is empty");
    imageFile.close();
    return false;
  }
  
  if (fileSize > videoBufferSize) {
    Serial.printf("Boot image too large: %d bytes (max %d)\n", fileSize, videoBufferSize);
    imageFile.close();
    return false;
  }
  
  // Check if buffer is allocated
  if (!videoBuffer) {
    Serial.println("Video buffer not available for boot image");
    imageFile.close();
    return false;
  }
  
  // Read the image file in chunks
  size_t bytesRead = 0;
  size_t chunkSize = min((size_t)1024, fileSize);
  
  while (bytesRead < fileSize) {
    size_t remainingBytes = fileSize - bytesRead;
    size_t currentChunkSize = min(chunkSize, remainingBytes);
    
    size_t chunkBytesRead = imageFile.read(videoBuffer + bytesRead, currentChunkSize);
    
    if (chunkBytesRead == 0) {
      Serial.printf("Boot image read failed at offset %d\n", bytesRead);
      break;
    }
    
    bytesRead += chunkBytesRead;
    
    if (chunkBytesRead < currentChunkSize) {
      break;
    }
  }
  
  imageFile.close();
  
  if (bytesRead == 0) {
    Serial.println("Failed to read boot image data");
    return false;
  }
  
  // Decode and display the JPEG - only clear screen right before displaying
  if (jpeg.openRAM(videoBuffer, bytesRead, JPEGDraw)) {
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    
    int width = jpeg.getWidth();
    int height = jpeg.getHeight();
    Serial.printf("Boot image dimensions: %dx%d\n", width, height);
    
    // Calculate centering
    int xOffset = (width < 240) ? (240 - width) / 2 : 0;
    int yOffset = (height < 320) ? (320 - height) / 2 : 0;
    
    // Clear the screen only when we're ready to display the new image
    tft.fillScreen(TFT_BLACK);
    
    if (jpeg.decode(xOffset, yOffset, 0)) {
      Serial.println("Boot image displayed successfully");
      jpeg.close();
      return true;
    } else {
      Serial.println("Boot image JPEG decode failed");
    }
    jpeg.close();
  } else {
    Serial.println("Boot image JPEG open failed");
  }
  
  return false;
}

// Battery monitoring functions
void initializeBatteryMonitoring() {
  Serial.println("=== INITIALIZING BATTERY MONITORING ===");
  
  // Configure ADC pin as input
  pinMode(BATTERY_PIN, INPUT);
  Serial.printf("Battery monitoring pin GPIO%d configured as INPUT\n", BATTERY_PIN);
  
  // Set ADC width to 12 bits
  analogReadResolution(12);
  Serial.println("ADC resolution set to 12 bits (0-4095)");
  
  // Set ADC attenuation for wider voltage range (0-3.3V)
  analogSetAttenuation(ADC_11db);  // ADC_11db = 0-3.3V range
  Serial.println("ADC attenuation set to 11dB (0-3.3V range)");
  
  // Attach pin to ADC
  adcAttachPin(BATTERY_PIN);
  Serial.printf("ADC explicitly attached to GPIO%d\n", BATTERY_PIN);
  
  // Warm up the ADC with multiple reads
  Serial.println("Warming up ADC with multiple reads...");
  for (int i = 0; i < 10; i++) {
    int warmupRead = analogRead(BATTERY_PIN);
    Serial.printf("Warmup read %d: %d (%.3fV)\n", i+1, warmupRead, (warmupRead/4095.0)*3.3);
    delay(50); // Longer delay for ADC to settle
  }
  
  // Test immediate reading
  int testRead = analogRead(BATTERY_PIN);
  float testVoltage = (testRead / 4095.0) * 3.3 * BATTERY_VOLTAGE_DIVIDER;
  Serial.printf("Initial test reading: %d ADC = %.3fV battery\n", testRead, testVoltage);
  
  Serial.println("Battery monitoring initialization complete");
  Serial.println("========================================\n");
}

float readBatteryVoltage() {
  // Read ADC value multiple times for better accuracy
  int adcSum = 0;
  const int samples = 10;
  
  Serial.printf("=== BATTERY MONITORING DEBUG ===\n");
  Serial.printf("Primary battery pin: GPIO%d\n", BATTERY_PIN);
  Serial.printf("Voltage divider ratio: %.1f\n", BATTERY_VOLTAGE_DIVIDER);
  Serial.printf("Expected range: %.1fV - %.1fV\n", BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
  
  // Test multiple ADC pins that might be used for battery monitoring
  int testPins[] = {34, 35, 36, 39, 32, 33}; // Common ADC pins
  String pinNames[] = {"GPIO34 (ADC1_CH6)", "GPIO35 (ADC1_CH7)", "GPIO36 (ADC1_CH0)", "GPIO39 (ADC1_CH3)", "GPIO32 (ADC1_CH4)", "GPIO33 (ADC1_CH5)"};
  
  Serial.println("Testing all possible ADC pins...");
  analogSetAttenuation(ADC_11db);  // 0-3.3V range
  
  for (int i = 0; i < 6; i++) {
    int testPin = testPins[i];
    int reading = analogRead(testPin);
    float voltage = (reading / 4095.0) * 3.3;
    Serial.printf("%s: ADC=%d, Voltage=%.3fV\n", pinNames[i].c_str(), reading, voltage);
  }
  
  // Continue with original pin testing
  Serial.printf("\nFocusing on primary pin GPIO%d...\n", BATTERY_PIN);
  
  // Test different attenuation settings
  Serial.println("Testing ADC configurations...");
  
  // Try different attenuation settings
  analogSetAttenuation(ADC_0db);   // 0-1.1V
  int test_0db = analogRead(BATTERY_PIN);
  Serial.printf("ADC_0db (0-1.1V): %d\n", test_0db);
  
  analogSetAttenuation(ADC_2_5db); // 0-1.5V  
  int test_2_5db = analogRead(BATTERY_PIN);
  Serial.printf("ADC_2_5db (0-1.5V): %d\n", test_2_5db);
  
  analogSetAttenuation(ADC_6db);   // 0-2.2V
  int test_6db = analogRead(BATTERY_PIN);
  Serial.printf("ADC_6db (0-2.2V): %d\n", test_6db);
  
  analogSetAttenuation(ADC_11db);  // 0-3.3V
  int test_11db = analogRead(BATTERY_PIN);
  Serial.printf("ADC_11db (0-3.3V): %d\n", test_11db);
  
  // Use 11dB attenuation for widest range
  analogSetAttenuation(ADC_11db);
  
  for (int i = 0; i < samples; i++) {
    int reading = analogRead(BATTERY_PIN);
    adcSum += reading;
    Serial.printf("ADC reading %d: %d\n", i+1, reading);
    delay(1);
  }
  
  float adcValue = adcSum / samples;
  Serial.printf("Average ADC value: %.2f (out of 4095)\n", adcValue);
  
  // Convert ADC reading to voltage
  // ESP32 ADC: 12-bit (0-4095) with 3.3V reference
  float voltage = (adcValue / 4095.0) * 3.3;
  Serial.printf("Raw ADC voltage: %.3fV\n", voltage);
  
  // Account for voltage divider if present
  voltage *= BATTERY_VOLTAGE_DIVIDER;
  Serial.printf("Final battery voltage: %.3fV (after divider correction)\n", voltage);
  
  // Additional diagnostics
  if (adcValue == 0) {
    Serial.println("WARNING: ADC reading is 0 - possible issues:");
    Serial.println("  - No voltage on GPIO34");
    Serial.println("  - GPIO34 not connected to battery circuit");
    Serial.println("  - ADC not properly initialized");
    Serial.println("  - Wrong GPIO pin for this board");
  } else if (adcValue >= 4095) {
    Serial.println("WARNING: ADC reading is maximum (4095) - possible issues:");
    Serial.println("  - Voltage too high for current attenuation");
    Serial.println("  - Short circuit or connection issue");
  }
  
  Serial.printf("=== END BATTERY DEBUG ===\n");
  
  return voltage;
}

int getBatteryPercentage() {
  float voltage = readBatteryVoltage();
  
  // Convert voltage to percentage
  if (voltage >= BATTERY_MAX_VOLTAGE) {
    return 100;
  } else if (voltage <= BATTERY_MIN_VOLTAGE) {
    return 0;
  } else {
    float percentage = ((voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0;
    int result = (int)percentage;
    Serial.printf("Calculated battery percentage: %d%%\n", result);
    return result;
  }
}

String getBatteryStatus() {
  int percentage = getBatteryPercentage();
  float voltage = readBatteryVoltage();

  String status;
  if (percentage >= 75) {
    status = "High";
  } else if (percentage >= 50) {
    status = "Good";
  } else if (percentage >= 25) {
    status = "Low";
  } else if (percentage >= 10) {
    status = "Critical";
  } else {
    status = "Very Low";
  }

  Serial.printf("Battery status: %s (%d%%, %.2fV)\n", status.c_str(), percentage, voltage);
  return status;
}

// ===== ENHANCED WEB SERVER FUNCTIONS =====

void setupWebServer() {
  // Main configuration page
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/config", HTTP_GET, handleConfigPage);
  
  // API endpoints
  webServer.on("/api/config", HTTP_GET, handleGetConfig);
  webServer.on("/api/config", HTTP_POST, handleSetConfig);
  webServer.on("/api/status", HTTP_GET, handleGetStatus);
  webServer.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  webServer.on("/api/restart", HTTP_POST, handleRestart);
  webServer.on("/api/videos", HTTP_GET, handleGetVideos);
  
  // File upload for videos (if needed)
  webServer.on("/upload", HTTP_POST, []() {
    webServer.send(200, "text/plain", "");
  }, handleFileUpload);
  
  // 404 handler
  webServer.onNotFound(handleNotFound);
  
  Serial.println("Web server routes configured");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>" + String(tricorderConfig.getDeviceLabel()) + "</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0;}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:30px;border-radius:10px;box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;text-align:center;margin-bottom:30px;}";
  html += ".status{background:#e7f3ff;padding:15px;border-radius:5px;margin:20px 0;}";
  html += ".btn{display:inline-block;padding:10px 20px;margin:10px 5px;background:#007cba;color:white;text-decoration:none;border-radius:5px;}";
  html += ".btn:hover{background:#005a87;}</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>🖥️ " + String(tricorderConfig.getDeviceLabel()) + "</h1>";
  
  html += "<div class='status'>";
  html += "<h3>Device Status</h3>";
  html += "<p><strong>Prop ID:</strong> " + String(tricorderConfig.getPropId()) + "</p>";
  html += "<p><strong>Description:</strong> " + String(tricorderConfig.getDescription()) + "</p>";
  html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>Firmware:</strong> Enhanced Tricorder v2.0</p>";
  html += "<p><strong>WiFi RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "<p><strong>Battery:</strong> " + getBatteryStatus() + "</p>";
  html += "</div>";
  
  html += "<h3>Configuration</h3>";
  html += "<a href='/config' class='btn'>📋 Device Configuration</a>";
  html += "<a href='/api/config' class='btn'>📄 View JSON Config</a>";
  html += "<a href='/api/status' class='btn'>📊 Status API</a>";
  html += "<a href='/api/videos' class='btn'>🎬 Available Videos</a>";
  
  html += "<h3>Actions</h3>";
  html += "<button class='btn' onclick='restart()'>🔄 Restart Device</button>";
  html += "<button class='btn' onclick='factoryReset()' style='background:#dc3545;'>⚠️ Factory Reset</button>";
  
  html += "<h3>Emergency Reset</h3>";
  html += "<div style='background:#fff3cd;border:1px solid #ffeaa7;padding:15px;border-radius:5px;margin:10px 0;'>";
  html += "<strong>⚠️ If device becomes unresponsive:</strong><br>";
  html += "1. <strong>Runtime Reset:</strong> Hold <strong>BOOT button for 5 seconds</strong> while device is running<br>";
  html += "2. <strong>Boot Reset:</strong> Short <strong>GPIO12 to Ground</strong> during startup<br>";
  html += "3. <strong>Alternative:</strong> Short <strong>GPIO13 to Ground</strong> during startup<br>";
  html += "4. Device will create an access point: <strong>Tricorder-" + deviceId + "</strong><br>";
  html += "5. Password: <strong>tricorder123</strong><br>";
  html += "6. Connect and visit <strong>http://192.168.4.1</strong>";
  html += "</div>";
  
  html += "</div>";
  
  html += "<script>";
  html += "function restart() { if(confirm('Restart device?')) fetch('/api/restart', {method:'POST'}); }";
  html += "function factoryReset() { if(confirm('Factory reset? This will erase all settings!')) fetch('/api/factory-reset', {method:'POST'}); }";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

void handleConfigPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Configuration - " + String(tricorderConfig.getDeviceLabel()) + "</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0;}";
  html += ".container{max-width:600px;margin:0 auto;background:white;padding:30px;border-radius:10px;box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;text-align:center;margin-bottom:30px;}";
  html += ".form-group{margin:20px 0;} label{display:block;margin-bottom:5px;font-weight:bold;}";
  html += "input,select,textarea{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}";
  html += "button{padding:10px 20px;margin:10px 5px;border:none;border-radius:5px;cursor:pointer;}";
  html += ".btn-primary{background:#007cba;color:white;} .btn-secondary{background:#6c757d;color:white;}";
  html += ".section{border:1px solid #ddd;padding:20px;margin:20px 0;border-radius:5px;background:#f9f9f9;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>⚙️ Device Configuration</h1>";
  
  html += "<form id='configForm'>";
  
  // Device Settings
  html += "<div class='section'>";
  html += "<h3>Device Settings</h3>";
  html += "<div class='form-group'>";
  html += "<label for='deviceLabel'>Device Label:</label>";
  html += "<input type='text' id='deviceLabel' name='deviceLabel' value='" + String(tricorderConfig.getDeviceLabel()) + "' required>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='propId'>Prop ID:</label>";
  html += "<input type='text' id='propId' name='propId' value='" + String(tricorderConfig.getPropId()) + "' required>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='description'>Description:</label>";
  html += "<textarea id='description' name='description' rows='2'>" + String(tricorderConfig.getDescription()) + "</textarea>";
  html += "</div>";
  html += "</div>";
  
  // LED Settings
  html += "<div class='section'>";
  html += "<h3>LED Settings</h3>";
  html += "<div class='form-group'>";
  html += "<label for='brightness'>Brightness (0-255):</label>";
  html += "<input type='number' id='brightness' name='brightness' min='0' max='255' value='" + String(tricorderConfig.getBrightness()) + "'>";
  html += "</div>";
  html += "</div>";
  
  // SACN Settings
  html += "<div class='section'>";
  html += "<h3>SACN/DMX Settings</h3>";
  html += "<div class='form-group'>";
  html += "<label for='sacnEnabled'>SACN Enabled:</label>";
  html += "<input type='checkbox' id='sacnEnabled' name='sacnEnabled' " + String(tricorderConfig.getSacnEnabled() ? "checked" : "") + ">";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='sacnUniverse'>SACN Universe (1-63999):</label>";
  html += "<input type='number' id='sacnUniverse' name='sacnUniverse' min='1' max='63999' value='" + String(tricorderConfig.getSacnUniverse()) + "'>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='dmxAddress'>DMX Address (1-512):</label>";
  html += "<input type='number' id='dmxAddress' name='dmxAddress' min='1' max='512' value='" + String(tricorderConfig.getDmxAddress()) + "'>";
  html += "</div>";
  html += "</div>";
  
  // Network Settings
  html += "<div class='section'>";
  html += "<h3>Network Settings</h3>";
  html += "<div class='form-group'>";
  html += "<label for='wifiSSID'>WiFi SSID:</label>";
  html += "<input type='text' id='wifiSSID' name='wifiSSID' value='" + String(tricorderConfig.getWiFiSSID()) + "' required>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='wifiPassword'>WiFi Password:</label>";
  html += "<input type='password' id='wifiPassword' name='wifiPassword' value='" + String(tricorderConfig.getWiFiPassword()) + "'>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='hostname'>Hostname:</label>";
  html += "<input type='text' id='hostname' name='hostname' value='" + String(tricorderConfig.getHostname()) + "' required>";
  html += "</div>";
  html += "</div>";
  
  html += "<div style='text-align:center;'>";
  html += "<button type='submit' class='btn-primary'>💾 Save Configuration</button>";
  html += "<button type='button' class='btn-secondary' onclick='window.location.href=\"/\"'>🔙 Back</button>";
  html += "</div>";
  
  html += "</form>";
  html += "</div>";
  
  html += "<script>";
  html += "document.getElementById('configForm').addEventListener('submit', function(e) {";
  html += "  e.preventDefault();";
  html += "  const formData = new FormData(e.target);";
  html += "  const config = {};";
  html += "  for (let [key, value] of formData.entries()) {";
  html += "    if (key === 'sacnEnabled') config[key] = true;";
  html += "    else if (key === 'brightness' || key === 'sacnUniverse' || key === 'dmxAddress') config[key] = parseInt(value);";
  html += "    else config[key] = value;";
  html += "  }";
  html += "  if (!formData.has('sacnEnabled')) config.sacnEnabled = false;";
  html += "  fetch('/api/config', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify(config)";
  html += "  }).then(response => response.json()).then(data => {";
  html += "    alert('Configuration saved successfully!');";
  html += "    window.location.href = '/';";
  html += "  }).catch(error => {";
  html += "    alert('Error saving configuration: ' + error);";
  html += "  });";
  html += "});";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

void handleGetConfig() {
  String json = tricorderConfig.toJson();
  webServer.send(200, "application/json", json);
}

void handleSetConfig() {
  if (!webServer.hasArg("plain")) {
    webServer.send(400, "application/json", "{\"error\":\"No JSON data received\"}");
    return;
  }
  
  String body = webServer.arg("plain");
  Serial.println("Received config update: " + body);
  
  if (tricorderConfig.fromJson(body)) {
    if (tricorderConfig.save()) {
      webServer.send(200, "application/json", "{\"status\":\"Configuration saved successfully\"}");
      Serial.println("Configuration updated and saved");
      
      // Apply new settings immediately where possible
      ledBrightness = tricorderConfig.getBrightness();
      FastLED.setBrightness(ledBrightness);
      
    } else {
      webServer.send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
    }
  } else {
    webServer.send(400, "application/json", "{\"error\":\"Invalid JSON configuration\"}");
  }
}

void handleGetStatus() {
  DynamicJsonDocument doc(1024);
  
  doc["deviceLabel"] = tricorderConfig.getDeviceLabel();
  doc["propId"] = tricorderConfig.getPropId();
  doc["firmwareVersion"] = "Enhanced Tricorder v2.0";
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["macAddress"] = WiFi.macAddress();
  doc["wifiRSSI"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["wifiConnected"] = wifiConnected;
  doc["sdCardInitialized"] = sdCardInitialized;
  doc["currentVideo"] = currentVideo;
  doc["videoPlaying"] = videoPlaying;
  doc["batteryVoltage"] = readBatteryVoltage();
  doc["batteryPercentage"] = getBatteryPercentage();
  doc["batteryStatus"] = getBatteryStatus();
  doc["ledBrightness"] = ledBrightness;
  doc["displayBrightness"] = tricorderConfig.getDisplayBrightness();
  
  String response;
  serializeJson(doc, response);
  webServer.send(200, "application/json", response);
}

void handleFactoryReset() {
  Serial.println("Factory reset requested via web interface");
  
  if (tricorderConfig.factoryReset()) {
    webServer.send(200, "application/json", "{\"status\":\"Factory reset completed - device will restart\"}");
    delay(1000);
    ESP.restart();
  } else {
    webServer.send(500, "application/json", "{\"error\":\"Factory reset failed\"}");
  }
}

void handleRestart() {
  webServer.send(200, "application/json", "{\"status\":\"Device restarting...\"}");
  delay(1000);
  ESP.restart();
}

void handleGetVideos() {
  String videoList = getVideoList();
  webServer.send(200, "application/json", videoList);
}

void handleFileUpload() {
  // This would handle video file uploads if needed
  // For now, just acknowledge
  webServer.send(200, "text/plain", "File upload not implemented yet");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: " + webServer.uri() + "\n";
  message += "Method: ";
  message += (webServer.method() == HTTP_GET ? "GET" : "POST");
  message += "\n";
  message += "Arguments: " + String(webServer.args()) + "\n";
  
  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  
  webServer.send(404, "text/plain", message);
}

void displaySystemStatus() {
  // Clear the screen and show current system status
  tft.fillScreen(TFT_BLACK);
  
  // Header
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("TRICORDER STATUS");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  int y = 40;
  int lineHeight = 16;
  
  // Device info
  tft.setCursor(10, y);
  tft.printf("Device: %s", tricorderConfig.getDeviceLabel());
  y += lineHeight;
  
  tft.setCursor(10, y);
  tft.printf("ID: %s", deviceId.c_str());
  y += lineHeight;
  
  // Network status
  y += 5; // Extra space
  if (WiFi.getMode() == WIFI_STA && wifiConnected) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, y);
    tft.println("WiFi: CONNECTED");
    y += lineHeight;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, y);
    tft.printf("IP: %s", WiFi.localIP().toString().c_str());
    y += lineHeight;
    
    tft.setCursor(10, y);
    tft.printf("Web: http://%s", WiFi.localIP().toString().c_str());
    y += lineHeight;
    
  } else if (WiFi.getMode() == WIFI_AP) {
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(10, y);
    tft.println("WiFi: ACCESS POINT");
    y += lineHeight;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, y);
    tft.printf("AP: Tricorder-%s", deviceId.c_str());
    y += lineHeight;
    
    tft.setCursor(10, y);
    tft.printf("IP: %s", WiFi.softAPIP().toString().c_str());
    y += lineHeight;
    
    tft.setCursor(10, y);
    tft.println("Password: tricorder123");
    y += lineHeight;
    
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, y);
    tft.println("WiFi: DISCONNECTED");
    y += lineHeight;
  }
  
  // SD Card status
  y += 5; // Extra space
  if (sdCardInitialized) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, y);
    tft.println("SD Card: OK");
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, y);
    tft.println("SD Card: FAILED");
  }
  y += lineHeight;
  
  // Battery status
  y += 5; // Extra space
  float batteryVoltage = readBatteryVoltage();
  int batteryPercent = getBatteryPercentage();
  
  if (batteryPercent > 50) {
    tft.setTextColor(TFT_GREEN);
  } else if (batteryPercent > 20) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  
  tft.setCursor(10, y);
  tft.printf("Battery: %d%% (%.2fV)", batteryPercent, batteryVoltage);
  y += lineHeight;
  
  // Reset instructions
  y += 10; // Extra space
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, y);
  tft.println("FACTORY RESET:");
  y += lineHeight;
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, y);
  tft.println("Hold BOOT btn 5s (runtime)");
  y += lineHeight;
  
  tft.setCursor(10, y);
  tft.println("Short GPIO12 to GND (boot)");
  y += lineHeight;
}

// Hardware Reset Functions
bool checkHardwareReset() {
  Serial.println("Checking for hardware reset conditions...");
  
  // Method 1: Check if reset pin (GPIO12) is shorted to ground
  if (checkResetPinShorted()) {
    Serial.println("Reset pin shorted - hardware reset triggered");
    return true;
  }
  
  // Method 2: Check if secondary reset pin (GPIO13) is shorted to ground
  pinMode(RESET_PIN_2, INPUT_PULLUP);
  delay(10);
  if (digitalRead(RESET_PIN_2) == LOW) {
    Serial.println("Secondary reset pin shorted - hardware reset triggered");
    return true;
  }
  
  Serial.println("No hardware reset conditions detected");
  return false;
}

// Rapid reboot detection functions removed to prevent boot loops

bool checkResetPinShorted() {
  // Check if reset pin (GPIO12) is shorted to ground
  Serial.println("Checking reset pin...");
  
  // Read pin state multiple times to ensure it's consistently LOW
  int lowCount = 0;
  for (int i = 0; i < 5; i++) {
    if (digitalRead(RESET_PIN) == LOW) {
      lowCount++;
    }
    delay(10);
  }
  
  bool pinShorted = (lowCount >= 4); // At least 4/5 reads must be LOW
  Serial.printf("Reset pin check: %d/5 reads were LOW, shorted=%s\n", lowCount, pinShorted ? "true" : "false");
  
  return pinShorted;
}

void performHardwareReset() {
  Serial.println("=== PERFORMING HARDWARE FACTORY RESET ===");
  
  // Initialize built-in RGB LED for feedback
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  
  // Show visual feedback that reset is happening
  blinkResetIndicator();
  
  // Initialize preferences and perform factory reset
  Preferences prefs;
  if (prefs.begin("tricorder", false)) {
    Serial.println("Clearing all stored preferences...");
    bool cleared = prefs.clear();
    prefs.end();
    
    if (cleared) {
      Serial.println("✓ Factory reset completed successfully");
      
      // Show success indication (green LED)
      digitalWrite(RGB_LED_R, LOW);
      digitalWrite(RGB_LED_G, HIGH);
      digitalWrite(RGB_LED_B, LOW);
      delay(2000);
      
    } else {
      Serial.println("✗ Factory reset failed");
      
      // Show error indication (red LED)
      digitalWrite(RGB_LED_R, HIGH);
      digitalWrite(RGB_LED_G, LOW);
      digitalWrite(RGB_LED_B, LOW);
      delay(2000);
    }
  } else {
    Serial.println("✗ Failed to initialize preferences for reset");
  }
  
  // Turn off LEDs
  digitalWrite(RGB_LED_R, LOW);
  digitalWrite(RGB_LED_G, LOW);
  digitalWrite(RGB_LED_B, LOW);
  
  Serial.println("Restarting device with factory defaults...");
  delay(1000);
  ESP.restart();
}

void blinkResetIndicator() {
  Serial.println("Showing hardware reset indicator (blinking LED)...");
  
  // Blink RGB LED rapidly to indicate reset is happening
  for (int i = 0; i < RESET_BLINK_COUNT; i++) {
    // Yellow blink (red + green)
    digitalWrite(RGB_LED_R, HIGH);
    digitalWrite(RGB_LED_G, HIGH);
    digitalWrite(RGB_LED_B, LOW);
    delay(200);
    
    // Off
    digitalWrite(RGB_LED_R, LOW);
    digitalWrite(RGB_LED_G, LOW);
    digitalWrite(RGB_LED_B, LOW);
    delay(200);
  }
}

void checkBootButtonReset() {
  // Monitor boot button for 5-second hold to trigger factory reset
  bool buttonCurrentlyPressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);
  
  if (buttonCurrentlyPressed && !bootButtonPressed) {
    // Button just pressed
    bootButtonPressed = true;
    bootButtonPressStart = millis();
    Serial.println("Boot button pressed - monitoring for reset");
    
  } else if (!buttonCurrentlyPressed && bootButtonPressed) {
    // Button just released
    bootButtonPressed = false;
    unsigned long holdTime = millis() - bootButtonPressStart;
    Serial.printf("Boot button released after %lu ms\n", holdTime);
    
    if (resetInProgress) {
      Serial.println("Reset cancelled - button released");
      resetInProgress = false;
      // Turn off any reset indication LEDs
      setBuiltinLED(0, 0, 0);
    }
    
  } else if (buttonCurrentlyPressed && bootButtonPressed && !resetInProgress) {
    // Button being held - check duration
    unsigned long holdTime = millis() - bootButtonPressStart;
    
    if (holdTime >= BOOT_HOLD_TIME) {
      // 5 seconds reached - trigger reset
      Serial.println("Boot button held for 5 seconds - triggering factory reset!");
      resetInProgress = true;
      
      // Visual feedback - flash LED to indicate reset starting
      setBuiltinLED(255, 255, 0); // Yellow to indicate reset
      delay(100);
      setBuiltinLED(0, 0, 0);
      delay(100);
      setBuiltinLED(255, 255, 0);
      delay(100);
      setBuiltinLED(0, 0, 0);
      
      // Perform factory reset
      performHardwareReset();
    } else if (holdTime >= (BOOT_HOLD_TIME - 1000)) {
      // 1 second warning before reset
      if ((holdTime % 200) < 100) {
        setBuiltinLED(255, 255, 0); // Yellow warning flash
      } else {
        setBuiltinLED(0, 0, 0);
      }
    }
  }
}