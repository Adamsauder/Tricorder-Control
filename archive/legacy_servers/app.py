"""
Tricorder Control Server
Central management server for ESP32-based film set props

Features:
- Device discovery and auto-pairing
- Real-time command dispatch
- Web interface for management
- File management for SD cards
- SACN lighting protocol integration
- REST API and WebSocket support
"""

from fastapi import FastAPI, WebSocket, HTTPException, UploadFile, File
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List, Optional, Dict, Any
import asyncio
import socket
import json
import uuid
import time
from datetime import datetime
import sqlite3
import redis
import aiofiles
import os
from pathlib import Path

# Initialize FastAPI app
app = FastAPI(
    title="Tricorder Control Server",
    description="Central management server for ESP32-based film set props",
    version="1.0.0"
)

# Enable CORS for web interface
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Configuration
CONFIG = {
    "udp_port": 8888,
    "web_port": 8080,
    "sacn_port": 5568,
    "multicast_ip": "239.255.0.1",
    "device_timeout": 30,  # seconds
    "command_timeout": 5,  # seconds
    "max_file_size": 100 * 1024 * 1024,  # 100MB
}

# Global state
devices: Dict[str, Dict] = {}
active_commands: Dict[str, Dict] = {}
websocket_connections: List[WebSocket] = []

# Data models
class DeviceInfo(BaseModel):
    device_id: str
    ip_address: str
    firmware_version: str
    status: str
    last_seen: datetime
    battery_voltage: Optional[float] = None
    temperature: Optional[float] = None
    current_video: Optional[str] = None
    video_playing: bool = False

class Command(BaseModel):
    target: str  # device_id or "ALL"
    action: str
    parameters: Dict[str, Any] = {}

class LEDConfig(BaseModel):
    device_id: str
    brightness: float = 1.0
    color: Dict[str, int] = {"r": 0, "g": 0, "b": 0}
    animation: str = "solid"
    sacn_universe: int = 1
    sacn_start_channel: int = 1

# Database setup
def init_database():
    """Initialize SQLite database for persistent storage"""
    conn = sqlite3.connect('tricorder.db')
    cursor = conn.cursor()
    
    # Create devices table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS devices (
            device_id TEXT PRIMARY KEY,
            mac_address TEXT,
            first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            last_configuration TEXT,
            notes TEXT
        )
    ''')
    
    # Create commands history table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS command_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            command_id TEXT,
            device_id TEXT,
            action TEXT,
            parameters TEXT,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            execution_time_ms INTEGER,
            status TEXT
        )
    ''')
    
    conn.commit()
    conn.close()

# Redis setup for real-time state
try:
    redis_client = redis.Redis(host='localhost', port=6379, db=0, decode_responses=True)
    redis_client.ping()
    print("Redis connected")
except:
    redis_client = None
    print("Redis not available, using memory storage")

@app.on_event("startup")
async def startup_event():
    """Initialize services on startup"""
    init_database()
    
    # Start background tasks
    asyncio.create_task(udp_server())
    asyncio.create_task(device_discovery())
    asyncio.create_task(heartbeat_monitor())
    asyncio.create_task(sacn_listener())
    
    print("Tricorder Control Server started")

@app.on_event("shutdown")
async def shutdown_event():
    """Cleanup on shutdown"""
    print("Tricorder Control Server shutting down")

# Device Management
@app.get("/api/devices", response_model=List[DeviceInfo])
async def get_devices():
    """Get all registered devices"""
    device_list = []
    for device_id, device_data in devices.items():
        device_list.append(DeviceInfo(**device_data))
    return device_list

@app.get("/api/devices/{device_id}", response_model=DeviceInfo)
async def get_device(device_id: str):
    """Get specific device information"""
    if device_id not in devices:
        raise HTTPException(status_code=404, detail="Device not found")
    return DeviceInfo(**devices[device_id])

@app.post("/api/devices/{device_id}/register")
async def register_device(device_id: str, device_info: dict):
    """Register a new device"""
    devices[device_id] = {
        "device_id": device_id,
        "ip_address": device_info.get("ip_address"),
        "firmware_version": device_info.get("firmware_version", "unknown"),
        "status": "online",
        "last_seen": datetime.now(),
        "battery_voltage": device_info.get("battery_voltage"),
        "temperature": device_info.get("temperature"),
        "current_video": "",
        "video_playing": False
    }
    
    # Store in database
    conn = sqlite3.connect('tricorder.db')
    cursor = conn.cursor()
    cursor.execute('''
        INSERT OR REPLACE INTO devices (device_id, mac_address)
        VALUES (?, ?)
    ''', (device_id, device_info.get("mac_address", "")))
    conn.commit()
    conn.close()
    
    # Notify websocket clients
    await broadcast_to_websockets({
        "type": "device_registered",
        "device": devices[device_id]
    })
    
    return {"status": "registered", "device_id": device_id}

