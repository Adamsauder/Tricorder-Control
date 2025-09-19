#include "PropConfig.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// NVS Namespace
const char* PropConfig::NAMESPACE = "blood_analyzer";

// NVS Keys
const char* PropConfig::KEY_DEVICE_LABEL = "device_label";
const char* PropConfig::KEY_SACN_UNIVERSE = "sacn_universe";
const char* PropConfig::KEY_DMX_START_ADDR = "dmx_start";
const char* PropConfig::KEY_DEVICE_TYPE = "device_type";
const char* PropConfig::KEY_NUM_LEDS = "num_leds";
const char* PropConfig::KEY_BRIGHTNESS = "brightness";
const char* PropConfig::KEY_WIFI_SSID = "wifi_ssid";
const char* PropConfig::KEY_WIFI_PASSWORD = "wifi_password";
const char* PropConfig::KEY_FIRST_BOOT = "first_boot";
const char* PropConfig::KEY_FIXTURE_NUMBER = "fixture_number";
const char* PropConfig::KEY_USE_DHCP = "use_dhcp";
const char* PropConfig::KEY_STATIC_IP = "static_ip";
const char* PropConfig::KEY_STATIC_GATEWAY = "static_gateway";
const char* PropConfig::KEY_STATIC_SUBNET = "static_subnet";
const char* PropConfig::KEY_STATIC_DNS = "static_dns";

Preferences PropConfig::prefs;

PropConfig::PropConfig() {
    // Constructor
}

bool PropConfig::resetToDefaults() {
    Config config;
    
    // Blood Analyzer defaults
    config.deviceLabel = "Blood Analyzer";
    config.sacnUniverse = 1;
    config.dmxStartAddress = 421;  // Blood Analyzer default DMX address
    config.deviceType = "blood_analyzer";
    config.numLeds = 7;  // 7 RGBW pixels
    config.brightness = 128;
    config.wifiSSID = "Rigging Electric";
    config.wifiPassword = "academy123";
    config.firstBoot = true;
    config.fixtureNumber = 4;
    
    // Network defaults
    config.useDHCP = true;
    config.staticIP = "192.168.1.100";
    config.staticGateway = "192.168.1.1";
    config.staticSubnet = "255.255.255.0";
    config.staticDNS = "8.8.8.8";
    
    return saveConfig(config);
}

bool PropConfig::loadConfig(Config& config) {
    if (!prefs.begin(NAMESPACE, true)) {
        // If we can't read preferences, return defaults
        config.deviceLabel = "Blood Analyzer";
        config.sacnUniverse = 1;
        config.dmxStartAddress = 421;
        config.deviceType = "blood_analyzer";
        config.numLeds = 7;
        config.brightness = 128;
        config.wifiSSID = "Rigging Electric";
        config.wifiPassword = "academy123";
        config.firstBoot = true;
        config.fixtureNumber = 4;
        config.useDHCP = true;
        config.staticIP = "192.168.1.100";
        config.staticGateway = "192.168.1.1";
        config.staticSubnet = "255.255.255.0";
        config.staticDNS = "8.8.8.8";
        return false;
    }
    
    config.deviceLabel = prefs.getString(KEY_DEVICE_LABEL, "Blood Analyzer");
    config.sacnUniverse = prefs.getInt(KEY_SACN_UNIVERSE, 1);
    config.dmxStartAddress = prefs.getInt(KEY_DMX_START_ADDR, 421);
    config.deviceType = prefs.getString(KEY_DEVICE_TYPE, "blood_analyzer");
    config.numLeds = prefs.getInt(KEY_NUM_LEDS, 7);
    config.brightness = prefs.getInt(KEY_BRIGHTNESS, 128);
    config.wifiSSID = prefs.getString(KEY_WIFI_SSID, "Rigging Electric");
    config.wifiPassword = prefs.getString(KEY_WIFI_PASSWORD, "academy123");
    config.firstBoot = prefs.getBool(KEY_FIRST_BOOT, true);
    config.fixtureNumber = prefs.getInt(KEY_FIXTURE_NUMBER, 4);
    
    // Network configuration
    config.useDHCP = prefs.getBool(KEY_USE_DHCP, true);
    config.staticIP = prefs.getString(KEY_STATIC_IP, "192.168.1.100");
    config.staticGateway = prefs.getString(KEY_STATIC_GATEWAY, "192.168.1.1");
    config.staticSubnet = prefs.getString(KEY_STATIC_SUBNET, "255.255.255.0");
    config.staticDNS = prefs.getString(KEY_STATIC_DNS, "8.8.8.8");
    
    prefs.end();
    return true;
}

