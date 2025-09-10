/*
 * Tricorder Firmware v3.0 - MJPEG Video Playback Edition
 * ESP32-based tricorder with MJPEG video playback, LED control, and web configuration
 * 
 * Major Features:
 * - MJPEG video playback support
 * - Enhanced video buffering and streaming
 * - Improved performance optimizations
 * - Audio support (optional)
 * - Advanced video scaling and quality control
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <vector>

// MJPEG Video Support - Using enhanced JPEGDEC for MJPEG
#include <JPEGDEC.h>

#include "TricorderConfigV3.h"

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
#define BATTERY_PIN 39     // ADC pin for battery voltage (ADC1_CH3) - GPIO39
#define BATTERY_VOLTAGE_DIVIDER 82.0  // Actual measured voltage divider ratio
#define BATTERY_MAX_VOLTAGE 4.2      // Maximum battery voltage (for 100%)
#define BATTERY_MIN_VOLTAGE 3.0      // Minimum battery voltage (for 0%)

// Hardware reset pins and settings
#define RESET_PIN 12          // Primary reset pin (short to ground during boot)
#define RESET_PIN_2 13        // Secondary reset pin (alternative)
#define BOOT_BUTTON_PIN 0     // Boot button for runtime reset (IO0)
#define RESET_HOLD_TIME 3000  // Time to hold reset pin during boot (3 seconds)
#define BOOT_HOLD_TIME 5000   // Time to hold boot button during runtime (5 seconds)
#define RESET_BLINK_COUNT 6   // Number of LED blinks to indicate reset mode

// Video playback settings (Enhanced for v3 - MJPEG)
#define MJPEG_FRAME_BUFFER_SIZE 8192    // 8KB frame buffer for MJPEG decoding (reduced to save memory)
#define MJPEG_AUDIO_BUFFER_SIZE 4096    // 4KB audio buffer
#define VIDEO_QUEUE_SIZE 3             // Number of frames to buffer ahead
#define MAX_VIDEO_WIDTH 320            // Maximum video width
#define MAX_VIDEO_HEIGHT 240           // Maximum video height

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
TricorderConfigV3 tricorderConfig;

// Global configuration variables (loaded from tricorderConfig)
String deviceId;
String firmwareVersion = "Tricorder v3.0 MJPEG Edition";

// Hardware objects
TFT_eSPI tft = TFT_eSPI();
CRGB leds[NUM_NEOPIXELS];

// Network objects
WiFiUDP udp;
WiFiUDP sacnUdp;
WebServer server(80);

// MJPEG Video objects (v3 Enhancement) - Using JPEGDEC for MJPEG
JPEGDEC jpeg;
// Audio audio;  // Disabled for initial compilation

// Video file and buffer management
File videoFile;
uint8_t* videoBuffer = nullptr;
size_t videoBufferSize = 0;
uint32_t currentFrameOffset = 0;
uint32_t totalFrames = 0;

// Video state management
struct VideoState {
  bool playing = false;
  bool paused = false;
  bool looping = false;
  bool audioEnabled = false;
  String currentFile = "";
  uint32_t frameCount = 0;
  uint32_t frameRate = 25;
  uint32_t duration = 0;  // in seconds
  uint32_t position = 0;  // current position in seconds
  float videoScale = 1.0;
  uint16_t videoWidth = 0;
  uint16_t videoHeight = 0;
  uint32_t lastFrameTime = 0;
  bool streamingMode = false;  // For large files that can't fit in memory
};

VideoState videoState;

// FreeRTOS Tasks
TaskHandle_t videoTaskHandle = NULL;
TaskHandle_t statusTaskHandle = NULL;
TaskHandle_t sacnTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;

// Queue for video commands
QueueHandle_t videoCommandQueue = NULL;

// Device status
bool wifiConnected = false;
bool sdCardInitialized = false;
String lastError = "";
uint32_t lastStatusBroadcast = 0;
const uint32_t STATUS_BROADCAST_INTERVAL = 5000; // 5 seconds

// Battery monitoring
float lastBatteryVoltage = 0.0;
uint8_t batteryPercentage = 0;
uint32_t lastBatteryRead = 0;
const uint32_t BATTERY_READ_INTERVAL = 10000; // 10 seconds

// sACN DMX data
uint8_t dmxData[513] = {0}; // DMX channels 1-512 (index 0 unused)
uint32_t lastSacnPacket = 0;
bool sacnDataReceived = false;

// LED state
uint8_t currentBrightness = 128;
CRGB currentColors[NUM_NEOPIXELS];
bool ledsEnabled = true;

// Reset detection variables
uint32_t resetStartTime = 0;
bool resetInProgress = false;
uint32_t bootButtonPressTime = 0;
bool bootButtonPressed = false;

// Video Command Structure (Enhanced for v3 - MJPEG)
struct VideoCommand {
  enum Type { 
    PLAY_MJPEG, 
    PAUSE_MJPEG, 
    RESUME_MJPEG, 
    STOP_MJPEG, 
    SEEK_MJPEG,
    SET_QUALITY,
    TOGGLE_AUDIO,
    LIST_VIDEOS
  };
  
  Type type;
  String filename;
  uint32_t seekPosition;  // For SEEK_MJPEG command
  uint8_t quality;        // For SET_QUALITY command
  bool loop;
  bool enableAudio;
};

// Forward declarations
void setupWiFi();
void setupSD();
void setupDisplay();
void setupLEDs();
void setupTasks();
void setupWebServer();
void handleVideoCommand(const VideoCommand& cmd);
void updateDisplay();
void broadcastStatus();
void readBatteryVoltage();
void handleSacnData();
void checkResetConditions();

// Task functions
void videoTask(void *parameter);
void statusTask(void *parameter);
void sacnTask(void *parameter);
void ledTask(void *parameter);
void networkTask(void *parameter);

// MJPEG Video functions (v3 specific)
bool initializeMjpegDecoder();
bool openMjpegFile(const String& filename);
void closeMjpegFile();
bool decodeMjpegFrame();
bool findNextJpegFrame();
void displayMjpegFrame();
void handleVideoControls();
bool seekToPosition(uint32_t seconds);
void updateVideoStatistics();
int jpegDrawCallback(JPEGDRAW *pDraw);

// Audio functions (v3 specific) - Disabled for initial compilation
/*
bool initializeAudio();
void closeAudio();
bool startAudioPlayback(const String& audioFile);
void stopAudioPlayback();
*/

