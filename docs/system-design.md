# Tricorder Control System - Design Document

## Executive Summary

The Tricorder Control System is a centralized management platform for ESP32-based film set props. The system enables real-time control of video playback, LED lighting, and device configuration across up to 20 devices with sub-50ms latency for professional film production requirements.

## System Architecture

### Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Film Set WiFi  │    │  Central Server │    │ Lighting Console│
│                 │    │   (Python)      │    │     (SACN)      │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       │
    ┌────▼────┐              ┌───▼───┐               ┌───▼───┐
    │ Router  │              │ Web   │               │ SACN  │
    │         │              │ UI    │               │ Input │
    └────┬────┘              └───────┘               └───────┘
         │
    ┌────▼────────────────────────────────────────────────────┐
    │                WiFi Network                             │
    └─┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬─┘
      │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
    ┌─▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐┌▼┐
    │T1││T2││T3││T4││T5││T6││T7││T8││T9││10││11││12││13││14││15││16││17││18││19││20│
    └──┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘
    ESP32 Tricorder Props (T1-T20)
```

### Component Breakdown

#### 1. ESP32 Tricorder Props (Hardware)
**Specifications:**
- **MCU**: ESP32-WROOM-32 (ESP32-2432S032C-I development board)
- **Display**: 3.2" IPS TFT LCD (240x320) with capacitive touch
- **Display Driver**: ST7789
- **Storage**: MicroSD card (up to 32GB, FAT32)
- **LEDs**: 12x WS2812B NeoPixels (addressable RGB)
- **Power**: 5V USB-C input
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Memory**: 4MB Flash, 520KB SRAM
- **Board Size**: 93.7 x 55.0mm

**Firmware Features:**
- Auto-connect to "Rigging Electrics" WiFi network
- mDNS service discovery for central server
- Video file playback from SD card (MP4/AVI)
- NeoPixel LED control with smooth transitions
- UDP command processing (< 10ms response)
- SACN E1.31 lighting protocol receiver
- OTA firmware update capability
- Device health monitoring and reporting

#### 2. Central Server (Python Backend)

**Technology Stack:**
- **Framework**: FastAPI (for high performance async operations)
- **Database**: SQLite (for device configs) + Redis (for real-time state)
- **Networking**: asyncio UDP/TCP servers
- **File Management**: Direct filesystem operations for SD card simulation
- **SACN**: Custom E1.31 protocol implementation

**Core Services:**

**a) Device Management Service**
- Auto-discovery via mDNS broadcasting
- Device registration and pairing
- Health monitoring and status tracking
- Configuration persistence
- Firmware update management

**b) Command Processing Service**
- Real-time command dispatch (UDP multicast)
- Command queuing and batching
- Acknowledgment and retry logic
- Performance metrics collection

**c) File Management Service**
- Virtual SD card file system
- File upload/download to/from devices
- Video file transcoding and optimization
- Storage quota management

**d) SACN Integration Service**
- E1.31 protocol listener
- DMX universe mapping to devices
- LED color/intensity calculations
- Priority handling (manual vs SACN control)

**e) Web API Service**
- RESTful API for all operations
- WebSocket for real-time updates
- Authentication and authorization
- Rate limiting and security

#### 3. Web Interface (Frontend)

**Technology Stack:**
- **Framework**: React with TypeScript
- **State Management**: Redux Toolkit + RTK Query
- **Real-time**: Socket.IO client
- **UI Components**: Material-UI or Chakra UI
- **Build Tool**: Vite

**User Interface Modules:**

**a) Device Dashboard**
- Real-time device status grid (20 devices)
- Connection status indicators
- Performance metrics (latency, battery, temperature)
- Quick action buttons (play/pause/next)

**b) File Manager**
- Device-specific file browsers
- Drag-and-drop file uploads
- Video preview and metadata
- Batch operations across devices

**c) LED Configuration**
- Per-device LED strip configuration
- Color picker and animation editor
- SACN universe assignment
- Preview mode with real-time updates

**d) Control Panel**
- Global device commands
- Scene creation and playback
- Cue list management
- Emergency stop functionality

## Communication Protocols

### 1. Device Discovery (mDNS)
```
Service: _tricorder._tcp.local
Port: 8080
TXT Records:
  - device_id=TRICORDER_001
  - firmware_version=1.0.0
  - capabilities=video,leds,sacn
```

### 2. Command Protocol (UDP)
```json
{
  "command_id": "unique_uuid",
  "timestamp": 1234567890,
  "target": "TRICORDER_001", // or "ALL" for broadcast
  "action": "video_play",
  "parameters": {
    "filename": "scene1.mp4",
    "loop": true,
    "volume": 0.8
  }
}
```

**Response:**
```json
{
  "command_id": "unique_uuid",
  "timestamp": 1234567891,
  "device_id": "TRICORDER_001",
  "status": "success", // or "error"
  "message": "Video playing: scene1.mp4",
  "execution_time_ms": 15
}
```

### 3. SACN Integration (E1.31)
```
Universe Mapping:
- Universe 1: Devices 1-10 (12 channels each = 120 channels)
- Universe 2: Devices 11-20 (12 channels each = 120 channels)