bool PropConfig::saveConfig(const Config& config) {
    if (!prefs.begin(NAMESPACE, false)) {
        return false;
    }
    
    prefs.putString(KEY_DEVICE_LABEL, config.deviceLabel);
    prefs.putInt(KEY_SACN_UNIVERSE, config.sacnUniverse);
    prefs.putInt(KEY_DMX_START_ADDR, config.dmxStartAddress);
    prefs.putString(KEY_DEVICE_TYPE, config.deviceType);
    prefs.putInt(KEY_NUM_LEDS, config.numLeds);
    prefs.putInt(KEY_BRIGHTNESS, config.brightness);
    prefs.putString(KEY_WIFI_SSID, config.wifiSSID);
    prefs.putString(KEY_WIFI_PASSWORD, config.wifiPassword);
    prefs.putBool(KEY_FIRST_BOOT, config.firstBoot);
    prefs.putInt(KEY_FIXTURE_NUMBER, config.fixtureNumber);
    
    // Network configuration
    prefs.putBool(KEY_USE_DHCP, config.useDHCP);
    prefs.putString(KEY_STATIC_IP, config.staticIP);
    prefs.putString(KEY_STATIC_GATEWAY, config.staticGateway);
    prefs.putString(KEY_STATIC_SUBNET, config.staticSubnet);
    prefs.putString(KEY_STATIC_DNS, config.staticDNS);
    
    prefs.end();
    return true;
}

// Individual parameter access
String PropConfig::getDeviceLabel() {
    if (!prefs.begin(NAMESPACE, true)) return "Blood Analyzer";
    String value = prefs.getString(KEY_DEVICE_LABEL, "Blood Analyzer");
    prefs.end();
    return value;
}

bool PropConfig::setDeviceLabel(const String& label) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_DEVICE_LABEL, label);
    prefs.end();
    return result > 0;
}

int PropConfig::getSACNUniverse() {
    if (!prefs.begin(NAMESPACE, true)) return 1;
    int value = prefs.getInt(KEY_SACN_UNIVERSE, 1);
    prefs.end();
    return value;
}

bool PropConfig::setSACNUniverse(int universe) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putInt(KEY_SACN_UNIVERSE, universe);
    prefs.end();
    return result > 0;
}

int PropConfig::getDMXStartAddress() {
    if (!prefs.begin(NAMESPACE, true)) return 421;
    int value = prefs.getInt(KEY_DMX_START_ADDR, 421);
    prefs.end();
    return value;
}

bool PropConfig::setDMXStartAddress(int address) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putInt(KEY_DMX_START_ADDR, address);
    prefs.end();
    return result > 0;
}

String PropConfig::getDeviceType() {
    if (!prefs.begin(NAMESPACE, true)) return "blood_analyzer";
    String value = prefs.getString(KEY_DEVICE_TYPE, "blood_analyzer");
    prefs.end();
    return value;
}

bool PropConfig::setDeviceType(const String& type) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_DEVICE_TYPE, type);
    prefs.end();
    return result > 0;
}

int PropConfig::getNumLeds() {
    if (!prefs.begin(NAMESPACE, true)) return 7;
    int value = prefs.getInt(KEY_NUM_LEDS, 7);
    prefs.end();
    return value;
}

bool PropConfig::setNumLeds(int count) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putInt(KEY_NUM_LEDS, count);
    prefs.end();
    return result > 0;
}

int PropConfig::getBrightness() {
    if (!prefs.begin(NAMESPACE, true)) return 128;
    int value = prefs.getInt(KEY_BRIGHTNESS, 128);
    prefs.end();
    return value;
}

