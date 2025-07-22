# Tricorder Control System

A comprehensive control system for ESP32-based film set props with embedded screens, NeoPixel LEDs, and centralized WiFi management.

## Project Overview

This system controls up to 20 ESP32-based "tricorder" props, each featuring:
- Embedded screen for video playback from SD card
- 12 controllable NeoPixel LEDs
- WiFi connectivity to central server
- Low-latency command response for film set cuing

## Architecture

### Components

1. **ESP32 Firmware** (`firmware/`)
   - Video playback from SD card
   - NeoPixel LED control
   - WiFi connectivity and auto-pairing
   - Command processing (play/pause/next/color control)
   - SACN lighting protocol integration

2. **Central Server** (`server/`)
   - Python Flask/FastAPI web server
   - Device discovery and auto-pairing
   - Web interface for device management
   - REST API for prop control
   - SD card file management
   - SACN lighting console integration

3. **Web Interface** (`web/`)
   - Device status dashboard
   - LED configuration interface
   - File management for SD cards
   - Real-time control panel

4. **Documentation** (`docs/`)
   - Technical specifications
   - API documentation
   - Setup and deployment guides

## Quick Start

### Prerequisites

- Python 3.8+ with pip
- Node.js 18+ (for web development)
- PlatformIO or Arduino IDE (for ESP32 firmware)
- WiFi network for device communication

### Installation

1. **Install Python dependencies:**
   ```bash
   cd server
   pip install -r requirements.txt
   ```

2. **Install web dependencies:**
   ```bash
   cd web
   npm install
   ```

3. **Flash ESP32 firmware:**
   ```bash
   cd firmware
   # Use PlatformIO or Arduino IDE to flash tricorder_firmware.ino
   ```

### Running the System

1. **Start the central server:**
   ```bash
   cd server
   python app.py
   ```

2. **Access the web interface:**
   Open `http://localhost:8080` in your browser

3. **Power on tricorder props** - they will auto-pair with the server

## Features

- **Automatic Device Discovery**: Props automatically connect to the central server
- **Real-time Control**: Low-latency commands for film set synchronization
- **Web-based Management**: Configure LEDs, manage files, monitor device status
- **SACN Integration**: Professional lighting console control
- **API/Webhook Support**: Programmable control for complex sequences

## Project Status

✅ **COMPLETE** - Project structure and documentation
✅ **COMPLETE** - Python environment setup and dependencies  
✅ **COMPLETE** - Node.js web application foundation
✅ **COMPLETE** - ESP32 firmware framework
✅ **COMPLETE** - VS Code development environment

� **READY** - All development tools configured and tested

## Quick Start

### Prerequisites ✅
- ✅ Python 3.11.9 (installed and working)
- ✅ Node.js 22.17.1 (installed and working)  
- ✅ All Python dependencies (FastAPI, uvicorn, etc.)
- ✅ All web dependencies (React, Material-UI, etc.)

### Current Status
Your development environment is **FULLY READY**! 🎉

### Next Steps
1. **Flash ESP32 firmware** to your hardware
2. **Configure WiFi network** for device communication
3. **Run the system** using VS Code tasks

### Running the System

1. **Start Python Server:**
   - Use VS Code Command Palette (Ctrl+Shift+P)
   - Run Task → "Start Python Server"
   - Server will start on http://localhost:8080

2. **Start Web Interface:**
   - Use VS Code Command Palette (Ctrl+Shift+P)  
   - Run Task → "Start Web Development Server"
   - Web UI will be available at http://localhost:3000

3. **View Documentation:**
   - See `docs/setup-guide.md` for detailed setup
   - See `docs/api-documentation.md` for API reference
   - See `docs/implementation-plan.md` for development roadmap

## License

MIT License - See LICENSE file for details
