#include "PolyinoculatorConfig.h"

PolyinoculatorConfig::PolyinoculatorConfig() : isLoaded(false) {
  setDefaults();
}

void PolyinoculatorConfig::setDefaults() {
  // Device settings
  strcpy(config.deviceLabel, "Polyinoculator");
  strcpy(config.propId, "POLY001");
  strcpy(config.description, "Enhanced Polyinoculator LED Controller");
  config.fixtureNumber = 1;
  
  // SACN/DMX settings
  config.sacnUniverse = 1;
  config.dmxAddress = 1;
  config.sacnEnabled = true;
  
  // LED settings
  config.brightness = 255;
  config.useDefaultColors = false;
  
  // LED strip configuration - D3, D4 (longer), D5
  config.strip1Length = 30;   // D3
  config.strip2Length = 60;   // D4 - longer ribbon
  config.strip3Length = 30;   // D5
  
  // Initialize default colors to off
  memset(config.strip1DefaultColorR, 0, sizeof(config.strip1DefaultColorR));
  memset(config.strip1DefaultColorG, 0, sizeof(config.strip1DefaultColorG));
  memset(config.strip1DefaultColorB, 0, sizeof(config.strip1DefaultColorB));
  memset(config.strip2DefaultColorR, 0, sizeof(config.strip2DefaultColorR));
  memset(config.strip2DefaultColorG, 0, sizeof(config.strip2DefaultColorG));
  memset(config.strip2DefaultColorB, 0, sizeof(config.strip2DefaultColorB));
  memset(config.strip3DefaultColorR, 0, sizeof(config.strip3DefaultColorR));
  memset(config.strip3DefaultColorG, 0, sizeof(config.strip3DefaultColorG));
  memset(config.strip3DefaultColorB, 0, sizeof(config.strip3DefaultColorB));
  
  // Network settings
  strcpy(config.wifiSSID, "Your_WiFi_SSID");
  strcpy(config.wifiPassword, "Your_WiFi_Password");
  strcpy(config.staticIP, "");
  strcpy(config.hostname, "polyinoculator");
  
  // Battery monitoring
  config.batteryVoltageCalibration = 1.0;
  config.batteryMonitoringEnabled = true;
  
  // Advanced settings
  config.udpPort = 8888;
  config.webPort = 80;
  config.debugMode = false;
  config.otaEnabled = true;
}

bool PolyinoculatorConfig::load() {
  if (!preferences.begin("polyinoculator", false)) {
    Serial.println("⚠️ Failed to initialize preferences");
    return false;
  }
  
  size_t configSize = preferences.getBytesLength("config");
  if (configSize == 0 || configSize != sizeof(PolyinoculatorConfigData)) {
    Serial.println("📝 No valid config found, using defaults");
    preferences.end();
    isLoaded = true;
    return save(); // Save defaults
  }
  
  size_t bytesRead = preferences.getBytes("config", &config, sizeof(PolyinoculatorConfigData));
  preferences.end();
  
  if (bytesRead != sizeof(PolyinoculatorConfigData)) {
    Serial.println("⚠️ Config size mismatch, using defaults");
    setDefaults();
    return save();
  }
  
  isLoaded = true;
  Serial.println("✅ Configuration loaded from NVS");
  return true;
}

bool PolyinoculatorConfig::save() {
  if (!preferences.begin("polyinoculator", false)) {
    Serial.println("⚠️ Failed to initialize preferences for saving");
    return false;
  }
  
  size_t bytesWritten = preferences.putBytes("config", &config, sizeof(PolyinoculatorConfigData));
  preferences.end();
  
  if (bytesWritten != sizeof(PolyinoculatorConfigData)) {
    Serial.println("❌ Failed to save configuration");
    return false;
  }
  
  Serial.println("✅ Configuration saved to NVS");
  return true;
}

void PolyinoculatorConfig::reset() {
  preferences.begin("polyinoculator", false);
  preferences.clear();
  preferences.end();
  setDefaults();
  Serial.println("🔄 Configuration reset to defaults");
}

// Device configuration methods
void PolyinoculatorConfig::setDeviceLabel(const String& label) {
  strncpy(config.deviceLabel, label.c_str(), sizeof(config.deviceLabel) - 1);
  config.deviceLabel[sizeof(config.deviceLabel) - 1] = '\0';
}

