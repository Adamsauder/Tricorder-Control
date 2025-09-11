<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Prop Control System - Copilot Instructions

Film set prop control system with ESP32 devices, real-time web dashboard, and hybrid UDP/SACN lighting protocol. **PRODUCTION FOCUS**: Preparing for upcoming shoot with specific prop requirements.

**IMPORTANT**: This system runs on Windows with PowerShell as the default shell. When generating terminal commands, always use PowerShell syntax (semicolons `;` for command chaining, not `&&`). Never use bash/Linux syntax.

## Production Requirements - Current Shoot Focus

**Priority Props for Upcoming Shoot**:
- **IV Injectors**: ESP32-C3 + 1 NeoPixel (✅ Complete with OTA system)
- **Tricorders**: ESP32 + TFT display + NeoPixels + SD video playback (✅ Complete)
- **Polyinoculators**: ESP32-C3 + 3 NeoPixel strips (15 LEDs total) (✅ Complete)
- **IV Stations**: New - Modified Tricorder firmware with adjusted LED count
- **Ostoregenerators**: New - Modified IV Injector firmware
- **Hand Scanners**: New - Modified IV Injector firmware

**Critical Requirements**:
- ✅ **OTA Firmware System**: All devices must support remote OTA updates like IV Injectors
- ✅ **Network Configuration**: DHCP toggle and static IP setting via server AND device web interface
- ✅ **SACN Control**: Universe and address setting via server AND device web interface  
- ✅ **Bulk Operations**: Table view with device selection and multi-device actions
- ✅ **Individual Device Config**: Each prop accessible via its IP for local configuration

## Architecture Overview

**Three-tier system**: React/TypeScript web dashboard → Flask/SocketIO Python server → ESP32 firmware via UDP (port 8888) + SACN E1.31 (port 5568). Uses SQLite for device persistence, WebSocket for real-time updates, and mDNS for auto-discovery.

**Prop-Type Grouping**: Server groups devices by type with table view for bulk operations like SACN address changes, firmware updates, and network configuration.

**Production Device Types**: 
**Production Device Types**:
- **IV Injectors**: ESP32-C3 + 1 NeoPixel (✅ OTA system complete)
- **Tricorders**: ESP32 + TFT display + NeoPixels + SD video playback (✅ Complete with folder controls)
- **Polyinoculators**: ESP32-C3 + 3 NeoPixel strips (15 LEDs total) (✅ Complete)
- **IV Stations**: ESP32 + TFT display + modified LED configuration (✅ Complete)
- **Ostoregenerators**: ESP32-C3 + 1 NeoPixel + specialized firmware (✅ Complete)  
- **Hand Scanners**: ESP32-C3 + 1 NeoPixel + scanner-specific features (✅ Complete)
- **Pin Stands**: ESP32-C3 + 1 NeoPixel + pin stand functionality (✅ Complete)

All devices support OTA updates, network configuration, and SACN/UDP hybrid control.

## Development Workflow

### Quick Start
```bash
# VS Code tasks (preferred)
Ctrl+Shift+P → Tasks: Run Task → "Start Python Server"  # Port 8080
Ctrl+Shift+P → Tasks: Run Task → "Start Web Development Server"  # Port 3002

# Manual start  
python server/enhanced_server.py  # Main server
cd web && npm run dev           # React dev server
```

### Firmware Development
```bash
# PlatformIO (preferred) - use individual project folders
cd firmware/tricorder/ && pio run -t upload        # ESP32 with display
cd firmware/polyinoculator/ && pio run -t upload   # ESP32-C3 with 3 LED strips
cd firmware/iv_injector/ && pio run -t upload      # ESP32-C3 with 1 LED
cd firmware/iv_station/ && pio run -t upload       # ESP32 with display (modified Tricorder)
cd firmware/ostoregenerator/ && pio run -t upload  # ESP32-C3 with 1 LED (modified IV Injector)
cd firmware/hand_scanner/ && pio run -t upload     # ESP32-C3 with 1 LED (modified IV Injector)
cd firmware/pin_stand/ && pio run -t upload        # ESP32-C3 with 1 LED (modified IV Injector)

# Or use VS Code tasks
Ctrl+Shift+P → Tasks: Run Task → "Upload [Device] Firmware"
```

### Testing Commands
```bash
python test_device_cleanup.py      # Device lifecycle testing  
python test_sacn_system.py         # SACN/UDP protocol testing
python server/quick_test.py        # End-to-end system test
```

