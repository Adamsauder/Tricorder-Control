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
    static const char* KEY_USE_DHCP;
    static const char* KEY_STATIC_IP;
    static const char* KEY_STATIC_GATEWAY;
    static const char* KEY_STATIC_SUBNET;
    static const char* KEY_STATIC_DNS;

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
        // Network configuration
        bool useDHCP;
        String staticIP;
        String staticGateway;
        String staticSubnet;
        String staticDNS;
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
    
    // WiFi credentials
    String getWiFiSSID();
    bool setWiFiSSID(const String& ssid);
    
    String getWiFiPassword();
    bool setWiFiPassword(const String& password);
    
    // Network configuration
    bool getUseDHCP();
    bool setUseDHCP(bool useDHCP);
    
    String getStaticIP();
    bool setStaticIP(const String& ip);
    
    String getStaticGateway();
    bool setStaticGateway(const String& gateway);
    
    String getStaticSubnet();
    bool setStaticSubnet(const String& subnet);
    
    String getStaticDNS();
    bool setStaticDNS(const String& dns);
    
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
