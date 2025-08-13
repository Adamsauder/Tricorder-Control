# Device Reset Guide

This guide provides multiple methods to reset your tricorder to factory defaults when it becomes unresponsive due to configuration issues (especially WiFi settings).

## 🚨 When to Use Reset

Use factory reset when:
- Device won't connect to WiFi due to wrong credentials
- Web interface is inaccessible
- Device appears frozen or unresponsive
- Network settings are corrupted
- You want to return to default settings

## 🔧 Reset Methods

### Method 1: Hardware Reset (GPIO12)
**Best for: Device is completely unresponsive**

1. **Power off** the tricorder
2. **Connect a jumper wire** from GPIO12 to Ground (GND)
3. **Power on** the device while maintaining the connection
4. **Keep connected** for 3+ seconds after power on
5. **Remove jumper** when you see the LED blinking yellow
6. Device will show **green LED** if reset successful
7. Device will **restart automatically** with factory defaults

### Method 2: Hardware Reset (GPIO13)
**Alternative hardware reset method**

1. **Power off** the tricorder
2. **Connect a jumper wire** from GPIO13 to Ground (GND)
3. **Power on** while maintaining the connection
4. **Keep connected** for 2+ seconds after power on
5. **Remove jumper** when LED blinks yellow
6. Device will restart with factory defaults

### Method 3: Rapid Reboot Reset
**Best for: Software issues or when hardware access is difficult**

1. **Restart the device 5 times quickly** (within 30 seconds)
2. **Each restart** should be a full power cycle
3. **On the 5th restart**, the device will automatically trigger factory reset
4. **LED will blink yellow** to indicate reset in progress
5. Device will restart with factory defaults

### Method 4: Web Interface Reset
**Best for: When WiFi is working but settings need reset**

1. Open web browser to device IP address
2. Navigate to the main status page
3. Click **"⚠️ Factory Reset"** button
4. Confirm the reset when prompted
5. Device will restart automatically

### Method 5: Access Point Recovery
**Automatic when WiFi fails**

When the device can't connect to WiFi, it automatically:
1. **Creates an Access Point** named `Tricorder-[DeviceID]`
2. **Default password**: `tricorder123`
3. **AP IP address**: `192.168.4.1`

To use recovery mode:
1. Connect your phone/computer to the AP
2. Open browser to `http://192.168.4.1`
3. Update WiFi settings or perform factory reset
4. Restart device to apply new settings

## 📱 Visual Indicators

### LED Status Codes
- **Blue**: Device booting/initializing
- **Green**: WiFi connected successfully
- **Orange**: Access Point mode (recovery)
- **Red**: WiFi connection failed
- **Yellow Blinking**: Factory reset in progress
- **Green Solid (2 sec)**: Reset successful
- **Red Solid (2 sec)**: Reset failed

### Display Status
The TFT screen shows comprehensive status including:
- Device name and ID
- WiFi connection status
- IP address information
- Battery level
- SD card status
- Reset instructions

## ⚙️ Default Settings After Reset

After factory reset, device returns to:

```
Device Label: "Tricorder-01"
WiFi SSID: "your_wifi_name"
WiFi Password: "your_wifi_password"
Static IP: "" (DHCP enabled)
Hostname: "tricorder"
sACN Universe: 1
DMX Address: 1
LED Brightness: 128
UDP Port: 8888
Web Port: 80
Battery Monitoring: Enabled
Debug Mode: Disabled
```

## 🔍 Troubleshooting

### Reset Not Working?
1. **Check connections** - Ensure GPIO12 or GPIO13 is properly grounded
2. **Verify power supply** - Ensure stable power during reset
3. **Hold longer** - Keep pin grounded for full 3+ seconds
4. **Try rapid reboot** - If hardware method fails, try 5 quick restarts
5. **Check LED feedback** - Look for yellow blinking during reset

### Still Can't Access?
1. **Check for Access Point** - Look for `Tricorder-[ID]` network after failed WiFi
2. **Use serial monitor** - Connect USB and check debug output at 115200 baud
3. **Try different reset method** - If one method fails, try another
4. **Re-flash firmware** - Use PlatformIO to upload fresh firmware

### Rapid Reboot Method Not Working?
1. **Ensure full power cycles** - Each restart must be complete power off/on
2. **Time the restarts** - All 5 restarts must be within 30 seconds
3. **Watch for patterns** - Device tracks boot count, may need to wait 30 seconds between attempts
4. **Check serial output** - Connect USB to see boot count messages

### Access Point Not Appearing?
1. **Wait 30 seconds** after WiFi connection fails
2. **Check 2.4GHz WiFi** - Device only supports 2.4GHz, not 5GHz
3. **Restart device** - Power cycle and wait for AP mode
4. **Check antenna** - Ensure WiFi antenna is properly connected

## 📋 Emergency Contact Information

If all reset methods fail:
1. Connect via USB and use PlatformIO serial monitor
2. Re-flash firmware using: `pio run --target upload --upload-port [PORT]`
3. Check hardware connections and power supply
4. Verify ESP32 board is not damaged

## 🔒 Security Notes

- **Default AP password** is `tricorder123` - change after recovery
- **Factory reset erases all settings** including custom configurations
- **No data loss** on SD card - only device settings are reset
- **WiFi credentials** will need to be re-entered after reset

## 📚 Additional Resources

- [Setup Guide](docs/setup-guide.md)
- [Network Troubleshooting](firmware/NETWORK_TROUBLESHOOTING.md)
- [API Documentation](docs/api-documentation.md)
- [Firmware Update Guide](FIRMWARE_UPDATE_GUIDE.md)

---

**Remember**: Factory reset is a powerful tool - use it when other troubleshooting methods fail. Always ensure the device has stable power during the reset process.
