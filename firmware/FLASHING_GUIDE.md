# ESP32-2432S032C-I Flashing Guide

## Hardware Preparation

### 1. Board Setup
- Connect your ESP32-2432S032C-I to your computer via USB-C cable
- The board should power on and show a demo screen
- Red LED should indicate power is on

### 2. Drivers (Windows only)
If the board isn't recognized, install CP2102 drivers:
- Download from: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- Or use automatic driver installation via Windows Update

## Option A: Arduino IDE Flashing

### 1. Install Arduino IDE
- Download Arduino IDE 2.0+ from: https://www.arduino.cc/en/software
- Install and open the IDE

### 2. Configure ESP32 Board Package
1. File → Preferences
2. Additional Board Manager URLs: 
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Tools → Board → Board Manager
4. Search "ESP32" and install "esp32 by Espressif Systems"

### 3. Install Required Libraries
Go to Tools → Manage Libraries and install:
- **FastLED** by Daniel Garcia (version 3.7.0+)
- **TFT_eSPI** by Bodmer (version 2.5.43+)
- **ArduinoJson** by Benoit Blanchon (version 7.2.0+)
- **ESPAsyncWebServer** by Me-No-Dev (version 1.2.6+)

### 4. Configure TFT_eSPI Library
1. Find TFT_eSPI library folder:
   - Windows: `Documents\Arduino\libraries\TFT_eSPI`
   - macOS: `~/Documents/Arduino/libraries/TFT_eSPI`
   - Linux: `~/Arduino/libraries/TFT_eSPI`

2. Copy the `User_Setup.h` file from the firmware folder to the TFT_eSPI library folder
3. OR edit the existing `User_Setup.h` in TFT_eSPI library with our configuration

### 5. Board Configuration
1. Tools → Board → ESP32 Arduino → ESP32 Dev Module
2. Tools → Upload Speed → 921600
3. Tools → CPU Frequency → 240MHz (WiFi/BT)
4. Tools → Flash Frequency → 80MHz
5. Tools → Flash Mode → QIO
6. Tools → Flash Size → 4MB (32Mb)
7. Tools → Partition Scheme → Huge APP (3MB No OTA/1MB SPIFFS)
8. Tools → PSRAM → Disabled
9. Tools → Port → Select your COM port (e.g., COM3, COM4)

### 6. Upload Firmware
1. Open `tricorder_firmware.ino` in Arduino IDE
2. Press and hold the **BOOT** button on the ESP32 board
3. Click **Upload** in Arduino IDE
4. Keep holding BOOT until "Connecting..." appears, then release
5. Wait for upload to complete (about 30-60 seconds)

## Option B: PlatformIO Flashing (Recommended)

### 1. Install VS Code and PlatformIO
1. Install Visual Studio Code
2. Install PlatformIO IDE extension
3. Restart VS Code

### 2. Open Project
1. File → Open Folder
2. Select the `firmware` folder
3. PlatformIO should detect the `platformio.ini` file

### 3. Flash the Firmware
1. Connect ESP32 via USB-C
2. Press **Ctrl+Shift+P** (or **Cmd+Shift+P** on macOS)
3. Type "PlatformIO: Upload" and press Enter
4. OR click the Upload button (→) in the PlatformIO toolbar

## Troubleshooting

### Upload Issues
1. **"Failed to connect to ESP32"**:
   - Hold BOOT button while clicking Upload
   - Try different USB cable
   - Check COM port selection
   - Try lower upload speed (115200)

2. **"Brownout detector was triggered"**:
   - Use better quality USB cable
   - Try powered USB hub
   - Check power supply

3. **"Sketch too big"**:
   - Use "Huge APP" partition scheme
   - Remove unused libraries/features

### WiFi Connection Issues
1. Verify WiFi credentials in code:
   ```cpp
   const char* WIFI_SSID = "Rigging Electrics";
   const char* WIFI_PASSWORD = "academy123";
   ```

2. Check WiFi signal strength
3. Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

### Display Issues
1. **Blank/corrupted display**:
   - Verify TFT_eSPI User_Setup.h configuration
   - Check pin definitions match hardware
   - Try different SPI frequency

2. **Upside-down display**:
   - Change `tft.setRotation(1)` to different value (0-3)

### Serial Monitor
1. Tools → Serial Monitor
2. Set baud rate to 115200
3. You should see:
   ```
   Tricorder Control Firmware v1.0.0
   LEDs initialized
   Display initialized
   SD card initialized
   WiFi connected
   IP address: 192.168.x.x
   System ready
   ```

## First Boot Verification

After successful flashing:

1. **Display Check**: Screen should show "Tricorder Control" and device information
2. **WiFi Check**: Device should connect to "Rigging Electrics" network
3. **LED Check**: Green LEDs should flash briefly on startup
4. **Serial Check**: Open Serial Monitor to see connection status

## Next Steps

1. **Add NeoPixel Strip**: Connect WS2812B LEDs to GPIO pin 2
2. **Add SD Card**: Insert microSD card (32GB max, FAT32 format)
3. **Test Commands**: Use the web interface or Python server to send commands
4. **Network Discovery**: Device should appear on network as "TRICORDER_[MAC].local"

## Pin Configuration Summary

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| TFT_CS   | 15       | Display chip select |
| TFT_DC   | 2        | Display data/command |
| TFT_RST  | 4        | Display reset |
| TFT_BL   | 21       | Display backlight |
| SD_CS    | 5        | SD card chip select |
| LED_PIN  | 2        | NeoPixel data (external) |
| SPI_MOSI | 23       | SPI data out |
| SPI_MISO | 19       | SPI data in |
| SPI_CLK  | 18       | SPI clock |

## Support Resources

- **ESP32-2432S032C-I Documentation**: Search "ESP32 Cheap Yellow Display" on GitHub
- **TFT_eSPI Library**: https://github.com/Bodmer/TFT_eSPI
- **FastLED Library**: https://github.com/FastLED/FastLED
- **Arduino ESP32**: https://docs.espressif.com/projects/arduino-esp32/
