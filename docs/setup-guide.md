# Setup and Installation Guide

## Prerequisites

### Required Software
- **Python 3.8+** with pip
- **Node.js 18+** with npm
- **Git** for version control
- **VS Code** (recommended IDE)

### Hardware Requirements
- **Development Computer**: 8GB+ RAM, SSD storage
- **Network**: WiFi router with 2.4GHz support
- **ESP32 Development**: 3-5 ESP32 boards for testing

### Optional Tools
- **Redis** (for enhanced performance)
- **PlatformIO** (for ESP32 development)
- **Postman** (for API testing)

## Installation Steps

### 1. Python Environment Setup

**Windows:**
```powershell
# Install Python from python.org or Microsoft Store
python --version  # Should be 3.8+

# Create virtual environment
python -m venv venv
venv\Scripts\activate

# Install dependencies
cd server
pip install -r requirements.txt
```

**macOS/Linux:**
```bash
# Install Python (if not already installed)
python3 --version  # Should be 3.8+

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
cd server
pip install -r requirements.txt
```

### 2. Node.js Setup

**Install Node.js:**
- Download from [nodejs.org](https://nodejs.org)
- Choose LTS version (18+)

**Install Web Dependencies:**
```bash
cd web
npm install
```

### 3. Database Setup

**SQLite (Default):**
- No setup required, database created automatically

**Redis (Optional):**
```bash
# Windows (using Chocolatey)
choco install redis-64

# macOS (using Homebrew)
brew install redis

# Ubuntu/Debian
sudo apt install redis-server

# Start Redis
redis-server
```

### 4. ESP32 Development Setup

**Arduino IDE Setup:**
1. Install Arduino IDE 2.0+
2. Add ESP32 board package:
   - File → Preferences
   - Additional Board Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

**PlatformIO Setup (Alternative):**
1. Install VS Code
2. Install PlatformIO extension
3. Open firmware folder as PlatformIO project

**Library Installation:**
```cpp
// Required libraries (install via Library Manager)
FastLED          // LED control
TFT_eSPI         // Display driver
ArduinoJson      // JSON parsing
ESPAsyncWebServer // Web server
```

## Configuration

### 1. Network Configuration

**WiFi Settings** (in `firmware/tricorder_firmware.ino`):
```cpp
const char* WIFI_SSID = "Rigging Electrics";
const char* WIFI_PASSWORD = "academy123";
```

**Server Settings** (in `server/app.py`):
```python
CONFIG = {
    "udp_port": 8888,
    "web_port": 8080,
    "sacn_port": 5568,
    # ... other settings
}
```

### 2. Hardware Pin Configuration

**ESP32-2432S032C-I Board Configuration:**
```cpp
// The ESP32-2432S032C-I has built-in display and SD card
// External NeoPixel strip connection only
#define LED_PIN 2       // NeoPixel data pin (external connection)
#define NUM_LEDS 12     // Number of LEDs in strip
#define SD_CS 5         // SD card chip select (built-in)
```

**External Hardware Connections:**
- NeoPixel strip data wire → GPIO2
- NeoPixel strip power → External 5V supply
- NeoPixel strip ground → ESP32 ground

### 3. Display Configuration

**TFT_eSPI Configuration for ESP32-2432S032C-I:**

The ESP32-2432S032C-I board has a built-in 3.2" display. Create or modify the TFT_eSPI library configuration:

1. **Arduino Libraries Folder**: Navigate to `Arduino/libraries/TFT_eSPI/`
2. **Edit User_Setup_Select.h**: Comment out `#include <User_Setup.h>` and uncomment a suitable setup
3. **Or create User_Setup.h**:

```cpp
// ESP32-2432S032C-I Configuration
#define ST7789_DRIVER      // Display driver for 3.2" IPS LCD
#define TFT_WIDTH  240     // Display width
#define TFT_HEIGHT 320     // Display height

// Pin definitions (hardware specific for this board)
#define TFT_MOSI 13
#define TFT_SCLK 14  
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1        // Reset handled by board
#define TFT_BL   21        // Backlight control

// Font and feature selection
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// SPI frequency
#define SPI_FREQUENCY  40000000
```

## Running the System

### 1. Start the Central Server

**Development Mode:**
```bash
cd server
python app.py
```

**Production Mode:**
```bash
uvicorn app:app --host 0.0.0.0 --port 8080
```

### 2. Start the Web Interface

**Development Mode:**
```bash
cd web
npm run dev
```

**Production Build:**
```bash
cd web
npm run build
# Serve static files via Python server
```

### 3. Flash ESP32 Firmware

**Using Arduino IDE:**
1. Open `firmware/tricorder_firmware.ino`
2. Select board: "ESP32 Dev Module"
3. Select correct COM port
4. Click Upload

**Using PlatformIO:**
```bash
cd firmware
pio run --target upload
```

### 4. Access the System

- **Web Interface**: http://localhost:3000 (dev) or http://localhost:8080 (production)
- **API Documentation**: http://localhost:8080/docs
- **Device Discovery**: Devices should auto-appear in web interface

## Troubleshooting

### Common Issues

#### Python Server Won't Start
```bash
# Check Python version
python --version

# Reinstall dependencies
pip install --upgrade -r requirements.txt

# Check port availability
netstat -an | findstr 8080  # Windows
lsof -i :8080              # macOS/Linux
```

#### Web Interface Issues
```bash
# Clear npm cache
npm cache clean --force

# Reinstall dependencies
rm -rf node_modules package-lock.json
npm install

# Check Node version
node --version  # Should be 18+
```

#### ESP32 Upload Issues
```cpp
// Check board settings
Board: "ESP32 Dev Module"
Upload Speed: "921600"
CPU Frequency: "240MHz"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB"
Partition Scheme: "Default 4MB"
```

#### WiFi Connection Problems
```cpp
// Enable debug output
#define DEBUG_ESP_WIFI
#define DEBUG_ESP_HTTP_CLIENT

// Check signal strength
int rssi = WiFi.RSSI();
Serial.println("Signal: " + String(rssi) + " dBm");

// Try different channels/frequencies
```

#### Device Discovery Issues
```bash
# Check network connectivity
ping 192.168.1.100  # Replace with device IP

# Verify UDP ports
netstat -an | findstr 8888

# Check firewall settings
# Windows: Windows Defender Firewall
# macOS: System Preferences → Security & Privacy → Firewall
# Linux: ufw status
```

### Debug Logs

**Enable Python Debug Logging:**
```python
import logging
logging.basicConfig(level=logging.DEBUG)
```

**Enable ESP32 Serial Debug:**
```cpp
Serial.begin(115200);
Serial.setDebugOutput(true);
```

**Web Browser Debug:**
- Open Developer Tools (F12)
- Check Console and Network tabs
- Monitor WebSocket connections

## Performance Optimization

### 1. Network Optimization
- Use 5GHz WiFi for server if possible
- Keep 2.4GHz for ESP32 devices
- Configure QoS on router for low latency
- Use wired connection for server if available

### 2. ESP32 Optimization
```cpp
// Increase CPU frequency
setCpuFrequencyMhz(240);

// Optimize WiFi settings
WiFi.setSleep(false);
WiFi.setTxPower(WIFI_POWER_19_5dBm);

// Use faster SD card (Class 10/UHS-I)
SD.begin(SD_CS, SPI, 80000000);  // 80MHz SPI
```

### 3. Server Optimization
```python
# Use Redis for caching
redis_client = redis.Redis(host='localhost')

# Enable compression
from fastapi.middleware.gzip import GZipMiddleware
app.add_middleware(GZipMiddleware, minimum_size=1000)

# Use async file operations
import aiofiles
```

## Production Deployment

### 1. Security Hardening
- Change default WiFi credentials
- Enable HTTPS/WSS
- Implement authentication
- Configure firewall rules
- Regular security updates

### 2. Monitoring Setup
```python
# Add health check endpoint
@app.get("/health")
async def health_check():
    return {"status": "healthy", "timestamp": time.time()}

# Add performance metrics
from prometheus_client import Counter, Histogram
command_counter = Counter('commands_total', 'Total commands processed')
```

### 3. Backup Strategy
- Database backups (SQLite file)
- Configuration backups
- Firmware version control
- File storage redundancy

This guide should get you up and running with the Prop Control System. For additional help, consult the API documentation and system design documents.
