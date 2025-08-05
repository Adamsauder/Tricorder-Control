# OTA (Over-The-Air) Update Guide for Tricorder ESP32

This guide explains how to update your Tricorder ESP32 firmware wirelessly using OTA (Over-The-Air) updates.

## Prerequisites

- Tricorder device powered on and connected to WiFi
- Computer on the same network as the Tricorder
- PlatformIO or Arduino IDE installed
- Firmware source code available

## OTA Configuration

The firmware is configured with the following OTA settings:
- **Hostname**: Device ID (e.g., "TRICORDER_001")
- **Password**: `tricorder123`
- **Port**: 3232 (default ArduinoOTA port)

## Method 1: PlatformIO OTA Updates (Recommended)

### Quick Setup

1. **Discover your device**:
   ```bash
   cd firmware
   python ota_update_helper.py --discover
   ```

2. **Add OTA environment to platformio.ini**:
   ```ini
   [env:esp32_ota]
   platform = espressif32
   board = esp32dev
   framework = arduino
   upload_protocol = espota
   upload_port = 192.168.1.100  ; Replace with your device IP
   upload_flags = --auth=tricorder123
   lib_deps = 
       fastled/FastLED@^3.5.0
       bodmer/TFT_eSPI@^2.5.0
       bblanchon/ArduinoJson@^6.21.0
       bitbank2/JPEGDEC@^1.2.8
   ```

3. **Upload firmware**:
   ```bash
   pio run -e esp32_ota --target upload
   ```

### Alternative: Command Line Upload

```bash
pio run --target upload --upload-port 192.168.1.100
```

## Method 2: Arduino IDE OTA Updates

### Setup Steps

1. **Open Arduino IDE**
2. **Install required libraries** (if not already installed):
   - FastLED
   - TFT_eSPI
   - ArduinoJson
   - JPEGDEC

3. **Configure board settings**:
   - Board: "ESP32 Dev Module"
   - Upload Speed: 921600
   - CPU Frequency: 240MHz
   - Flash Frequency: 80MHz
   - Flash Mode: QIO
   - Flash Size: 4MB
   - Partition Scheme: "Default 4MB with spiffs"

4. **Select network port**:
   - Go to Tools → Port
   - Look for your device in "Network ports" section
   - Select "TRICORDER_XXX at 192.168.1.XXX"

5. **Upload**:
   - Click Upload button
   - Enter password when prompted: `tricorder123`
   - Wait for upload to complete

## Method 3: Using OTA Helper Script

The included Python helper script makes OTA updates easier:

```bash
cd firmware
python ota_update_helper.py
```

### Helper Script Features

- **Device Discovery**: Automatically finds Tricorder devices on network
- **Connectivity Testing**: Pings devices to verify they're reachable
- **OTA Instructions**: Shows device-specific upload commands
- **Device Information**: Gets current firmware version and OTA status

### Helper Script Options

```bash
# Quick discovery
python ota_update_helper.py --discover

# Interactive mode (default)
python ota_update_helper.py
```

## Troubleshooting

### Device Not Found in Network Ports

**Problem**: Device doesn't appear in Arduino IDE network ports

**Solutions**:
1. Verify device is powered on and WiFi connected (check device display)
2. Ensure computer and ESP32 are on same network
3. Restart Arduino IDE
4. Try manual IP entry in PlatformIO method

### Authentication Failed

**Problem**: "Auth Failed" error during upload

**Solutions**:
1. Verify password is exactly: `tricorder123`
2. Check device is not busy (stop any running videos)
3. Reset device and try again
4. Use fresh terminal/IDE session

### Connection Timeout

**Problem**: Upload times out or fails to connect

**Solutions**:
1. Check device IP address (may have changed)
2. Verify network connectivity with ping:
   ```bash
   ping 192.168.1.100
   ```
3. Restart device WiFi by power cycling
4. Check firewall settings on computer

### Upload Interrupted

**Problem**: Upload starts but fails partway through

**Solutions**:
1. Ensure stable power supply to ESP32
2. Move closer to WiFi router (improve signal strength)
3. Close other network-intensive applications
4. Try again - sometimes temporary network issues cause this

### OTA Not Available

**Problem**: Device shows "OTA Error" or doesn't support OTA

**Solutions**:
1. Upload firmware via USB first to enable OTA
2. Check firmware compilation included ArduinoOTA library
3. Verify WiFi connection is stable

## Verification

After successful OTA update:

1. **Check device display**: Should show "UPDATE COMPLETE!" then reboot
2. **Verify firmware version**: Use discovery script or UDP status command
3. **Test functionality**: Check LEDs, display, and video playback work

## Security Notes

- OTA updates are password protected (`tricorder123`)
- Only devices on same network can perform updates
- Consider changing default password for production use
- OTA is disabled if WiFi connection fails

## Advanced: Custom OTA Settings

To modify OTA settings, edit the `setupOTA()` function in `main.cpp`:

```cpp
void setupOTA() {
  ArduinoOTA.setHostname("YOUR_HOSTNAME");
  ArduinoOTA.setPassword("YOUR_PASSWORD");
  // ... rest of setup
}
```

## Batch Updates

To update multiple devices:

1. Use discovery script to find all devices
2. Create platformio.ini environments for each device IP
3. Upload to each environment sequentially:
   ```bash
   pio run -e device1_ota --target upload
   pio run -e device2_ota --target upload
   ```

## Status Monitoring

Monitor OTA progress via:
- **Device display**: Shows progress bar and status
- **Serial monitor**: Detailed progress information
- **LED indicator**: Orange during update, green when complete, red on error

---

**Note**: Always keep a USB cable available as backup in case OTA updates fail and serial upload is needed for recovery.
