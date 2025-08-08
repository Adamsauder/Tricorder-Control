# Tricorder GUI Server v0.1 - "Mission Control"

The GUI server provides a native desktop application for controlling Tricorder devices with an intuitive visual interface. Perfect for users who prefer desktop applications over web interfaces or command-line tools.

## Features

- **🖥️ Native Desktop Interface**: Clean, professional Windows application
- **🎨 Dark Theme**: Easy on the eyes with modern dark UI
- **🔴 Visual LED Controls**: Click-to-set color buttons for instant LED control
- **📊 Real-time Monitoring**: Live device status updates and statistics
- **📋 Device Management**: Tabbed interface for organized device control
- **📝 Comprehensive Logging**: Built-in log viewer with save functionality
- **⚡ Quick Commands**: One-click buttons for common operations
- **🎛️ Custom Commands**: Advanced JSON parameter support
- **📈 Statistics Dashboard**: Server performance and usage metrics
- **🔍 Auto-Discovery**: Automatic device detection with detailed views

## Quick Start

### Windows (Batch File)
```bash
# Double-click or run from command prompt
start_gui_server.bat
```

### Windows (PowerShell)
```powershell
# Run from PowerShell
.\start_gui_server.ps1
```

### Direct Python Execution
```bash
# From the project root directory
.venv\Scripts\python.exe server\gui_server.py
```

## Interface Overview

### Main Window Layout

The GUI is organized into several key areas:

#### 1. **Server Status Panel** (Top)
- **Status Indicator**: Shows current server state (Running/Stopped)
- **Uptime Display**: Shows how long the server has been running
- **Device Counter**: Number of currently connected devices
- **Control Buttons**: Start/Stop server, Discover devices, Refresh

#### 2. **Tabbed Interface** (Main Area)

##### **Devices Tab**
- **Device List**: Table showing all discovered devices with:
  - Device ID
  - IP Address  
  - Firmware Version
  - Connection Status
  - Last Seen timestamp
- **Device Details**: JSON view of selected device information
- **Selection**: Click any device to view details and select for commands

##### **Commands Tab**
- **Device Selection**: Dropdown to choose target device or "All Devices"
- **Quick Commands**: One-click buttons for:
  - Ping - Test device connectivity
  - Status - Get detailed device status
  - Boot Screen - Show startup screen
  - Stop Video - Stop any playing video
- **LED Controls**: 
  - Color buttons for instant LED color changes
  - Brightness slider with visual feedback
  - Preset colors: Red, Green, Blue, Yellow, Cyan, Magenta, White, Off
- **Custom Commands**:
  - Action field for any command
  - JSON parameters editor
  - Send button for advanced control

##### **Server Log Tab**
- **Live Log Display**: Real-time server activity log
- **Color Coding**: Different colors for INFO, ERROR, WARN, DEBUG messages
- **Log Management**: Clear and Save log functionality
- **Auto-scroll**: Automatically scrolls to show latest messages

##### **Statistics Tab**
- **Server Metrics**: Comprehensive server statistics including:
  - Commands sent/received
  - Devices discovered
  - Uptime
  - Active connections
  - Performance data
- **Refresh Button**: Update statistics on demand

## Using the GUI

### Starting the Server

1. **Launch the Application**: Use one of the start scripts or run directly
2. **Click "Start Server"**: The server status will change to "Starting..." then "Running"
3. **Wait for Discovery**: The server will automatically scan for devices
4. **View Devices**: Check the Devices tab to see discovered tricorders

### Controlling Devices

#### Quick LED Control
1. **Select Device**: Choose a device from the dropdown or select "All Devices"
2. **Choose Color**: Click any color button (Red, Green, Blue, etc.)
3. **Adjust Brightness**: Use the brightness slider and click "Set"
4. **Instant Feedback**: LEDs change immediately

#### Sending Commands
1. **Select Target**: Choose device from dropdown
2. **Quick Commands**: Click Ping, Status, Boot Screen, or Stop Video
3. **Custom Commands**: 
   - Enter action name (e.g., "display_image")
   - Add JSON parameters: `{"filename": "test.jpg"}`
   - Click "Send Custom Command"

#### Device Management
1. **View Device List**: All devices appear in the Devices tab table
2. **Select Device**: Click any row to see detailed device information
3. **Monitor Status**: Last seen times and status updates automatically
4. **Device Details**: Full JSON device information in the details panel

### Monitoring and Logging

#### Real-time Monitoring
- **Auto-refresh**: Enable for automatic device list updates
- **Status Indicators**: Color-coded device status
- **Live Counters**: Device count and uptime in top panel

#### Log Management
- **Live Logging**: All server activity appears in real-time
- **Color Coding**: Easy identification of different message types
- **Log Saving**: Save logs to file for later analysis
- **Clear Function**: Reset log display when needed

## Command Examples