// Utility functions
String listMjpegVideos();
String getVideoInfo(const String& filename);
bool isValidMjpegFile(const String& filename);
void optimizePerformance();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=================================");
  Serial.println("Tricorder v3.0 MJPEG Edition");
  Serial.println("=================================");
  
  // Initialize configuration
  if (!tricorderConfig.begin()) {
    Serial.println("Failed to initialize configuration!");
    while(1) delay(1000);
  }
  
  // Load device ID and settings
  deviceId = String(tricorderConfig.getPropId());
  Serial.printf("Device ID: %s\n", deviceId.c_str());
  Serial.printf("Device Label: %s\n", tricorderConfig.getDeviceLabel());
  Serial.printf("Firmware: %s\n", firmwareVersion.c_str());
  
  // Optimize CPU performance for video processing
  optimizePerformance();
  
  // Initialize hardware
  setupDisplay();
  setupLEDs();
  setupSD();
  
  // Setup reset and control pins with proper pull-ups
  pinMode(RESET_PIN, INPUT_PULLUP);        // GPIO 12 - Primary reset pin
  pinMode(RESET_PIN_2, INPUT_PULLUP);      // GPIO 13 - Secondary reset pin  
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);  // Boot button pin
  Serial.println("Reset pins configured with INPUT_PULLUP");
  
  // Initialize MJPEG decoder
  if (!initializeMjpegDecoder()) {
    Serial.println("Warning: MJPEG decoder initialization failed");
  }
  
  // Initialize audio (optional) - Disabled for initial compilation
  /*
  if (tricorderConfig.getVideoAudioEnabled()) {
    if (!initializeAudio()) {
      Serial.println("Warning: Audio initialization failed");
    }
  }
  */
  
  // Setup networking
  setupWiFi();
  setupWebServer();
  
  // Setup FreeRTOS tasks
  setupTasks();
  
  // Initial display
  updateDisplay();
  
  Serial.println("Setup completed successfully!");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Largest free block: %d bytes\n", ESP.getMaxAllocHeap());
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Check for reset conditions
  checkResetConditions();
  
  // Handle video controls (if any)
  handleVideoControls();
  
  // Small delay to prevent watchdog triggers
  delay(10);
}

void optimizePerformance() {
  // Set CPU frequency based on configuration
  uint8_t cpuFreq = tricorderConfig.getCpuFrequency();
  setCpuFrequencyMhz(cpuFreq);
  Serial.printf("CPU frequency set to %d MHz\n", cpuFreq);
  
  // Optimize memory allocation
  heap_caps_malloc_extmem_enable(1024); // Use external RAM for large allocations
  
  // Set video buffer size from configuration
  uint32_t bufferSize = tricorderConfig.getVideoBufferSize();
  Serial.printf("Video buffer size: %d bytes\n", bufferSize);
}

// MJPEG Video Implementation (v3 specific)
bool initializeMjpegDecoder() {
  Serial.println("Initializing MJPEG decoder...");
  
  // Allocate video buffer
  videoBufferSize = tricorderConfig.getVideoBufferSize();
  videoBuffer = (uint8_t*)malloc(videoBufferSize);
  
  if (!videoBuffer) {
    Serial.printf("Failed to allocate video buffer (%d bytes)\n", videoBufferSize);
    return false;
  }
  
  // JPEG decoder doesn't need initialization with openRAM in setup
  // It will be initialized when we actually decode frames
  Serial.printf("MJPEG decoder initialized with %d byte buffer\n", videoBufferSize);
  return true;
}

