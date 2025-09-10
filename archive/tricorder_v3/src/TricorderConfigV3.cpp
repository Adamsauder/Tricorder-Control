#include "TricorderConfigV3.h"
#include <esp_system.h>

TricorderConfigV3::TricorderConfigV3() : initialized(false) {
}

TricorderConfigV3::~TricorderConfigV3() {
  if (initialized) {
    preferences.end();
  }
}

bool TricorderConfigV3::begin() {
  if (!preferences.begin("tricorderv3", false)) {
    Serial.println("Failed to initialize preferences");
    return false;
  }
  
  initialized = true;
  
  // Load existing configuration or set defaults
  if (!load()) {
    Serial.println("No existing configuration found, setting defaults");
    setDefaults();
    save();
  }
  
  return true;
}

String TricorderConfigV3::generateUniqueId() {
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
  snprintf(uniqueId, sizeof(uniqueId), "TRV3%04X", uniquePart);  // V3 identifier
  
  Serial.printf("Generated device ID: %s\n", uniqueId);
  
  return String(uniqueId);
}

void TricorderConfigV3::setDefaults() {
  // Generate a unique device ID based on chip ID
  String uniqueId = generateUniqueId();
  
  // Device settings
  strcpy(config.deviceLabel, ("Tricorder-V3-" + uniqueId.substring(4)).c_str()); // Use last part after "TRV3"
  strcpy(config.propId, uniqueId.c_str());
  strcpy(config.description, "Enhanced Tricorder V3 with MPEG Video");
  config.fixtureNumber = 1;
  
  // SACN/DMX settings
  config.sacnUniverse = 1;
  config.dmxAddress = 1;
  config.sacnEnabled = true;
  
  // LED settings
  config.brightness = 128;
  
  // Set default colors to off (black)
  for (int i = 0; i < 12; i++) {
    config.defaultColorR[i] = 0;
    config.defaultColorG[i] = 0;
    config.defaultColorB[i] = 0;
  }
  config.useDefaultColors = false;
  
  // Network settings
  strcpy(config.wifiSSID, "Rigging Electric");
  strcpy(config.wifiPassword, "academy123");
  strcpy(config.staticIP, "");
  
  String hostnameSuffix = uniqueId.substring(4);
  hostnameSuffix.toLowerCase();
  strcpy(config.hostname, ("tricorder-v3-" + hostnameSuffix).c_str());
  
  // Enhanced Video settings for v3
  strcpy(config.defaultVideo, "startup.mpeg");  // Default to MPEG format
  config.videoAutoPlay = true;
  config.displayBrightness = 200;
  config.videoQuality = 7;              // Medium-high quality (1-10 scale)
  config.videoAudioEnabled = false;     // Disable audio by default (no speaker)
  config.videoFrameRate = 25;           // 25 FPS for smooth playback
  config.videoBufferSize = 8192;        // 8KB buffer for streaming (reduced for memory)
  config.videoLooping = true;           // Loop videos by default
  config.videoScaling = true;           // Scale to fit display
  
  // Battery monitoring settings
  config.batteryVoltageCalibration = 82.0;
  config.batteryMonitoringEnabled = true;
  
  // Advanced settings
  config.udpPort = 8888;
  config.webPort = 80;
  config.debugMode = false;
  
  // Performance settings (New for v3)
  config.cpuFrequency = 240;            // 240 MHz for video processing
  config.heapThreshold = 32768;         // 32KB minimum free heap
  config.videoPreloading = true;        // Preload for smoother playback
}

bool TricorderConfigV3::load() {
  if (!initialized) return false;
  
  size_t loaded = preferences.getBytes("config", &config, sizeof(config));
  if (loaded != sizeof(config)) {
    Serial.printf("Config size mismatch: expected %d, got %d\n", sizeof(config), loaded);
    return false;
  }
  
  Serial.println("Configuration loaded successfully");
  return true;
}

