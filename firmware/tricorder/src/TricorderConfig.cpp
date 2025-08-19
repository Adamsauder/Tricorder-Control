#include "TricorderConfig.h"

TricorderConfig::TricorderConfig() : initialized(false) {
}

TricorderConfig::~TricorderConfig() {
  if (initialized) {
    preferences.end();
  }
}

bool TricorderConfig::begin() {
  if (!preferences.begin("tricorder", false)) {
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

void TricorderConfig::setDefaults() {
  // Device settings
  strcpy(config.deviceLabel, "Tricorder-01");
  strcpy(config.propId, "TRIC001");
  strcpy(config.description, "Enhanced Tricorder Prop");
  config.fixtureNumber = 1;
  
  // SACN/DMX settings
  config.sacnUniverse = 1;
  config.dmxAddress = 1;
  config.sacnEnabled = true;
  
  // LED settings
  config.brightness = 128;
  
  // Network settings
  strcpy(config.wifiSSID, "Rigging Electric");
  strcpy(config.wifiPassword, "academy123");
  strcpy(config.staticIP, "");
  strcpy(config.hostname, "tricorder-01");
  
  // Video settings
  strcpy(config.defaultVideo, "startup.jpg");
  config.videoAutoPlay = true;
  config.displayBrightness = 200;
  
  // Battery monitoring settings
  config.batteryVoltageCalibration = 82.0;
  config.batteryMonitoringEnabled = true;
  
  // Advanced settings
  config.udpPort = 8888;
  config.webPort = 80;
  config.debugMode = false;
}

bool TricorderConfig::load() {
  if (!initialized) return false;
  
  size_t len = preferences.getBytesLength("config");
  if (len == 0 || len != sizeof(TricorderConfigData)) {
    return false;
  }
  
  return preferences.getBytes("config", &config, sizeof(TricorderConfigData)) == sizeof(TricorderConfigData);
}

bool TricorderConfig::save() {
  if (!initialized) return false;
  
  return preferences.putBytes("config", &config, sizeof(TricorderConfigData)) == sizeof(TricorderConfigData);
}

bool TricorderConfig::factoryReset() {
  if (!initialized) return false;
  
  bool cleared = preferences.clear();
  if (cleared) {
    setDefaults();
    save();
  }
  return cleared;
}

// Device settings
void TricorderConfig::setDeviceLabel(const char* label) {
  strncpy(config.deviceLabel, label, sizeof(config.deviceLabel) - 1);
  config.deviceLabel[sizeof(config.deviceLabel) - 1] = '\0';
}

const char* TricorderConfig::getDeviceLabel() const {
  return config.deviceLabel;
}

void TricorderConfig::setPropId(const char* id) {
  strncpy(config.propId, id, sizeof(config.propId) - 1);
  config.propId[sizeof(config.propId) - 1] = '\0';
}

const char* TricorderConfig::getPropId() const {
  return config.propId;
}

void TricorderConfig::setDescription(const char* desc) {
  strncpy(config.description, desc, sizeof(config.description) - 1);
  config.description[sizeof(config.description) - 1] = '\0';
}

const char* TricorderConfig::getDescription() const {
  return config.description;
}

void TricorderConfig::setFixtureNumber(uint16_t number) {
  config.fixtureNumber = number;
}

uint16_t TricorderConfig::getFixtureNumber() const {
  return config.fixtureNumber;
}

// SACN/DMX settings
void TricorderConfig::setSacnUniverse(uint16_t universe) {
  if (universe >= 1 && universe <= 63999) {
    config.sacnUniverse = universe;
  }
}

uint16_t TricorderConfig::getSacnUniverse() const {
  return config.sacnUniverse;
}

void TricorderConfig::setDmxAddress(uint16_t address) {
  if (address >= 1 && address <= 512) {
    config.dmxAddress = address;
  }
}

uint16_t TricorderConfig::getDmxAddress() const {
  return config.dmxAddress;
}

void TricorderConfig::setSacnEnabled(bool enabled) {
  config.sacnEnabled = enabled;
}

bool TricorderConfig::getSacnEnabled() const {
  return config.sacnEnabled;
}

// LED settings
void TricorderConfig::setBrightness(uint8_t brightness) {
  config.brightness = brightness;
}

uint8_t TricorderConfig::getBrightness() const {
  return config.brightness;
}

// Network settings
void TricorderConfig::setWiFiSSID(const char* ssid) {
  strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1);
  config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
}

const char* TricorderConfig::getWiFiSSID() const {
  return config.wifiSSID;
}

void TricorderConfig::setWiFiPassword(const char* password) {
  strncpy(config.wifiPassword, password, sizeof(config.wifiPassword) - 1);
  config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
}

const char* TricorderConfig::getWiFiPassword() const {
  return config.wifiPassword;
}

void TricorderConfig::setStaticIP(const char* ip) {
  strncpy(config.staticIP, ip, sizeof(config.staticIP) - 1);
  config.staticIP[sizeof(config.staticIP) - 1] = '\0';
}

const char* TricorderConfig::getStaticIP() const {
  return config.staticIP;
}

void TricorderConfig::setHostname(const char* hostname) {
  strncpy(config.hostname, hostname, sizeof(config.hostname) - 1);
  config.hostname[sizeof(config.hostname) - 1] = '\0';
}

const char* TricorderConfig::getHostname() const {
  return config.hostname;
}

// Video settings
void TricorderConfig::setDefaultVideo(const char* video) {
  strncpy(config.defaultVideo, video, sizeof(config.defaultVideo) - 1);
  config.defaultVideo[sizeof(config.defaultVideo) - 1] = '\0';
}

const char* TricorderConfig::getDefaultVideo() const {
  return config.defaultVideo;
}

void TricorderConfig::setVideoAutoPlay(bool autoPlay) {
  config.videoAutoPlay = autoPlay;
}

bool TricorderConfig::getVideoAutoPlay() const {
  return config.videoAutoPlay;
}

void TricorderConfig::setDisplayBrightness(uint8_t brightness) {
  config.displayBrightness = brightness;
}

uint8_t TricorderConfig::getDisplayBrightness() const {
  return config.displayBrightness;
}

// Battery monitoring settings
void TricorderConfig::setBatteryVoltageCalibration(float calibration) {
  if (calibration > 0) {
    config.batteryVoltageCalibration = calibration;
  }
}

float TricorderConfig::getBatteryVoltageCalibration() const {
  return config.batteryVoltageCalibration;
}

void TricorderConfig::setBatteryMonitoringEnabled(bool enabled) {
  config.batteryMonitoringEnabled = enabled;
}

bool TricorderConfig::getBatteryMonitoringEnabled() const {
  return config.batteryMonitoringEnabled;
}

// Advanced settings
void TricorderConfig::setUdpPort(uint16_t port) {
  if (port > 0) {
    config.udpPort = port;
  }
}

uint16_t TricorderConfig::getUdpPort() const {
  return config.udpPort;
}

void TricorderConfig::setWebPort(uint16_t port) {
  if (port > 0) {
    config.webPort = port;
  }
}

uint16_t TricorderConfig::getWebPort() const {
  return config.webPort;
}

void TricorderConfig::setDebugMode(bool enabled) {
  config.debugMode = enabled;
}

bool TricorderConfig::getDebugMode() const {
  return config.debugMode;
}

// JSON serialization
String TricorderConfig::toJson() const {
  DynamicJsonDocument doc(1024);
  
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
  
  // Network settings
  doc["wifiSSID"] = config.wifiSSID;
  doc["wifiPassword"] = config.wifiPassword;
  doc["staticIP"] = config.staticIP;
  doc["hostname"] = config.hostname;
  
  // Video settings
  doc["defaultVideo"] = config.defaultVideo;
  doc["videoAutoPlay"] = config.videoAutoPlay;
  doc["displayBrightness"] = config.displayBrightness;
  
  // Battery monitoring settings
  doc["batteryVoltageCalibration"] = config.batteryVoltageCalibration;
  doc["batteryMonitoringEnabled"] = config.batteryMonitoringEnabled;
  
  // Advanced settings
  doc["udpPort"] = config.udpPort;
  doc["webPort"] = config.webPort;
  doc["debugMode"] = config.debugMode;
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool TricorderConfig::fromJson(const String& json) {
  DynamicJsonDocument doc(1024);
  
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    return false;
  }
  
  // Device settings
  if (doc.containsKey("deviceLabel")) {
    setDeviceLabel(doc["deviceLabel"]);
  }
  if (doc.containsKey("propId")) {
    setPropId(doc["propId"]);
  }
  if (doc.containsKey("description")) {
    setDescription(doc["description"]);
  }
  if (doc.containsKey("fixtureNumber")) {
    setFixtureNumber(doc["fixtureNumber"]);
  }
  
  // SACN/DMX settings
  if (doc.containsKey("sacnUniverse")) {
    setSacnUniverse(doc["sacnUniverse"]);
  }
  if (doc.containsKey("dmxAddress")) {
    setDmxAddress(doc["dmxAddress"]);
  }
  if (doc.containsKey("sacnEnabled")) {
    setSacnEnabled(doc["sacnEnabled"]);
  }
  
  // LED settings
  if (doc.containsKey("brightness")) {
    setBrightness(doc["brightness"]);
  }
  
  // Network settings
  if (doc.containsKey("wifiSSID")) {
    setWiFiSSID(doc["wifiSSID"]);
  }
  if (doc.containsKey("wifiPassword")) {
    setWiFiPassword(doc["wifiPassword"]);
  }
  if (doc.containsKey("staticIP")) {
    setStaticIP(doc["staticIP"]);
  }
  if (doc.containsKey("hostname")) {
    setHostname(doc["hostname"]);
  }
  
  // Video settings
  if (doc.containsKey("defaultVideo")) {
    setDefaultVideo(doc["defaultVideo"]);
  }
  if (doc.containsKey("videoAutoPlay")) {
    setVideoAutoPlay(doc["videoAutoPlay"]);
  }
  if (doc.containsKey("displayBrightness")) {
    setDisplayBrightness(doc["displayBrightness"]);
  }
  
  // Battery monitoring settings
  if (doc.containsKey("batteryVoltageCalibration")) {
    setBatteryVoltageCalibration(doc["batteryVoltageCalibration"]);
  }
  if (doc.containsKey("batteryMonitoringEnabled")) {
    setBatteryMonitoringEnabled(doc["batteryMonitoringEnabled"]);
  }
  
  // Advanced settings
  if (doc.containsKey("udpPort")) {
    setUdpPort(doc["udpPort"]);
  }
  if (doc.containsKey("webPort")) {
    setWebPort(doc["webPort"]);
  }
  if (doc.containsKey("debugMode")) {
    setDebugMode(doc["debugMode"]);
  }
  
  return true;
}

bool TricorderConfig::isValid() const {
  // Basic validation
  if (strlen(config.deviceLabel) == 0) return false;
  if (strlen(config.propId) == 0) return false;
  if (strlen(config.wifiSSID) == 0) return false;
  if (config.sacnUniverse < 1 || config.sacnUniverse > 63999) return false;
  if (config.dmxAddress < 1 || config.dmxAddress > 512) return false;
  if (config.udpPort == 0 || config.webPort == 0) return false;
  
  return true;
}

String TricorderConfig::getValidationErrors() const {
  String errors = "";
  
  if (strlen(config.deviceLabel) == 0) {
    errors += "Device label cannot be empty. ";
  }
  if (strlen(config.propId) == 0) {
    errors += "Prop ID cannot be empty. ";
  }
  if (strlen(config.wifiSSID) == 0) {
    errors += "WiFi SSID cannot be empty. ";
  }
  if (config.sacnUniverse < 1 || config.sacnUniverse > 63999) {
    errors += "SACN universe must be between 1 and 63999. ";
  }
  if (config.dmxAddress < 1 || config.dmxAddress > 512) {
    errors += "DMX address must be between 1 and 512. ";
  }
  if (config.udpPort == 0) {
    errors += "UDP port must be greater than 0. ";
  }
  if (config.webPort == 0) {
    errors += "Web port must be greater than 0. ";
  }
  
  return errors;
}