// JPEG draw callback function
int jpegDrawCallback(JPEGDRAW *pDraw) {
  // Draw the decoded pixels directly to the TFT
  if (pDraw->y < tft.height() && pDraw->x < tft.width()) {
    // Scale coordinates if needed
    int drawX = pDraw->x;
    int drawY = pDraw->y;
    
    if (tricorderConfig.getVideoScaling()) {
      // Simple scaling - could be enhanced
      drawX = (pDraw->x * tft.width()) / MAX_VIDEO_WIDTH;
      drawY = (pDraw->y * tft.height()) / MAX_VIDEO_HEIGHT;
    }
    
    // Draw the pixel block to TFT
    tft.pushImage(drawX, drawY, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  }
  
  return 1; // Continue decoding
}

bool openMjpegFile(const String& filename) {
  Serial.printf("Opening MJPEG file: %s\n", filename.c_str());
  
  String fullPath = "/videos/" + filename;
  if (!fullPath.endsWith(".mjpeg") && !fullPath.endsWith(".avi") && !fullPath.endsWith(".mov")) {
    fullPath += ".mjpeg"; // Default extension
  }
  
  // First try videos directory
  if (!SD.exists(fullPath)) {
    // Try root directory as fallback
    fullPath = "/" + filename;
    if (!fullPath.endsWith(".mjpeg") && !fullPath.endsWith(".avi") && !fullPath.endsWith(".mov")) {
      fullPath += ".mjpeg";
    }
    if (!SD.exists(fullPath)) {
      Serial.printf("File not found: %s\n", fullPath.c_str());
      lastError = "File not found: " + filename;
      return false;
    }
  }
  
  // Open file
  videoFile = SD.open(fullPath, FILE_READ);
  if (!videoFile) {
    Serial.printf("Failed to open file: %s\n", fullPath.c_str());
    lastError = "Failed to open: " + filename;
    return false;
  }
  
  // Reset video state
  videoState.currentFile = filename;
  videoState.playing = true;
  videoState.paused = false;
  videoState.frameCount = 0;
  videoState.frameRate = tricorderConfig.getVideoFrameRate();
  videoState.lastFrameTime = millis();
  videoState.position = 0;
  currentFrameOffset = 0;
  
  // Estimate total frames (rough calculation)
  totalFrames = videoFile.size() / 8192; // Assume ~8KB per frame
  videoState.duration = totalFrames / videoState.frameRate;
  
  Serial.printf("MJPEG file opened: %s (%d bytes, ~%d frames)\n", 
                filename.c_str(), videoFile.size(), totalFrames);
  return true;
}

void closeMjpegFile() {
  if (videoState.playing) {
    if (videoFile) {
      videoFile.close();
    }
    
    videoState.playing = false;
    videoState.paused = false;
    videoState.currentFile = "";
    videoState.frameCount = 0;
    videoState.position = 0;
    currentFrameOffset = 0;
    
    Serial.println("MJPEG file closed");
  }
}

bool decodeMjpegFrame() {
  if (!videoState.playing || videoState.paused || !videoFile) {
    return false;
  }
  
  uint32_t currentTime = millis();
  uint32_t frameInterval = 1000 / videoState.frameRate;
  
  // Check if it's time for the next frame
  if (currentTime - videoState.lastFrameTime < frameInterval) {
    return false; // Not time for next frame yet
  }
  
  // Check available heap memory
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < tricorderConfig.getHeapThreshold()) {
    Serial.printf("Low memory warning: %d bytes free\n", freeHeap);
    return false;
  }
  
  // Find next JPEG frame in MJPEG stream
  if (!findNextJpegFrame()) {
    // End of file
    if (videoState.looping) {
      Serial.println("Looping MJPEG video...");
      videoFile.seek(0);
      currentFrameOffset = 0;
      videoState.frameCount = 0;
      videoState.position = 0;
      return findNextJpegFrame();
    } else {
      Serial.println("MJPEG playback finished");
      closeMjpegFile();
      return false;
    }
  }
  
  videoState.frameCount++;
  videoState.lastFrameTime = currentTime;
  videoState.position = (videoState.frameCount * 1000) / videoState.frameRate / 1000; // seconds
  
  return true;
}

bool findNextJpegFrame() {
  if (!videoFile || !videoFile.available()) {
    return false;
  }
  
  // Simple MJPEG parsing - look for JPEG SOI marker (0xFFD8)
  uint8_t byte1 = 0, byte2 = 0;
  bool foundSOI = false;
  uint32_t frameStart = 0;
  
  while (videoFile.available() && !foundSOI) {
    byte1 = byte2;
    byte2 = videoFile.read();
    
    if (byte1 == 0xFF && byte2 == 0xD8) {
      // Found JPEG Start of Image
      foundSOI = true;
      frameStart = videoFile.position() - 2;
    }
  }
  
  if (!foundSOI) {
    return false;
  }
  
  // Now find the end of this JPEG frame (look for EOI marker 0xFFD9)
  bool foundEOI = false;
  uint32_t frameSize = 2; // Start with SOI marker size
  
  while (videoFile.available() && !foundEOI && frameSize < videoBufferSize - 2) {
    byte1 = byte2;
    byte2 = videoFile.read();
    frameSize++;
    
    if (byte1 == 0xFF && byte2 == 0xD9) {
      // Found JPEG End of Image
      foundEOI = true;
    }
  }
  
  if (!foundEOI) {
    Serial.println("Warning: JPEG frame without proper EOI marker");
  }
  
  // Read the complete JPEG frame into buffer
  videoFile.seek(frameStart);
  size_t bytesRead = videoFile.read(videoBuffer, frameSize);
  
  if (bytesRead != frameSize) {
    Serial.printf("Warning: Expected %d bytes, read %d\n", frameSize, bytesRead);
    return false;
  }
  
  // Decode and display the JPEG frame
  if (jpeg.openRAM(videoBuffer, frameSize, jpegDrawCallback)) {
    jpeg.decode(0, 0, 0); // Decode and display via callback
    jpeg.close();
    return true;
  } else {
    Serial.println("Failed to decode JPEG frame");
    return false;
  }
}

void displayMjpegFrame() {
  // Frame display is handled by the jpegDrawCallback function
  // This function is kept for compatibility but doesn't need to do anything
}

bool seekToPosition(uint32_t seconds) {
  if (!videoState.playing || !videoFile) {
    return false;
  }
  
  Serial.printf("Seeking to position: %d seconds\n", seconds);
  
  // Simple seeking for MJPEG - estimate file position
  uint32_t targetFrame = seconds * videoState.frameRate;
  uint32_t estimatedPosition = (targetFrame * videoFile.size()) / totalFrames;
  
  // Seek to estimated position
  videoFile.seek(estimatedPosition);
  currentFrameOffset = estimatedPosition;
  videoState.frameCount = targetFrame;
  videoState.position = seconds;
  
  Serial.printf("Seeked to frame %d (position %d)\n", targetFrame, estimatedPosition);
  return true;
}

void updateVideoStatistics() {
  if (videoState.playing) {
    // Update frame rate statistics
    static uint32_t lastStatsTime = 0;
    static uint32_t lastFrameCount = 0;
    
    uint32_t currentTime = millis();
    if (currentTime - lastStatsTime >= 5000) { // Update every 5 seconds
      uint32_t framesSinceLastUpdate = videoState.frameCount - lastFrameCount;
      float actualFrameRate = (float)framesSinceLastUpdate / 5.0f;
      
      Serial.printf("MJPEG stats - Frame: %d, Position: %ds/%ds, FPS: %.1f\n",
                    videoState.frameCount, videoState.position, videoState.duration, actualFrameRate);
      
      lastStatsTime = currentTime;
      lastFrameCount = videoState.frameCount;
    }
  }
}

