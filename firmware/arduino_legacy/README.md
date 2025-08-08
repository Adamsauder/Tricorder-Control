# Arduino IDE Legacy Files

This folder contains the original Arduino IDE firmware files that have been superseded by the PlatformIO-based structure.

## Files
- `tricorder_esp32_firmware.ino` - Original tricorder firmware for Arduino IDE
- `polyinoculator_esp32c3_firmware.ino` - Original polyinoculator firmware for Arduino IDE  
- `User_Setup.h` - TFT_eSPI configuration (now handled by PlatformIO build flags)
- `platformio_old.ini` - Original combined PlatformIO configuration

## Migration
These files have been converted to the new PlatformIO structure:
- **Tricorder**: `../tricorder/src/main.cpp`
- **Polyinoculator**: `../polyinoculator/src/main.cpp`

## Status
These files are kept for reference only and are no longer actively maintained. Use the PlatformIO projects in the parent directories instead.

For current build instructions, see `../FLASHING_GUIDE.md`.