bool TricorderConfigV3::save() {
  if (!initialized) return false;
  
  size_t written = preferences.putBytes("config", &config, sizeof(config));
  if (written != sizeof(config)) {
    Serial.printf("Config save failed: expected %d, wrote %d\n", sizeof(config), written);
    return false;
  }
  
  Serial.println("Configuration saved successfully");
  return true;
}

bool TricorderConfigV3::factoryReset() {
  if (!initialized) return false;
  
  preferences.clear();
  setDefaults();
  return save();
}

// Device settings implementation
void TricorderConfigV3::setDeviceLabel(const char* label) {
  if (label && strlen(label) < sizeof(config.deviceLabel)) {
    strcpy(config.deviceLabel, label);
  }
}

const char* TricorderConfigV3::getDeviceLabel() const {
  return config.deviceLabel;
}

void TricorderConfigV3::setPropId(const char* id) {
  if (id && strlen(id) < sizeof(config.propId)) {
    strcpy(config.propId, id);
  }
}

const char* TricorderConfigV3::getPropId() const {
  return config.propId;
}

void TricorderConfigV3::setDescription(const char* desc) {
  if (desc && strlen(desc) < sizeof(config.description)) {
    strcpy(config.description, desc);
  }
}

const char* TricorderConfigV3::getDescription() const {
  return config.description;
}

void TricorderConfigV3::setFixtureNumber(uint16_t number) {
  config.fixtureNumber = number;
}

uint16_t TricorderConfigV3::getFixtureNumber() const {
  return config.fixtureNumber;
}

// SACN/DMX settings implementation
void TricorderConfigV3::setSacnUniverse(uint16_t universe) {
  config.sacnUniverse = universe;
}

uint16_t TricorderConfigV3::getSacnUniverse() const {
  return config.sacnUniverse;
}

void TricorderConfigV3::setDmxAddress(uint16_t address) {
  config.dmxAddress = address;
}

uint16_t TricorderConfigV3::getDmxAddress() const {
  return config.dmxAddress;
}

void TricorderConfigV3::setSacnEnabled(bool enabled) {
  config.sacnEnabled = enabled;
}

bool TricorderConfigV3::getSacnEnabled() const {
  return config.sacnEnabled;
}

// LED settings implementation
void TricorderConfigV3::setBrightness(uint8_t brightness) {
  config.brightness = brightness;
}

uint8_t TricorderConfigV3::getBrightness() const {
  return config.brightness;
}

void TricorderConfigV3::setDefaultColors(const uint8_t* red, const uint8_t* green, const uint8_t* blue) {
  if (red && green && blue) {
    memcpy(config.defaultColorR, red, 12);
    memcpy(config.defaultColorG, green, 12);
    memcpy(config.defaultColorB, blue, 12);
  }
}

void TricorderConfigV3::getDefaultColors(uint8_t* red, uint8_t* green, uint8_t* blue) const {
  if (red && green && blue) {
    memcpy(red, config.defaultColorR, 12);
    memcpy(green, config.defaultColorG, 12);
    memcpy(blue, config.defaultColorB, 12);
  }
}

void TricorderConfigV3::setUseDefaultColors(bool use) {
  config.useDefaultColors = use;
}

bool TricorderConfigV3::getUseDefaultColors() const {
  return config.useDefaultColors;
}

// Network settings implementation
void TricorderConfigV3::setWiFiSSID(const char* ssid) {
  if (ssid && strlen(ssid) < sizeof(config.wifiSSID)) {
    strcpy(config.wifiSSID, ssid);
  }
}

const char* TricorderConfigV3::getWiFiSSID() const {
  return config.wifiSSID;
}

void TricorderConfigV3::setWiFiPassword(const char* password) {
  if (password && strlen(password) < sizeof(config.wifiPassword)) {
    strcpy(config.wifiPassword, password);
  }
}

const char* TricorderConfigV3::getWiFiPassword() const {
  return config.wifiPassword;
}

