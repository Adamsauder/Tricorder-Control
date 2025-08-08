# Tricorder Standalone Server v0.1

The standalone server provides a command-line interface for controlling Tricorder devices without requiring the web GUI. This is perfect for automated scripts, headless operation, or when you prefer terminal-based control.

## Features

- **Interactive Command-Line Interface**: Full control through terminal commands
- **Automatic Device Discovery**: Continuously scans for new devices on the network
- **Real-time Device Monitoring**: Track device status, video playback, and hardware state
- **Command History & Statistics**: View recent commands and server performance metrics
- **Background Operation**: Can run as a service or daemon process
- **Comprehensive Logging**: All activities logged to file with rotation
- **Broadcast Commands**: Send commands to all devices simultaneously

## Quick Start

### Windows (Batch File)
```bash
# Double-click or run from command prompt
start_standalone_server.bat
```

### Windows (PowerShell)
```powershell
# Run from PowerShell
.\start_standalone_server.ps1
```

### Direct Python Execution
```bash
# From the project root directory
.venv\Scripts\python.exe server\standalone_server.py
```

## Command-Line Options

```bash
python server/standalone_server.py [options]

Options:
  --non-interactive     Run without CLI (for service mode)
  --port PORT          UDP port to listen on (default: 8888)
  --log-file PATH      Log file location (default: tricorder_server.log)
  --help               Show help message
```

## Interactive Commands

Once the server is running, you can use these commands:

### Device Management
- `devices` or `list` - Show all discovered devices
- `discover` - Scan network for new devices  
- `status` - Show server status and uptime
- `stats` - Display server statistics
- `ping [device_id]` - Ping specific device or all devices

### Device Control
- `send <device_id> <action> [params]` - Send command to specific device
- `broadcast <action> [params]` - Send command to all devices

### System Commands
- `history` - Show recent command history
- `clear` - Clear screen
- `help` - Show available commands
- `quit`, `exit`, or `q` - Exit server

## Command Examples

### LED Control
```bash
# Set LED color on specific device
send TRICORDER_001 set_led_color r=255 g=0 b=0

# Set LED brightness on all devices
broadcast set_led_brightness brightness=100

# Turn LEDs off on all devices
broadcast set_led_color r=0 g=0 b=0
```

### Image Display
```bash
# Display specific image
send TRICORDER_001 display_image filename=test.jpg

# Show boot screen
send TRICORDER_001 display_boot_screen

# Display image on all devices
broadcast display_image filename=greenscreen.jpg
```

### Video Playback
```bash
# Play video (looping)
send TRICORDER_001 play_video filename=animated_test loop=true

# Stop video playback
send TRICORDER_001 stop_video

# List available videos
send TRICORDER_001 list_videos
```

### Device Information
```bash
# Get device status
send TRICORDER_001 status

# Ping device to check connectivity
ping TRICORDER_001

# Ping all devices
ping
```

## Running as a Service

### Windows Service (Requires pywin32)

1. **Install pywin32**:
   ```bash
   .venv\Scripts\pip install pywin32
   ```

2. **Install the service**:
   ```bash
   .venv\Scripts\python.exe server\tricorder_service.py install
   ```

3. **Start the service**:
   ```bash
   .venv\Scripts\python.exe server\tricorder_service.py start
   ```

4. **Stop the service**:
   ```bash
   .venv\Scripts\python.exe server\tricorder_service.py stop
   ```

5. **Remove the service**:
   ```bash
   .venv\Scripts\python.exe server\tricorder_service.py remove
   ```

### Linux Daemon

For Linux systems, create a systemd service file:

```ini
[Unit]
Description=Tricorder Control Server
After=network.target

[Service]
Type=simple
User=tricorder
WorkingDirectory=/path/to/Tricorder-Control
ExecStart=/path/to/Tricorder-Control/.venv/bin/python server/standalone_server.py --non-interactive
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

## Configuration

The server uses these default settings:

- **UDP Port**: 8888
- **Device Timeout**: 30 seconds
- **Discovery Interval**: 30 seconds
- **Command Timeout**: 5 seconds
- **Log File**: `tricorder_server.log`
- **Max Log Size**: 10MB (auto-rotation)

## Log Files

The server creates detailed logs in `tricorder_server.log`:

- Device discoveries and disconnections
- Commands sent and responses received
- Error messages and debugging information
- Server startup and shutdown events

Logs are automatically rotated when they exceed 10MB.

## Network Discovery

The server automatically discovers Tricorder devices by:

1. **Scanning local network** - Sends discovery packets to all IPs in the subnet
2. **Listening for responses** - Devices respond with their information
3. **Periodic re-scanning** - Runs discovery every 30 seconds
4. **Device timeout handling** - Removes devices that haven't responded recently

## Troubleshooting

### No Devices Found

1. **Check network connectivity**:
   ```bash
   ping 192.168.1.48  # Replace with your device IP
   ```

2. **Verify UDP port is not blocked**:
   - Check Windows Firewall
   - Ensure port 8888 is open

3. **Manual discovery**:
   ```bash
   discover
   ```

### Commands Not Working

1. **Check device status**:
   ```bash
   devices
   ping TRICORDER_001
   ```

2. **Verify command syntax**:
   ```bash
   help
   ```

3. **Check logs**:
   - Review `tricorder_server.log` for errors

### Service Issues

1. **Check service status** (Windows):
   ```bash
   sc query TricorderControlServer
   ```

2. **View service logs**:
   - Check `logs/tricorder_service.log`

## Integration with Other Systems

The standalone server can be easily integrated with other automation systems:

### Python Scripts
```python
import subprocess
import time

# Start server in background
server = subprocess.Popen([
    'python', 'server/standalone_server.py', 
    '--non-interactive'
])

# Send commands via separate process
subprocess.run([
    'python', '-c', '''
import socket, json
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
command = {"commandId": "test", "action": "ping"}
sock.sendto(json.dumps(command).encode(), ("192.168.1.48", 8888))
sock.close()
'''
])
```

### Batch Scripts
```batch
@echo off
echo Starting automated light show...

REM Start server
start /B python server\standalone_server.py --non-interactive

REM Wait for startup
timeout /t 5

REM Send commands (would need additional UDP client script)
echo Sending commands...

REM Stop server
taskkill /F /IM python.exe
```

## Performance

The standalone server is optimized for:

- **Low latency**: Commands typically respond in <50ms
- **Multiple devices**: Supports monitoring 20+ devices simultaneously
- **Reliability**: Automatic reconnection and error recovery
- **Resource efficiency**: Minimal CPU and memory usage

## Comparison with Web GUI

| Feature | Standalone Server | Web GUI |
|---------|-------------------|---------|
| Interface | Command-line | Web browser |
| Resource Usage | Lower | Higher |
| Automation | Excellent | Limited |
| Remote Access | Terminal only | Any browser |
| Service Mode | Yes | No |
| Scripting | Native | API calls |

The standalone server is ideal for:
- Automated control systems
- Headless operation
- Script integration
- Service/daemon deployment
- Terminal-based workflows

The web GUI is better for:
- Visual device management
- Remote browser access
- Interactive exploration
- Real-time monitoring dashboards

## Version History

### Version 0.1 - "Command Center"
- Initial standalone server implementation
- Interactive command-line interface
- Automatic device discovery
- Background service capability
- Comprehensive logging and statistics
