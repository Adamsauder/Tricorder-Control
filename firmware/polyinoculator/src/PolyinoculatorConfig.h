#ifndef POLYINOCULATOR_CONFIG_H
#define POLYINOCULATOR_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

struct PolyinoculatorConfigData {
  // Device settings
  char deviceLabel[32];
  char propId[16];
  char description[64];
  uint16_t fixtureNumber;     // Fixture/Channel number for lighting console
  
  // SACN/DMX settings
  uint16_t sacnUniverse;
  uint16_t dmxAddress;
  bool sacnEnabled;
  
  // LED settings
  uint8_t brightness;
  uint8_t strip1DefaultColorR[30];  // Default red values for Strip 1 LEDs (D3)
  uint8_t strip1DefaultColorG[30];  // Default green values for Strip 1 LEDs
  uint8_t strip1DefaultColorB[30];  // Default blue values for Strip 1 LEDs
  uint8_t strip2DefaultColorR[60];  // Default red values for Strip 2 LEDs (D4 - longer ribbon)
  uint8_t strip2DefaultColorG[60];  // Default green values for Strip 2 LEDs
  uint8_t strip2DefaultColorB[60];  // Default blue values for Strip 2 LEDs
  uint8_t strip3DefaultColorR[30];  // Default red values for Strip 3 LEDs (D5)
  uint8_t strip3DefaultColorG[30];  // Default green values for Strip 3 LEDs
  uint8_t strip3DefaultColorB[30];  // Default blue values for Strip 3 LEDs
  bool useDefaultColors;      // Whether to use default colors on startup
  
  // LED strip configuration
  uint16_t strip1Length;      // Number of LEDs in strip 1 (D3)
  uint16_t strip2Length;      // Number of LEDs in strip 2 (D4 - longer)
  uint16_t strip3Length;      // Number of LEDs in strip 3 (D5)
  
  // Network settings
  char wifiSSID[32];
  char wifiPassword[64];
  char staticIP[16];
  char hostname[32];
  
  // Battery monitoring settings
  float batteryVoltageCalibration;
  bool batteryMonitoringEnabled;
  
  // Advanced settings
  uint16_t udpPort;
  uint16_t webPort;
  bool debugMode;
  bool otaEnabled;
};

class PolyinoculatorConfig {
private:
  Preferences preferences;
  PolyinoculatorConfigData config;
  bool isLoaded;

public:
  PolyinoculatorConfig();
  
  // Core configuration methods
  bool load();
  bool save();
  void reset();
  void setDefaults();
  
  // Device configuration
  void setDeviceLabel(const String& label);
  void setPropId(const String& id);
  void setDescription(const String& desc);
  void setFixtureNumber(uint16_t number);
  
  // SACN/DMX configuration
  void setSacnUniverse(uint16_t universe);
  void setDmxAddress(uint16_t address);
  void setSacnEnabled(bool enabled);
  
  // LED configuration
  void setBrightness(uint8_t brightness);
  void setDefaultColors(uint8_t strip, const uint8_t* r, const uint8_t* g, const uint8_t* b, uint16_t length);
  void setUseDefaultColors(bool use);
  void setStripLength(uint8_t strip, uint16_t length);
  
  // Network configuration
  void setWiFiCredentials(const String& ssid, const String& password);
  void setStaticIP(const String& ip);
  void setHostname(const String& hostname);
  
  // Battery configuration
  void setBatteryCalibration(float calibration);
  void setBatteryMonitoringEnabled(bool enabled);
  
  // Advanced configuration
  void setUdpPort(uint16_t port);
  void setWebPort(uint16_t port);
  void setDebugMode(bool debug);
  void setOtaEnabled(bool enabled);
  
  // Getters
  const PolyinoculatorConfigData& getConfig() const { return config; }
  String getDeviceLabel() const { return String(config.deviceLabel); }
  String getPropId() const { return String(config.propId); }
  String getDescription() const { return String(config.description); }
  uint16_t getFixtureNumber() const { return config.fixtureNumber; }
  uint16_t getSacnUniverse() const { return config.sacnUniverse; }
  uint16_t getDmxAddress() const { return config.dmxAddress; }
  bool isSacnEnabled() const { return config.sacnEnabled; }
  uint8_t getBrightness() const { return config.brightness; }
  bool useDefaultColors() const { return config.useDefaultColors; }
  uint16_t getStripLength(uint8_t strip) const;
  String getWiFiSSID() const { return String(config.wifiSSID); }
  String getWiFiPassword() const { return String(config.wifiPassword); }
  String getStaticIP() const { return String(config.staticIP); }
  String getHostname() const { return String(config.hostname); }
  float getBatteryCalibration() const { return config.batteryVoltageCalibration; }
  bool isBatteryMonitoringEnabled() const { return config.batteryMonitoringEnabled; }
  uint16_t getUdpPort() const { return config.udpPort; }
  uint16_t getWebPort() const { return config.webPort; }
  bool isDebugMode() const { return config.debugMode; }
  bool isOtaEnabled() const { return config.otaEnabled; }
  
  // JSON serialization
  String toJson() const;
  bool fromJson(const String& json);
};

#endif