void TricorderConfigV3::setStaticIP(const char* ip) {
  if (ip && strlen(ip) < sizeof(config.staticIP)) {
    strcpy(config.staticIP, ip);
  }
}

const char* TricorderConfigV3::getStaticIP() const {
  return config.staticIP;
}

void TricorderConfigV3::setHostname(const char* hostname) {
  if (hostname && strlen(hostname) < sizeof(config.hostname)) {
    strcpy(config.hostname, hostname);
  }
}

const char* TricorderConfigV3::getHostname() const {
  return config.hostname;
}

// Enhanced Video settings implementation (v3)
void TricorderConfigV3::setDefaultVideo(const char* video) {
  if (video && strlen(video) < sizeof(config.defaultVideo)) {
    strcpy(config.defaultVideo, video);
  }
}

const char* TricorderConfigV3::getDefaultVideo() const {
  return config.defaultVideo;
}

void TricorderConfigV3::setVideoAutoPlay(bool autoPlay) {
  config.videoAutoPlay = autoPlay;
}

bool TricorderConfigV3::getVideoAutoPlay() const {
  return config.videoAutoPlay;
}

void TricorderConfigV3::setDisplayBrightness(uint8_t brightness) {
  config.displayBrightness = brightness;
}

uint8_t TricorderConfigV3::getDisplayBrightness() const {
  return config.displayBrightness;
}

void TricorderConfigV3::setVideoQuality(uint8_t quality) {
  config.videoQuality = constrain(quality, 1, 10);
}

uint8_t TricorderConfigV3::getVideoQuality() const {
  return config.videoQuality;
}

void TricorderConfigV3::setVideoAudioEnabled(bool enabled) {
  config.videoAudioEnabled = enabled;
}

bool TricorderConfigV3::getVideoAudioEnabled() const {
  return config.videoAudioEnabled;
}

void TricorderConfigV3::setVideoFrameRate(uint16_t frameRate) {
  config.videoFrameRate = constrain(frameRate, 1, 60);
}

uint16_t TricorderConfigV3::getVideoFrameRate() const {
  return config.videoFrameRate;
}

void TricorderConfigV3::setVideoBufferSize(uint32_t bufferSize) {
  config.videoBufferSize = constrain(bufferSize, 8192, 131072); // 8KB to 128KB
}

uint32_t TricorderConfigV3::getVideoBufferSize() const {
  return config.videoBufferSize;
}

void TricorderConfigV3::setVideoLooping(bool looping) {
  config.videoLooping = looping;
}

bool TricorderConfigV3::getVideoLooping() const {
  return config.videoLooping;
}

void TricorderConfigV3::setVideoScaling(bool scaling) {
  config.videoScaling = scaling;
}

bool TricorderConfigV3::getVideoScaling() const {
  return config.videoScaling;
}

// Battery monitoring settings implementation
void TricorderConfigV3::setBatteryVoltageCalibration(float calibration) {
  config.batteryVoltageCalibration = calibration;
}

float TricorderConfigV3::getBatteryVoltageCalibration() const {
  return config.batteryVoltageCalibration;
}

void TricorderConfigV3::setBatteryMonitoringEnabled(bool enabled) {
  config.batteryMonitoringEnabled = enabled;
}

bool TricorderConfigV3::getBatteryMonitoringEnabled() const {
  return config.batteryMonitoringEnabled;
}

// Advanced settings implementation
void TricorderConfigV3::setUdpPort(uint16_t port) {
  config.udpPort = port;
}

uint16_t TricorderConfigV3::getUdpPort() const {
  return config.udpPort;
}

void TricorderConfigV3::setWebPort(uint16_t port) {
  config.webPort = port;
}

uint16_t TricorderConfigV3::getWebPort() const {
  return config.webPort;
}

void TricorderConfigV3::setDebugMode(bool enabled) {
  config.debugMode = enabled;
}