# Command System
@app.post("/api/commands/send")
async def send_command(command: Command):
    """Send command to device(s)"""
    command_id = str(uuid.uuid4())
    
    # Create command packet
    command_packet = {
        "command_id": command_id,
        "timestamp": int(time.time()),
        "target": command.target,
        "action": command.action,
        "parameters": command.parameters
    }
    
    # Store active command
    active_commands[command_id] = {
        "packet": command_packet,
        "timestamp": time.time(),
        "responses": {}
    }
    
    # Send via UDP
    await send_udp_command(command_packet)
    
    return {
        "command_id": command_id,
        "status": "sent",
        "target": command.target
    }

@app.get("/api/commands/{command_id}/status")
async def get_command_status(command_id: str):
    """Get command execution status"""
    if command_id not in active_commands:
        raise HTTPException(status_code=404, detail="Command not found")
    
    command_info = active_commands[command_id]
    return {
        "command_id": command_id,
        "status": "completed" if command_info["responses"] else "pending",
        "responses": command_info["responses"],
        "elapsed_time": time.time() - command_info["timestamp"]
    }

# File Management
@app.get("/api/devices/{device_id}/files")
async def get_device_files(device_id: str):
    """Get files on device SD card"""
    if device_id not in devices:
        raise HTTPException(status_code=404, detail="Device not found")
    
    # Request file list from device
    command = {
        "command_id": str(uuid.uuid4()),
        "timestamp": int(time.time()),
        "target": device_id,
        "action": "list_files",
        "parameters": {}
    }
    
    await send_udp_command(command)
    # In real implementation, wait for response and return file list
    
    return {"files": [], "status": "requested"}

@app.post("/api/devices/{device_id}/files/upload")
async def upload_file(device_id: str, file: UploadFile = File(...)):
    """Upload file to device SD card"""
    if device_id not in devices:
        raise HTTPException(status_code=404, detail="Device not found")
    
    if file.size > CONFIG["max_file_size"]:
        raise HTTPException(status_code=413, detail="File too large")
    
    # Save file temporarily
    upload_dir = Path("uploads") / device_id
    upload_dir.mkdir(parents=True, exist_ok=True)
    
    file_path = upload_dir / file.filename
    async with aiofiles.open(file_path, 'wb') as f:
        content = await file.read()
        await f.write(content)
    
    # Transfer to device (implementation specific)
    # This would use FTP, HTTP POST, or custom protocol
    
    return {
        "filename": file.filename,
        "size": file.size,
        "status": "uploaded"
    }

# LED Configuration
@app.post("/api/devices/{device_id}/leds/config")
async def configure_leds(device_id: str, config: LEDConfig):
    """Configure LED settings for device"""
    command = Command(
        target=device_id,
        action="led_config",
        parameters={
            "brightness": config.brightness,
            "color": config.color,
            "animation": config.animation,
            "sacn_universe": config.sacn_universe,
            "sacn_start_channel": config.sacn_start_channel
        }
    )
    
    result = await send_command(command)
    return result

@app.post("/api/devices/{device_id}/leds/color")
async def set_led_color(device_id: str, color: Dict[str, int]):
    """Set LED color for device"""
    command = Command(
        target=device_id,
        action="led_color",
        parameters=color
    )
    
    result = await send_command(command)
    return result

# Video Control
@app.post("/api/devices/{device_id}/video/play")
async def play_video(device_id: str, filename: str, loop: bool = False):
    """Play video on device"""
    command = Command(
        target=device_id,
        action="video_play",
        parameters={"filename": filename, "loop": loop}
    )
    
    result = await send_command(command)
    return result

@app.post("/api/devices/{device_id}/video/pause")
async def pause_video(device_id: str):
    """Pause video on device"""
    command = Command(target=device_id, action="video_pause")
    result = await send_command(command)
    return result

@app.post("/api/devices/{device_id}/video/stop")
async def stop_video(device_id: str):
    """Stop video on device"""
    command = Command(target=device_id, action="video_stop")
    result = await send_command(command)
    return result

