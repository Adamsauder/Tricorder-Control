# Tricorder Firmware

ESP32-based firmware for film set prop cont```

## 🔄 Over-The-Air (OTA) Updates

### OTA Update Features
- **🌐 Web-based Updates**: Upload firmware via web interface, no USB needed
- **📡 ArduinoOTA Support**: Standard Arduino OTA library integration  
- **📤 HTTP Upload Server**: Direct firmware upload to device
- **📊 Visual Progress**: Real-time update progress on device screen
- **🔐 Password Protection**: Secure updates with configurable password
- **⚠️ Error Recovery**: Automatic rollback on update failure
- **🔄 Auto-restart**: Device restarts automatically after successful update

### How OTA Updates Work
1. **Device advertises** OTA capability via mDNS
2. **Central server** detects available devices
3. **Web interface** allows firmware file upload and device selection
4. **Device receives** firmware via HTTP upload or ArduinoOTA
5. **Progress displayed** on device screen during update
6. **Device restarts** with new firmware automatically

### OTA Update Process
```
Server                     ESP32 Device
------                     ------------
1. Upload .bin file  →    
2. Select device     →    
3. Send update cmd   →     4. Enter OTA mode
                           5. Display progress
                           6. Validate firmware
                           7. Flash new firmware
                           8. Restart device
```

### OTA Configuration
```cpp
// OTA settings in main.cpp
#define OTA_PASSWORD "tricorder123"  // Change this!
#define OTA_PORT 3232
#define OTA_HOSTNAME "tricorder_001"  // Auto-generated from device ID
```

## Video Playback Setup

The tricorder supports video playback from SD card using JPEG image sequences. For detailed setup instructions, see:

📹 **[VIDEO_PLAYBACK_GUIDE.md](VIDEO_PLAYBACK_GUIDE.md)** - Complete video setup and usage guide

### Quick Start for Videos
1. Format SD card as FAT32
2. Create `/videos/` directory on SD card
3. Convert videos to JPEG format (320x240 recommended)
4. Use UDP commands to control playbackembedded screens, NeoPixel LEDs, and **Over-The-Air (OTA) update capabilities**.

## 🚀 Features

- **🔄 Over-The-Air Updates**: ArduinoOTA + HTTP upload server with visual progress
- **📺 Video Playback**: JPEG sequence playback from SD card with loop controls
- **🌈 NeoPixel Control**: 12-channel RGB LED control with animations and SACN support
- **🖥️ TFT Display**: 3.2" ST7789 color display (320x240) with video playback
- **📡 WiFi Connectivity**: Auto-connect with reconnection handling and mDNS discovery
- **⚡ UDP Commands**: Low-latency command processing (< 50ms)
- **🔍 Health Monitoring**: Memory, WiFi, and performance monitoring
- **💾 SD Card Support**: Video storage and file management
- **🔐 Secure Updates**: Password-protected OTA with error recoveryrmware

ESP32-based firmware for film set prop controllers with embedded screens and NeoPixel LEDs.

## Features

- **WiFi Connectivity**: Auto-connect to designated network with reconnection handling
- **Video Playback**: JPEG sequence playback from SD card with loop controls
- **NeoPixel Control**: 12-channel RGB LED control with animations and SACN support
- **TFT Display**: 3.2" color display with video playback and status information
- **UDP Commands**: Low-latency command processing (< 50ms)
- **mDNS Discovery**: Automatic service advertisement for central server discovery
- **Health Monitoring**: Memory, WiFi, and performance monitoring
- **SD Card Support**: Video storage and file management

## Hardware Requirements

### Target Development Board: ESP32-2432S032C-I
- **MCU**: ESP32-WROOM-32 (4MB Flash, 520KB RAM)
- **Display**: 3.2" IPS TFT LCD (240x320) with ST7789 driver
- **Storage**: MicroSD card slot (up to 32GB, Class 10 recommended)
- **LEDs**: Connection for 12x WS2812B NeoPixel strip
- **Power**: 5V USB-C input
- **Board Size**: 93.7 x 55.0mm

### Additional Components Required
- **LEDs**: 12x WS2812B NeoPixel strip
- **Connections**: Dupont wires for LED strip connection
- **Storage**: MicroSD card (32GB max, Class 10 recommended)
- **Power**: 5V USB-C power supply (2A recommended)

## Pin Configuration

### ESP32-2432S032C-I Board Pinout
```cpp
// Built-in peripherals (pre-configured)
// Display: 3.2" IPS TFT LCD with ST7789 driver
// SD Card: Built-in slot using SPI
// Touch: Capacitive touch controller (built-in)

// External connections for NeoPixel LEDs
#define LED_PIN    2       // NeoPixel data pin (GPIO2)
#define NUM_LEDS   12      // Number of LEDs in strip

// SD Card pins (built-in slot)
#define SD_CS      5       // Chip select for SD card
#define SD_MOSI    23      // SD card MOSI
#define SD_MISO    19      // SD card MISO
#define SD_SCLK    18      // SD card SCLK