bool TricorderConfigV3::getDebugMode() const {
  return config.debugMode;
}

// Performance settings implementation (New for v3)
void TricorderConfigV3::setCpuFrequency(uint8_t frequency) {
  // Only allow valid frequencies: 80, 160, 240 MHz
  if (frequency == 80 || frequency == 160 || frequency == 240) {
    config.cpuFrequency = frequency;
  }
}

uint8_t TricorderConfigV3::getCpuFrequency() const {
  return config.cpuFrequency;
}

void TricorderConfigV3::setHeapThreshold(uint16_t threshold) {
  config.heapThreshold = threshold;
}

uint16_t TricorderConfigV3::getHeapThreshold() const {
  return config.heapThreshold;
}

void TricorderConfigV3::setVideoPreloading(bool preloading) {
  config.videoPreloading = preloading;
}

bool TricorderConfigV3::getVideoPreloading() const {
  return config.videoPreloading;
}

// JSON serialization implementation
String TricorderConfigV3::toJson() const {
  DynamicJsonDocument doc(2048);
  
  // Device settings
  doc["deviceLabel"] = config.deviceLabel;
  doc["propId"] = config.propId;
  doc["description"] = config.description;
  doc["fixtureNumber"] = config.fixtureNumber;
  
  // SACN/DMX settings
  doc["sacnUniverse"] = config.sacnUniverse;
  doc["dmxAddress"] = config.dmxAddress;
  doc["sacnEnabled"] = config.sacnEnabled;
  
  // LED settings
  doc["brightness"] = config.brightness;
  doc["useDefaultColors"] = config.useDefaultColors;
  
  // Network settings
  doc["wifiSSID"] = config.wifiSSID;
  doc["staticIP"] = config.staticIP;
  doc["hostname"] = config.hostname;
  
  // Enhanced Video settings
  doc["defaultVideo"] = config.defaultVideo;
  doc["videoAutoPlay"] = config.videoAutoPlay;
  doc["displayBrightness"] = config.displayBrightness;
  doc["videoQuality"] = config.videoQuality;
  doc["videoAudioEnabled"] = config.videoAudioEnabled;
  doc["videoFrameRate"] = config.videoFrameRate;
  doc["videoBufferSize"] = config.videoBufferSize;
  doc["videoLooping"] = config.videoLooping;
  doc["videoScaling"] = config.videoScaling;
  
  // Battery settings
  doc["batteryVoltageCalibration"] = config.batteryVoltageCalibration;
  doc["batteryMonitoringEnabled"] = config.batteryMonitoringEnabled;
  
  // Advanced settings
  doc["udpPort"] = config.udpPort;
  doc["webPort"] = config.webPort;
  doc["debugMode"] = config.debugMode;
  
  // Performance settings
  doc["cpuFrequency"] = config.cpuFrequency;
  doc["heapThreshold"] = config.heapThreshold;
  doc["videoPreloading"] = config.videoPreloading;
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool TricorderConfigV3::fromJson(const String& json) {
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.printf("JSON parsing failed: %s\n", error.c_str());
    return false;
  }
  
  // Update configuration from JSON
  if (doc.containsKey("deviceLabel")) setDeviceLabel(doc["deviceLabel"]);
  if (doc.containsKey("propId")) setPropId(doc["propId"]);
  if (doc.containsKey("description")) setDescription(doc["description"]);
  if (doc.containsKey("fixtureNumber")) setFixtureNumber(doc["fixtureNumber"]);
  
  if (doc.containsKey("sacnUniverse")) setSacnUniverse(doc["sacnUniverse"]);
  if (doc.containsKey("dmxAddress")) setDmxAddress(doc["dmxAddress"]);
  if (doc.containsKey("sacnEnabled")) setSacnEnabled(doc["sacnEnabled"]);
  
  if (doc.containsKey("brightness")) setBrightness(doc["brightness"]);
  if (doc.containsKey("useDefaultColors")) setUseDefaultColors(doc["useDefaultColors"]);
  
  if (doc.containsKey("wifiSSID")) setWiFiSSID(doc["wifiSSID"]);
  if (doc.containsKey("staticIP")) setStaticIP(doc["staticIP"]);
  if (doc.containsKey("hostname")) setHostname(doc["hostname"]);
  
  // Enhanced video settings
  if (doc.containsKey("defaultVideo")) setDefaultVideo(doc["defaultVideo"]);
  if (doc.containsKey("videoAutoPlay")) setVideoAutoPlay(doc["videoAutoPlay"]);
  if (doc.containsKey("displayBrightness")) setDisplayBrightness(doc["displayBrightness"]);
  if (doc.containsKey("videoQuality")) setVideoQuality(doc["videoQuality"]);
  if (doc.containsKey("videoAudioEnabled")) setVideoAudioEnabled(doc["videoAudioEnabled"]);
  if (doc.containsKey("videoFrameRate")) setVideoFrameRate(doc["videoFrameRate"]);
  if (doc.containsKey("videoBufferSize")) setVideoBufferSize(doc["videoBufferSize"]);
  if (doc.containsKey("videoLooping")) setVideoLooping(doc["videoLooping"]);
  if (doc.containsKey("videoScaling")) setVideoScaling(doc["videoScaling"]);
  
  if (doc.containsKey("batteryVoltageCalibration")) setBatteryVoltageCalibration(doc["batteryVoltageCalibration"]);
  if (doc.containsKey("batteryMonitoringEnabled")) setBatteryMonitoringEnabled(doc["batteryMonitoringEnabled"]);
  
  if (doc.containsKey("udpPort")) setUdpPort(doc["udpPort"]);
  if (doc.containsKey("webPort")) setWebPort(doc["webPort"]);
  if (doc.containsKey("debugMode")) setDebugMode(doc["debugMode"]);
  
  // Performance settings
  if (doc.containsKey("cpuFrequency")) setCpuFrequency(doc["cpuFrequency"]);
  if (doc.containsKey("heapThreshold")) setHeapThreshold(doc["heapThreshold"]);
  if (doc.containsKey("videoPreloading")) setVideoPreloading(doc["videoPreloading"]);
  
  return true;
}