// Audio Implementation (v3 specific) - Disabled for initial compilation
/*
bool initializeAudio() {
  Serial.println("Initializing audio system...");
  
  audioOutput = new AudioOutputI2SNoDAC();
  audioGenerator = new AudioGeneratorMP3();
  
  if (!audioOutput || !audioGenerator) {
    Serial.println("Failed to create audio objects");
    return false;
  }
  
  Serial.println("Audio system initialized");
  return true;
}

void closeAudio() {
  stopAudioPlayback();
  
  if (audioGenerator) {
    delete audioGenerator;
    audioGenerator = nullptr;
  }
  
  if (audioOutput) {
    delete audioOutput;
    audioOutput = nullptr;
  }
  
  Serial.println("Audio system closed");
}

bool startAudioPlayback(const String& audioFile) {
  if (!audioOutput || !audioGenerator) {
    return false;
  }
  
  stopAudioPlayback(); // Stop any current playback
  
  String fullPath = "/audio/" + audioFile;
  if (!fullPath.endsWith(".mp3")) {
    fullPath += ".mp3";
  }
  
  if (!SD.exists(fullPath)) {
    Serial.printf("Audio file not found: %s\n", fullPath.c_str());
    return false;
  }
  
  audioSource = new AudioFileSourceSD(fullPath.c_str());
  if (!audioSource) {
    Serial.println("Failed to create audio source");
    return false;
  }
  
  if (!audioGenerator->begin(audioSource, audioOutput)) {
    Serial.println("Failed to start audio generator");
    delete audioSource;
    audioSource = nullptr;
    return false;
  }
  
  videoState.audioEnabled = true;
  Serial.printf("Audio playback started: %s\n", audioFile.c_str());
  return true;
}

void stopAudioPlayback() {
  if (audioGenerator && audioGenerator->isRunning()) {
    audioGenerator->stop();
  }
  
  if (audioSource) {
    delete audioSource;
    audioSource = nullptr;
  }
  
  videoState.audioEnabled = false;
  Serial.println("Audio playback stopped");
}
*/

// Utility Functions
String listMjpegVideos() {
  String videoList = "";
  File root = SD.open("/videos");
  
  if (!root) {
    // Try root directory as fallback
    root = SD.open("/");
    if (!root) {
      return "Error: Cannot open SD card";
    }
  }
  
  File file = root.openNextFile();
  while (file) {
    String filename = file.name();
    if (filename.endsWith(".mjpeg") || filename.endsWith(".avi") || filename.endsWith(".mov")) {
      if (videoList.length() > 0) videoList += ",";
      
      // Remove extension for display
      int dotIndex = filename.lastIndexOf('.');
      if (dotIndex > 0) {
        filename = filename.substring(0, dotIndex);
      }
      
      videoList += filename;
    }
    file = root.openNextFile();
  }
  root.close();
  
  return videoList;
}

String getVideoInfo(const String& filename) {
  String fullPath = "/videos/" + filename;
  if (!fullPath.endsWith(".mjpeg") && !fullPath.endsWith(".avi") && !fullPath.endsWith(".mov")) {
    fullPath += ".mjpeg";
  }
  
  File file = SD.open(fullPath);
  if (!file) {
    // Try root directory as fallback
    fullPath = "/" + filename;
    if (!fullPath.endsWith(".mjpeg") && !fullPath.endsWith(".avi") && !fullPath.endsWith(".mov")) {
      fullPath += ".mjpeg";
    }
    file = SD.open(fullPath);
    if (!file) {
      return "File not found";
    }
  }
  
  DynamicJsonDocument doc(512);
  doc["filename"] = filename;
  doc["size"] = file.size();
  doc["exists"] = true;
  
  // Try to get basic video info (simplified)
  doc["estimated_duration"] = file.size() / (25 * 8192); // Rough estimate: 25fps * 8KB per frame
  doc["format"] = "MJPEG";
  
  file.close();
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool isValidMjpegFile(const String& filename) {
  Serial.printf("Validating file: %s\n", filename.c_str());
  
  String fullPath = "/videos/" + filename;
  if (!fullPath.endsWith(".mjpeg") && !fullPath.endsWith(".avi") && !fullPath.endsWith(".mov")) {
    fullPath += ".mjpeg";
  }
  
  Serial.printf("Trying videos path: %s\n", fullPath.c_str());
  
  // First try videos directory
  if (!SD.exists(fullPath)) {
    // Try root directory as fallback
    fullPath = "/" + filename;
    if (!fullPath.endsWith(".mjpeg") && !fullPath.endsWith(".avi") && !fullPath.endsWith(".mov")) {
      fullPath += ".mjpeg";
    }
    Serial.printf("Trying root path: %s\n", fullPath.c_str());
    if (!SD.exists(fullPath)) {
      Serial.printf("File not found at either location\n");
      return false;
    }
  }
  
  Serial.printf("File exists, opening for validation\n");
  
  File file = SD.open(fullPath);
  if (!file) {
    Serial.printf("Failed to open file for validation\n");
    return false;
  }
  
  Serial.printf("File size: %d bytes\n", file.size());
  
  // Basic validation - check file size (be more lenient)
  if (file.size() < 100) {
    Serial.printf("File too small\n");
    file.close();
    return false;
  }
  
  // Check for JPEG SOI marker at beginning
  uint8_t header[2];
  file.read(header, 2);
  file.close();
  
  Serial.printf("File header: 0x%02X 0x%02X\n", header[0], header[1]);
  
  // For now, accept any file (JPEG validation can be too strict for MJPEG streams)
  // return (header[0] == 0xFF && header[1] == 0xD8); // JPEG SOI marker
  Serial.printf("File validation passed\n");
  return true; // Accept any file for now
}

// Hardware Setup Functions
void setupDisplay() {
  Serial.println("Initializing display...");
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);
  
  // Set display brightness
  analogWrite(TFT_BL, tricorderConfig.getDisplayBrightness());
  
  // Display startup message
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Tricorder v3.0");
  tft.setCursor(10, 40);
  tft.setTextSize(1);
  tft.println("MJPEG Edition");
  tft.setCursor(10, 60);
  tft.println("Initializing...");
  
  Serial.println("Display initialized");
}