# WebSocket for real-time updates
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time updates"""
    await websocket.accept()
    websocket_connections.append(websocket)
    
    try:
        # Send initial device state
        await websocket.send_json({
            "type": "device_list",
            "devices": list(devices.values())
        })
        
        # Keep connection alive
        while True:
            await websocket.receive_text()
            
    except Exception as e:
        print(f"WebSocket error: {e}")
    finally:
        websocket_connections.remove(websocket)

async def broadcast_to_websockets(message: dict):
    """Broadcast message to all connected WebSocket clients"""
    if websocket_connections:
        disconnected = []
        for websocket in websocket_connections:
            try:
                await websocket.send_json(message)
            except:
                disconnected.append(websocket)
        
        # Remove disconnected clients
        for ws in disconnected:
            websocket_connections.remove(ws)

# Background Tasks
async def udp_server():
    """UDP server for device communication"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', CONFIG["udp_port"]))
    sock.setblocking(False)
    
    print(f"UDP server listening on port {CONFIG['udp_port']}")
    
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            message = json.loads(data.decode())
            
            # Handle different message types
            if message.get("type") == "heartbeat":
                await handle_heartbeat(message, addr)
            elif message.get("command_id"):
                await handle_command_response(message, addr)
            
        except socket.error:
            await asyncio.sleep(0.01)
        except Exception as e:
            print(f"UDP server error: {e}")
            await asyncio.sleep(0.1)

async def send_udp_command(command: dict):
    """Send UDP command to device(s)"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    
    message = json.dumps(command).encode()
    
    if command["target"] == "ALL":
        # Broadcast to all devices
        sock.sendto(message, ('<broadcast>', CONFIG["udp_port"]))
    else:
        # Send to specific device
        device = devices.get(command["target"])
        if device and device.get("ip_address"):
            sock.sendto(message, (device["ip_address"], CONFIG["udp_port"]))
    
    sock.close()

async def handle_heartbeat(message: dict, addr):
    """Handle device heartbeat messages"""
    device_id = message.get("device_id")
    if device_id:
        # Update device status
        if device_id in devices:
            devices[device_id].update({
                "status": message.get("status", "online"),
                "last_seen": datetime.now(),
                "battery_voltage": message.get("battery_voltage"),
                "temperature": message.get("temperature"),
                "ip_address": addr[0]
            })
        else:
            # Auto-register new device
            devices[device_id] = {
                "device_id": device_id,
                "ip_address": addr[0],
                "firmware_version": "unknown",
                "status": "online",
                "last_seen": datetime.now(),
                "battery_voltage": message.get("battery_voltage"),
                "temperature": message.get("temperature"),
                "current_video": "",
                "video_playing": False
            }
        
        # Broadcast update
        await broadcast_to_websockets({
            "type": "device_update",
            "device": devices[device_id]
        })

async def handle_command_response(message: dict, addr):
    """Handle command response from device"""
    command_id = message.get("command_id")
    device_id = message.get("device_id")
    
    if command_id in active_commands:
        active_commands[command_id]["responses"][device_id] = {
            "status": message.get("status"),
            "message": message.get("message"),
            "execution_time_ms": message.get("execution_time_ms"),
            "timestamp": time.time()
        }
        
        # Store in database
        conn = sqlite3.connect('tricorder.db')
        cursor = conn.cursor()
        cursor.execute('''
            INSERT INTO command_history 
            (command_id, device_id, action, parameters, execution_time_ms, status)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (
            command_id,
            device_id,
            active_commands[command_id]["packet"]["action"],
            json.dumps(active_commands[command_id]["packet"]["parameters"]),
            message.get("execution_time_ms"),
            message.get("status")
        ))
        conn.commit()
        conn.close()
        
        # Broadcast update
        await broadcast_to_websockets({
            "type": "command_response",
            "command_id": command_id,
            "device_id": device_id,
            "response": active_commands[command_id]["responses"][device_id]
        })

async def device_discovery():
    """mDNS device discovery service"""
    # Placeholder for mDNS implementation
    # This would scan for _tricorder._tcp.local services
    while True:
        await asyncio.sleep(10)
        # Scan for new devices and auto-register them

async def heartbeat_monitor():
    """Monitor device heartbeats and mark offline devices"""
    while True:
        now = datetime.now()
        offline_devices = []
        
        for device_id, device in devices.items():
            last_seen = device.get("last_seen")
            if last_seen and (now - last_seen).seconds > CONFIG["device_timeout"]:
                if device["status"] != "offline":
                    device["status"] = "offline"
                    offline_devices.append(device_id)
        
        # Broadcast offline status
        for device_id in offline_devices:
            await broadcast_to_websockets({
                "type": "device_offline",
                "device_id": device_id
            })
        
        await asyncio.sleep(5)

async def sacn_listener():
    """SACN (E1.31) lighting protocol listener"""
    # Placeholder for SACN implementation
    # This would listen for E1.31 packets and convert to LED commands
    while True:
        await asyncio.sleep(0.1)
        # Process SACN packets and update device LEDs

# Mount static files for web interface (only if dist directory exists)
import os
if os.path.exists("../web/dist"):
    app.mount("/", StaticFiles(directory="../web/dist", html=True), name="static")
else:
    print("Web dist directory not found - serving API only")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=CONFIG["web_port"])
