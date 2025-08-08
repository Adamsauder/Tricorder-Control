# Tricorder Network Troubleshooting Guide

## Common Connection Issues and Solutions

### 1. **"getaddrinfo failed" Error**

**Problem:** You see `[Errno 11001] getaddrinfo failed` when running the test script.

**Cause:** Incorrect IP address format or DNS resolution issue.

**Solutions:**
```bash
# ❌ Wrong: Don't use brackets
python test_video.py [192.168.1.48]

# ✅ Correct: Use IP address directly
python test_video.py 192.168.1.48
```

### 2. **Device Not Responding**

**Problem:** Test script times out or device doesn't respond.

**Check List:**
1. **Power**: Is the tricorder powered on?
2. **WiFi**: Is the device connected to WiFi? (Check the display)
3. **Network**: Are both devices on the same network?
4. **IP Address**: Is the IP address correct?

**Diagnostics:**
```bash
# Test basic connectivity
ping 192.168.1.48

# Check if device is advertising via mDNS (if supported)
# Windows: Use Bonjour Browser
# Linux/Mac: avahi-browse -rt _tricorder._udp
```

### 3. **Wrong IP Address**

**Problem:** You don't know the tricorder's IP address.

**Finding the IP:**
1. **Check the tricorder display** - IP should be shown on boot
2. **Router admin page** - Look for connected devices
3. **Network scan** - Use tools like `nmap` or `Advanced IP Scanner`
4. **Serial monitor** - Connect via USB and check serial output

**Example with nmap:**
```bash
# Scan your local network (adjust range as needed)
nmap -sn 192.168.1.0/24
```

### 4. **Firewall Blocking UDP**

**Problem:** Firewall is blocking UDP port 8888.

**Solutions:**
- **Windows**: Add exception for UDP port 8888 in Windows Firewall
- **Router**: Check if UDP traffic is being filtered
- **Antivirus**: Some antivirus software blocks UDP traffic

### 5. **Network Configuration Issues**

**Problem:** Devices are on different network segments.

**Check:**
```bash
# Check your computer's IP
ipconfig  # Windows
ifconfig  # Linux/Mac

# Ensure both devices are in same subnet
# Example: Computer 192.168.1.100, Tricorder 192.168.1.48 = ✅ Same network
# Example: Computer 192.168.0.100, Tricorder 192.168.1.48 = ❌ Different networks
```

## Step-by-Step Troubleshooting

### Step 1: Basic Network Test
```bash
# Test if device responds to ping
ping 192.168.1.48

# If ping fails:
# - Check device is powered on
# - Verify IP address
# - Check WiFi connection on device
```

### Step 2: Test UDP Communication
```bash
# Run the test script with verbose output
python test_video.py 192.168.1.48

# Select 'i' for interactive mode
# Try 'status' command first
```

### Step 3: Check Tricorder Status
Look at the tricorder's display and serial output:
- Is WiFi connected? (Should show IP address)
- Is the firmware running? (Should show "Setup complete!")
- Any error messages?

### Step 4: Network Debugging

**If ping works but UDP doesn't:**
```bash
# Test with different UDP tool (Windows)
# Download netcat/ncat and test:
nc -u 192.168.1.48 8888

# Or use PowerShell:
$client = [System.Net.Sockets.UdpClient]::new()
$client.Connect("192.168.1.48", 8888)
```

## Common WiFi Network Names

The tricorder firmware is configured to connect to:
```cpp
const char* WIFI_SSID = "Rigging Electric";
const char* WIFI_PASSWORD = "academy123";
```

**If your network is different:**
1. Update the firmware with your network credentials
2. Reflash the ESP32
3. Or modify the firmware to use WiFi Manager for setup

## Quick Test Commands

Once connected, try these simple commands:

```bash
# Check device status
python test_video.py 192.168.1.48
# Select 'i' then type: status

# List available videos
# Type: list

# Test LED control
# Type: led 255 0 0  (red)
# Type: led 0 255 0  (green)
# Type: led 0 0 255  (blue)
```

## Expected Serial Output

When working correctly, the serial monitor should show:
```
Starting Prop Control System...
SD card initialized successfully!
WiFi connected!
IP address: 192.168.1.48
UDP server started on port 8888
mDNS responder started
Setup complete!
```

## Getting Help

If you're still having issues:

1. **Check serial output** - Connect USB and monitor at 115200 baud
2. **Verify firmware** - Ensure latest firmware is flashed
3. **Test with different network** - Try mobile hotspot
4. **Check ESP32 board** - Ensure it's the correct ESP32-2432S032C model

## Advanced Diagnostics

### Wireshark Packet Capture
```bash
# Capture UDP traffic on port 8888
# Filter: udp.port == 8888
# Look for outgoing packets to 192.168.1.48
```

### ESP32 Debug Output
```cpp
// Enable in firmware for more verbose output
#define DEBUG_UDP 1
```

This should help you get connected and testing video functionality! 🔧