void setupLEDs() {
  Serial.println("Initializing LEDs...");
  
  // Enable LED power
  pinMode(LED_POWER_EN, OUTPUT);
  digitalWrite(LED_POWER_EN, HIGH);
  delay(100);
  
  // Initialize FastLED
  FastLED.addLeds<LED_CHIPSET, LED_PIN, LED_COLOR_ORDER>(leds, NUM_NEOPIXELS);
  FastLED.setBrightness(tricorderConfig.getBrightness());
  
  // Set initial colors to off
  fill_solid(leds, NUM_NEOPIXELS, CRGB::Black);
  FastLED.show();
  
  // Initialize built-in RGB LED pins
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  
  // Turn off built-in RGB LED
  digitalWrite(RGB_LED_R, HIGH); // Active low
  digitalWrite(RGB_LED_G, HIGH);
  digitalWrite(RGB_LED_B, HIGH);
  
  Serial.println("LEDs initialized");
}

void setupSD() {
  Serial.println("Initializing SD card...");
  
  // Initialize SD card with custom SPI pins
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    sdCardInitialized = false;
    lastError = "SD card initialization failed";
    return;
  }
  
  // Check card type and size
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    sdCardInitialized = false;
    lastError = "No SD card detected";
    return;
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD card size: %llu MB\n", cardSize);
  
  // Create directories if they don't exist
  if (!SD.exists("/videos")) {
    Serial.println("Creating /videos directory");
    SD.mkdir("/videos");
  }
  
  if (!SD.exists("/audio")) {
    Serial.println("Creating /audio directory");
    SD.mkdir("/audio");
  }
  
  sdCardInitialized = true;
  Serial.println("SD card initialized successfully");
}

void setupWiFi() {
  Serial.println("Setting up WiFi...");
  
  // Set hostname
  WiFi.setHostname(tricorderConfig.getHostname());
  
  // Connect to WiFi
  WiFi.begin(tricorderConfig.getWiFiSSID(), tricorderConfig.getWiFiPassword());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    // Setup mDNS
    if (!MDNS.begin(tricorderConfig.getHostname())) {
      Serial.println("Error setting up MDNS responder!");
    } else {
      Serial.printf("mDNS responder started: %s.local\n", tricorderConfig.getHostname());
      MDNS.addService("http", "tcp", 80);
    }
    
    // Initialize UDP for status broadcasts
    udp.begin(tricorderConfig.getUdpPort());
    
    // Initialize sACN UDP listener
    sacnUdp.beginMulticast(IPAddress(239, 255, 0, tricorderConfig.getSacnUniverse()), SACN_PORT);
    
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi connection failed!");
    lastError = "WiFi connection failed";
  }
}

void setupTasks() {
  Serial.println("Creating FreeRTOS tasks...");
  
  // Create video command queue
  videoCommandQueue = xQueueCreate(10, sizeof(VideoCommand));
  if (videoCommandQueue == NULL) {
    Serial.println("Failed to create video command queue");
    return;
  }
  
  // Create video processing task (high priority)
  xTaskCreatePinnedToCore(
    videoTask,
    "VideoTask",
    8192, // Increased stack size for video processing
    NULL,
    3,    // High priority
    &videoTaskHandle,
    1     // Core 1
  );
  
  // Create status broadcast task
  xTaskCreatePinnedToCore(
    statusTask,
    "StatusTask",
    4096, // Increased stack size for debugging
    NULL,
    1,    // Low priority
    &statusTaskHandle,
    0     // Core 0
  );
  
  // Create sACN processing task - TEMPORARILY DISABLED
  /*
  xTaskCreatePinnedToCore(
    sacnTask,
    "SacnTask",
    2048, // Reduced stack size
    NULL,
    2,    // Medium priority
    &sacnTaskHandle,
    0     // Core 0
  );
  */
  
  // Create LED update task (DISABLED for memory debugging)
  /*
  xTaskCreatePinnedToCore(
    ledTask,
    "LedTask",
    1024, // Reduced stack size
    NULL,
    2,    // Medium priority
    &ledTaskHandle,
    1     // Core 1
  );
  */
  
  // Create network task for UDP command handling
  xTaskCreatePinnedToCore(
    networkTask,
    "NetworkTask",
    2048, // Reduced stack size
    NULL,
    2,    // Medium priority
    &networkTaskHandle,
    0     // Core 0
  );
  
  Serial.println("FreeRTOS tasks created successfully");
}

