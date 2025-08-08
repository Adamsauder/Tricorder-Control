# Prop Control System - PlatformIO Flashing Guide

## Prerequisites

### 1. Install VS Code and PlatformIO
1. **Download and Install VS Code**: https://code.visualstudio.com/
2. **Install PlatformIO IDE Extension**:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search "PlatformIO IDE"
   - Install the official PlatformIO extension
   - Restart VS Code when prompted

### 2. Hardware Preparation
- **Tricorder**: Connect ESP32-2432S032C-I via USB-C cable
- **Polyinoculator**: Connect Seeed XIAO ESP32-C3 via USB-C cable
- The boards should power on (red LED indicates power)

### 3. Drivers (Windows only)
If the board isn't recognized, install drivers:
- **ESP32-2432S032C-I**: CP2102 drivers from https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- **XIAO ESP32-C3**: Usually auto-detected, but CH340 drivers may be needed

## Firmware Structure

The project is organized into separate PlatformIO projects:

```
firmware/
├── tricorder/           # ESP32 Tricorder firmware
│   ├── src/main.cpp
│   └── platformio.ini
├── polyinoculator/      # ESP32-C3 Polyinoculator firmware  
│   ├── src/main.cpp
│   └── platformio.ini
└── FLASHING_GUIDE.md   # This file
```

## Flashing Instructions

### Method 1: Open Individual Project Folders

#### For Tricorder (ESP32):
1. **Open Tricorder Project**:
   - File → Open Folder
   - Navigate to `firmware/tricorder/`
   - PlatformIO will detect the `platformio.ini` file

2. **Flash Tricorder**:
   - Connect ESP32-2432S032C-I via USB-C
   - Press **Ctrl+Shift+P** → "PlatformIO: Upload"
   - OR click Upload button (→) in PlatformIO toolbar
   - **If upload fails**: Hold BOOT button while clicking Upload

#### For Polyinoculator (ESP32-C3):
1. **Open Polyinoculator Project**:
   - File → Open Folder  
   - Navigate to `firmware/polyinoculator/`
   - PlatformIO will detect the `platformio.ini` file

2. **Flash Polyinoculator**:
   - Connect Seeed XIAO ESP32-C3 via USB-C
   - Press **Ctrl+Shift+P** → "PlatformIO: Upload"
   - OR click Upload button (→) in PlatformIO toolbar

### Method 2: Multi-Project Workspace (Advanced)

1. **Create VS Code Workspace**:
   ```json
   {
     "folders": [
       { "path": "./tricorder" },
       { "path": "./polyinoculator" }
     ]
   }
   ```
   
2. **Save as**: `firmware/tricorder-workspace.code-workspace`

3. **Switch Projects**: Use PlatformIO Project Tasks panel to select environment

## Build Commands

### Command Line Interface (Optional)
```bash
# Navigate to project folder
cd firmware/tricorder/
# or 
cd firmware/polyinoculator/

# Build only
pio run

# Build and upload
pio run -t upload

# Clean build
pio run -t clean

# Monitor serial output
pio device monitor --baud 115200
```

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
   Starting Prop Control System...
   SD card initialized successfully!
   SD Card: OK
   Created /videos directory
   Found X video files
   WiFi connected!
   IP address: 192.168.x.x
   Setup complete!
   ```

## Video Functionality Testing

After successful flashing and video features are enabled:

### 1. SD Card Setup
1. **Format SD Card**: Use FAT32 format
2. **Create Directory**: Make a `/videos/` folder on the SD card
3. **Add Test Files**: Copy test videos (JPEG format) to `/videos/`
4. **Insert Card**: Insert SD card into the ESP32 board

### 2. Video Test Commands
Use the test script or send UDP commands:
```json
{"action": "list_videos", "commandId": "test1"}
{"action": "play_video", "commandId": "test2", "parameters": {"filename": "test.jpg", "loop": true}}
{"action": "stop_video", "commandId": "test3"}
```

### 3. Expected Video Output
- **Boot Screen**: Shows SD card status and video count
- **Video Playback**: Smooth display on TFT screen
- **Serial Output**: Video status and debugging information

## Verification

### First Boot Check
After successful flashing:

1. **Tricorder**:
   - Display shows "Tricorder Booting..." then status information
   - SD card check: "SD Card: OK" if card is inserted
   - WiFi connection to "Rigging Electric" network
   - Built-in RGB LED changes colors during boot
   - Serial output at 115200 baud shows initialization

2. **Polyinoculator**:
   - 12 LEDs briefly flash during boot
   - WiFi connection to "Rigging Electric" network  
   - Serial output shows device registration
   - Responds to UDP commands on port 8888

### Expected Serial Output

**Tricorder**:
```
Starting Prop Control System...
SD card initialized successfully!
SD Card: OK
Created /videos directory
Found X video files
WiFi connected!
IP address: 192.168.x.x
mDNS responder started: TRICORDER_[MAC].local
Setup complete!
```

**Polyinoculator**:
```
Starting Polyinoculator Control System...
Connecting to WiFi: Rigging Electric
WiFi connected!
IP address: 192.168.x.x  
mDNS responder started: POLYINOCULATOR_001.local
UDP server listening on port 8888
Setup complete!
```

## Testing & Setup

### Video Setup (Tricorder Only)
1. **Prepare Videos**: Use provided scripts to convert videos to JPEG format
   ```bash
   cd firmware/
   python generate_test_patterns.py  # Create test patterns
   ./prepare_video.sh input.mp4      # Convert existing videos (Linux/macOS)
   ./prepare_video.bat input.mp4     # Convert existing videos (Windows)
   ```

2. **SD Card Structure**:
   ```
   SD:/
   └── videos/
       ├── test_pattern.jpg
       ├── startup_animation.jpg
       └── color_cycle.jpg
   ```

3. **Test Video Playback**: 
   ```bash
   python test_video.py [TRICORDER_IP]
   ```

### LED Testing (Both Devices)
Use the UDP command interface:
```json
{"action": "set_leds", "commandId": "test1", "parameters": {"color": "#FF0000"}}
{"action": "set_leds_array", "commandId": "test2", "parameters": {"leds": ["#FF0000", "#00FF00", "#0000FF"]}}
```

### Network Discovery
Both devices register mDNS services:
- **Tricorder**: `TRICORDER_[MAC].local`
- **Polyinoculator**: `POLYINOCULATOR_001.local`

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