bool PropConfig::setBrightness(int brightness) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putInt(KEY_BRIGHTNESS, brightness);
    prefs.end();
    return result > 0;
}

// WiFi credentials
String PropConfig::getWiFiSSID() {
    if (!prefs.begin(NAMESPACE, true)) return "Rigging Electric";
    String value = prefs.getString(KEY_WIFI_SSID, "Rigging Electric");
    prefs.end();
    return value;
}

bool PropConfig::setWiFiSSID(const String& ssid) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_WIFI_SSID, ssid);
    prefs.end();
    return result > 0;
}

String PropConfig::getWiFiPassword() {
    if (!prefs.begin(NAMESPACE, true)) return "academy123";
    String value = prefs.getString(KEY_WIFI_PASSWORD, "academy123");
    prefs.end();
    return value;
}

bool PropConfig::setWiFiPassword(const String& password) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_WIFI_PASSWORD, password);
    prefs.end();
    return result > 0;
}

// Network configuration
bool PropConfig::getUseDHCP() {
    if (!prefs.begin(NAMESPACE, true)) return true;
    bool value = prefs.getBool(KEY_USE_DHCP, true);
    prefs.end();
    return value;
}

bool PropConfig::setUseDHCP(bool useDHCP) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putBool(KEY_USE_DHCP, useDHCP);
    prefs.end();
    return result > 0;
}

String PropConfig::getStaticIP() {
    if (!prefs.begin(NAMESPACE, true)) return "192.168.1.100";
    String value = prefs.getString(KEY_STATIC_IP, "192.168.1.100");
    prefs.end();
    return value;
}

bool PropConfig::setStaticIP(const String& ip) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_STATIC_IP, ip);
    prefs.end();
    return result > 0;
}

String PropConfig::getStaticGateway() {
    if (!prefs.begin(NAMESPACE, true)) return "192.168.1.1";
    String value = prefs.getString(KEY_STATIC_GATEWAY, "192.168.1.1");
    prefs.end();
    return value;
}

bool PropConfig::setStaticGateway(const String& gateway) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_STATIC_GATEWAY, gateway);
    prefs.end();
    return result > 0;
}

String PropConfig::getStaticSubnet() {
    if (!prefs.begin(NAMESPACE, true)) return "255.255.255.0";
    String value = prefs.getString(KEY_STATIC_SUBNET, "255.255.255.0");
    prefs.end();
    return value;
}

bool PropConfig::setStaticSubnet(const String& subnet) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_STATIC_SUBNET, subnet);
    prefs.end();
    return result > 0;
}

String PropConfig::getStaticDNS() {
    if (!prefs.begin(NAMESPACE, true)) return "8.8.8.8";
    String value = prefs.getString(KEY_STATIC_DNS, "8.8.8.8");
    prefs.end();
    return value;
}

bool PropConfig::setStaticDNS(const String& dns) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putString(KEY_STATIC_DNS, dns);
    prefs.end();
    return result > 0;
}

// Fixture configuration
int PropConfig::getFixtureNumber() {
    if (!prefs.begin(NAMESPACE, true)) return 4;
    int value = prefs.getInt(KEY_FIXTURE_NUMBER, 4);
    prefs.end();
    return value;
}

bool PropConfig::setFixtureNumber(int number) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.putInt(KEY_FIXTURE_NUMBER, number);
    prefs.end();
    return result > 0;
}

// Utility functions
bool PropConfig::isFirstBoot() {
    if (!prefs.begin(NAMESPACE, true)) return true;
    bool value = prefs.getBool(KEY_FIRST_BOOT, true);
    prefs.end();
    return value;
}

void PropConfig::setFirstBoot(bool firstBoot) {
    prefs.begin(NAMESPACE, false);
    prefs.putBool(KEY_FIRST_BOOT, firstBoot);
    prefs.end();
}