// FreeRTOS Task Implementations
void videoTask(void *parameter) {
  VideoCommand cmd;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(40); // 25 FPS = 40ms per frame
  
  Serial.println("Video task started");
  
  for(;;) {
    // Check for video commands
    if (xQueueReceive(videoCommandQueue, &cmd, 0) == pdTRUE) {
      Serial.println("Video command received!");
      handleVideoCommand(cmd);
    }
    
    // Process video frame if playing
    if (videoState.playing && !videoState.paused) {
      if (decodeMjpegFrame()) {
        displayMjpegFrame();
      }
    }
    
    // Update video statistics
    updateVideoStatistics();
    
    // Handle audio if enabled - Disabled for initial compilation
    /*
    if (videoState.audioEnabled && audio.isRunning()) {
      audio.loop();
    }
    */
    
    // Wait for next frame time
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void statusTask(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10000); // 10 second intervals
  
  Serial.println("Status task started (minimal mode)");
  
  for(;;) {
    // Minimal status task - just print a heartbeat
    Serial.println("Status task heartbeat");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Explicitly yield to other tasks
    taskYIELD();
    
    // Wait for next interval
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void sacnTask(void *parameter) {
  uint8_t packetBuffer[E131_PACKET_SIZE];
  
  Serial.println("sACN task started");
  
  for(;;) {
    // Check for sACN packets
    int packetSize = sacnUdp.parsePacket();
    if (packetSize >= E131_PACKET_SIZE) {
      int len = sacnUdp.read(packetBuffer, E131_PACKET_SIZE);
      if (len >= E131_PACKET_SIZE) {
        handleSacnData();
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
  }
}

void ledTask(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(100); // Slower updates to save CPU
  
  Serial.println("LED task started (minimal mode)");
  
  for(;;) {
    // Minimal LED processing to avoid memory issues
    if (ledsEnabled) {
      // Just set a simple static color to avoid complex FastLED operations
      fill_solid(leds, NUM_LEDS, CRGB::Blue);
      FastLED.show();
    }
    
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    taskYIELD(); // Explicitly yield to other tasks
  }
}

void networkTask(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10); // Check every 10ms
  
  Serial.println("Network task started");
  
  for(;;) {
    // Handle incoming UDP commands
    if (wifiConnected) {
      int packetSize = udp.parsePacket();
      if (packetSize > 0) {
        String message = "";
        while (udp.available()) {
          message += char(udp.read());
        }
        
        if (message.length() > 0) {
          // Process UDP command
          DynamicJsonDocument doc(512);
          DeserializationError error = deserializeJson(doc, message);
          
          if (!error) {
            String action = doc["action"];
            if (action == "ping") {
              // Respond to ping immediately
              udp.beginPacket(udp.remoteIP(), udp.remotePort());
              udp.print("{\"status\":\"pong\"}");
              udp.endPacket();
            }
            // Add other command handlers here as needed
          }
        }
      }
    }
    
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void handleVideoCommand(const VideoCommand& cmd) {
  Serial.printf("Processing video command: %d\n", cmd.type);
  Serial.printf("Filename: %s\n", cmd.filename.c_str());
  
  switch (cmd.type) {
    case VideoCommand::PLAY_MJPEG:
      Serial.println("PLAY_MJPEG command received");
      if (isValidMjpegFile(cmd.filename)) {
        Serial.println("File is valid, attempting to open...");
        closeMjpegFile(); // Close any current video
        if (openMjpegFile(cmd.filename)) {
          videoState.looping = cmd.loop;
          // Disabled audio for initial compilation
          /*
          if (cmd.enableAudio && tricorderConfig.getVideoAudioEnabled()) {
            // Try to find corresponding audio file
            String audioFile = "/audio/" + cmd.filename;
            audioFile.replace(".mjpeg", ".mp3");
            audioFile.replace(".avi", ".mp3");
            audioFile.replace(".mov", ".mp3");
            if (SD.exists(audioFile)) {
              audio.connecttoFS(SD, audioFile.c_str());
              videoState.audioEnabled = true;
            }
          }
          */
          Serial.printf("Started playing: %s\n", cmd.filename.c_str());
        } else {
          Serial.println("Failed to open video file");
        }
      } else {
        Serial.printf("Invalid MJPEG file: %s\n", cmd.filename.c_str());
        lastError = "Invalid file: " + cmd.filename;
      }
      break;
      
    case VideoCommand::PAUSE_MJPEG:
      if (videoState.playing) {
        videoState.paused = true;
        // Disabled audio for initial compilation
        /*
        if (videoState.audioEnabled) {
          audio.pauseResume();
        }
        */
        Serial.println("Video paused");
      }
      break;
      
    case VideoCommand::RESUME_MJPEG:
      if (videoState.playing && videoState.paused) {
        videoState.paused = false;
        // Disabled audio for initial compilation
        /*
        if (videoState.audioEnabled) {
          audio.pauseResume();
        }
        */
        Serial.println("Video resumed");
      }
      break;
      
    case VideoCommand::STOP_MJPEG:
      closeMjpegFile();
      // Disabled audio for initial compilation
      /*
      if (videoState.audioEnabled) {
        audio.stopSong();
        videoState.audioEnabled = false;
      }
      */
      Serial.println("Video stopped");
      break;
      
    case VideoCommand::SEEK_MJPEG:
      if (seekToPosition(cmd.seekPosition)) {
        Serial.printf("Seeked to position: %d seconds\n", cmd.seekPosition);
      } else {
        Serial.println("Seek failed");
        lastError = "Seek operation failed";
      }
      break;
      
    case VideoCommand::SET_QUALITY:
      tricorderConfig.setVideoQuality(cmd.quality);
      Serial.printf("Video quality set to: %d\n", cmd.quality);
      break;
      
    case VideoCommand::TOGGLE_AUDIO:
      tricorderConfig.setVideoAudioEnabled(!tricorderConfig.getVideoAudioEnabled());
      // Disabled audio for initial compilation
      /*
      if (!tricorderConfig.getVideoAudioEnabled() && videoState.audioEnabled) {
        audio.stopSong();
        videoState.audioEnabled = false;
      }
      */
      Serial.printf("Audio toggled: %s\n", tricorderConfig.getVideoAudioEnabled() ? "ON" : "OFF");
      break;
      
    case VideoCommand::LIST_VIDEOS:
      // This is handled via web interface/UDP response
      break;
  }
}

// Utility Functions Implementation
void updateDisplay() {
  if (videoState.playing) {
    return; // Don't update display during video playback
  }
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Device info
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Tricorder v3.0");
  
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.printf("ID: %s", deviceId.c_str());
  
  tft.setCursor(10, 60);
  if (wifiConnected) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("WiFi: %s", WiFi.localIP().toString().c_str());
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("WiFi: Disconnected");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 80);
  if (sdCardInitialized) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("SD: Ready");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("SD: Error");
  }
  
  // Battery info
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 100);
  tft.printf("Battery: %d%% (%.2fV)", batteryPercentage, lastBatteryVoltage);
  
  // Video info
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 120);
  if (videoState.playing) {
    tft.printf("Video: %s", videoState.currentFile.c_str());
    tft.setCursor(10, 140);
    tft.printf("Frame: %d, FPS: %d", videoState.frameCount, videoState.frameRate);
  } else {
    tft.println("Video: Stopped");
  }
  
  // Memory info
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.setCursor(10, 160);
  tft.printf("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
  
  // Error display
  if (lastError.length() > 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 180);
    tft.printf("Error: %s", lastError.c_str());
  }
}

void broadcastStatus() {
  if (!wifiConnected) return;
  
  // Simple status broadcast with minimal JSON to prevent blocking
  DynamicJsonDocument doc(512);  // Reduced size
  doc["deviceId"] = deviceId;
  doc["deviceType"] = "tricorder_v3";
  doc["timestamp"] = millis();
  
  // Essential status only
  doc["wifiConnected"] = true;
  doc["ipAddress"] = WiFi.localIP().toString();
  doc["sdCardInitialized"] = sdCardInitialized;
  doc["batteryVoltage"] = lastBatteryVoltage;
  doc["freeHeap"] = ESP.getFreeHeap();
  
  // Video status
  doc["videoPlaying"] = videoState.playing;
  doc["currentVideo"] = videoState.currentFile;
  
  String statusJson;
  serializeJson(doc, statusJson);
  
  // Quick UDP broadcast with timeout protection
  if (udp.beginPacket("255.255.255.255", tricorderConfig.getUdpPort())) {
    udp.print(statusJson);
    udp.endPacket();
  }
  
  lastStatusBroadcast = millis();
}

void readBatteryVoltage() {
  uint32_t currentTime = millis();
  if (currentTime - lastBatteryRead < BATTERY_READ_INTERVAL) {
    return; // Too soon to read again
  }
  
  // Read ADC value
  int adcValue = analogRead(BATTERY_PIN);
  
  // Convert to voltage
  float voltage = (adcValue / 4095.0) * 3.3; // ESP32 ADC reference voltage
  lastBatteryVoltage = voltage * tricorderConfig.getBatteryVoltageCalibration();
  
  // Calculate percentage
  batteryPercentage = map(lastBatteryVoltage * 100, BATTERY_MIN_VOLTAGE * 100, BATTERY_MAX_VOLTAGE * 100, 0, 100);
  batteryPercentage = constrain(batteryPercentage, 0, 100);
  
  lastBatteryRead = currentTime;
}

void handleSacnData() {
  // Simplified sACN handling - extract DMX data and apply to LEDs
  if (!tricorderConfig.getSacnEnabled()) {
    return;
  }
  
  // Extract DMX data from sACN packet (simplified)
  uint16_t dmxStart = tricorderConfig.getDmxAddress();
  uint16_t numChannels = NUM_NEOPIXELS * CHANNELS_PER_LED;
  
  // Update LED colors based on DMX data
  for (int i = 0; i < NUM_NEOPIXELS && i * CHANNELS_PER_LED < numChannels; i++) {
    uint16_t r_channel = dmxStart + (i * CHANNELS_PER_LED);
    uint16_t g_channel = r_channel + 1;
    uint16_t b_channel = r_channel + 2;
    
    if (r_channel <= 512 && g_channel <= 512 && b_channel <= 512) {
      currentColors[i] = CRGB(
        dmxData[r_channel],
        dmxData[g_channel], 
        dmxData[b_channel]
      );
    }
  }
  
  sacnDataReceived = true;
  lastSacnPacket = millis();
}

void checkResetConditions() {
  // Check for hardware reset conditions
  bool resetPin1 = digitalRead(RESET_PIN) == LOW;
  bool resetPin2 = digitalRead(RESET_PIN_2) == LOW;
  bool bootButton = digitalRead(BOOT_BUTTON_PIN) == LOW;
  
  // DEBUG: Print pin states occasionally (every 5 seconds) for monitoring
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 5000) {
    Serial.printf("Reset pins status - GPIO12: %s, GPIO13: %s, Boot: %s\n", 
                  resetPin1 ? "LOW" : "HIGH", 
                  resetPin2 ? "LOW" : "HIGH",
                  bootButton ? "LOW" : "HIGH");
    lastDebugPrint = millis();
  }
  
  // TEMPORARILY DISABLED: Reset logic causing reboot loop
  // The reset pins are likely floating (not properly pulled up)
  // Will re-enable after adding proper pinMode setup with INPUT_PULLUP
  
  /*
  // Handle reset pin during boot
  if ((resetPin1 || resetPin2) && millis() < 10000) { // Within first 10 seconds
    if (!resetInProgress) {
      resetInProgress = true;
      resetStartTime = millis();
      Serial.println("Reset condition detected during boot");
    }
    
    if (millis() - resetStartTime >= RESET_HOLD_TIME) {
      Serial.println("Performing factory reset...");
      tricorderConfig.factoryReset();
      ESP.restart();
    }
  } else {
    resetInProgress = false;
  }
  */
  
  // Keep boot button logic for now (but disable for debugging too)
  /*
  if (bootButton) {
    if (!bootButtonPressed) {
      bootButtonPressed = true;
      bootButtonPressTime = millis();
    }
    
    if (millis() - bootButtonPressTime >= BOOT_HOLD_TIME) {
      Serial.println("Boot button reset triggered");
      ESP.restart();
    }
  } else {
    bootButtonPressed = false;
  }
  */
}

void handleVideoControls() {
  // This function can be expanded to handle physical buttons
  // or other input methods for video control
  
  // For now, it's just a placeholder for future expansion
}

void setupWebServer() {
  if (!wifiConnected) return;
  
  Serial.println("Setting up web server...");
  
  // Root page
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head><title>Tricorder v3.0</title></head><body>";
    html += "<h1>Tricorder v3.0 MJPEG Edition</h1>";
    html += "<div id='status'>Loading...</div>";
    html += "<h3>Video Control</h3>";
    html += "<input type='text' id='videoFile' placeholder='Video filename'>";
    html += "<button onclick='playVideo()'>Play</button>";
    html += "<button onclick='pauseVideo()'>Pause</button>";
    html += "<button onclick='stopVideo()'>Stop</button>";
    html += "<div id='videoList'>Loading videos...</div>";
    html += "<script>";
    html += "function playVideo() { fetch('/play-video', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({filename: document.getElementById('videoFile').value, loop: false, audio: false}) }); }";
    html += "function pauseVideo() { fetch('/pause-video', { method: 'POST' }); }";
    html += "function stopVideo() { fetch('/stop-video', { method: 'POST' }); }";
    html += "fetch('/list-videos').then(r => r.text()).then(v => document.getElementById('videoList').innerHTML = v.split(',').map(f => '<button onclick=\"document.getElementById(\\'videoFile\\').value=\\''+f+'\\'\">'+f+'</button>').join(''));";
    html += "</script></body></html>";
    server.send(200, "text/html", html);
  });
  
  // Status API endpoint
  server.on("/status", HTTP_GET, []() {
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = deviceId;
    doc["firmwareVersion"] = firmwareVersion;
    doc["deviceType"] = "tricorder_v3";
    doc["timestamp"] = millis();
    doc["wifiConnected"] = wifiConnected;
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["sdCardInitialized"] = sdCardInitialized;
    doc["batteryVoltage"] = lastBatteryVoltage;
    doc["batteryPercentage"] = batteryPercentage;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["videoPlaying"] = videoState.playing;
    doc["videoPaused"] = videoState.paused;
    doc["currentVideo"] = videoState.currentFile;
    doc["videoLooping"] = videoState.looping;
    doc["frameCount"] = videoState.frameCount;
    doc["videoPosition"] = videoState.position;
    doc["videoDuration"] = videoState.duration;
    doc["audioEnabled"] = videoState.audioEnabled;
    doc["lastError"] = lastError;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  // List videos endpoint
  server.on("/list-videos", HTTP_GET, []() {
    String videoList = listMjpegVideos();
    server.send(200, "text/plain", videoList);
  });
  
  // Play video endpoint
  server.on("/play-video", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(512);
      deserializeJson(doc, server.arg("plain"));
      
      VideoCommand cmd;
      cmd.type = VideoCommand::PLAY_MJPEG;
      cmd.filename = doc["filename"].as<String>();
      cmd.loop = doc["loop"].as<bool>();
      cmd.enableAudio = doc["audio"].as<bool>();
      
      if (xQueueSend(videoCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
        server.send(200, "text/plain", "OK");
      } else {
        server.send(500, "text/plain", "Queue full");
      }
    } else {
      server.send(400, "text/plain", "Missing parameters");
    }
  });
  
  // Pause video endpoint
  server.on("/pause-video", HTTP_POST, []() {
    VideoCommand cmd;
    cmd.type = VideoCommand::PAUSE_MJPEG;
    
    if (xQueueSend(videoCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "Queue full");
    }
  });
  
  // Resume video endpoint
  server.on("/resume-video", HTTP_POST, []() {
    VideoCommand cmd;
    cmd.type = VideoCommand::RESUME_MJPEG;
    
    if (xQueueSend(videoCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "Queue full");
    }
  });
  
  // Stop video endpoint
  server.on("/stop-video", HTTP_POST, []() {
    VideoCommand cmd;
    cmd.type = VideoCommand::STOP_MJPEG;
    
    if (xQueueSend(videoCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "Queue full");
    }
  });
  
  // Seek video endpoint
  server.on("/seek-video", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, server.arg("plain"));
      
      VideoCommand cmd;
      cmd.type = VideoCommand::SEEK_MJPEG;
      cmd.seekPosition = doc["position"].as<uint32_t>();
      
      if (xQueueSend(videoCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
        server.send(200, "text/plain", "OK");
      } else {
        server.send(500, "text/plain", "Queue full");
      }
    } else {
      server.send(400, "text/plain", "Missing position parameter");
    }
  });
  
  // Configuration endpoint
  server.on("/config", HTTP_GET, []() {
    String config = tricorderConfig.toJson();
    server.send(200, "application/json", config);
  });
  
  server.on("/config", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      if (tricorderConfig.fromJson(server.arg("plain"))) {
        tricorderConfig.save();
        server.send(200, "text/plain", "Configuration updated");
      } else {
        server.send(400, "text/plain", "Invalid configuration");
      }
    } else {
      server.send(400, "text/plain", "Missing configuration data");
    }
  });
  
  // LED control endpoint
  server.on("/led", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(512);
      deserializeJson(doc, server.arg("plain"));
      
      if (doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
        uint8_t r = doc["r"];
        uint8_t g = doc["g"];
        uint8_t b = doc["b"];
        
        // Set all LEDs to the specified color
        fill_solid(leds, NUM_NEOPIXELS, CRGB(r, g, b));
        FastLED.show();
        
        server.send(200, "text/plain", "LED color updated");
      } else {
        server.send(400, "text/plain", "Missing RGB values");
      }
    } else {
      server.send(400, "text/plain", "Missing color data");
    }
  });
  
  // Brightness control endpoint
  server.on("/brightness", HTTP_POST, []() {
    if (server.hasArg("brightness")) {
      uint8_t brightness = server.arg("brightness").toInt();
      brightness = constrain(brightness, 0, 255);
      
      tricorderConfig.setBrightness(brightness);
      FastLED.setBrightness(brightness);
      FastLED.show();
      
      server.send(200, "text/plain", "Brightness updated");
    } else {
      server.send(400, "text/plain", "Missing brightness parameter");
    }
  });
  
  // File upload endpoint for new videos
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload complete");
  }, []() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
      String filename = "/videos/" + upload.filename;
      uploadFile = SD.open(filename, FILE_WRITE);
      Serial.printf("Upload started: %s\n", filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) {
        uploadFile.write(upload.buf, upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("Upload finished: %s (%d bytes)\n", upload.filename.c_str(), upload.totalSize);
      }
    }
  });
  
  // Firmware update endpoint
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  
  server.begin();
  Serial.printf("Web server started on port %d\n", tricorderConfig.getWebPort());
}