### LED Control Commands
```json
// Set LED color to red
Action: set_led_color
Parameters: {"r": 255, "g": 0, "b": 0}

// Set brightness to 50%
Action: set_led_brightness  
Parameters: {"brightness": 128}

// Turn off LEDs
Action: set_led_color
Parameters: {"r": 0, "g": 0, "b": 0}
```

### Image Display Commands
```json
// Display specific image
Action: display_image
Parameters: {"filename": "test.jpg"}

// Show boot screen
Action: display_boot_screen
Parameters: {}
```

### Video Control Commands
```json
// Play video with looping
Action: play_video
Parameters: {"filename": "animated_test", "loop": true}

// Stop video playback
Action: stop_video
Parameters: {}

// List available videos
Action: list_videos
Parameters: {}
```

## Advanced Features

### Broadcast Commands
- Select "All Devices" from the device dropdown
- Any command sent will go to all connected devices
- Useful for synchronized LED shows or mass updates

### Custom JSON Parameters
The GUI supports complex JSON parameters for advanced commands:

```json
{
  "filename": "custom_image.jpg",
  "loop": true,
  "brightness": 200,
  "duration": 5000
}
```

### Auto-Discovery
- Runs automatically every 30 seconds when server is running
- Can be triggered manually with "Discover Devices" button
- Scans entire local network subnet for tricorder devices

## Keyboard Shortcuts

- **Ctrl+S**: Start/Stop server
- **Ctrl+D**: Discover devices
- **Ctrl+R**: Refresh device list
- **F5**: Refresh current tab
- **Ctrl+L**: Clear log
- **Alt+F4**: Exit application

## Configuration

### Command-Line Options
```bash
python server/gui_server.py [options]

Options:
  --port PORT          UDP port to listen on (default: 8888)
  --log-file PATH      Log file location (default: tricorder_gui_server.log)
  --help               Show help message
```

### Settings
- **Auto Refresh**: Enable/disable automatic device list updates
- **Log Level**: Control verbosity of logging output
- **Discovery Interval**: How often to scan for new devices

## Troubleshooting

### GUI Won't Start
1. **Check Python**: Ensure `.venv\Scripts\python.exe` exists
2. **Check tkinter**: Run `python -m tkinter` to test GUI support
3. **Run Test**: Execute `python server\test_gui.py` for diagnostics

### No Devices Found
1. **Check Network**: Ensure devices and computer are on same network
2. **Check Firewall**: Port 8888 must be open for UDP traffic
3. **Manual Discovery**: Click "Discover Devices" button
4. **Check Server Log**: Look for discovery scan messages

### Commands Not Working
1. **Select Device**: Ensure a device is selected in dropdown
2. **Check Connection**: Verify device status is "online"
3. **View Log**: Check server log for error messages
4. **Test Ping**: Use ping command to test basic connectivity

### GUI Freezing
1. **Server Thread**: The server runs in background thread
2. **Heavy Operations**: Large device scans may cause brief delays
3. **Log Size**: Clear log if it becomes very large
4. **Restart**: Stop and restart server if issues persist

## Performance Tips

### Optimal Usage
- **Close Unused Tabs**: Focus on active tab for better performance
- **Clear Logs Regularly**: Large logs can slow down display
- **Limit Auto-refresh**: Disable if monitoring many devices
- **Use Broadcast**: More efficient than individual commands

### Resource Usage
- **Memory**: Approximately 50-100MB depending on log size
- **CPU**: Minimal when idle, brief spikes during discovery
- **Network**: UDP packets only, very low bandwidth usage

## Comparison with Other Servers

| Feature | **GUI Server** | **Web Server** | **Standalone** |
|---------|----------------|----------------|----------------|
| **Interface** | Native desktop | Web browser | Command-line |
| **Visual Controls** | Color buttons | Web buttons | Text commands |
| **Real-time Updates** | Automatic | WebSocket | Manual refresh |
| **Device Details** | Tabbed view | Cards/modal | Text output |
| **Logging** | Built-in viewer | Browser console | File only |
| **Accessibility** | Desktop app | Any browser | Terminal/SSH |
| **Resource Usage** | Medium | Higher | Lower |
| **Ease of Use** | High | High | Medium |
| **Automation** | Manual clicks | REST API | Excellent |

## Integration

### With Other Tools
The GUI server can work alongside other components:

```python
# Access server instance programmatically
from gui_server import TricorderGUIServer

app = TricorderGUIServer()
server = app.server

# Send commands programmatically
server.send_command("TRICORDER_001", "ping")
```

### Scripting
While the GUI is primarily interactive, you can still script common operations:

```python
# Example automation script
import time
from threading import Thread

def automated_light_show():
    colors = [(255,0,0), (0,255,0), (0,0,255)]
    for color in colors:
        app.set_led_color(color)
        time.sleep(2)

# Run automation in background
Thread(target=automated_light_show, daemon=True).start()
```

## Version History

### Version 0.1 - "Mission Control"
- Initial GUI server implementation
- Native Windows desktop application
- Tabbed interface with device management
- Visual LED color controls
- Real-time logging and statistics
- Auto-discovery with device details
- Integration with standalone server backend
