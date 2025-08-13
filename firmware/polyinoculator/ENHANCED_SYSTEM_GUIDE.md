# Enhanced Firmware System - Complete Guide

## Overview

The enhanced firmware system provides professional-grade prop management with persistent configuration, web-based setup, and advanced SACN integration. This guide covers deployment, configuration, and operation.

## Quick Start

### 1. Flash Enhanced Firmware

Choose your platform and run the appropriate script:

**Windows (Automatic):**
```bash
flash_enhanced_auto.bat
```

**Windows (PowerShell - Recommended):**
```bash
flash_enhanced.ps1
```

**Windows (Basic):**
```bash
flash_enhanced.bat
```

**Linux/macOS:**
```bash
./flash_enhanced.sh
```

### 2. Configure Your Prop

After flashing, run the configuration utility:
```bash
configure_prop.bat  # Windows
```

Or manually access the web interface at `http://prop-ip-address`

### 3. Start Enhanced Server (Optional)

For advanced management:
```bash
python server/enhanced_server.py
```

Access dashboard at: `http://localhost:8000/dashboard`

## Hardware Configuration

### LED Strip Layout
- **Strip 1**: 7 LEDs on pin D5 (GPIO5) - Main display
- **Strip 2**: 4 LEDs on pin D6 (GPIO16) - Secondary indicators  
- **Strip 3**: 4 LEDs on pin D8 (GPIO20) - Status/effects

### ESP32-C3 Pin Mapping
```
D5  (GPIO5)  -> LED Strip 1 Data
D6  (GPIO16) -> LED Strip 2 Data  
D8  (GPIO20) -> LED Strip 3 Data
```

**Note**: Pins avoid GPIO0/1 (USB-JTAG), GPIO2/8/9 (strapping), GPIO18/19 (USB D+/D-)

## Enhanced Features

### Persistent Configuration
- All settings stored in ESP32 NVRAM
- Survives power cycles and firmware updates
- Factory reset option available

### Web Interface Endpoints

**Configuration Pages:**
- `/` - Main prop interface
- `/config` - Configuration form
- `/status` - Current status display

**API Endpoints:**
- `/api/config` - Get/set configuration (JSON)
- `/api/status` - Current prop status
- `/api/factory-reset` - Reset to defaults
- `/api/restart` - Restart prop

### SACN Integration
- Individual universe and address per prop
- Enhanced server with conflict detection
- Database-backed prop discovery
- Real-time status monitoring

## Configuration Options

### Device Settings
```json
{
  "device_label": "Tricorder-01",
  "prop_id": "TRIC001", 
  "description": "Hero prop for Scene 1"
}
```

### SACN Settings
```json
{
  "sacn_universe": 1,
  "dmx_address": 1,
  "sacn_enabled": true
}
```

### LED Settings
```json
{
  "brightness": 128,
  "strip1_count": 7,
  "strip2_count": 4,
  "strip3_count": 4
}
```

### Network Settings
```json
{
  "wifi_ssid": "Rigging Electric",
  "wifi_password": "your_password",
  "static_ip": "192.168.1.100"
}
```

## Enhanced Server Features

### Automatic Discovery
- Scans network for props automatically
- Maintains database of known devices
- Tracks configuration history

### Conflict Detection
- Identifies SACN universe/address conflicts
- Suggests resolution strategies
- Prevents addressing errors

### Centralized Management
- Web dashboard for all props
- Bulk configuration updates
- Status monitoring and alerts

## Professional Usage

### Film Set Deployment

1. **Pre-Production Setup:**
   - Flash all props with enhanced firmware
   - Configure unique IDs and addresses
   - Document prop assignments in server database

2. **On-Set Operation:**
   - Start enhanced server on lighting console computer
   - Access dashboard for real-time prop status
   - Use conflict detection for troubleshooting

3. **Configuration Management:**
   - Backup prop configurations via server
   - Restore settings for different scenes
   - Track changes and maintain history

### Network Architecture

```
Lighting Console ─── Enhanced Server ─── WiFi Network
                          │                   │
                     [Dashboard]         [Prop 1] [Prop 2] [Prop N]
                     [Database]            │        │        │
                     [Conflict              └── SACN Universe ──┘
                      Detection]
```

## Troubleshooting

### Flash Issues
- Ensure ESP32-C3 is in download mode (hold BOOT, press RESET)
- Check USB cable and driver installation
- Verify correct COM port selection

### Network Issues
- Confirm WiFi credentials in configuration
- Check IP address conflicts
- Verify network connectivity with ping

### Configuration Issues
- Use factory reset if settings corrupted
- Check web interface accessibility
- Verify NVRAM storage with serial monitor

### SACN Issues
- Use enhanced server conflict detection
- Verify universe and address settings
- Check network multicast support

## File Structure

```
polyinoculator/
├── src/
│   ├── main.cpp              # Enhanced firmware
│   ├── PropConfig.h          # Configuration management
│   └── PropConfig.cpp        # Implementation
├── flash_enhanced_auto.bat   # Auto-platform flash script
├── flash_enhanced.ps1        # PowerShell flash (recommended)
├── flash_enhanced.bat        # Windows batch flash
├── flash_enhanced.sh         # Linux/macOS flash
├── configure_prop.bat        # Configuration utility
└── ENHANCED_SYSTEM_GUIDE.md  # This file
```

## Development Notes

### Adding New Configuration Parameters

1. Add to `PropConfig.h` struct
2. Implement getter/setter in `PropConfig.cpp`
3. Add NVRAM storage in save/load methods
4. Update web interface form
5. Add server-side validation

### Extending LED Functionality

- Additional strips can be added up to ESP32-C3 pin limits
- FastLED supports multiple controller types
- Consider memory usage for large strip counts

### Network Protocol Extensions

- UDP commands for low-latency control
- WebSocket support for real-time updates
- Custom protocol for advanced features

## Support

For issues or questions:
1. Check troubleshooting section above
2. Review firmware README.md
3. Consult NETWORK_TROUBLESHOOTING.md
4. Enable serial monitor for debug output

---

**Version**: Enhanced System v2.0  
**Compatible Hardware**: ESP32-C3 (Seeed Studio XIAO)  
**Required Software**: PlatformIO, Python 3.8+  
**Last Updated**: December 2024
