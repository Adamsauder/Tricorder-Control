/*
 * PropConfig.cpp - Implementation of persistent configuration storage for IV Injector
 */

#include "PropConfig.h"

// Static constants
const char* PropConfig::NAMESPACE = "propconfig";
const char* PropConfig::KEY_DEVICE_LABEL = "dev_label";
const char* PropConfig::KEY_SACN_UNIVERSE = "sacn_univ";
const char* PropConfig::KEY_DMX_START_ADDR = "dmx_start";
const char* PropConfig::KEY_DEVICE_TYPE = "dev_type";
const char* PropConfig::KEY_NUM_LEDS = "num_leds";
const char* PropConfig::KEY_BRIGHTNESS = "brightness";
const char* PropConfig::KEY_WIFI_SSID = "wifi_ssid";
const char* PropConfig::KEY_WIFI_PASSWORD = "wifi_pass";
const char* PropConfig::KEY_FIRST_BOOT = "first_boot";
const char* PropConfig::KEY_FIXTURE_NUMBER = "fixture_num";
const char* PropConfig::KEY_USE_DHCP = "use_dhcp";
const char* PropConfig::KEY_STATIC_IP = "static_ip";
const char* PropConfig::KEY_STATIC_GATEWAY = "static_gw";
const char* PropConfig::KEY_STATIC_SUBNET = "static_sub";
const char* PropConfig::KEY_STATIC_DNS = "static_dns";

PropConfig::PropConfig() {
}

PropConfig::~PropConfig() {
    prefs.end();
}

bool PropConfig::begin() {
    return prefs.begin(NAMESPACE, false); // false = read/write mode
}

bool PropConfig::loadConfig(Config& config) {
    if (!prefs.begin(NAMESPACE, true)) { // true = read-only mode for loading
        return false;
    }
    
    // Load configuration with defaults for IV Injector
    config.deviceLabel = prefs.getString(KEY_DEVICE_LABEL, "PIN_STAND_001");
    config.sacnUniverse = prefs.getInt(KEY_SACN_UNIVERSE, 1);
    config.dmxStartAddress = prefs.getInt(KEY_DMX_START_ADDR, 1);
    config.deviceType = prefs.getString(KEY_DEVICE_TYPE, "iv_injector");
    config.numLeds = prefs.getInt(KEY_NUM_LEDS, 1);  // Single LED for IV Injector
    config.brightness = prefs.getInt(KEY_BRIGHTNESS, 128);
    config.wifiSSID = prefs.getString(KEY_WIFI_SSID, "Rigging Electric");
    config.wifiPassword = prefs.getString(KEY_WIFI_PASSWORD, "academy123");
    config.firstBoot = prefs.getBool(KEY_FIRST_BOOT, true);
    config.fixtureNumber = prefs.getInt(KEY_FIXTURE_NUMBER, 4);  // Default to 4 for IV injector
    // Network configuration defaults
    config.useDHCP = prefs.getBool(KEY_USE_DHCP, true);  // Default to DHCP
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
    
    bool success = true;
    success &= prefs.putString(KEY_DEVICE_LABEL, config.deviceLabel);
    success &= prefs.putInt(KEY_SACN_UNIVERSE, config.sacnUniverse);
    success &= prefs.putInt(KEY_DMX_START_ADDR, config.dmxStartAddress);
    success &= prefs.putString(KEY_DEVICE_TYPE, config.deviceType);
    success &= prefs.putInt(KEY_NUM_LEDS, config.numLeds);
    success &= prefs.putInt(KEY_BRIGHTNESS, config.brightness);
    success &= prefs.putString(KEY_WIFI_SSID, config.wifiSSID);
    success &= prefs.putString(KEY_WIFI_PASSWORD, config.wifiPassword);
    success &= prefs.putBool(KEY_FIRST_BOOT, config.firstBoot);
    success &= prefs.putInt(KEY_FIXTURE_NUMBER, config.fixtureNumber);
    // Network configuration
    success &= prefs.putBool(KEY_USE_DHCP, config.useDHCP);
    success &= prefs.putString(KEY_STATIC_IP, config.staticIP);
    success &= prefs.putString(KEY_STATIC_GATEWAY, config.staticGateway);
    success &= prefs.putString(KEY_STATIC_SUBNET, config.staticSubnet);
    success &= prefs.putString(KEY_STATIC_DNS, config.staticDNS);
    
    prefs.end();
    return success;
}

