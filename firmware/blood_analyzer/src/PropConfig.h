/*
 * PropConfig.h - Persistent configuration storage for Blood Analyzer props
 * Stores device configuration in ESP32 NVS (Non-Volatile Storage)
 */

#ifndef PROP_CONFIG_H
#define PROP_CONFIG_H

#include <Preferences.h>
#include <ArduinoJson.h>

class PropConfig {
private:
    static Preferences prefs;
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
    
    // Configuration management
    static bool loadConfig(Config& config);
    static bool saveConfig(const Config& config);
    static bool resetToDefaults();
    
    // Individual parameter access
    static String getDeviceLabel();
    static bool setDeviceLabel(const String& label);
    
    static int getSACNUniverse();
    static bool setSACNUniverse(int universe);
    
    static int getDMXStartAddress();
    static bool setDMXStartAddress(int address);
    
    static String getDeviceType();
    static bool setDeviceType(const String& type);
    
    static int getNumLeds();
    static bool setNumLeds(int count);
    
    static int getBrightness();
    static bool setBrightness(int brightness);
    
    // WiFi credentials
    static String getWiFiSSID();
    static bool setWiFiSSID(const String& ssid);
    
    static String getWiFiPassword();
    static bool setWiFiPassword(const String& password);
    
    // Network configuration
    static bool getUseDHCP();
    static bool setUseDHCP(bool useDHCP);
    
    static String getStaticIP();
    static bool setStaticIP(const String& ip);
    
    static String getStaticGateway();
    static bool setStaticGateway(const String& gateway);
    
    static String getStaticSubnet();
    static bool setStaticSubnet(const String& subnet);
    
    static String getStaticDNS();
    static bool setStaticDNS(const String& dns);
    
    // Fixture configuration
    static int getFixtureNumber();
    static bool setFixtureNumber(int number);
    
    // Utility functions
    static bool isFirstBoot();
    static void setFirstBoot(bool firstBoot);
    static String toJSON();
    static bool fromJSON(const String& json);
    static void printConfig();
    
    // Factory reset
    static bool factoryReset();
};

#endif // PROP_CONFIG_H