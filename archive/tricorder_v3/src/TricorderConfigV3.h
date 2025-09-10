#ifndef TRICORDER_CONFIG_V3_H
#define TRICORDER_CONFIG_V3_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

struct TricorderConfigV3Data {
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
  uint8_t defaultColorR[12];  // Default red values for each LED
  uint8_t defaultColorG[12];  // Default green values for each LED
  uint8_t defaultColorB[12];  // Default blue values for each LED
  bool useDefaultColors;      // Whether to use default colors on startup
  
  // Network settings
  char wifiSSID[32];
  char wifiPassword[64];
  char staticIP[16];
  char hostname[32];
  
  // Video settings (Enhanced for v3)
  char defaultVideo[64];      // Support longer MPEG filenames
  bool videoAutoPlay;
  uint8_t displayBrightness;
  uint8_t videoQuality;       // MPEG quality setting (1-10)
  bool videoAudioEnabled;     // Enable/disable audio track
  uint16_t videoFrameRate;    // Target frame rate for MPEG playback
  uint32_t videoBufferSize;   // Buffer size for video streaming
  bool videoLooping;          // Default looping behavior
  bool videoScaling;          // Enable video scaling to fit display
  
  // Battery monitoring settings
  float batteryVoltageCalibration;
  bool batteryMonitoringEnabled;
  
  // Advanced settings
  uint16_t udpPort;
  uint16_t webPort;
  bool debugMode;
  
  // Performance settings (New for v3)
  uint8_t cpuFrequency;       // CPU frequency setting (80, 160, 240 MHz)
  uint16_t heapThreshold;     // Minimum free heap before video pause
  bool videoPreloading;       // Preload next frame in memory
};

class TricorderConfigV3 {
private:
  Preferences preferences;
  TricorderConfigV3Data config;
  bool initialized;
  
  // Helper functions
  String generateUniqueId();
  
public:
  TricorderConfigV3();
  ~TricorderConfigV3();
  
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
  void setFixtureNumber(uint16_t number);
  uint16_t getFixtureNumber() const;
  
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
  void setDefaultColors(const uint8_t* red, const uint8_t* green, const uint8_t* blue);
  void getDefaultColors(uint8_t* red, uint8_t* green, uint8_t* blue) const;
  void setUseDefaultColors(bool use);
  bool getUseDefaultColors() const;
  
  // Network settings
  void setWiFiSSID(const char* ssid);
  const char* getWiFiSSID() const;
  void setWiFiPassword(const char* password);
  const char* getWiFiPassword() const;
  void setStaticIP(const char* ip);
  const char* getStaticIP() const;
  void setHostname(const char* hostname);
  const char* getHostname() const;
  
  // Enhanced Video settings (v3)
  void setDefaultVideo(const char* video);
  const char* getDefaultVideo() const;
  void setVideoAutoPlay(bool autoPlay);
  bool getVideoAutoPlay() const;
  void setDisplayBrightness(uint8_t brightness);
  uint8_t getDisplayBrightness() const;
  void setVideoQuality(uint8_t quality);
  uint8_t getVideoQuality() const;
  void setVideoAudioEnabled(bool enabled);
  bool getVideoAudioEnabled() const;
  void setVideoFrameRate(uint16_t frameRate);
  uint16_t getVideoFrameRate() const;
  void setVideoBufferSize(uint32_t bufferSize);
  uint32_t getVideoBufferSize() const;
  void setVideoLooping(bool looping);
  bool getVideoLooping() const;
  void setVideoScaling(bool scaling);
  bool getVideoScaling() const;
  
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
  
  // Performance settings (New for v3)
  void setCpuFrequency(uint8_t frequency);
  uint8_t getCpuFrequency() const;
  void setHeapThreshold(uint16_t threshold);
  uint16_t getHeapThreshold() const;
  void setVideoPreloading(bool preloading);
  bool getVideoPreloading() const;
  
  // JSON serialization
  String toJson() const;
  bool fromJson(const String& json);
  
  // Configuration validation
  bool isValid() const;
  String getValidationErrors() const;
};

#endif // TRICORDER_CONFIG_V3_H
