/*
 * Enhanced Tricorder Firmware - With Persistent Configuration and Web Interface
 * ESP32-based tricorder with video playback, LED control, and web configuration
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
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
#include <vector>
#include "TricorderConfig.h"

// Pin definitions for ESP32-2432S032C-I
#define LED_PIN 21         // NeoPixel data pin (external connection) - IO21
#define NUM_LEDS 4         // Total LEDs: 1 onboard + 3 front NeoPixels  
#define NUM_NEOPIXELS 3    // Number of NeoPixels in external strip (front LEDs)
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
#define FRAME_DELAY_MS 200  // ~5 FPS (200ms per frame) - More realistic for SD card + JPEG decode
#define VIDEO_BUFFER_SIZE 65536  // 64KB buffer - reduced from 128KB due to ESP32 memory constraints
#define MAX_FRAME_DECODE_TIME 500  // Maximum time to spend decoding a frame (ms)
#define FRAME_SKIP_THRESHOLD 1000  // Skip frame if decode takes longer than this (ms)
#define LOW_MEMORY_THRESHOLD 32768 // Skip frame if free heap below this (bytes)

// Network settings
#define UDP_PORT 8888      // Port for UDP status broadcasts (matches server)
#define SACN_PORT 5568     // sACN E1.31 standard port
#define SACN_MULTICAST_BASE "239.255.0.0"  // sACN multicast base address

// sACN E1.31 Constants
#define ACN_PACKET_IDENTIFIER "ASC-E1.17\0\0\0"
#define E131_PACKET_SIZE 638
#define E131_DATA_OFFSET 126
#define E131_UNIVERSE_OFFSET 113

// Enhanced Configuration System
TricorderConfig tricorderConfig;

// Global configuration variables (loaded from tricorderConfig)
String deviceId;
String firmwareVersion = "Enhanced Tricorder v2.3 OTA";

// Forward declaration of LED array (defined later)
extern CRGB leds[NUM_NEOPIXELS];

// Helper functions for LED color handling
void setLEDColor(int index, int r, int g, int b, int w = 0) {
  if (index < 0 || index >= NUM_NEOPIXELS) return;
  
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
    fill_solid(leds, NUM_NEOPIXELS, CRGB(r, g, b));
  #elif defined(LED_TYPE_RGBW)
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
      setLEDColor(i, r, g, b, w);
    }
  #endif
}

// Hardware objects
CRGB leds[NUM_NEOPIXELS];  // Only for NeoPixel strip (front 3 LEDs)
TFT_eSPI tft = TFT_eSPI();
WiFiUDP udp;
WiFiUDP sacnUdp;  // Separate UDP socket for sACN
WebServer webServer(80);
JPEGDEC jpeg;

// Server Discovery
std::vector<IPAddress> serverIPs;  // Discovered server IPs
unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 30000;  // Rediscover servers every 30 seconds

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

// Global variables for current UDP sender (for response sending)
IPAddress currentSenderIP;
uint16_t currentSenderPort;

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

// Video timing and performance
unsigned long lastVideoFrameTime = 0;
int frameSkipCount = 0; // For automatic frame skipping
unsigned long consecutiveSlowFrames = 0; // Track performance issues

// State variables
bool wifiConnected = false;
bool videoPlaying = false;
bool videoLooping = false;
bool videoResumed = false;  // True when video was resumed from preferences
bool sdCardInitialized = false;
String currentVideo = "";
String currentFolder = "";
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

// Streaming video system - no large arrays, generate paths dynamically
String currentVideoFolder = "";  // Folder containing frames for streaming playback
int maxFramesInFolder = 0;       // Total frames available in current folder
bool isAnimatedSequence = false;

// Timing variables
unsigned long lastStatusSend = 0;
const unsigned long STATUS_INTERVAL = 10000; // Send status every 10 seconds

// sACN State Variables
bool sacnEnabled = true;
int sacnUniverse = 1;
int sacnStartAddress = 1;  // DMX starting address for this device
unsigned long lastSacnPacket = 0;
uint8_t lastSacnData[512] = {0};  // Store last received DMX data
bool sacnActive = false;  // True when receiving sACN data
uint8_t sacnSequence = 0;  // Track sACN sequence numbers
bool sacnPriority = false;  // True when sACN should override UDP LED commands

// Software dimming function for RGB565 pixels
void dimPixelBuffer(uint16_t* pixels, int count, uint8_t brightness) {
  if (brightness == 255) return; // No dimming needed at full brightness
  
  // Convert brightness from 0-255 to 0-256 for more efficient multiplication
  uint16_t scale = (brightness + 1);
  
  for (int i = 0; i < count; i++) {
    uint16_t pixel = pixels[i];
    
    // Extract RGB components from RGB565
    uint8_t r = (pixel >> 11) & 0x1F;  // 5 bits
    uint8_t g = (pixel >> 5) & 0x3F;   // 6 bits  
    uint8_t b = pixel & 0x1F;          // 5 bits
    
    // Apply brightness scaling
    r = (r * scale) >> 8;
    g = (g * scale) >> 8;
    b = (b * scale) >> 8;
    
    // Recombine into RGB565
    pixels[i] = (r << 11) | (g << 5) | b;
  }
}

// Video frame callback without brightness filtering for clean display
int JPEGDraw(JPEGDRAW *pDraw) {
  // Draw the JPEG frame directly to the TFT display without brightness modifications
  // This prevents image corruption and ensures proper display quality
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
void discoverServers();
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

// sACN E1.31 Functions
void initializeSACN();
void handleSACNPackets();
bool processSACNPacket(uint8_t* packet, size_t length);
void updateLEDsFromDMX(uint8_t* dmxData);
void setSACNPriority(bool enabled);
String getMulticastAddress(int universe);

// Dual-core task functions
void ledTask(void *pvParameters);
void networkTask(void *pvParameters);
void videoTask(void *pvParameters);
void processNetworkCommand(NetworkCommand &netCmd);

// OTA Update functions
void performOTAUpdate(String firmwareUrl, String commandId);
void handleOTAUpdate(String firmwareUrl, String commandId);
void handleRemoteFileUpload(String filename, String fileUrl, String commandId);

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

// Folder-based video functions
bool playVideoFromFolder(String folderName);
String getFirstVideoInFolder(String folderName);
String getFolderVideoList();
void handlePlayFolder();
void resumeLastVideo();

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
  FastLED.addLeds<LED_CHIPSET, LED_PIN, LED_COLOR_ORDER>(leds, NUM_NEOPIXELS);
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
  videoCommandQueue = xQueueCreate(20, sizeof(VideoCommand));  // Increased from 5 to 20
  
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
    
    // No startup animation - default colors will be applied after configuration loads
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
    
    // Apply default colors if enabled
    if (tricorderConfig.getUseDefaultColors()) {
      uint8_t red[NUM_NEOPIXELS], green[NUM_NEOPIXELS], blue[NUM_NEOPIXELS];
      tricorderConfig.getDefaultColors(red, green, blue);
      
      for (int i = 0; i < NUM_NEOPIXELS; i++) {
        leds[i] = CRGB(red[i], green[i], blue[i]);
      }
      FastLED.show();
      Serial.println("Applied default LED colors from configuration");
    }
    
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
    
    // Create numbered folders 1-10 and GS if they don't exist
    String folders[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "GS"};
    int folderCount = sizeof(folders) / sizeof(folders[0]);
    for (int i = 0; i < folderCount; i++) {
      String folderPath = videoDirectory + "/" + folders[i];
      if (!SD.exists(folderPath)) {
        SD.mkdir(folderPath);
        Serial.printf("Created folder: %s\n", folderPath.c_str());
      }
    }
    
    // List available videos
    listVideos();
    
    // Resume last played video after a delay
    resumeLastVideo();
  } else {
    Serial.println("SD card initialization failed!");
  }
  
  // Update the boot screen with final status instead of separate screen
  // Always show boot status for user feedback, regardless of video resume
  Serial.println("=== CALLING updateBootScreenWithStatus() ===");
  updateBootScreenWithStatus();
  
  // Show boot status for 3 seconds so user can see WiFi and SD card status
  Serial.println("Displaying boot status for 3 seconds...");
  delay(3000);
  Serial.println("Boot status display period complete");
  
  // Auto-display test image after boot (only if no video resumed)
  if (!videoResumed) {
    Serial.println("Auto-displaying SFA2_202_211_Med_Tricorder_BODY.jpg after boot...");
    if (sdCardInitialized && videoCommandQueue) {
      VideoCommand autoCmd;
      autoCmd.type = VideoCommand::DISPLAY_IMAGE;
      strncpy(autoCmd.filename, "SFA2_202_211_Med_Tricorder_BODY.jpg", sizeof(autoCmd.filename) - 1);
      autoCmd.filename[sizeof(autoCmd.filename) - 1] = '\0';
      
      // Give the tasks a moment to start up, then send the command
      delay(1000);
      BaseType_t result = xQueueSend(videoCommandQueue, &autoCmd, pdMS_TO_TICKS(2000));
      if (result == pdPASS) {
        Serial.println("Auto-display command queued successfully");
      } else {
        Serial.println("Failed to queue auto-display command");
      }
    } else {
      Serial.println("Cannot auto-display: SD card not initialized or video queue not ready");
    }
  } else {
    Serial.println("Skipping boot screens - video was resumed");
  }
  
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
  Serial.println("=== UPDATING BOOT SCREEN WITH STATUS ===");
  // Update the boot screen with status info using the existing black center area
  // Draw a black background first to ensure text visibility
  
  // Clear the center area with a black background for better text visibility
  int boxX = 40;
  int boxY = 60;
  int boxWidth = 160;
  int boxHeight = 200;
  tft.fillRect(boxX, boxY, boxWidth, boxHeight, TFT_BLACK);
  
  // Set text properties for the LCARS center display area
  tft.setTextSize(1);
  int textX = 50;      // Left margin for center box
  int textY = 70;      // Top of center box area  
  int lineHeight = 14; // Line spacing
  int currentLine = 0;
  
  Serial.printf("Drawing status at position %d,%d\n", textX, textY);
  
  // Header - Device info
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(textX, textY + (currentLine * lineHeight));
  tft.printf("%s", tricorderConfig.getDeviceLabel());
  Serial.printf("Wrote device label: %s\n", tricorderConfig.getDeviceLabel());
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
  
  Serial.println("=== BOOT STATUS DISPLAY COMPLETE ===");
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

// ============================================================================
// sACN E1.31 Implementation
// ============================================================================

// Initialize sACN receiver
void initializeSACN() {
  if (!sacnEnabled) {
    Serial.println("sACN disabled in configuration");
    return;
  }
  
  // Get sACN configuration from tricorderConfig
  sacnUniverse = tricorderConfig.getSacnUniverse();
  sacnStartAddress = tricorderConfig.getDmxAddress();
  
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
  
  // Disable sACN priority if no packets received for 2 seconds
  if (sacnActive && (millis() - lastSacnPacket > 2000)) {
    sacnActive = false;
    sacnPriority = false;
    Serial.println("sACN timeout - switching to UDP control");
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
  if (dmxLength >= sacnStartAddress + (NUM_LEDS * CHANNELS_PER_LED)) {
    memcpy(lastSacnData, dmxData, (dmxLength < 512) ? dmxLength : 512);
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
  
  // Update NeoPixel strip (first 3 LEDs)
  for (int i = 0; i < NUM_NEOPIXELS; i++) {
    #ifdef LED_TYPE_RGB
      // RGB: 3 channels per LED
      int r = dmxData[dmxIndex + (i * 3) + 0];
      int g = dmxData[dmxIndex + (i * 3) + 1];
      int b = dmxData[dmxIndex + (i * 3) + 2];
      leds[i] = CRGB(r, g, b);
    #elif defined(LED_TYPE_RGBW)
      // RGBW: 4 channels per LED
      int r = dmxData[dmxIndex + (i * 4) + 0];
      int g = dmxData[dmxIndex + (i * 4) + 1];
      int b = dmxData[dmxIndex + (i * 4) + 2];
      int w = dmxData[dmxIndex + (i * 4) + 3];
      
      // For RGBW, we need to handle the white channel
      // This is a simplified approach - actual RGBW mixing is more complex
      leds[i] = CRGB(r + w/3, g + w/3, b + w/3);  // Approximate white mixing
    #endif
  }
  
  // Update onboard LED (4th pixel) - separate PWM control
  #ifdef LED_TYPE_RGB
    // Onboard LED uses next 3 channels after the NeoPixel strip
    int onboardIndex = dmxIndex + (NUM_NEOPIXELS * 3);
    
    // Bounds check to prevent reading beyond DMX array
    if (onboardIndex + 2 < 512) {
      int onboard_r = dmxData[onboardIndex + 0];
      int onboard_g = dmxData[onboardIndex + 1];
      int onboard_b = dmxData[onboardIndex + 2];
      setBuiltinLED(onboard_r, onboard_g, onboard_b);
    } else {
      // If beyond bounds, turn off onboard LED
      setBuiltinLED(0, 0, 0);
    }
  #elif defined(LED_TYPE_RGBW)
    // Onboard LED uses next 4 channels after the NeoPixel strip
    int onboardIndex = dmxIndex + (NUM_NEOPIXELS * 4);
    
    // Bounds check to prevent reading beyond DMX array
    if (onboardIndex + 2 < 512) {
      int onboard_r = dmxData[onboardIndex + 0];
      int onboard_g = dmxData[onboardIndex + 1];
      int onboard_b = dmxData[onboardIndex + 2];
      // Skip white channel for onboard LED (only RGB)
      setBuiltinLED(onboard_r, onboard_g, onboard_b);
    } else {
      // If beyond bounds, turn off onboard LED
      setBuiltinLED(0, 0, 0);
    }
  #endif
  
  // Apply brightness from configuration and update NeoPixels
  FastLED.setBrightness(tricorderConfig.getBrightness());
  FastLED.show();
}

// Set sACN priority mode
void setSACNPriority(bool enabled) {
  sacnPriority = enabled;
  if (enabled) {
    Serial.println("sACN priority enabled - ignoring UDP LED commands");
  } else {
    Serial.println("sACN priority disabled - accepting UDP LED commands");
    // When sACN priority is disabled, ensure onboard LED is off to prevent flicker
    setBuiltinLED(0, 0, 0);
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
  
  // Initialize sACN after network is ready
  initializeSACN();
  
  while (true) {
    // Handle UDP commands if connected
    if (wifiConnected) {
      // Handle sACN packets (high priority for lighting)
      handleSACNPackets();
      
      int packetSize = udp.parsePacket();
      if (packetSize) {
        // Store remote IP and port for response sending
        IPAddress senderIP = udp.remoteIP();
        uint16_t senderPort = udp.remotePort();
        
        char incomingPacket[255];
        int len = udp.read(incomingPacket, 255);
        if (len > 0) {
          incomingPacket[len] = 0;
        }
        
        // Store the sender info globally for sendResponse function
        NetworkCommand netCmd;
        netCmd.data = String(incomingPacket);
        netCmd.remoteIP = senderIP;
        netCmd.remotePort = senderPort;
        
        // Process the command with stored sender info
        processNetworkCommand(netCmd);
      }
      
      // Send periodic status to server (every 10 seconds)
      unsigned long currentTime = millis();
      if (currentTime - lastStatusSend > STATUS_INTERVAL) {
        sendPeriodicStatus();
        lastStatusSend = currentTime;
      }
      
      // Discover servers periodically (every 30 seconds)
      if (currentTime - lastDiscoveryTime > DISCOVERY_INTERVAL) {
        discoverServers();
        lastDiscoveryTime = currentTime;
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
      // Shorter delay during video playback for better frame timing
      delay(5);
    } else {
      // Longer delay when not playing video to save CPU
      delay(20);
    }
  }
}

// Simplified network command processor for network task
void processNetworkCommand(NetworkCommand &netCmd) {
  // Store sender info globally for response functions
  currentSenderIP = netCmd.remoteIP;
  currentSenderPort = netCmd.remotePort;
  
  String jsonCommand = netCmd.data;
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
      udp.beginPacket(currentSenderIP, currentSenderPort);
      udp.print(responseStr);
      udp.endPacket();
    }
    // Handle server discovery response
    else if (action == "server_discovery_response") {
      String serverIP = doc["server_ip"];
      int serverPort = doc["server_port"];
      
      Serial.printf("📡 Discovered server at %s:%d\n", serverIP.c_str(), serverPort);
      
      // Add server to discovered servers list
      IPAddress serverAddr;
      if (serverAddr.fromString(serverIP)) {
        bool alreadyKnown = false;
        for (size_t i = 0; i < serverIPs.size(); i++) {
          if (serverIPs[i] == serverAddr) {
            alreadyKnown = true;
            break;
          }
        }
        
        if (!alreadyKnown) {
          serverIPs.push_back(serverAddr);
          Serial.printf("✅ Added server %s to discovery list (total: %d)\n", serverIP.c_str(), serverIPs.size());
        } else {
          Serial.printf("🔄 Server %s already known\n", serverIP.c_str());
        }
      } else {
        Serial.printf("❌ Invalid server IP address: %s\n", serverIP.c_str());
      }
    }
    // Handle LED commands by sending to LED task
    else if (action == "set_led_color") {
      LEDCommand ledCmd;
      ledCmd.type = LEDCommand::SET_COLOR;
      ledCmd.r = doc["r"];
      ledCmd.g = doc["g"];
      ledCmd.b = doc["b"];
      ledCmd.w = doc["w"].is<int>() ? doc["w"] : 0;  // White channel optional for RGB compatibility
      
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
      if (doc["parameters"].is<JsonObject>()) {
        if (doc["parameters"]["filename"].is<String>()) {
          filename = doc["parameters"]["filename"].as<String>();
        }
        if (doc["parameters"]["loop"].is<bool>()) {
          loop = doc["parameters"]["loop"];
        }
      } else {
        // Legacy format
        if (doc.containsKey("filename")) {
          filename = doc["filename"].as<String>();
        }
        if (doc["loop"].is<bool>()) {
          loop = doc["loop"];
        }
      }
      
      VideoCommand vidCmd;
      vidCmd.type = VideoCommand::PLAY_VIDEO;
      strncpy(vidCmd.filename, filename.c_str(), sizeof(vidCmd.filename) - 1);
      vidCmd.filename[sizeof(vidCmd.filename) - 1] = '\0';  // Ensure null termination
      vidCmd.loop = loop;
      
      // Check queue status before sending
      UBaseType_t queueSpaces = uxQueueSpacesAvailable(videoCommandQueue);
      Serial.printf("Video queue spaces available: %d\n", queueSpaces);
      
      BaseType_t result = xQueueSend(videoCommandQueue, &vidCmd, pdMS_TO_TICKS(100)); // Reduced timeout
      
      if (result == pdPASS) {
        sendResponse(commandId, "Video playback started");
      } else {
        Serial.printf("Failed to queue video command - queue full or timeout\n");
        sendResponse(commandId, "Failed to queue video command - system busy");
      }
    }
    else if (action == "display_image") {
      String filename;
      
      // Check if filename is in parameters object (new format) or directly (legacy)
      if (doc["parameters"].is<JsonObject>() && doc["parameters"]["filename"].is<String>()) {
        filename = doc["parameters"]["filename"].as<String>();
      } else if (doc["filename"].is<String>()) {
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
      
      // Check queue status before sending
      UBaseType_t queueSpaces = uxQueueSpacesAvailable(videoCommandQueue);
      Serial.printf("Video queue spaces available: %d\n", queueSpaces);
      
      BaseType_t result = xQueueSend(videoCommandQueue, &vidCmd, pdMS_TO_TICKS(100)); // Reduced timeout
      
      if (result == pdPASS) {
        sendResponse(commandId, "Image command queued");
      } else {
        Serial.printf("Failed to queue image command - queue full or timeout\n");
        sendResponse(commandId, "Failed to queue image command - system busy");
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
      
      udp.beginPacket(currentSenderIP, currentSenderPort);
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
      JsonArray adcReadings = debugDoc["adcReadings"].to<JsonArray>();
      
      for (int i = 0; i < 6; i++) {
        int pin = testPins[i];
        int rawReading = analogRead(pin);
        float voltage = (rawReading / 4095.0) * 3.3;
        
        JsonObject reading = adcReadings.add<JsonObject>();
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
      
      udp.beginPacket(currentSenderIP, currentSenderPort);
      udp.write((const uint8_t*)response.c_str(), response.length());
      udp.endPacket();
    }
    // sACN Control Commands
    else if (action == "enable_sacn") {
      sacnEnabled = true;
      if (wifiConnected) {
        initializeSACN();
      }
      sendResponse(commandId, "sACN enabled");
    }
    else if (action == "disable_sacn") {
      sacnEnabled = false;
      sacnPriority = false;
      sacnActive = false;
      sacnUdp.stop();
      sendResponse(commandId, "sACN disabled");
    }
    else if (action == "set_sacn_universe") {
      int universe = 1;
      if (doc.containsKey("parameters") && doc["parameters"].containsKey("universe")) {
        universe = doc["parameters"]["universe"];
      } else if (doc.containsKey("universe")) {
        universe = doc["universe"];
      }
      sacnUniverse = universe;
      tricorderConfig.setSacnUniverse(universe);
      tricorderConfig.save();
      
      // Restart sACN with new universe
      if (sacnEnabled && wifiConnected) {
        sacnUdp.stop();
        initializeSACN();
      }
      sendResponse(commandId, "sACN universe set to " + String(universe));
    }
    else if (action == "set_sacn_address") {
      int address = 0;
      if (doc.containsKey("parameters") && doc["parameters"].containsKey("address")) {
        address = doc["parameters"]["address"];
      } else if (doc.containsKey("address")) {
        address = doc["address"];
      }
      sacnStartAddress = address;
      tricorderConfig.setDmxAddress(address);
      tricorderConfig.save();
      sendResponse(commandId, "sACN start address set to " + String(address));
    }
    else if (action == "get_sacn_status") {
      JsonDocument sacnStatus;
      sacnStatus["commandId"] = commandId;
      sacnStatus["deviceId"] = deviceId;
      sacnStatus["sacnEnabled"] = sacnEnabled;
      sacnStatus["sacnUniverse"] = sacnUniverse;
      sacnStatus["sacnStartAddress"] = sacnStartAddress;
      sacnStatus["sacnActive"] = sacnActive;
      sacnStatus["sacnPriority"] = sacnPriority;
      sacnStatus["lastPacketTime"] = lastSacnPacket;
      sacnStatus["channelsPerLed"] = CHANNELS_PER_LED;
      sacnStatus["totalChannels"] = NUM_LEDS * CHANNELS_PER_LED;
      
      String response;
      serializeJson(sacnStatus, response);
      udp.beginPacket(currentSenderIP, currentSenderPort);
      udp.print(response);
      udp.endPacket();
    }
    else if (action == "save_current_as_default") {
      // Save current LED colors as default startup colors
      uint8_t red[NUM_NEOPIXELS], green[NUM_NEOPIXELS], blue[NUM_NEOPIXELS];
      
      for (int i = 0; i < NUM_NEOPIXELS; i++) {
        red[i] = leds[i].r;
        green[i] = leds[i].g;
        blue[i] = leds[i].b;
      }
      
      tricorderConfig.setDefaultColors(red, green, blue);
      tricorderConfig.setUseDefaultColors(true);
      tricorderConfig.save();
      
      sendResponse(commandId, "Current LED colors saved as default startup colors");
    }
    else if (action == "ota_update") {
      String firmwareUrl = "";
      if (doc["parameters"].is<JsonObject>() && doc["parameters"]["firmware_url"].is<String>()) {
        firmwareUrl = doc["parameters"]["firmware_url"].as<String>();
      }
      
      if (firmwareUrl.length() > 0) {
        Serial.printf("🔄 Starting OTA update from: %s\n", firmwareUrl.c_str());
        sendResponse(commandId, "OTA update started");
        
        // Perform OTA update in background to avoid blocking the response
        handleOTAUpdate(firmwareUrl, commandId);
      } else {
        Serial.println("❌ OTA update failed: No firmware URL provided");
        sendResponse(commandId, "OTA update failed: No firmware URL provided");
      }
    }
    else if (action == "set_display_brightness") {
      int brightness = 255; // Default to full brightness
      
      if (doc["parameters"].is<JsonObject>() && doc["parameters"]["brightness"].is<int>()) {
        brightness = doc["parameters"]["brightness"].as<int>();
      } else if (doc["brightness"].is<int>()) {
        brightness = doc["brightness"].as<int>();
      }
      
      // Clamp brightness to valid range (0-255)
      brightness = constrain(brightness, 0, 255);
      
      Serial.printf("Network Task: Setting display brightness to %d (software dimming)\n", brightness);
      
      // Save to configuration for software dimming in JPEGDraw
      tricorderConfig.setDisplayBrightness(brightness);
      
      // Also try hardware brightness control (may not work on all boards)
      ledcWrite(0, brightness);
      
      sendResponse(commandId, String("Display brightness set to ") + String(brightness) + " (software dimming)");
    }
    else if (action == "upload_file") {
      String filename = "";
      String fileUrl = "";
      
      if (doc["parameters"].is<JsonObject>()) {
        if (doc["parameters"]["filename"].is<String>()) {
          filename = doc["parameters"]["filename"].as<String>();
        }
        if (doc["parameters"]["url"].is<String>()) {
          fileUrl = doc["parameters"]["url"].as<String>();
        }
      }
      
      if (filename.length() > 0 && fileUrl.length() > 0) {
        Serial.printf("📁 Starting file upload: %s from %s\n", filename.c_str(), fileUrl.c_str());
        sendResponse(commandId, "File upload started");
        
        // Perform file download and save to SD card
        handleRemoteFileUpload(filename, fileUrl, commandId);
      } else {
        Serial.println("❌ File upload failed: Missing filename or URL");
        sendResponse(commandId, "File upload failed: Missing filename or URL");
      }
    }
  }
}

void handleUDPCommands() {
  // This function is now simplified since network handling is done by networkTask
  // Keep it for legacy compatibility but it does minimal work
}

void setLEDColorCommand(int r, int g, int b, int w = 0) {
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP LED command - sACN active");
    return;
  }
  
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
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP LED brightness command - sACN active");
    return;
  }
  
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
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP individual LED command - sACN active");
    return;
  }
  
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
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP scanner effect command - sACN active");
    return;
  }
  
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
  // Check if sACN has priority - if so, ignore UDP LED commands
  if (sacnPriority && sacnActive) {
    Serial.println("Ignoring UDP pulse effect command - sACN active");
    return;
  }
  
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
  
  // Use globally stored sender IP and port instead of udp.remoteIP()
  udp.beginPacket(currentSenderIP, currentSenderPort);
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
  
  Serial.printf("Sent response to %s:%d - %s\n", currentSenderIP.toString().c_str(), currentSenderPort, response.c_str());
}

void sendStatus(String commandId) {
  JsonDocument doc;
  doc["commandId"] = commandId;
  doc["deviceId"] = deviceId;
  doc["fixtureNumber"] = tricorderConfig.getFixtureNumber();
  doc["firmwareVersion"] = firmwareVersion;
  doc["wifiConnected"] = wifiConnected;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["sdCardInitialized"] = sdCardInitialized;
  doc["videoPlaying"] = videoPlaying;
  doc["currentVideo"] = currentVideo;
  doc["currentFolder"] = currentFolder;
  doc["videoLooping"] = videoLooping;
  doc["currentFrame"] = currentFrame;
  
  // Battery information
  doc["batteryVoltage"] = readBatteryVoltage();
  doc["batteryPercentage"] = getBatteryPercentage();
  doc["batteryStatus"] = getBatteryStatus();
  
  String response;
  serializeJson(doc, response);
  
  // Use globally stored sender IP and port instead of udp.remoteIP()
  udp.beginPacket(currentSenderIP, currentSenderPort);
  udp.write((const uint8_t*)response.c_str(), response.length());
  udp.endPacket();
  
  Serial.printf("Sent status to %s:%d - %s\n", currentSenderIP.toString().c_str(), currentSenderPort, response.c_str());
}

void discoverServers() {
  // Only discover if enough time has passed
  if (millis() - lastDiscoveryTime < DISCOVERY_INTERVAL && serverIPs.size() > 0) {
    return;
  }
  
  lastDiscoveryTime = millis();
  Serial.println("🔍 Discovering servers...");
  
  // Clear existing servers if they're old
  serverIPs.clear();
  
  // Create discovery message
  JsonDocument discoveryDoc;
  discoveryDoc["deviceId"] = deviceId;
  discoveryDoc["action"] = "server_discovery";
  discoveryDoc["timestamp"] = millis();
  
  String discoveryMsg;
  serializeJson(discoveryDoc, discoveryMsg);
  
  // Broadcast discovery to multiple subnets
  IPAddress localIP = WiFi.localIP();
  
  // Try multiple subnet broadcasts
  IPAddress broadcastIPs[] = {
    IPAddress(localIP[0], localIP[1], localIP[2], 255),    // Local subnet broadcast
    IPAddress(192, 168, 1, 255),                           // 192.168.1.x broadcast
    IPAddress(192, 168, 0, 255),                           // 192.168.0.x broadcast
    IPAddress(10, 0, 0, 255)                               // 10.0.0.x broadcast
  };
  
  // Send discovery to all broadcast addresses
  for (int i = 0; i < 4; i++) {
    udp.beginPacket(broadcastIPs[i], UDP_PORT);
    udp.write((const uint8_t*)discoveryMsg.c_str(), discoveryMsg.length());
    udp.endPacket();
  }
  
  Serial.printf("📡 Sent discovery broadcast: %s\n", discoveryMsg.c_str());
  
  // Wait a moment for responses
  delay(100);
  
  // Check for discovery responses
  int packetSize = udp.parsePacket();
  while (packetSize) {
    String response = "";
    for (int i = 0; i < packetSize; i++) {
      response += (char)udp.read();
    }
    
    // Parse response
    JsonDocument responseDoc;
    if (deserializeJson(responseDoc, response) == DeserializationError::Ok) {
      if (responseDoc["action"] == "server_discovery_response") {
        IPAddress serverIP = udp.remoteIP();
        
        // Add server IP if not already in list
        bool found = false;
        for (IPAddress ip : serverIPs) {
          if (ip == serverIP) {
            found = true;
            break;
          }
        }
        
        if (!found) {
          serverIPs.push_back(serverIP);
          Serial.printf("✅ Discovered server at: %s\n", serverIP.toString().c_str());
        }
      }
    }
    
    packetSize = udp.parsePacket();
  }
  
  Serial.printf("🎯 Found %d servers\n", serverIPs.size());
}

void sendPeriodicStatus() {
  // Send periodic status to server (broadcast to server IP)
  JsonDocument doc;
  doc["deviceId"] = deviceId;
  doc["type"] = "tricorder";
  doc["deviceLabel"] = tricorderConfig.getDeviceLabel();
  doc["fixtureNumber"] = tricorderConfig.getFixtureNumber();
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
  
  // Send to discovered servers, or broadcast discovery if none found
  if (serverIPs.size() == 0) {
    // No servers discovered yet, send broadcast discovery
    discoverServers();
  }
  
  if (serverIPs.size() > 0) {
    // Send to all discovered servers
    for (IPAddress serverIP : serverIPs) {
      udp.beginPacket(serverIP, UDP_PORT);
      udp.write((const uint8_t*)statusMsg.c_str(), statusMsg.length());
      udp.endPacket();
    }
  } else {
    // Fallback: broadcast to local subnet
    IPAddress localIP = WiFi.localIP();
    IPAddress broadcastIP = IPAddress(localIP[0], localIP[1], localIP[2], 255);
    udp.beginPacket(broadcastIP, UDP_PORT);
    udp.write((const uint8_t*)statusMsg.c_str(), statusMsg.length());
    udp.endPacket();
  }
}

void setBuiltinLED(int r, int g, int b) {
  // Apply minimum threshold to prevent flicker from very low values
  // Values below 8 (about 3% of 255) are treated as completely off
  const int MIN_THRESHOLD = 8;
  
  if (r < MIN_THRESHOLD) r = 0;
  if (g < MIN_THRESHOLD) g = 0;
  if (b < MIN_THRESHOLD) b = 0;
  
  // Note: These LEDs are typically inverted (LOW = ON, HIGH = OFF)
  // Adjust based on your board's behavior
  analogWrite(RGB_LED_R, 255 - r);  // Inverted PWM
  analogWrite(RGB_LED_G, 255 - g);  // Inverted PWM  
  analogWrite(RGB_LED_B, 255 - b);  // Inverted PWM
  
  Serial.printf("Built-in RGB LED set to R:%d G:%d B:%d (thresholded)\n", r, g, b);
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
  maxFramesInFolder = 0;
  currentVideoFolder = "";
  isAnimatedSequence = false;
  
  // Check if this is a folder-based animation
  String folderPath = videoDirectory + "/" + filename;
  
  if (SD.exists(folderPath)) {
    // Check if it's a directory
    File testDir = SD.open(folderPath);
    if (testDir && testDir.isDirectory()) {
      testDir.close();
      
      Serial.printf("Analyzing folder contents: %s\n", folderPath.c_str());
      
      // First, try to detect frame sequence animation (frame_XXX.jpg pattern)
      // Use directory listing instead of sequential file existence checks for better performance
      int frameCount = 0;
      File dir = SD.open(folderPath);
      if (dir) {
        File file = dir.openNextFile();
        while (file) {
          if (!file.isDirectory()) {
            String fileName = file.name();
            // Check for frame_XXX.jpg pattern (support various padding formats)
            if (fileName.startsWith("frame_") && (fileName.endsWith(".jpg") || fileName.endsWith(".JPG"))) {
              // Extract frame number - handle zero-padded numbers
              String frameNumStr = fileName.substring(6); // Remove "frame_" prefix
              frameNumStr.replace(".jpg", "");
              frameNumStr.replace(".JPG", "");
              
              // Convert to int - this automatically handles zero padding
              int frameNum = frameNumStr.toInt();
              if (frameNum > 0 && frameNum > frameCount) {  // Ensure valid frame number
                frameCount = frameNum;
              }
              if (frameCount <= 10) {  // Only log first 10 for debugging
                Serial.printf("Found frame %d: %s (extracted from: %s)\n", frameNum, fileName.c_str(), frameNumStr.c_str());
              }
            }
          }
          file = dir.openNextFile();
        }
        dir.close();
      }
      
      if (frameCount > 0) {
        // Successfully found frames using directory listing approach
        currentVideoFolder = folderPath;
        maxFramesInFolder = frameCount;
        totalFrames = frameCount;
        isAnimatedSequence = true;
        
        Serial.printf("SUCCESS: Streaming animation setup: %d frames in folder %s\n", totalFrames, filename.c_str());
        Serial.printf("Frame detection: Found %d frames using directory listing\n", frameCount);
      } else {
        // No frame sequence found, check for any JPEG files (static image mode)
        Serial.printf("No frame sequence found, checking for static images...\n");
        
        // Reopen directory to look for any JPEG files
        File dir = SD.open(folderPath);
        if (dir) {
          File file = dir.openNextFile();
          String firstImageFile = "";
          
          while (file) {
            if (!file.isDirectory()) {
              String fileName = file.name();
              if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg") || 
                  fileName.endsWith(".JPG") || fileName.endsWith(".JPEG")) {
                firstImageFile = fileName;
                Serial.printf("Found static image: %s\n", fileName.c_str());
                break; // Use first image found
              }
            }
            file = dir.openNextFile();
          }
          dir.close();
          
          if (firstImageFile != "") {
            // Set up for single static image display
            currentVideoFolder = folderPath + "/" + firstImageFile;  // Store full path to specific image
            totalFrames = 1;
            maxFramesInFolder = 1;
            isAnimatedSequence = false;
            
            Serial.printf("Static image mode: displaying %s from folder %s\n", firstImageFile.c_str(), filename.c_str());
          } else {
            Serial.printf("No image files found in folder: %s\n", folderPath.c_str());
            return false;
          }
        } else {
          Serial.printf("Failed to open folder for reading: %s\n", folderPath.c_str());
          return false;
        }
      }
      
      if (totalFrames > 0) {
        
        // Set video state for animation
        videoPlaying = true;
        videoLooping = loop;
        currentVideo = filename;
        currentFrame = 0;
        lastFrameTime = millis();
        
        // Initialize performance tracking variables properly
        lastVideoFrameTime = millis();  // Initialize to current time, not 0
        frameSkipCount = 0;
        consecutiveSlowFrames = 0;
        
        return true;
      } else {
        Serial.printf("No sequential frame files found in folder: %s\n", folderPath.c_str());
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
  
  // Single file mode - store path in currentVideoFolder for compatibility
  currentVideoFolder = fullPath;  // Store full path for single images
  totalFrames = 1;
  maxFramesInFolder = 1;
  isAnimatedSequence = false;
  
  Serial.printf("Starting single image playback: %s -> %s (Loop: %s)\n", 
                filename.c_str(), actualFile.c_str(), loop ? "Yes" : "No");
  
  // Set video state
  videoPlaying = true;
  videoLooping = loop;
  currentVideo = filename;
  currentFrame = 0;
  lastFrameTime = millis();
  
  // Initialize performance tracking variables properly
  lastVideoFrameTime = millis();  // Initialize to current time, not 0
  frameSkipCount = 0;
  consecutiveSlowFrames = 0;
  
  return true;
}

void stopVideo() {
  if (videoPlaying) {
    videoPlaying = false;
    videoLooping = false;
    currentFrame = 0;
    totalFrames = 0;
    maxFramesInFolder = 0;
    currentVideoFolder = "";
    isAnimatedSequence = false;
    
    // Close video file if it was opened (legacy single file mode)
    if (videoFile) {
      videoFile.close();
    }
    
    // Keep the last frame displayed - don't clear the screen
    // The last displayed content will remain visible until new content is played
    
    Serial.printf("Video stopped: %s\n", currentVideo.c_str());
    currentVideo = "";
    currentFolder = "";
    
    // Clear the last played folder preference
    Preferences prefs;
    prefs.begin("tricorder", false);
    prefs.remove("lastFolder");
    prefs.end();
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
    
    // Show the frame (simplified - no complex frame skipping for now)
    unsigned long frameStart = millis();
    showVideoFrame();
    unsigned long frameTime = millis() - frameStart;
    
    Serial.printf("Frame %d decode time: %lums\n", currentFrame + 1, frameTime);
    
    lastFrameTime = currentTime;
    lastVideoFrameTime = millis();
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
  
  // Check available memory before processing large frames
  size_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 32768) { // Less than 32KB free
    Serial.printf("WARNING: Low memory %d bytes - skipping frame %d\n", freeHeap, currentFrame);
    currentFrame++;
    if (currentFrame >= totalFrames) {
      if (videoLooping) {
        currentFrame = 0;
      } else {
        stopVideo();
      }
    }
    return;
  }
  
  // Validate frame index
  if (currentFrame < 0 || currentFrame >= totalFrames) {
    Serial.printf("ERROR: Invalid frame index %d (totalFrames: %d)\n", currentFrame, totalFrames);
    return;
  }
  
  // Generate current frame path dynamically (streaming approach)
  String currentFramePath;
  if (isAnimatedSequence && currentVideoFolder != "") {
    // Generate path for sequential frames: frame_001.jpg, frame_002.jpg, etc.
    int frameNum = currentFrame + 1;  // Frame numbering starts from 1
    if (frameNum < 10) {
      currentFramePath = currentVideoFolder + "/frame_00" + String(frameNum) + ".jpg";
    } else if (frameNum < 100) {
      currentFramePath = currentVideoFolder + "/frame_0" + String(frameNum) + ".jpg";
    } else {
      currentFramePath = currentVideoFolder + "/frame_" + String(frameNum) + ".jpg";
    }
  } else if (!isAnimatedSequence && currentVideoFolder != "") {
    // Single image mode - currentVideoFolder contains the full file path
    currentFramePath = currentVideoFolder;
  } else {
    Serial.println("ERROR: No valid frame path generation method");
    return;
  }
  
  Serial.printf("Attempting to show frame %d: %s\n", currentFrame + 1, currentFramePath.c_str());
  
  // Open and display the current frame
  File frameFile = SD.open(currentFramePath, FILE_READ);
  if (!frameFile) {
    Serial.printf("ERROR: Failed to open frame file: %s\n", currentFramePath.c_str());
    Serial.printf("This was frame %d of %d\n", currentFrame + 1, totalFrames);
    
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
    Serial.printf("Frame file too large: %d bytes (max %d) - SKIPPING\n", fileSize, videoBufferSize);
    frameFile.close();
    // Skip to next frame instead of hanging
    currentFrame++;
    if (currentFrame >= totalFrames) {
      if (videoLooping) {
        currentFrame = 0;
      } else {
        stopVideo();
      }
    }
    return;
  }
  
  // Add watchdog yield during file read for large files
  size_t bytesRead = 0;
  size_t remainingBytes = fileSize;
  size_t chunkSize = min((size_t)8192, remainingBytes); // 8KB chunks
  
  while (remainingBytes > 0 && bytesRead < videoBufferSize) {
    size_t currentChunk = min(chunkSize, remainingBytes);
    size_t thisRead = frameFile.read(videoBuffer + bytesRead, currentChunk);
    bytesRead += thisRead;
    remainingBytes -= thisRead;
    
    // Yield to watchdog for large files
    if (bytesRead % 16384 == 0) { // Every 16KB
      yield();
      delayMicroseconds(100); // Brief pause
    }
    
    if (thisRead != currentChunk) break; // Read error
  }
  frameFile.close();
  
  if (bytesRead > 0) {
    // Yield before starting JPEG decode for large images
    if (bytesRead > 32768) { // For files larger than 32KB
      yield();
      delayMicroseconds(200);
    }
    
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
      
      // Clear screen only for the first frame of a new video (not for animated sequences)
      // For animated sequences, frames should overwrite each other without clearing
      if (currentFrame == 0 && !isAnimatedSequence) {
        tft.fillScreen(TFT_BLACK);
      }
      
      // Yield before decode for responsiveness
      yield();
      
      // Decode and display with timeout protection
      unsigned long decodeStart = millis();
      bool decodeSuccess = jpeg.decode(xOffset, yOffset, 0);
      unsigned long decodeTime = millis() - decodeStart;
      
      if (decodeSuccess) {
        Serial.printf("SUCCESS: Displayed frame %d/%d: %s (%dx%d) [%lums]\n", 
                     currentFrame + 1, totalFrames, currentFramePath.c_str(), width, height, decodeTime);
        
        // Adaptive frame skipping for performance
        if (decodeTime > FRAME_SKIP_THRESHOLD) {
          Serial.printf("WARNING: Very slow decode %lums - consider frame skipping\n", decodeTime);
        } else if (decodeTime > MAX_FRAME_DECODE_TIME) {
          Serial.printf("WARNING: Slow decode time %lums for frame %d\n", decodeTime, currentFrame);
        }
      } else {
        Serial.printf("ERROR: JPEG decode failed for frame %d: %s\n", currentFrame, currentFramePath.c_str());
        // Skip failed frames to prevent getting stuck
        currentFrame++;
        if (currentFrame >= totalFrames && videoLooping) {
          currentFrame = 0;
        }
      }
      
      jpeg.close();
      
      // Yield after decode to prevent watchdog timeout
      yield();
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
  return getFolderVideoList();
}

// New folder-based video functions
String getFolderVideoList() {
  String result = "[";
  
  if (!sdCardInitialized) {
    return "[{\"error\":\"SD card not initialized\"}]";
  }
  
  // Check for folders 1-10 and GS
  String folders[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "GS"};
  int folderCount = sizeof(folders) / sizeof(folders[0]);
  bool first = true;
  
  for (int i = 0; i < folderCount; i++) {
    String folderPath = videoDirectory + "/" + folders[i];
    File dir = SD.open(folderPath);
    
    if (dir && dir.isDirectory()) {
      String firstVideo = getFirstVideoInFolder(folders[i]);
      
      if (!first) result += ",";
      result += "{";
      result += "\"folder\":\"" + folders[i] + "\",";
      result += "\"hasContent\":";
      result += (firstVideo != "" ? "true" : "false");
      if (firstVideo != "") {
        result += ",\"firstFile\":\"" + firstVideo + "\"";
      }
      result += "}";
      first = false;
      
      dir.close();
    }
  }
  
  result += "]";
  return result;
}

String getFirstVideoInFolder(String folderName) {
  String folderPath = videoDirectory + "/" + folderName;
  File dir = SD.open(folderPath);
  
  if (!dir || !dir.isDirectory()) {
    return "";
  }
  
  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg") || 
          fileName.endsWith(".JPG") || fileName.endsWith(".JPEG")) {
        dir.close();
        return fileName;
      }
    }
    file = dir.openNextFile();
  }
  
  dir.close();
  return "";
}

bool playVideoFromFolder(String folderName) {
  String firstVideo = getFirstVideoInFolder(folderName);
  if (firstVideo == "") {
    Serial.printf("No video files found in folder: %s\n", folderName.c_str());
    return false;
  }
  
  currentFolder = folderName;
  
  // Save the last played folder for resume on boot
  Preferences prefs;
  prefs.begin("tricorder", false);
  prefs.putString("lastFolder", folderName);
  prefs.end();
  
  Serial.printf("Playing folder-based animation from folder %s (first frame: %s)\n", folderName.c_str(), firstVideo.c_str());
  return playVideo(folderName, true); // Pass folder name directly for folder-based animation
}

void resumeLastVideo() {
  Serial.println("=== RESUME LAST VIDEO START ===");
  Preferences prefs;
  prefs.begin("tricorder", true);
  String lastFolder = prefs.getString("lastFolder", "");
  prefs.end();
  
  Serial.printf("Last folder from preferences: '%s'\n", lastFolder.c_str());
  videoResumed = false;  // Reset flag
  
  if (lastFolder != "" && sdCardInitialized) {
    Serial.printf("Resuming last video from folder: %s\n", lastFolder.c_str());
    delay(1000); // Give system time to stabilize
    playVideoFromFolder(lastFolder);
    videoResumed = true;  // Set flag when video is resumed
    Serial.println("Video resume flag set - skipping boot screens");
  } else {
    if (lastFolder == "") {
      Serial.println("No last folder saved - first time boot or no previous video");
    }
    if (!sdCardInitialized) {
      Serial.println("SD card not initialized - cannot resume video");
    }
  }
  Serial.println("=== RESUME LAST VIDEO END ===");
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

// Battery monitoring functions (disabled for performance)
void initializeBatteryMonitoring() {
  // Battery monitoring disabled for performance optimization
  Serial.println("Battery monitoring disabled for performance");
}

float readBatteryVoltage() {
  // Battery monitoring disabled for performance - return fixed value indicating USB power
  return 5.0; // Indicates USB/external power (above typical battery range)
}

int getBatteryPercentage() {
  // Battery monitoring disabled for performance - return fixed 100%
  return 100;
}

String getBatteryStatus() {
  // Battery monitoring disabled for performance - return fixed status
  return "External Power (100%, 5.00V)";
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
  webServer.on("/api/play-folder", HTTP_POST, handlePlayFolder);
  
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
  html += "<p><strong>Firmware:</strong> Enhanced Tricorder v2.1 OTA</p>";
  html += "<p><strong>WiFi RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "<p><strong>Battery:</strong> " + getBatteryStatus() + "</p>";
  html += "</div>";
  
  html += "<h3>Configuration</h3>";
  html += "<a href='/config' class='btn'>📋 Device Configuration</a>";
  html += "<a href='/api/config' class='btn'>📄 View JSON Config</a>";
  html += "<a href='/api/status' class='btn'>📊 Status API</a>";
  html += "<a href='/api/videos' class='btn'>🎬 Available Videos</a>";
  
  html += "<h3>Video Folders</h3>";
  html += "<div style='display:grid;grid-template-columns:repeat(5,1fr);gap:10px;margin:20px 0;'>";
  for (int i = 1; i <= 10; i++) {
    html += "<button class='btn' onclick='playFolder(\"" + String(i) + "\")'>" + String(i) + "</button>";
  }
  html += "</div>";
  html += "<button class='btn' onclick='playFolder(\"GS\")' style='background:#28a745;width:100%;margin:10px 0;'>🟢 Green Screen (GS)</button>";
  html += "<button class='btn' onclick='stopVideo()' style='background:#dc3545;width:100%;margin:10px 0;'>⏹️ Stop Video</button>";
  
  if (currentFolder != "") {
    html += "<p style='text-align:center;color:#666;'>Currently playing: Folder " + currentFolder + "</p>";
  }
  
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
  html += "function playFolder(folder) { ";
  html += "  fetch('/api/play-folder', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({folder:folder})})";
  html += "  .then(response => response.text())";
  html += "  .then(data => { console.log('Playing folder:', folder); })";
  html += "  .catch(error => console.error('Error:', error)); }";
  html += "function stopVideo() { ";
  html += "  fetch('/api/play-folder', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({action:'stop'})})";
  html += "  .then(response => response.text())";
  html += "  .then(data => { console.log('Video stopped'); })";
  html += "  .catch(error => console.error('Error:', error)); }";
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
  JsonDocument doc;
  
  doc["deviceLabel"] = tricorderConfig.getDeviceLabel();
  doc["propId"] = tricorderConfig.getPropId();
  doc["firmwareVersion"] = "Enhanced Tricorder v2.1 OTA";
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["macAddress"] = WiFi.macAddress();
  doc["wifiRSSI"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis();
  doc["wifiConnected"] = wifiConnected;
  doc["sdCardInitialized"] = sdCardInitialized;
  doc["currentVideo"] = currentVideo;
  doc["currentFolder"] = currentFolder;
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

void handlePlayFolder() {
  Serial.println("=== HANDLE PLAY FOLDER CALLED ===");
  
  if (!webServer.hasArg("plain")) {
    Serial.println("ERROR: No body provided");
    webServer.send(400, "text/plain", "No body provided");
    return;
  }
  
  String body = webServer.arg("plain");
  Serial.printf("Received body: %s\n", body.c_str());
  
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    Serial.printf("JSON parsing error: %s\n", error.c_str());
    webServer.send(400, "text/plain", "Invalid JSON");
    return;
  }
  
  if (doc.containsKey("action") && doc["action"] == "stop") {
    Serial.println("Stopping video");
    stopVideo();
    currentFolder = "";
    webServer.send(200, "text/plain", "Video stopped");
    return;
  }
  
  if (!doc.containsKey("folder")) {
    Serial.println("ERROR: No folder specified in JSON");
    webServer.send(400, "text/plain", "No folder specified");
    return;
  }
  
  String folder = doc["folder"];
  Serial.printf("=== ATTEMPTING TO PLAY FOLDER: %s ===\n", folder.c_str());
  
  if (playVideoFromFolder(folder)) {
    Serial.printf("SUCCESS: Playing folder %s\n", folder.c_str());
    webServer.send(200, "text/plain", "Playing folder " + folder);
  } else {
    Serial.printf("FAILED: No content found in folder %s\n", folder.c_str());
    webServer.send(404, "text/plain", "No content found in folder " + folder);
  }
  
  Serial.println("=== HANDLE PLAY FOLDER COMPLETE ===");
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

// OTA Update Functions
void handleOTAUpdate(String firmwareUrl, String commandId) {
  Serial.printf("🔄 Handling OTA update from URL: %s\n", firmwareUrl.c_str());
  
  // Show OTA update status on display
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.println("OTA UPDATE");
  tft.setCursor(10, 80);
  tft.println("Starting...");
  
  // Set LED to indicate OTA in progress
  setBuiltinLED(0, 0, 255); // Blue for OTA
  
  // Perform the OTA update
  performOTAUpdate(firmwareUrl, commandId);
}

void performOTAUpdate(String firmwareUrl, String commandId) {
  Serial.printf("📡 Starting OTA update from: %s\n", firmwareUrl.c_str());
  
  // Update display
  tft.setCursor(10, 110);
  tft.println("Downloading...");
  
  // Configure HTTPUpdate
  httpUpdate.setLedPin(2, LOW); // Use GPIO2 (our onboard LED) for progress
  httpUpdate.rebootOnUpdate(false); // We'll handle reboot ourselves
  
  // Set up progress callback
  httpUpdate.onProgress([](int cur, int total) {
    if (total > 0) {
      int progress = (cur * 100) / total;
      Serial.printf("📊 OTA Progress: %d%% (%d/%d bytes)\n", progress, cur, total);
      
      // Update display
      tft.fillRect(10, 140, 220, 30, TFT_BLACK); // Clear progress area
      tft.setCursor(10, 140);
      tft.printf("Progress: %d%%", progress);
      
      // Update LED brightness based on progress
      int brightness = map(progress, 0, 100, 50, 255);
      setBuiltinLED(0, 0, brightness);
    }
  });
  
  // Perform the update
  WiFiClient client;
  t_httpUpdate_return result = httpUpdate.update(client, firmwareUrl);
  
  switch (result) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Update failed: %s\n", httpUpdate.getLastErrorString().c_str());
      tft.fillRect(10, 110, 220, 60, TFT_BLACK);
      tft.setTextColor(TFT_RED);
      tft.setCursor(10, 110);
      tft.println("UPDATE FAILED");
      tft.setCursor(10, 140);
      tft.println(httpUpdate.getLastErrorString().c_str());
      setBuiltinLED(255, 0, 0); // Red for error
      break;
      
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("⚠️ No updates available");
      tft.fillRect(10, 110, 220, 60, TFT_BLACK);
      tft.setTextColor(TFT_YELLOW);
      tft.setCursor(10, 110);
      tft.println("NO UPDATES");
      setBuiltinLED(255, 255, 0); // Yellow for no update
      break;
      
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Update successful! Rebooting...");
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.setTextSize(2);
      tft.setCursor(10, 50);
      tft.println("UPDATE");
      tft.setCursor(10, 80);
      tft.println("SUCCESS!");
      tft.setCursor(10, 110);
      tft.println("Rebooting...");
      setBuiltinLED(0, 255, 0); // Green for success
      
      delay(2000); // Give time to see the message
      
      // Gracefully stop tasks before reboot
      if (videoTaskHandle) vTaskDelete(videoTaskHandle);
      if (ledTaskHandle) vTaskDelete(ledTaskHandle);
      if (networkTaskHandle) vTaskDelete(networkTaskHandle);
      
      ESP.restart();
      break;
  }
}

// File Upload Functions
void handleRemoteFileUpload(String filename, String fileUrl, String commandId) {
  Serial.printf("📁 Handling file upload: %s from %s\n", filename.c_str(), fileUrl.c_str());
  
  // Check if SD card is available
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD card not available for file upload");
    sendResponse(commandId, "File upload failed: SD card not available");
    return;
  }
  
  // Show file upload status on display
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.println("FILE UPLOAD");
  tft.setCursor(10, 80);
  tft.printf("File: %s", filename.c_str());
  tft.setCursor(10, 110);
  tft.println("Downloading...");
  
  // Set LED to indicate upload in progress
  setBuiltinLED(255, 165, 0); // Orange for file upload
  
  // Create HTTP client
  HTTPClient http;
  http.begin(fileUrl);
  http.setTimeout(30000); // 30 second timeout
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();
    Serial.printf("📊 File size: %d bytes\n", len);
    
    // Create file on SD card
    File file = SD.open("/" + filename, FILE_WRITE);
    if (!file) {
      Serial.printf("❌ Failed to create file: %s\n", filename.c_str());
      sendResponse(commandId, "File upload failed: Could not create file on SD card");
      http.end();
      setBuiltinLED(255, 0, 0); // Red for error
      return;
    }
    
    // Get stream and write to file
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    int totalWritten = 0;
    
    tft.setCursor(10, 140);
    tft.println("Writing to SD...");
    
    while (http.connected() && (len > 0 || len == -1)) {
      size_t available = stream->available();
      if (available) {
        int c = stream->readBytes(buffer, min(available, sizeof(buffer)));
        file.write(buffer, c);
        totalWritten += c;
        
        if (len > 0) {
          len -= c;
          int progress = (totalWritten * 100) / (totalWritten + len);
          if (progress % 10 == 0) {
            Serial.printf("📊 Upload Progress: %d%% (%d bytes)\n", progress, totalWritten);
          }
        }
      }
      delay(1);
    }
    
    file.close();
    http.end();
    
    Serial.printf("✅ File upload successful: %s (%d bytes)\n", filename.c_str(), totalWritten);
    
    // Update display
    tft.fillRect(10, 110, 220, 60, TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 110);
    tft.println("UPLOAD SUCCESS!");
    tft.setCursor(10, 140);
    tft.printf("%d bytes", totalWritten);
    
    setBuiltinLED(0, 255, 0); // Green for success
    sendResponse(commandId, "File upload successful");
    
  } else {
    Serial.printf("❌ HTTP GET failed: %d\n", httpCode);
    http.end();
    
    tft.fillRect(10, 110, 220, 60, TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 110);
    tft.println("UPLOAD FAILED!");
    tft.setCursor(10, 140);
    tft.printf("HTTP: %d", httpCode);
    
    setBuiltinLED(255, 0, 0); // Red for error
    sendResponse(commandId, "File upload failed: HTTP error " + String(httpCode));
  }
  
  // Return LED to normal after delay
  delay(2000);
  setBuiltinLED(0, 0, 0);
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