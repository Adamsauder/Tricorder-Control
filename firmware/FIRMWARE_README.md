# Firmware Directory

This directory contains the PlatformIO-based firmware for both device types in the Prop Control System.

## Project Structure

```
firmware/
├── tricorder/                # ESP32 Tricorder Project
│   ├── src/main.cpp          # Main firmware code
│   └── platformio.ini        # PlatformIO configuration
├── polyinoculator/           # ESP32-C3 Polyinoculator Project  
│   ├── src/main.cpp          # Main firmware code
│   └── platformio.ini        # PlatformIO configuration
├── arduino_legacy/           # Archived Arduino IDE files
├── FLASHING_GUIDE.md         # PlatformIO flashing instructions
└── FIRMWARE_README.md        # This file
```

## Device Firmware

### 🔷 **Tricorder (ESP32-2432S032C-I)**
- **Location**: `tricorder/src/main.cpp`
- **PlatformIO Environment**: `[env:tricorder]` 
- **Hardware**: ESP32 with 2.8" TFT display, SD card, LED strip
- **Features**: Video playback, touch interface, WiFi control, sACN support

### 🔸 **Polyinoculator (Seeed XIAO ESP32-C3)**
- **Location**: `polyinoculator/src/main.cpp`
- **PlatformIO Environment**: `[env:polyinoculator]`
- **Hardware**: ESP32-C3 with 15x WS2812B LEDs across 3 strips
  - Strip 1: 7 LEDs on GPIO10
  - Strip 2: 4 LEDs on GPIO4 (avoiding GPIO0 boot conflicts)
  - Strip 3: 4 LEDs on GPIO5 (avoiding GPIO1 UART conflicts)
- **Features**: Multi-strip LED control, WiFi control, sACN support

## Build Instructions

### Prerequisites
1. Install VS Code
2. Install PlatformIO IDE extension
3. See `FLASHING_GUIDE.md` for detailed setup

### Quick Build & Upload
```bash
# For Tricorder
cd firmware/tricorder/
pio run -t upload

# For Polyinoculator  
cd firmware/polyinoculator/
pio run -t upload
```

### VS Code Integration
1. **Single Project**: Open `firmware/tricorder/` or `firmware/polyinoculator/` folder
2. **Multi Project**: Use the provided workspace file
3. **Upload**: Click PlatformIO Upload button or Ctrl+Shift+P → "PlatformIO: Upload"

## Key Features

### Tricorder
- Video playback from SD card
- Touch screen interface  
- WiFi control via UDP
- LED strip control
- Battery monitoring
- sACN lighting support

### Polyinoculator  
- 12-channel LED array
- WiFi control via UDP
- sACN lighting support
- Low power ESP32-C3
- Compact form factor

## Network Configuration
Both devices connect to WiFi network "Rigging Electric" and respond to UDP commands on port 8888.

## Configuration

### Library Dependencies
Libraries are automatically managed by PlatformIO:

**Tricorder**:
- FastLED ^3.7.0
- TFT_eSPI ^2.5.43  
- ArduinoJson ^7.2.0
- JPEGDEC (GitHub)

**Polyinoculator**:
- FastLED ^3.6.0
- ArduinoJson ^7.0.0

### Hardware Configuration
TFT_eSPI settings are defined in build flags (no manual User_Setup.h editing required):
- ST7789 driver for 240x320 display
- SPI pins optimized for ESP32-2432S032C-I
- RGB color order and frequencies pre-configured

## Legacy Files
Arduino IDE files are preserved in `arduino_legacy/` for reference but are no longer actively maintained.

## Documentation
- `FLASHING_GUIDE.md` - Complete PlatformIO setup and flashing guide
- `VIDEO_PLAYBACK_GUIDE.md` - Video setup for tricorders  
- `NETWORK_TROUBLESHOOTING.md` - Network configuration issues

## Troubleshooting

### Build Issues
```bash
# Clean and rebuild
pio run -t clean
pio run

# Update libraries
pio pkg update

# Check environment
pio system info
```

### Upload Issues
1. Hold BOOT button during upload for ESP32 boards
2. Check USB drivers (CP2102 for tricorder, CH340 for polyinoculator)
3. Try different USB cables
4. Verify correct COM port selection