bool PropConfig::resetToDefaults() {
    Config defaultConfig;
    defaultConfig.deviceLabel = "PIN_STAND_001";
    defaultConfig.sacnUniverse = 1;
    defaultConfig.dmxStartAddress = 1;
    defaultConfig.deviceType = "iv_injector";
    defaultConfig.numLeds = 1;  // Single LED
    defaultConfig.brightness = 128;
    defaultConfig.wifiSSID = "Rigging Electric";
    defaultConfig.wifiPassword = "academy123";
    defaultConfig.firstBoot = true;
    defaultConfig.fixtureNumber = 4;
    // Network defaults
    defaultConfig.useDHCP = true;
    defaultConfig.staticIP = "192.168.1.100";
    defaultConfig.staticGateway = "192.168.1.1";
    defaultConfig.staticSubnet = "255.255.255.0";
    defaultConfig.staticDNS = "8.8.8.8";
    
    return saveConfig(defaultConfig);
}

String PropConfig::getDeviceLabel() {
    if (!prefs.begin(NAMESPACE, true)) return "ERROR";
    String value = prefs.getString(KEY_DEVICE_LABEL, "IV_INJECTOR_001");
    prefs.end();
    return value;
}

bool PropConfig::setDeviceLabel(const String& label) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_DEVICE_LABEL, label);
    prefs.end();
    return success;
}

int PropConfig::getSACNUniverse() {
    if (!prefs.begin(NAMESPACE, true)) return 1;
    int value = prefs.getInt(KEY_SACN_UNIVERSE, 1);
    prefs.end();
    return value;
}

bool PropConfig::setSACNUniverse(int universe) {
    if (universe < 1 || universe > 63999) return false;
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putInt(KEY_SACN_UNIVERSE, universe);
    prefs.end();
    return success;
}

int PropConfig::getDMXStartAddress() {
    if (!prefs.begin(NAMESPACE, true)) return 1;
    int value = prefs.getInt(KEY_DMX_START_ADDR, 1);
    prefs.end();
    return value;
}

bool PropConfig::setDMXStartAddress(int address) {
    if (address < 1 || address > 512) return false;
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putInt(KEY_DMX_START_ADDR, address);
    prefs.end();
    return success;
}

String PropConfig::getDeviceType() {
    if (!prefs.begin(NAMESPACE, true)) return "iv_injector";
    String value = prefs.getString(KEY_DEVICE_TYPE, "iv_injector");
    prefs.end();
    return value;
}

bool PropConfig::setDeviceType(const String& type) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_DEVICE_TYPE, type);
    prefs.end();
    return success;
}

int PropConfig::getNumLEDs() {
    if (!prefs.begin(NAMESPACE, true)) return 1;
    int value = prefs.getInt(KEY_NUM_LEDS, 1);
    prefs.end();
    return value;
}

bool PropConfig::setNumLEDs(int count) {
    if (count < 1 || count > 1000) return false;
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putInt(KEY_NUM_LEDS, count);
    prefs.end();
    return success;
}

int PropConfig::getBrightness() {
    if (!prefs.begin(NAMESPACE, true)) return 128;
    int value = prefs.getInt(KEY_BRIGHTNESS, 128);
    prefs.end();
    return value;
}

bool PropConfig::setBrightness(int brightness) {
    if (brightness < 0 || brightness > 255) return false;
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putInt(KEY_BRIGHTNESS, brightness);
    prefs.end();
    return success;
}

String PropConfig::getWiFiSSID() {
    if (!prefs.begin(NAMESPACE, true)) return "Rigging Electric";
    String value = prefs.getString(KEY_WIFI_SSID, "Rigging Electric");
    prefs.end();
    return value;
}

bool PropConfig::setWiFiSSID(const String& ssid) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_WIFI_SSID, ssid);
    prefs.end();
    return success;
}

String PropConfig::getWiFiPassword() {
    if (!prefs.begin(NAMESPACE, true)) return "academy123";
    String value = prefs.getString(KEY_WIFI_PASSWORD, "academy123");
    prefs.end();
    return value;
}

bool PropConfig::setWiFiPassword(const String& password) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_WIFI_PASSWORD, password);
    prefs.end();
    return success;
}

int PropConfig::getFixtureNumber() {
    if (!prefs.begin(NAMESPACE, true)) return 4;
    int value = prefs.getInt(KEY_FIXTURE_NUMBER, 4);
    prefs.end();
    return value;
}

bool PropConfig::setFixtureNumber(int number) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putInt(KEY_FIXTURE_NUMBER, number);
    prefs.end();
    return success;
}

bool PropConfig::isFirstBoot() {
    if (!prefs.begin(NAMESPACE, true)) return true;
    bool value = prefs.getBool(KEY_FIRST_BOOT, true);
    prefs.end();
    return value;
}

void PropConfig::setFirstBoot(bool firstBoot) {
    if (!prefs.begin(NAMESPACE, false)) return;
    prefs.putBool(KEY_FIRST_BOOT, firstBoot);
    prefs.end();
}

