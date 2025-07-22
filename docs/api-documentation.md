# Tricorder Control System - API Documentation

## Base URL
```
http://localhost:8080/api
```

## Authentication
Currently no authentication required for development. Production deployment should implement JWT tokens.

## Device Management

### Get All Devices
```http
GET /api/devices
```

**Response:**
```json
[
  {
    "device_id": "TRICORDER_001",
    "ip_address": "192.168.1.100",
    "firmware_version": "1.0.0",
    "status": "online",
    "last_seen": "2024-01-15T10:30:00Z",
    "battery_voltage": 4.2,
    "temperature": 25.0,
    "current_video": "scene1.mp4",
    "video_playing": true
  }
]
```

### Get Device Details
```http
GET /api/devices/{device_id}
```

### Register Device
```http
POST /api/devices/{device_id}/register
```

**Request Body:**
```json
{
  "ip_address": "192.168.1.100",
  "firmware_version": "1.0.0",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "battery_voltage": 4.2,
  "temperature": 25.0
}
```

## Command System

### Send Command
```http
POST /api/commands/send
```

**Request Body:**
```json
{
  "target": "TRICORDER_001",
  "action": "video_play",
  "parameters": {
    "filename": "scene1.mp4",
    "loop": true
  }
}
```

**Response:**
```json
{
  "command_id": "uuid-string",
  "status": "sent",
  "target": "TRICORDER_001"
}
```

### Get Command Status
```http
GET /api/commands/{command_id}/status
```

## Video Control

### Play Video
```http
POST /api/devices/{device_id}/video/play?filename=scene1.mp4&loop=false
```

### Pause Video
```http
POST /api/devices/{device_id}/video/pause
```

### Stop Video
```http
POST /api/devices/{device_id}/video/stop
```

## LED Control

### Set LED Color
```http
POST /api/devices/{device_id}/leds/color
```

**Request Body:**
```json
{
  "r": 255,
  "g": 0,
  "b": 0
}
```

### Configure LEDs
```http
POST /api/devices/{device_id}/leds/config
```

**Request Body:**
```json
{
  "device_id": "TRICORDER_001",
  "brightness": 0.8,
  "color": {"r": 255, "g": 255, "b": 255},
  "animation": "pulse",
  "sacn_universe": 1,
  "sacn_start_channel": 1
}
```

## File Management

### Get Device Files
```http
GET /api/devices/{device_id}/files
```

### Upload File
```http
POST /api/devices/{device_id}/files/upload
Content-Type: multipart/form-data

file: [binary data]
```

## WebSocket Events

### Connection
```javascript
const socket = io('ws://localhost:8080/ws');
```

### Event Types

#### Device List
```json
{
  "type": "device_list",
  "devices": [...]
}
```

#### Device Update
```json
{
  "type": "device_update", 
  "device": {...}
}
```

#### Device Registered
```json
{
  "type": "device_registered",
  "device": {...}
}
```

#### Device Offline
```json
{
  "type": "device_offline",
  "device_id": "TRICORDER_001"
}
```

#### Command Response
```json
{
  "type": "command_response",
  "command_id": "uuid",
  "device_id": "TRICORDER_001", 
  "response": {
    "status": "success",
    "message": "Video playing: scene1.mp4",
    "execution_time_ms": 15,
    "timestamp": 1234567890
  }
}
```

## Error Responses

### Standard Error Format
```json
{
  "detail": "Error description",
  "status_code": 400
}
```

### Common HTTP Status Codes
- `200` - Success
- `201` - Created
- `400` - Bad Request
- `404` - Not Found
- `413` - Payload Too Large
- `500` - Internal Server Error

## Command Actions Reference

### Video Commands
- `video_play` - Start video playback
- `video_pause` - Pause current video
- `video_stop` - Stop and reset video
- `video_next` - Play next video in sequence

### LED Commands
- `led_color` - Set solid color (r, g, b)
- `led_brightness` - Set brightness (0.0-1.0)
- `led_animation` - Start animation (solid, pulse, rainbow)
- `led_off` - Turn off all LEDs

### System Commands  
- `ping` - Health check (responds with "pong")
- `restart` - Reboot device
- `get_status` - Request status update
- `list_files` - Get SD card file list

## Rate Limiting

Current limits (may change in production):
- 100 requests per minute per IP
- 10 concurrent WebSocket connections
- 100MB max file upload size

## Examples

### JavaScript API Client
```javascript
class TricorderAPI {
  constructor(baseUrl = 'http://localhost:8080') {
    this.baseUrl = baseUrl;
  }

  async getDevices() {
    const response = await fetch(`${this.baseUrl}/api/devices`);
    return response.json();
  }

  async sendCommand(target, action, parameters = {}) {
    const response = await fetch(`${this.baseUrl}/api/commands/send`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ target, action, parameters })
    });
    return response.json();
  }

  async playVideo(deviceId, filename, loop = false) {
    return this.sendCommand(deviceId, 'video_play', { filename, loop });
  }

  async setLEDColor(deviceId, r, g, b) {
    return this.sendCommand(deviceId, 'led_color', { r, g, b });
  }
}

// Usage
const api = new TricorderAPI();
await api.playVideo('TRICORDER_001', 'scene1.mp4');
await api.setLEDColor('TRICORDER_001', 255, 0, 0);
```

### Python API Client
```python
import requests
import json

class TricorderAPI:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url

    def get_devices(self):
        response = requests.get(f"{self.base_url}/api/devices")
        return response.json()

    def send_command(self, target, action, parameters=None):
        data = {
            "target": target,
            "action": action, 
            "parameters": parameters or {}
        }
        response = requests.post(
            f"{self.base_url}/api/commands/send",
            json=data
        )
        return response.json()

    def play_video(self, device_id, filename, loop=False):
        return self.send_command(device_id, "video_play", {
            "filename": filename,
            "loop": loop
        })

# Usage
api = TricorderAPI()
api.play_video("TRICORDER_001", "scene1.mp4")
```
