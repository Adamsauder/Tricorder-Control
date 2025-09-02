/*
 * PropConfig.h - Persistent configuration storage for Tricorder/Polyinoculator props
 * Stores device configuration in ESP32 NVS (Non-Volatile Storage)
 */

#ifndef PROP_CONFIG_H
#define PROP_CONFIG_H

#include <Preferences.h>
#include <ArduinoJson.h>

class PropConfig {
private:
    Preferences prefs;
    static const char* NAMESPACE;
    
    // Configuration keys
    static const char* KEY_DEVICE_LABEL;
    static const char* KEY_SACN_UNIVERSE;
    static const char* KEY_DMX_START_ADDR;
    static const char* KEY_DEVICE_TYPE;
    static const char* KEY_NUM_LEDS;
    static const char* KEY_BRIGHTNESS;
    static const char* KEY_WIFI_SSID;
    static const char* KEY_WIFI_PASSWORD;
    static const char* KEY_FIRST_BOOT;
    static const char* KEY_FIXTURE_NUMBER;
    static const char* KEY_DEFAULT_COLORS_R;
    static const char* KEY_DEFAULT_COLORS_G;
    static const char* KEY_DEFAULT_COLORS_B;
    static const char* KEY_USE_DEFAULT_COLORS;

public:
    struct Config {
        String deviceLabel;
        int sacnUniverse;
        int dmxStartAddress;
        String deviceType;
        int numLeds;
        int brightness;
        String wifiSSID;
        String wifiPassword;
        bool firstBoot;
        int fixtureNumber;
        uint8_t defaultColorsR[30];  // Default red values for all LEDs
        uint8_t defaultColorsG[30];  // Default green values for all LEDs
        uint8_t defaultColorsB[30];  // Default blue values for all LEDs
        bool useDefaultColors;       // Whether to use default colors on startup
    };

    PropConfig();
    ~PropConfig();
    
    // Configuration management
    bool begin();
    bool loadConfig(Config& config);
    bool saveConfig(const Config& config);
    bool resetToDefaults();
    
    // Individual parameter access
    String getDeviceLabel();
    bool setDeviceLabel(const String& label);
    
    int getSACNUniverse();
    bool setSACNUniverse(int universe);
    
    int getDMXStartAddress();
    bool setDMXStartAddress(int address);
    
    String getDeviceType();
    bool setDeviceType(const String& type);
    
    int getNumLEDs();
    bool setNumLEDs(int count);
    
    int getBrightness();
    bool setBrightness(int brightness);
    
    // Default LED colors
    bool setDefaultColors(const uint8_t* red, const uint8_t* green, const uint8_t* blue, int numLeds);
    bool getDefaultColors(uint8_t* red, uint8_t* green, uint8_t* blue, int maxLeds);
    bool setUseDefaultColors(bool use);
    bool getUseDefaultColors();
    
    // WiFi credentials
    String getWiFiSSID();
    bool setWiFiSSID(const String& ssid);
    
    String getWiFiPassword();
    bool setWiFiPassword(const String& password);
    
    // Fixture configuration
    int getFixtureNumber();
    bool setFixtureNumber(int number);
    
    // Utility functions
    bool isFirstBoot();
    void setFirstBoot(bool firstBoot);
    String toJSON();
    bool fromJSON(const String& json);
    void printConfig();
    
    // Factory reset
    bool factoryReset();
};

#endif // PROP_CONFIG_H
