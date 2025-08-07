# Polyinoculator Control System

The Polyinoculator is a secondary prop device in the Tricorder Control System, designed for simple LED strip control using the Seeed Studio XIAO ESP32-C3 microcontroller.

## Hardware Specifications

- **Microcontroller**: Seeed Studio XIAO ESP32-C3
- **LEDs**: 12x WS2812B addressable RGB LEDs
- **Communication**: WiFi (2.4GHz)
- **Power**: USB-C or external 5V supply
- **Form Factor**: Compact, suitable for small props

## Pin Configuration

- **LED Data Pin**: GPIO2 (D0)
- **Status LED**: GPIO3 (D1) - Optional
- **Power**: 5V for LED strip, 3.3V for ESP32-C3

## Features

### LED Control
- 12 individually addressable RGB LEDs
- Brightness control (0-255)
- Various built-in effects:
  - Rainbow cycle
  - Scanner/Kitt effect
  - Pulse/breathing effect
  - Individual LED control

### Network Communication
- **UDP Control**: Port 8888 for direct commands
- **SACN (E1.31)**: Port 5568 for lighting protocol
- **mDNS Discovery**: Automatic device discovery
- **Universe**: Default SACN universe 2 (configurable)

### SACN Integration
- Command-based SACN control (like Tricorders)
- 36 DMX channels (12 LEDs × 3 colors)
- Server translates SACN data to UDP commands
- Compatible with industry-standard lighting software

## Firmware Setup

### Using Arduino IDE

1. **Install ESP32 Board Package**:
   - Add to Board Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Install "esp32" by Espressif Systems

2. **Install Libraries**:
   ```
   FastLED by Daniel Garcia
   ArduinoJson by Benoit Blanchon
   WiFi (built-in)
   ESPmDNS (built-in)
   ```

3. **Board Settings**:
   - Board: "XIAO_ESP32C3"
   - Upload Speed: 460800
   - CPU Frequency: 160MHz
   - Flash Size: 4MB
   - Partition Scheme: Default

### Using PlatformIO

Use the provided `platformio_polyinoculator.ini` configuration:

```bash
pio run -e polyinoculator
pio run -e polyinoculator --target upload
```

## Network Configuration

Update these settings in the firmware:

```cpp
const char* WIFI_SSID = "Your_WiFi_Network";
const char* WIFI_PASSWORD = "Your_WiFi_Password";
const int SACN_UNIVERSE = 2;  // Change universe if needed
String deviceId = "POLYINOCULATOR_001";  // Unique device ID
```

## API Commands

### Discovery
```json
{
  "action": "discovery"
}
```

### Set All LEDs Color
```json
{
  "action": "set_led_color",
  "r": 255,
  "g": 0,
  "b": 0
}
```

### Set Individual LED
```json
{
  "action": "set_individual_led",
  "ledIndex": 0,
  "r": 255,
  "g": 255,
  "b": 0
}
```

### Set Brightness
```json
{
  "action": "set_brightness",
  "brightness": 128
}
```

### Effects
```json
{"action": "rainbow"}
{"action": "scanner", "r": 255, "g": 0, "b": 0}
{"action": "pulse", "r": 0, "g": 255, "b": 255}
```

### SACN Control
```json
{"action": "toggle_sacn"}
```

### Status Request
```json
{"action": "status"}
```

## SACN DMX Mapping

The polyinoculator receives SACN data via the server, which translates DMX channels to UDP commands:

| DMX Channel | LED Index | Color |
|-------------|-----------|-------|
| 1           | 0         | Red   |
| 2           | 0         | Green |
| 3           | 0         | Blue  |
| 4           | 1         | Red   |
| 5           | 1         | Green |
| 6           | 1         | Blue  |
| ...         | ...       | ...   |
| 34          | 11        | Red   |
| 35          | 11        | Green |
| 36          | 11        | Blue  |

The server receives SACN packets and sends individual LED commands to the polyinoculator via UDP.

## Testing

Use the provided test script:

```bash
python test_polyinoculator.py
```

Update the IP address in the script to match your device's IP.

## Integration with Tricorder System

1. **Server Auto-Discovery**: Polyinoculators automatically register with the Tricorder server
2. **Web Interface**: Devices appear in the web dashboard alongside Tricorders
3. **SACN Integration**: Controlled via professional lighting software or the server's SACN output
4. **Unified Control**: Can be controlled through the same web interface as Tricorders

## Troubleshooting

### WiFi Connection Issues
- Check SSID and password in firmware
- Ensure 2.4GHz WiFi (ESP32-C3 doesn't support 5GHz)
- Monitor serial output for connection status

### LED Issues
- Verify 5V power supply for LED strip
- Check data pin connection (GPIO2)
- Ensure proper grounding between ESP32-C3 and LED strip

### SACN Not Working
- Check universe number matches lighting console
- Verify network connectivity
- Monitor serial output for SACN packet reception

### Server Integration
- Ensure device shows up in server logs
- Check device appears in web interface
- Verify UDP port 8888 is accessible

## Expansion Ideas

- **Battery Power**: Add battery pack for portable operation
- **More LEDs**: Increase NUM_LEDS for longer strips
- **Sensors**: Add motion or sound sensors for reactive lighting
- **Multiple Strips**: Support multiple LED strip outputs
- **Dimmer Curves**: Add gamma correction for smoother dimming