void PolyinoculatorConfig::setPropId(const String& id) {
  strncpy(config.propId, id.c_str(), sizeof(config.propId) - 1);
  config.propId[sizeof(config.propId) - 1] = '\0';
}

void PolyinoculatorConfig::setDescription(const String& desc) {
  strncpy(config.description, desc.c_str(), sizeof(config.description) - 1);
  config.description[sizeof(config.description) - 1] = '\0';
}

void PolyinoculatorConfig::setFixtureNumber(uint16_t number) {
  config.fixtureNumber = number;
}

// SACN/DMX configuration methods
void PolyinoculatorConfig::setSacnUniverse(uint16_t universe) {
  config.sacnUniverse = universe;
}

void PolyinoculatorConfig::setDmxAddress(uint16_t address) {
  config.dmxAddress = address;
}

void PolyinoculatorConfig::setSacnEnabled(bool enabled) {
  config.sacnEnabled = enabled;
}

// LED configuration methods
void PolyinoculatorConfig::setBrightness(uint8_t brightness) {
  config.brightness = brightness;
}

void PolyinoculatorConfig::setDefaultColors(uint8_t strip, const uint8_t* r, const uint8_t* g, const uint8_t* b, uint16_t length) {
  uint8_t* targetR = nullptr;
  uint8_t* targetG = nullptr;
  uint8_t* targetB = nullptr;
  uint16_t maxLength = 0;
  
  switch (strip) {
    case 1:
      targetR = config.strip1DefaultColorR;
      targetG = config.strip1DefaultColorG;
      targetB = config.strip1DefaultColorB;
      maxLength = sizeof(config.strip1DefaultColorR);
      break;
    case 2:
      targetR = config.strip2DefaultColorR;
      targetG = config.strip2DefaultColorG;
      targetB = config.strip2DefaultColorB;
      maxLength = sizeof(config.strip2DefaultColorR);
      break;
    case 3:
      targetR = config.strip3DefaultColorR;
      targetG = config.strip3DefaultColorG;
      targetB = config.strip3DefaultColorB;
      maxLength = sizeof(config.strip3DefaultColorR);
      break;
    default:
      return;
  }
  
  uint16_t copyLength = min(length, maxLength);
  memcpy(targetR, r, copyLength);
  memcpy(targetG, g, copyLength);
  memcpy(targetB, b, copyLength);
}

void PolyinoculatorConfig::setUseDefaultColors(bool use) {
  config.useDefaultColors = use;
}

void PolyinoculatorConfig::setStripLength(uint8_t strip, uint16_t length) {
  switch (strip) {
    case 1:
      config.strip1Length = min(length, (uint16_t)30);
      break;
    case 2:
      config.strip2Length = min(length, (uint16_t)60);
      break;
    case 3:
      config.strip3Length = min(length, (uint16_t)30);
      break;
  }
}

uint16_t PolyinoculatorConfig::getStripLength(uint8_t strip) const {
  switch (strip) {
    case 1: return config.strip1Length;
    case 2: return config.strip2Length;
    case 3: return config.strip3Length;
    default: return 0;
  }
}

// Network configuration methods
void PolyinoculatorConfig::setWiFiCredentials(const String& ssid, const String& password) {
  strncpy(config.wifiSSID, ssid.c_str(), sizeof(config.wifiSSID) - 1);
  config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
  strncpy(config.wifiPassword, password.c_str(), sizeof(config.wifiPassword) - 1);
  config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
}

void PolyinoculatorConfig::setStaticIP(const String& ip) {
  strncpy(config.staticIP, ip.c_str(), sizeof(config.staticIP) - 1);
  config.staticIP[sizeof(config.staticIP) - 1] = '\0';
}

void PolyinoculatorConfig::setHostname(const String& hostname) {
  strncpy(config.hostname, hostname.c_str(), sizeof(config.hostname) - 1);
  config.hostname[sizeof(config.hostname) - 1] = '\0';
}

// Battery configuration methods
void PolyinoculatorConfig::setBatteryCalibration(float calibration) {
  config.batteryVoltageCalibration = calibration;
}