## Key Code Patterns

### Server Architecture (Flask + SocketIO)
- **Main server**: `server/enhanced_server.py` - Flask app with UDP listener thread, OTA system, network config
- **SACN integration**: `server/enhanced_sacn_controller.py` - E1.31 protocol handler
- **Device state**: Global `devices` dict updated via UDP heartbeats, broadcast via SocketIO
- **Command flow**: Web → Flask endpoint → UDP to device → response via SocketIO
- **Prop grouping**: Group devices by `device_type` field for table view and bulk operations

### Bulk Operations Pattern
- **SACN address setting**: `/api/props/{prop_type}/sacn/address` - Set universe.address for all online devices of type
- **Network configuration**: `/api/props/{prop_type}/network/config` - Set DHCP toggle and static IP for all devices
- **Firmware updates**: `/api/props/{prop_type}/firmware/update` - Push .bin file to all devices of type via OTA
- **Group commands**: `/api/props/{prop_type}/command` - Send action to all devices of prop type
- **Table view selection**: Multi-device selection in web interface for bulk operations

### ESP32 Firmware Patterns
- **WiFi + UDP setup** in `setup()`: Auto-connect, mDNS registration, UDP port 8888
- **Main loop**: `handleUDPCommands()` + status broadcasting + WiFi reconnection
- **Command structure**: JSON via UDP with `action`, `commandId`, device-specific params
- **Status updates**: Periodic UDP broadcast to server with device state

### Web Interface (React + Material-UI)
- **Main component**: `TricorderFarmDashboard.tsx` - device grid with real-time updates
- **Redesign needed**: Group devices into prop-type cards instead of individual device cards
- **Prop-type cards**: Single card per device type showing aggregate status (online/total count)
- **Bulk controls**: SACN address input, firmware upload, group commands per prop type
- **State management**: Custom hook `useTricorderFarm.ts` with WebSocket integration  
- **Device interaction**: Prop-type selection, bulk operations, individual device drill-down
- **Real-time updates**: `wsService` handles device_update/command_response events

## Project-Specific Conventions

### Device Communication
- **UDP commands**: JSON with `action`, `commandId`, parameters. Always include timeout handling.
- **SACN/UDP hybrid**: SACN for lighting console integration, UDP for direct commands
- **Device discovery**: mDNS service registration + periodic status broadcasts
- **OTA updates**: HTTP upload + ArduinoOTA, never requires USB after initial flash
- **Bulk SACN addressing**: Command format `{"action": "set_sacn_address", "universe": 221, "address": 1}` sent to filtered device list
- **Group firmware updates**: Iterate through devices of same type, send OTA update sequentially with progress tracking

### File Organization  
- **Active code**: `/server/enhanced_server.py`, `/web/src/`, `/firmware/{tricorder,polyinoculator,iv_injector}/`
- **Archived code**: `/archive/` - legacy servers, old firmware, test scripts  
- **Convenience scripts**: Root-level `.bat`/`.ps1` files for common tasks
- **New firmware needed**: `/firmware/iv_station/`, `/firmware/ostoregenerator/`, `/firmware/hand_scanner/`
- **Archived code**: `/archive/` - legacy servers, old firmware, test scripts  
- **Convenience scripts**: Root-level `.bat`/`.ps1` files for common tasks
### Hardware-Specific Details
- **Tricorder**: ESP32-2432S032C-I with ST7789 TFT (320x240), SD card, 12x WS2812B on GPIO2
- **Polyinoculator**: ESP32-C3 XIAO with 3 LED strips (GPIO10: 7 LEDs, GPIO6: 4 LEDs, GPIO7: 4 LEDs)
- **IV Injector**: ESP32-C3 XIAO with 1x WS2812B LED (GPIO5)
- **IV Station**: ESP32-2432S032C-I (same as Tricorder) with modified LED count for station configuration
- **Ostoregenerator**: ESP32-C3 XIAO (same as IV Injector) with specialized osteo regeneration features
- **Hand Scanner**: ESP32-C3 XIAO (same as IV Injector) with scanner-specific UI and features
- **Pin configurations**: Defined in platformio.ini build flags, not in code comments

### Performance Requirements
- **Command latency**: <50ms via UDP. Use background threads, avoid blocking operations.
- **SACN processing**: 30fps DMX updates. Cache device mappings, batch LED updates.
- **Memory management**: ESP32 heap monitoring in firmware, device cleanup in server.
