# Prop Control System - Project Structure

## Core Files (Active Development)

### Server
- `server/enhanced_server.py` - Main server application with sACN support
- `server/sacn_controller.py` - sACN E1.31 lighting protocol handler
- `server/requirements.txt` - Python dependencies
- `server/quick_test.py` - Quick device testing utility

### Firmware
- `firmware/tricorder_esp32_firmware.ino` - ESP32 tricorder firmware (Arduino IDE)
- `firmware/polyinoculator_esp32c3_firmware.ino` - ESP32-C3 polyinoculator firmware (Arduino IDE)
- `firmware/src/main.cpp` - ESP32 tricorder firmware (PlatformIO)
- `firmware/platformio.ini` - PlatformIO configuration (both devices)
- `firmware/platformio_polyinoculator.ini` - Standalone polyinoculator config
- `firmware/User_Setup.h` - TFT display configuration

### Web Interface
- `web/` - React-based web control interface

### Launchers
- `start_enhanced_server.bat` - Windows batch launcher
- `start_enhanced_server.ps1` - PowerShell launcher

### Documentation
- `README.md` - Main project documentation
- `DEPLOYMENT.md` - Deployment instructions
- `FIRMWARE_UPDATE_GUIDE.md` - Firmware update procedures
- `RELEASE_NOTES.md` - Version history
- `docs/` - Technical documentation

### Configuration
- `tricorder.db` - SQLite database
- `uploads/` - File upload storage
- `sample_animations/` - Animation templates

## Archived Files

### `/archive/legacy_servers/`
- Old server implementations (app.py, fixed_server.py, gui_server.py, etc.)
- Legacy standalone and simulator servers

### `/archive/test_scripts/`
- Development test scripts (test_*.py)
- Debug utilities (debug_udp.py, check_sacn_data.py)
- Conversion utilities

### `/archive/legacy_launchers/`
- Old launcher scripts for archived servers
- Legacy batch files

### `/archive/development_docs/`
- Development progress documentation
- Historical project files
- Version tracking files

### `/archive/`
- Log files
- Deprecated firmware directories
- Other legacy files

## Quick Start

1. **Install Dependencies**: `pip install -r server/requirements.txt`
2. **Start Server**: Run `start_enhanced_server.bat` or `start_enhanced_server.ps1`
3. **Access Interface**: Open http://localhost:8080
4. **Flash Firmware**: Use PlatformIO with `firmware/platformio.ini`

## Device Types

- **Tricorders**: ESP32 with TFT display, SD card, and LED strip
- **Polyinoculators**: ESP32-C3 with 12x WS2812B LEDs

## Protocols

- **UDP**: Device control and status (port 8888)
- **sACN (E1.31)**: Lighting control (port 5568)
- **WebSocket**: Real-time web interface updates
- **mDNS**: Device discovery