String PropConfig::toJSON() {
    Config config;
    if (!loadConfig(config)) {
        return "{}";
    }
    
    JsonDocument doc;
    doc["deviceLabel"] = config.deviceLabel;
    doc["sacnUniverse"] = config.sacnUniverse;
    doc["dmxStartAddress"] = config.dmxStartAddress;
    doc["deviceType"] = config.deviceType;
    doc["numLeds"] = config.numLeds;
    doc["brightness"] = config.brightness;
    doc["wifiSSID"] = config.wifiSSID;
    // Note: WiFi password not included in JSON for security
    doc["firstBoot"] = config.firstBoot;
    doc["fixtureNumber"] = config.fixtureNumber;
    
    // Network configuration
    doc["useDHCP"] = config.useDHCP;
    doc["staticIP"] = config.staticIP;
    doc["staticGateway"] = config.staticGateway;
    doc["staticSubnet"] = config.staticSubnet;
    doc["staticDNS"] = config.staticDNS;
    
    String json;
    serializeJson(doc, json);
    return json;
}

bool PropConfig::fromJSON(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        return false;
    }
    
    Config config;
    loadConfig(config); // Load current config as base
    
    // Update only provided fields
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
    if (!loadConfig(config)) {
        Serial.println("Failed to load configuration!");
        return;
    }
    
    Serial.println("=== IV Injector Configuration ===");
    Serial.printf("Device Label: %s\n", config.deviceLabel.c_str());
    Serial.printf("Device Type: %s\n", config.deviceType.c_str());
    Serial.printf("sACN Universe: %d\n", config.sacnUniverse);
    Serial.printf("DMX Start Address: %d\n", config.dmxStartAddress);
    Serial.printf("Number of LEDs: %d\n", config.numLeds);
    Serial.printf("LED Brightness: %d\n", config.brightness);
    Serial.printf("WiFi SSID: %s\n", config.wifiSSID.c_str());
    Serial.printf("Fixture Number: %d\n", config.fixtureNumber);
    Serial.printf("First Boot: %s\n", config.firstBoot ? "true" : "false");
    
    Serial.println("--- Network Configuration ---");
    Serial.printf("Use DHCP: %s\n", config.useDHCP ? "true" : "false");
    Serial.printf("Static IP: %s\n", config.staticIP.c_str());
    Serial.printf("Gateway: %s\n", config.staticGateway.c_str());
    Serial.printf("Subnet: %s\n", config.staticSubnet.c_str());
    Serial.printf("DNS: %s\n", config.staticDNS.c_str());
    Serial.println("==================================");
}

bool PropConfig::factoryReset() {
    if (!prefs.begin(NAMESPACE, false)) {
        return false;
    }
    
    bool success = prefs.clear();
    prefs.end();
    
    if (success) {
        return resetToDefaults();
    }
    
    return false;
}

// Network configuration methods
bool PropConfig::getUseDHCP() {
    if (!prefs.begin(NAMESPACE, true)) return true;
    bool value = prefs.getBool(KEY_USE_DHCP, true);
    prefs.end();
    return value;
}

bool PropConfig::setUseDHCP(bool useDHCP) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putBool(KEY_USE_DHCP, useDHCP);
    prefs.end();
    return success;
}

String PropConfig::getStaticIP() {
    if (!prefs.begin(NAMESPACE, true)) return "192.168.1.100";
    String value = prefs.getString(KEY_STATIC_IP, "192.168.1.100");
    prefs.end();
    return value;
}

bool PropConfig::setStaticIP(const String& ip) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_STATIC_IP, ip);
    prefs.end();
    return success;
}

String PropConfig::getStaticGateway() {
    if (!prefs.begin(NAMESPACE, true)) return "192.168.1.1";
    String value = prefs.getString(KEY_STATIC_GATEWAY, "192.168.1.1");
    prefs.end();
    return value;
}

bool PropConfig::setStaticGateway(const String& gateway) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_STATIC_GATEWAY, gateway);
    prefs.end();
    return success;
}

String PropConfig::getStaticSubnet() {
    if (!prefs.begin(NAMESPACE, true)) return "255.255.255.0";
    String value = prefs.getString(KEY_STATIC_SUBNET, "255.255.255.0");
    prefs.end();
    return value;
}

bool PropConfig::setStaticSubnet(const String& subnet) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_STATIC_SUBNET, subnet);
    prefs.end();
    return success;
}

String PropConfig::getStaticDNS() {
    if (!prefs.begin(NAMESPACE, true)) return "8.8.8.8";
    String value = prefs.getString(KEY_STATIC_DNS, "8.8.8.8");
    prefs.end();
    return value;
}

bool PropConfig::setStaticDNS(const String& dns) {
    if (!prefs.begin(NAMESPACE, false)) return false;
    bool success = prefs.putString(KEY_STATIC_DNS, dns);
    prefs.end();
    return success;
}