String PropConfig::toJSON() {
    Config config;
    loadConfig(config);
    
    JsonDocument doc;
    doc["deviceLabel"] = config.deviceLabel;
    doc["sacnUniverse"] = config.sacnUniverse;
    doc["dmxStartAddress"] = config.dmxStartAddress;
    doc["deviceType"] = config.deviceType;
    doc["numLeds"] = config.numLeds;
    doc["brightness"] = config.brightness;
    doc["wifiSSID"] = config.wifiSSID;
    // Don't include password in JSON output for security
    doc["firstBoot"] = config.firstBoot;
    doc["fixtureNumber"] = config.fixtureNumber;
    
    // Network configuration
    doc["useDHCP"] = config.useDHCP;
    doc["staticIP"] = config.staticIP;
    doc["staticGateway"] = config.staticGateway;
    doc["staticSubnet"] = config.staticSubnet;
    doc["staticDNS"] = config.staticDNS;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

bool PropConfig::fromJSON(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    Config config;
    loadConfig(config);  // Load current config as base
    
    // Update only the fields present in the JSON
    if (doc.containsKey("deviceLabel")) {
        config.deviceLabel = doc["deviceLabel"].as<String>();
    }
    if (doc.containsKey("sacnUniverse")) {
        config.sacnUniverse = doc["sacnUniverse"];
    }
    if (doc.containsKey("dmxStartAddress")) {
        config.dmxStartAddress = doc["dmxStartAddress"];
    }
    if (doc.containsKey("deviceType")) {
        config.deviceType = doc["deviceType"].as<String>();
    }
    if (doc.containsKey("numLeds")) {
        config.numLeds = doc["numLeds"];
    }
    if (doc.containsKey("brightness")) {
        config.brightness = doc["brightness"];
    }
    if (doc.containsKey("wifiSSID")) {
        config.wifiSSID = doc["wifiSSID"].as<String>();
    }
    if (doc.containsKey("wifiPassword")) {
        config.wifiPassword = doc["wifiPassword"].as<String>();
    }
    if (doc.containsKey("firstBoot")) {
        config.firstBoot = doc["firstBoot"];
    }
    if (doc.containsKey("fixtureNumber")) {
        config.fixtureNumber = doc["fixtureNumber"];
    }
    
    // Network configuration
    if (doc.containsKey("useDHCP")) {
        config.useDHCP = doc["useDHCP"];
    }
    if (doc.containsKey("staticIP")) {
        config.staticIP = doc["staticIP"].as<String>();
    }
    if (doc.containsKey("staticGateway")) {
        config.staticGateway = doc["staticGateway"].as<String>();
    }
    if (doc.containsKey("staticSubnet")) {
        config.staticSubnet = doc["staticSubnet"].as<String>();
    }
    if (doc.containsKey("staticDNS")) {
        config.staticDNS = doc["staticDNS"].as<String>();
    }
    
    return saveConfig(config);
}

void PropConfig::printConfig() {
    Config config;
    loadConfig(config);
    
    Serial.println("=== Blood Analyzer Configuration ===");
    Serial.printf("Device Label: %s\n", config.deviceLabel.c_str());
    Serial.printf("sACN Universe: %d\n", config.sacnUniverse);
    Serial.printf("DMX Start Address: %d\n", config.dmxStartAddress);
    Serial.printf("Device Type: %s\n", config.deviceType.c_str());
    Serial.printf("Number of LEDs: %d\n", config.numLeds);
    Serial.printf("Brightness: %d\n", config.brightness);
    Serial.printf("WiFi SSID: %s\n", config.wifiSSID.c_str());
    Serial.printf("First Boot: %s\n", config.firstBoot ? "true" : "false");
    Serial.printf("Fixture Number: %d\n", config.fixtureNumber);
    Serial.printf("Use DHCP: %s\n", config.useDHCP ? "true" : "false");
    if (!config.useDHCP) {
        Serial.printf("Static IP: %s\n", config.staticIP.c_str());
        Serial.printf("Gateway: %s\n", config.staticGateway.c_str());
        Serial.printf("Subnet: %s\n", config.staticSubnet.c_str());
        Serial.printf("DNS: %s\n", config.staticDNS.c_str());
    }
    Serial.println("=====================================");
}

bool PropConfig::factoryReset() {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool result = prefs.clear();
    prefs.end();
    
    if (result) {
        // Set default configuration
        resetToDefaults();
    }
    
    return result;
}