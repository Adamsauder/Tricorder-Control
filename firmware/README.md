# Tricorder Firmware

ESP32-based firmware for film set prop controllers with embedded screens and NeoPixel LEDs.

## Features

- **WiFi Connectivity**: Auto-connect to designated network with reconnection handling
- **Video Playback**: MP4/AVI playback from SD card with play/pause/next controls
- **NeoPixel Control**: 12-channel RGB LED control with animations and SACN support
- **Web API**: RESTful endpoints for status and file management
- **UDP Commands**: Low-latency command processing (< 50ms)
- **mDNS Discovery**: Automatic service advertisement for central server discovery
- **Health Monitoring**: Battery, temperature, and performance monitoring
- **OTA Updates**: Over-the-air firmware update capability

## Hardware Requirements

### Minimum Configuration
- **MCU**: ESP32-WROOM-32 (4MB Flash, 520KB RAM)
- **Display**: 3.5" TFT LCD (320x240, SPI interface)
- **Storage**: MicroSD card (16GB+, Class 10)
- **LEDs**: 12x WS2812B NeoPixels
- **Power**: 5V USB-C or battery pack (2A+)

### Recommended Configuration
- **MCU**: ESP32-S3 (8MB Flash, 512KB SRAM, faster CPU)
- **Display**: 4.3" TFT LCD with touch (480x272)
- **Storage**: High-speed microSD (32GB+, UHS-I)
- **LEDs**: 12x SK6812 (RGBW for better color mixing)
- **Power**: LiPo battery with charging circuit

## Pin Configuration

```cpp
// Display (SPI)
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RST    4
#define TFT_MOSI   23
#define TFT_SCLK   18

// SD Card (SPI)
#define SD_CS      5
#define SD_MOSI    23  // Shared with TFT
#define SD_SCLK    18  // Shared with TFT
#define SD_MISO    19

// NeoPixels
#define LED_PIN    2
#define NUM_LEDS   12

// Optional sensors
#define BATTERY_PIN  A0
#define TEMP_PIN     A1
```

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
const char* WIFI_SSID = "TRICORDER_CONTROL";
const char* WIFI_PASSWORD = "filmset2024";
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
  "command_id": "uuid-string",
  "timestamp": 1234567890,
  "target": "TRICORDER_001",
  "action": "video_play",
  "parameters": {
    "filename": "scene1.mp4",
    "loop": true
  }
}
```

### Response Format
```json
{
  "command_id": "uuid-string",
  "timestamp": 1234567891,
  "device_id": "TRICORDER_001",
  "status": "success",
  "message": "Video playing: scene1.mp4",
  "execution_time_ms": 15
}
```

### Supported Commands

#### Video Control
- `video_play`: Start video playback
- `video_pause`: Pause current video
- `video_stop`: Stop and reset video
- `video_next`: Play next video in sequence

#### LED Control
- `led_color`: Set solid color (r, g, b values)
- `led_brightness`: Set brightness (0.0-1.0)
- `led_animation`: Start predefined animation
- `led_off`: Turn off all LEDs

#### System Commands
- `ping`: Health check (responds with "pong")
- `restart`: Reboot device
- `get_status`: Request status update

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