bool TricorderConfigV3::isValid() const {
  // Basic validation
  if (strlen(config.deviceLabel) == 0) return false;
  if (strlen(config.propId) == 0) return false;
  if (config.sacnUniverse == 0 || config.sacnUniverse > 63999) return false;
  if (config.dmxAddress == 0 || config.dmxAddress > 512) return false;
  if (config.videoQuality < 1 || config.videoQuality > 10) return false;
  if (config.videoFrameRate == 0 || config.videoFrameRate > 60) return false;
  if (config.cpuFrequency != 80 && config.cpuFrequency != 160 && config.cpuFrequency != 240) return false;
  
  return true;
}

String TricorderConfigV3::getValidationErrors() const {
  String errors = "";
  
  if (strlen(config.deviceLabel) == 0) errors += "Device label is required. ";
  if (strlen(config.propId) == 0) errors += "Prop ID is required. ";
  if (config.sacnUniverse == 0 || config.sacnUniverse > 63999) errors += "sACN universe must be 1-63999. ";
  if (config.dmxAddress == 0 || config.dmxAddress > 512) errors += "DMX address must be 1-512. ";
  if (config.videoQuality < 1 || config.videoQuality > 10) errors += "Video quality must be 1-10. ";
  if (config.videoFrameRate == 0 || config.videoFrameRate > 60) errors += "Video frame rate must be 1-60. ";
  if (config.cpuFrequency != 80 && config.cpuFrequency != 160 && config.cpuFrequency != 240) {
    errors += "CPU frequency must be 80, 160, or 240 MHz. ";
  }
  
  return errors;
}
