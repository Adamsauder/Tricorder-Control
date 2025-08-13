#ifndef TRICORDER_CONFIG_H
#define TRICORDER_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

struct TricorderConfigData {
  // Device settings
  char deviceLabel[32];
  char propId[16];
  char description[64];
  
  // SACN/DMX settings
  uint16_t sacnUniverse;
  uint16_t dmxAddress;
  bool sacnEnabled;
  
  // LED settings
  uint8_t brightness;
  
  // Network settings
  char wifiSSID[32];
  char wifiPassword[64];
  char staticIP[16];
  char hostname[32];
  
  // Video settings
  char defaultVideo[32];
  bool videoAutoPlay;
  uint8_t displayBrightness;
  
  // Battery monitoring settings
  float batteryVoltageCalibration;
  bool batteryMonitoringEnabled;
  
  // Advanced settings
  uint16_t udpPort;
  uint16_t webPort;
  bool debugMode;
};

class TricorderConfig {
private:
  Preferences preferences;
  TricorderConfigData config;
  bool initialized;
  
public:
  TricorderConfig();
  ~TricorderConfig();
  
  // Initialization
  bool begin();
  void setDefaults();
  
  // Load/Save
  bool load();
  bool save();
  bool factoryReset();
  
  // Device settings
  void setDeviceLabel(const char* label);
  const char* getDeviceLabel() const;
  void setPropId(const char* id);
  const char* getPropId() const;
  void setDescription(const char* desc);
  const char* getDescription() const;
  
  // SACN/DMX settings
  void setSacnUniverse(uint16_t universe);
  uint16_t getSacnUniverse() const;
  void setDmxAddress(uint16_t address);
  uint16_t getDmxAddress() const;
  void setSacnEnabled(bool enabled);
  bool getSacnEnabled() const;
  
  // LED settings
  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const;
  
  // Network settings
  void setWiFiSSID(const char* ssid);
  const char* getWiFiSSID() const;
  void setWiFiPassword(const char* password);
  const char* getWiFiPassword() const;
  void setStaticIP(const char* ip);
  const char* getStaticIP() const;
  void setHostname(const char* hostname);
  const char* getHostname() const;
  
  // Video settings
  void setDefaultVideo(const char* video);
  const char* getDefaultVideo() const;
  void setVideoAutoPlay(bool autoPlay);
  bool getVideoAutoPlay() const;
  void setDisplayBrightness(uint8_t brightness);
  uint8_t getDisplayBrightness() const;
  
  // Battery monitoring settings
  void setBatteryVoltageCalibration(float calibration);
  float getBatteryVoltageCalibration() const;
  void setBatteryMonitoringEnabled(bool enabled);
  bool getBatteryMonitoringEnabled() const;
  
  // Advanced settings
  void setUdpPort(uint16_t port);
  uint16_t getUdpPort() const;
  void setWebPort(uint16_t port);
  uint16_t getWebPort() const;
  void setDebugMode(bool enabled);
  bool getDebugMode() const;
  
  // JSON serialization
  String toJson() const;
  bool fromJson(const String& json);
  
  // Configuration validation
  bool isValid() const;
  String getValidationErrors() const;
};

#endif // TRICORDER_CONFIG_H
