# Enhanced Firmware Flashing Guide

## 🚀 Quick Start - Flash Enhanced Polyinoculator

### Windows Users
1. **Double-click**: `flash_enhanced.bat`
2. **Follow prompts** - the script handles everything automatically
3. **Monitor progress** - watch for success message
4. **Configure prop** - run `configure_prop.bat` to find and configure your prop

### PowerShell Users (Recommended)
1. **Right-click** → "Run with PowerShell": `flash_enhanced.ps1`
2. **Better progress display** with colored output
3. **Automatic error handling** and troubleshooting

### Linux/macOS Users
1. **Make executable**: `chmod +x flash_enhanced.sh`
2. **Run script**: `./flash_enhanced.sh`
3. **May need sudo** for device permissions

## 🔧 What the Enhanced Firmware Includes

### New Features
- **Persistent Configuration**: Settings stored in NVRAM, survive power cycles
- **Web Configuration Interface**: Configure via web browser at device IP
- **Individual Addressing**: Each prop manages its own SACN/DMX settings
- **Device Labels**: Give props friendly names like "Hero Prop Left"
- **Conflict Detection**: Server automatically detects address conflicts

### Hardware Configuration
```
LED Strip 1: D5 (GPIO5)  → 7 LEDs  → DMX Channels 1-21
LED Strip 2: D6 (GPIO16) → 4 LEDs  → DMX Channels 22-33  
LED Strip 3: D8 (GPIO20) → 4 LEDs  → DMX Channels 34-45
Total: 15 LEDs (45 DMX channels)
```

## 📋 Flashing Process

### Step 1: Prepare Device
1. **Connect ESP32-C3** via USB-C cable
2. **Install drivers** if needed (usually automatic)
3. **Close other serial applications** (Arduino IDE, etc.)

### Step 2: Flash Firmware
1. **Run flash script** for your platform
2. **Wait for compilation** (first time takes longer)
3. **Watch for "FLASH SUCCESSFUL!"** message
4. **Device restarts automatically**

### Step 3: Initial Configuration
1. **Device connects to WiFi**: "Rigging Electric" (default)
2. **Check serial monitor** for assigned IP address
3. **Open web browser** to device IP
4. **Configure device settings**:
   - Device Label (e.g., "Hero Prop Left")
   - SACN Universe (1-63999)
   - DMX Start Address (1-512)
   - Brightness (0-255)

## 🌐 Web Configuration Interface

### Access Methods
- **Direct IP**: `http://192.168.1.xxx` (from serial monitor)
- **Auto-discovery**: Run `configure_prop.bat` to find props
- **Enhanced Dashboard**: Open `enhanced-prop-dashboard.html`

### Configuration Options
| Setting | Description | Range | Default |
|---------|-------------|-------|---------|
| Device Label | Friendly name | Text | "Polyinoculator_XXXX" |
| SACN Universe | Lighting universe | 1-63999 | 1 |
| DMX Start Address | First DMX channel | 1-512 | 1 |
| Brightness | LED brightness | 0-255 | 128 |
| WiFi SSID | Network name | Text | "Rigging Electric" |
| WiFi Password | Network password | Text | "academy123" |

## 🔍 Troubleshooting

### Flash Fails
1. **Hold BOOT button** while connecting USB
2. **Try different USB cable/port**
3. **Check Device Manager** for COM port
4. **Close other applications** using serial port
5. **Restart ESP32** and try again

### Device Not Found on Network
1. **Check serial monitor** for actual IP address
2. **Verify WiFi credentials** match your network
3. **Ping device IP** to test connectivity
4. **Check network firewall** settings

### Configuration Not Saving
1. **Use web interface** instead of direct commands
2. **Check serial output** for error messages
3. **Factory reset** if needed: POST to `/api/factory-reset`
4. **Reflash firmware** if persistent issues

## 📱 Using Enhanced Dashboard

### Features
- **Auto-discovery** of all props on network
- **Live configuration** via web interface
- **Conflict detection** for DMX addresses
- **Status monitoring** (online/offline)
- **Bulk configuration** capabilities

### Access Dashboard
1. **Open web browser**
2. **Navigate to**: `enhanced-prop-dashboard.html`
3. **Wait for prop discovery**
4. **Configure props** via dashboard interface

## 🎬 Film Set Usage

### Typical Setup
1. **Flash all props** with enhanced firmware
2. **Power on props** at location
3. **Connect to set WiFi** (configure SSID/password)
4. **Open dashboard** on operator tablet/laptop
5. **Configure addressing** per scene requirements
6. **Run enhanced server** for SACN control

### Pro Tips
- **Label props clearly** (e.g., "Car Dashboard", "Hero Phone")
- **Document addressing** for complex scenes
- **Use universe separation** for different areas
- **Test configuration** before rolling cameras
- **Keep factory reset procedure** handy for emergencies

## 🔄 Factory Reset

### When Needed
- Configuration corruption
- Wrong WiFi credentials
- Testing/development
- Preparing for new show

### Reset Methods
1. **Web Interface**: POST to `http://<ip>/api/factory-reset`
2. **Configuration Tool**: Use reset option in dashboard
3. **Hardware Reset**: Reflash firmware (nuclear option)

### After Factory Reset
- Device creates new random ID
- All settings return to defaults
- Reconfigure WiFi and addressing
- Update prop labels as needed

## 📞 Support

### Common Issues
- **IP Address Changes**: Use DHCP reservation or static IP
- **WiFi Connection**: Check SSID/password in serial monitor
- **DMX Conflicts**: Use enhanced dashboard conflict detection
- **Performance**: Monitor free heap in status messages

### Getting Help
- Check serial output for error messages
- Use enhanced dashboard for diagnostics
- Test with simple commands first
- Document exact steps to reproduce issues

---

**Note**: This enhanced firmware provides professional-grade configuration management for film set prop control. All settings persist through power cycles and can be managed remotely via web interface.