void PolyinoculatorConfig::setBatteryMonitoringEnabled(bool enabled) {
  config.batteryMonitoringEnabled = enabled;
}

// Advanced configuration methods
void PolyinoculatorConfig::setUdpPort(uint16_t port) {
  config.udpPort = port;
}

void PolyinoculatorConfig::setWebPort(uint16_t port) {
  config.webPort = port;
}

void PolyinoculatorConfig::setDebugMode(bool debug) {
  config.debugMode = debug;
}

void PolyinoculatorConfig::setOtaEnabled(bool enabled) {
  config.otaEnabled = enabled;
}

String PolyinoculatorConfig::toJson() const {
  DynamicJsonDocument doc(2048);
  
  doc["deviceLabel"] = config.deviceLabel;
  doc["propId"] = config.propId;
  doc["description"] = config.description;
  doc["fixtureNumber"] = config.fixtureNumber;
  doc["sacnUniverse"] = config.sacnUniverse;
  doc["dmxAddress"] = config.dmxAddress;
  doc["sacnEnabled"] = config.sacnEnabled;
  doc["brightness"] = config.brightness;
  doc["useDefaultColors"] = config.useDefaultColors;
  doc["strip1Length"] = config.strip1Length;
  doc["strip2Length"] = config.strip2Length;
  doc["strip3Length"] = config.strip3Length;
  doc["wifiSSID"] = config.wifiSSID;
  doc["staticIP"] = config.staticIP;
  doc["hostname"] = config.hostname;
  doc["batteryCalibration"] = config.batteryVoltageCalibration;
  doc["batteryMonitoringEnabled"] = config.batteryMonitoringEnabled;
  doc["udpPort"] = config.udpPort;
  doc["webPort"] = config.webPort;
  doc["debugMode"] = config.debugMode;
  doc["otaEnabled"] = config.otaEnabled;
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool PolyinoculatorConfig::fromJson(const String& json) {
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.printf("❌ JSON parsing failed: %s\n", error.c_str());
    return false;
  }
  
  if (doc.containsKey("deviceLabel")) setDeviceLabel(doc["deviceLabel"].as<String>());
  if (doc.containsKey("propId")) setPropId(doc["propId"].as<String>());
  if (doc.containsKey("description")) setDescription(doc["description"].as<String>());
  if (doc.containsKey("fixtureNumber")) setFixtureNumber(doc["fixtureNumber"]);
  if (doc.containsKey("sacnUniverse")) setSacnUniverse(doc["sacnUniverse"]);
  if (doc.containsKey("dmxAddress")) setDmxAddress(doc["dmxAddress"]);
  if (doc.containsKey("sacnEnabled")) setSacnEnabled(doc["sacnEnabled"]);
  if (doc.containsKey("brightness")) setBrightness(doc["brightness"]);
  if (doc.containsKey("useDefaultColors")) setUseDefaultColors(doc["useDefaultColors"]);
  if (doc.containsKey("strip1Length")) setStripLength(1, doc["strip1Length"]);
  if (doc.containsKey("strip2Length")) setStripLength(2, doc["strip2Length"]);
  if (doc.containsKey("strip3Length")) setStripLength(3, doc["strip3Length"]);
  if (doc.containsKey("wifiSSID") && doc.containsKey("wifiPassword")) {
    setWiFiCredentials(doc["wifiSSID"], doc["wifiPassword"]);
  }
  if (doc.containsKey("staticIP")) setStaticIP(doc["staticIP"]);
  if (doc.containsKey("hostname")) setHostname(doc["hostname"]);
  if (doc.containsKey("batteryCalibration")) setBatteryCalibration(doc["batteryCalibration"]);
  if (doc.containsKey("batteryMonitoringEnabled")) setBatteryMonitoringEnabled(doc["batteryMonitoringEnabled"]);
  if (doc.containsKey("udpPort")) setUdpPort(doc["udpPort"]);
  if (doc.containsKey("webPort")) setWebPort(doc["webPort"]);
  if (doc.containsKey("debugMode")) setDebugMode(doc["debugMode"]);
  if (doc.containsKey("otaEnabled")) setOtaEnabled(doc["otaEnabled"]);
  
  return true;
}