Channel Layout per device:
- Channels 1-3: LED 1 (R,G,B)
- Channels 4-6: LED 2 (R,G,B)
- ...
- Channels 34-36: LED 12 (R,G,B)
```

## Data Models

### Device Configuration
```python
class DeviceConfig:
    device_id: str
    ip_address: str
    mac_address: str
    firmware_version: str
    last_seen: datetime
    status: DeviceStatus
    led_config: LEDConfiguration
    video_files: List[VideoFile]
    battery_level: Optional[float]
    temperature: Optional[float]
```

### LED Configuration
```python
class LEDConfiguration:
    num_leds: int = 12
    brightness: float = 1.0
    color_profile: str = "default"
    sacn_universe: int
    sacn_start_channel: int
    animations: List[LEDAnimation]
```

### Command Queue
```python
class Command:
    command_id: str
    timestamp: datetime
    target_devices: List[str]
    action: CommandAction
    parameters: Dict[str, Any]
    priority: int
    retry_count: int
    max_retries: int
    status: CommandStatus
```

## Performance Requirements

### Latency Targets
- **Command Response**: < 50ms end-to-end
- **Video Start**: < 200ms from command to first frame
- **LED Update**: < 10ms for SACN commands
- **Web UI Update**: < 100ms for status changes

### Throughput Targets
- **Concurrent Commands**: 100+ commands/second
- **Device Connections**: 20+ simultaneous connections
- **File Transfer**: 10MB/s sustained per device
- **SACN Processing**: 44 universes @ 44Hz refresh rate

### Reliability Targets
- **Uptime**: 99.9% (film set operational requirements)
- **Command Success Rate**: 99.95%
- **Connection Recovery**: < 5 seconds after network interruption
- **Data Loss**: < 0.01% for file transfers

## Security Considerations

### Network Security
- WPA2/WPA3 WiFi encryption
- Device MAC address filtering
- Network segmentation (isolated VLAN)
- VPN access for remote management

### Application Security
- API authentication (JWT tokens)
- Rate limiting on all endpoints
- Input validation and sanitization
- Secure file upload handling
- HTTPS/WSS for all web traffic

### Device Security
- Firmware signing and verification
- Secure boot process
- Device certificate authentication
- OTA update encryption

## Deployment Architecture

### Production Environment
```
┌─────────────────────────────────────────────────────────────┐
│                    Film Set Network                         │
│  ┌─────────────────┐  ┌─────────────────┐                  │
│  │ Central Server  │  │ WiFi Access     │                  │
│  │ (Raspberry Pi 4)│  │ Point           │                  │
│  │ - Python App    │  │ (UniFi/Cisco)   │                  │
│  │ - Redis Cache   │  │                 │                  │
│  │ - SQLite DB     │  │                 │                  │
│  └─────────────────┘  └─────────────────┘                  │
│           │                     │                          │
│           └─────────────────────┼─────────────────────────┐│
│                                 │                         ││
│  ┌─────────────────┐            │                         ││
│  │ Lighting Console│            │                         ││
│  │ (SACN Output)   │            │                         ││
│  └─────────────────┘            │                         ││
│                                 │                         ││
│                        ┌────────▼─────────┐               ││
│                        │                  │               ││
│                     20x ESP32 Tricorder Props             ││
│                                                           ││
└───────────────────────────────────────────────────────────┘│
                                                             │
┌─────────────────────────────────────────────────────────────┘
│ Optional: Remote Access via VPN
│ ┌─────────────────┐
│ │ Director's      │
│ │ Tablet/Laptop   │
│ │ (Web Interface) │
│ └─────────────────┘
└─────────────────────
```

### Hardware Recommendations

**Central Server:**
- Raspberry Pi 4 (8GB RAM) or equivalent mini PC
- 128GB+ SSD storage
- Gigabit Ethernet
- UPS backup power
- Heat management (fan/heatsink)

**Network Infrastructure:**
- Enterprise WiFi 6 access point
- Managed switch with QoS
- Dedicated VLAN for props
- UPS for network equipment

**Tricorder Props:**
- ESP32-S3 (preferred) or ESP32-WROOM-32
- High-speed microSD cards (Class 10/UHS-I)
- Quality USB-C power delivery
- Robust enclosures for film set use

## Risk Assessment

### High Risk Items
1. **WiFi Interference**: Film sets often have RF-heavy environments
   - *Mitigation*: Site survey, channel planning, multiple APs
2. **Power Management**: Battery life during long shoots
   - *Mitigation*: External battery packs, power monitoring
3. **SD Card Reliability**: Mechanical failure of storage
   - *Mitigation*: Redundant storage, regular health checks

### Medium Risk Items
1. **Firmware Bugs**: Complex embedded software
   - *Mitigation*: Extensive testing, OTA update capability
2. **Network Congestion**: Multiple video streams
   - *Mitigation*: QoS configuration, bandwidth monitoring
3. **Thermal Issues**: ESP32s in enclosed props
   - *Mitigation*: Temperature monitoring, thermal design

### Low Risk Items
1. **Central Server Failure**: Single point of failure
   - *Mitigation*: Hot standby server, automatic failover
2. **LED Driver Failure**: Hardware component failure
   - *Mitigation*: Modular design, field-replaceable parts

This design document provides the foundation for implementing a robust, scalable, and reliable tricorder control system suitable for professional film production environments.