// Display pins (handled by TFT_eSPI library)
// These are already configured for the ESP32-2432S032C-I board
// No manual pin definitions needed for the built-in display
```

## Video Playback Setup

The tricorder supports video playback from SD card using JPEG image sequences. For detailed setup instructions, see:

📹 **[VIDEO_PLAYBACK_GUIDE.md](VIDEO_PLAYBACK_GUIDE.md)** - Complete video setup and usage guide

### Quick Start for Videos
1. Format SD card as FAT32
2. Create `/videos/` directory on SD card
3. Convert videos to JPEG format (320x240 recommended)
4. Use UDP commands to control playback

### Supported Formats
- JPEG images (.jpg, .jpeg)
- Resolution: 320x240 recommended
- Frame rate: Up to 15 FPS
- File size: <50KB per frame recommended

### Test Files
Use the included scripts to prepare video content:
- `prepare_video.sh` (Linux/Mac)
- `prepare_video.bat` (Windows)
- `test_video.py` (Testing script)

### External Component Connections
- **NeoPixel Strip**: Connect data pin to GPIO2
- **Power**: Use built-in USB-C connector (5V)
- **SD Card**: Use built-in microSD slot

## Software Dependencies

### Arduino Libraries
```cpp
#include <WiFi.h>           // ESP32 WiFi
#include <ESPmDNS.h>        // Service discovery
#include <WiFiUdp.h>        // UDP communication
#include <ArduinoJson.h>    // JSON parsing
#include <FastLED.h>        // NeoPixel control
#include <TFT_eSPI.h>       // Display driver
#include <SD.h>             // SD card access
#include <AsyncWebServer.h> // Web server
```

### Installation via Arduino IDE
1. Install ESP32 board package
2. Install required libraries via Library Manager
3. Configure TFT_eSPI for your display (User_Setup.h)

### Installation via PlatformIO
```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    fastled/FastLED@^3.5.0
    bodmer/TFT_eSPI@^2.5.0
    bblanchon/ArduinoJson@^6.21.0
    me-no-dev/ESP Async WebServer@^1.2.3
```

## Configuration

### Network Settings
```cpp
const char* WIFI_SSID = "Rigging Electrics";
const char* WIFI_PASSWORD = "academy123";
const int UDP_PORT = 8888;
```

### Device Settings
```cpp
String deviceId = "TRICORDER_001";  // Auto-generated from MAC
String firmwareVersion = "1.0.0";
int sacnUniverse = 1;               // SACN universe assignment
int sacnStartChannel = 1;           // Starting DMX channel
```

## API Reference

### Status Endpoint
```
GET /status
Response: {
  "device_id": "TRICORDER_001",
  "firmware_version": "1.0.0",
  "wifi_connected": true,
  "ip_address": "192.168.1.100",
  "current_video": "scene1.mp4",
  "video_playing": true,
  "battery_voltage": 4.2,
  "temperature": 25.0,
  "uptime": 12345
}
```

### File List Endpoint
```
GET /files
Response: {
  "files": [
    {"name": "scene1.mp4", "size": 1048576},
    {"name": "scene2.mp4", "size": 2097152}
  ]
}
```

## Command Protocol

### UDP Command Format
```json
{
  "commandId": "uuid-string",
  "action": "play_video",
  "parameters": {
    "filename": "animation.jpg",
    "loop": true
  }
}
```

### Response Format
```json
{
  "commandId": "uuid-string",
  "result": "Video started: animation.jpg",
  "timestamp": 1234567890,
  "deviceId": "TRICORDER_001"
}
```

### Supported Commands

#### Video Control
- `play_video`: Start video playback from SD card
- `stop_video`: Stop current video playback
- `list_videos`: List available videos on SD card

#### LED Control
- `set_led_color`: Set solid color (r, g, b values 0-255)
- `set_led_brightness`: Set brightness (0-255)
- `set_builtin_led`: Set built-in RGB LED color

#### System Commands
- `status`: Get device status and video information

## SACN Integration

### Universe Configuration
- **Universe 1**: Devices 1-10 (channels 1-120)
- **Universe 2**: Devices 11-20 (channels 1-120)

### Channel Mapping
Each device uses 12 channels (one per LED):
```
Channels 1-3:   LED 1 (R, G, B)
Channels 4-6:   LED 2 (R, G, B)
...
Channels 34-36: LED 12 (R, G, B)
```

### Priority Handling
1. **SACN Commands**: Highest priority (lighting console)
2. **Manual Commands**: Medium priority (web interface)
3. **Automatic Modes**: Lowest priority (default animations)

## Performance Optimization

### Memory Management
- Use PROGMEM for static strings
- Minimize JSON document sizes
- Clear buffers after use
- Monitor heap usage

### Network Optimization
- UDP for low-latency commands
- TCP for file transfers
- Connection pooling
- Automatic reconnection

### Power Management
- LED brightness control
- WiFi power saving modes
- Display dimming/sleep
- Battery monitoring

## Troubleshooting

### Common Issues

#### WiFi Connection Problems
```cpp
// Check signal strength
int rssi = WiFi.RSSI();
if (rssi < -70) {
    Serial.println("Weak WiFi signal: " + String(rssi));
}

// Monitor connection status
if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
}
```

#### SD Card Issues
```cpp
// Test SD card functionality
if (!SD.begin(SD_CS)) {
    Serial.println("SD card failed or not present");
    // Try different speeds or check connections
}
```

#### LED Problems
```cpp
// Test LED strip
for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
    FastLED.show();
    delay(100);
}
```

### Debug Output
Enable serial debugging for detailed diagnostics:
```cpp
#define DEBUG_MODE 1

#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif
```

## Future Enhancements

### Planned Features
- **Advanced Video**: Multiple format support, streaming
- **Enhanced LEDs**: RGBW support, complex animations
- **Audio**: Synchronized sound playback
- **Sensors**: Motion detection, environmental monitoring
- **Security**: Encrypted communications, device authentication

### Hardware Considerations
- **Thermal Management**: Heat sinks for continuous operation
- **Mechanical**: Robust enclosures for film set use
- **Power**: Fast charging, power estimation
- **Expansion**: I2C for additional sensors/displays

## License

MIT License - See main project LICENSE file
