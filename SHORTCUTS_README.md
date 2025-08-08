# 🚀 Prop Control System - Quick Start Shortcuts

This directory contains convenient shortcuts for common Prop Control System tasks.

## 📋 Available Shortcuts

### 🔧 **Firmware Flashing**
| File | Purpose | Hardware |
|------|---------|----------|
| `flash_tricorder.bat` | Flash ESP32 Tricorder | ESP32-2432S032C-I |
| `flash_tricorder.ps1` | Flash ESP32 Tricorder (PowerShell) | ESP32-2432S032C-I |
| `flash_polyinoculator.bat` | Flash ESP32-C3 Polyinoculator | Seeed XIAO ESP32-C3 |
| `flash_polyinoculator.ps1` | Flash ESP32-C3 Polyinoculator (PowerShell) | Seeed XIAO ESP32-C3 |

### 🖥️ **Server Control**
| File | Purpose | Description |
|------|---------|-------------|
| `start_server.bat` | Start Enhanced Server | Web dashboard + sACN support |
| `start_server.ps1` | Start Enhanced Server (PowerShell) | Web dashboard + sACN support |
| `start_gui_server.bat` | Start GUI Server | Desktop application |
| `start_standalone_server.bat` | Start Standalone Server | Command-line interface |

### 🎯 **Master Control**
| File | Purpose | Description |
|------|---------|-------------|
| `prop_control.bat` | **ALL-IN-ONE MENU** | Interactive menu for all actions |

## 🎮 Quick Start Guide

### 1️⃣ **Flash Your Hardware**
```bash
# For Tricorder (ESP32 with display)
double-click: flash_tricorder.bat

# For Polyinoculator (ESP32-C3 with LEDs)  
double-click: flash_polyinoculator.bat
```

### 2️⃣ **Start the Server**
```bash
# Start web server
double-click: start_server.bat

# Access at: http://localhost:5000
```

### 3️⃣ **Use the Master Menu**
```bash
# One-stop-shop for everything
double-click: prop_control.bat
```

## ⚡ Prerequisites

### Required Software
- ✅ **Python 3.8+** with virtual environment at `.venv/`
- ✅ **PlatformIO** (via VS Code extension)
- ✅ **VS Code** (for development shortcuts)

### Hardware Connections
- **Tricorder**: Connect ESP32-2432S032C-I via USB-C
- **Polyinoculator**: Connect Seeed XIAO ESP32-C3 via USB-C

## 🔧 Troubleshooting

### Firmware Upload Issues
1. **Hold BOOT button** during upload for ESP32 boards
2. **Check USB drivers** (CP2102 for tricorder, CH340 for polyinoculator)
3. **Try different USB cable** 
4. **Verify device connection** in Device Manager

### Server Start Issues
1. **Check virtual environment**: `.venv/Scripts/python.exe` should exist
2. **Install dependencies**: 
   ```bash
   .venv\Scripts\pip install -r server\requirements.txt
   ```
3. **Check Python version**: Python 3.8+ required

### VS Code Integration
1. **Install PlatformIO extension** in VS Code
2. **Open individual project folders** or use workspace file
3. **Use PlatformIO Upload button** or Ctrl+Shift+P → "PlatformIO: Upload"

## 📂 File Structure
```
Tricorder-Control/
├── prop_control.bat              # 🎯 Master menu
├── flash_tricorder.bat          # Flash tricorder
├── flash_polyinoculator.bat     # Flash polyinoculator  
├── start_server.bat             # Start web server
├── firmware/
│   ├── tricorder/               # ESP32 project
│   ├── polyinoculator/          # ESP32-C3 project
│   └── tricorder-workspace.code-workspace
└── server/
    └── enhanced_server.py       # Main server
```

## 🎬 Production Ready!

These shortcuts make the Prop Control System ready for:
- **Film set deployment** - Quick device flashing and server start
- **Development workflow** - Easy project switching and testing
- **Team collaboration** - Standardized build and deploy process

Just double-click and go! 🚀
