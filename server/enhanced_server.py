#!/usr/bin/env python3
"""
Enhanced Simple Server with ESP32 Simulator Support
Combines the main tricorder server with simulator endpoints
"""

import asyncio
import socket
import json
import time
import uuid
from datetime import datetime
from typing import Dict, List, Optional
from flask import Flask, render_template, request, jsonify, send_file, abort
from flask_socketio import SocketIO, emit
import threading
import ipaddress
import io
from PIL import Image, ImageDraw, ImageFont
import colorsys

# Flask app setup
app = Flask(__name__)
app.config['SECRET_KEY'] = 'tricorder_control_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

# Configuration
CONFIG = {
    "udp_port": 8888,
    "web_port": 5000,
    "device_timeout": 30,  # seconds
    "command_timeout": 5,  # seconds
}

# Global state
devices: Dict[str, Dict] = {}
active_commands: Dict[str, Dict] = {}
command_history: List[Dict] = []
server_ip: Optional[str] = None
server_start_time = time.time()

# Simulator color mappings
COLORS = {
    'red': (255, 0, 0),
    'green': (0, 255, 0),
    'blue': (0, 0, 255),
    'cyan': (0, 255, 255),
    'magenta': (255, 0, 255),
    'yellow': (255, 255, 0),
    'white': (255, 255, 255),
    'black': (0, 0, 0),
}

def get_server_ip():
    """Get the server's IP address"""
    global server_ip
    if server_ip is None:
        try:
            # Connect to a remote address to determine local IP
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.connect(("8.8.8.8", 80))
                server_ip = s.getsockname()[0]
        except Exception:
            server_ip = "127.0.0.1"
    return server_ip or "127.0.0.1"

def create_color_frame(color_name, width=320, height=240):
    """Create a solid color frame"""
    color = COLORS.get(color_name, COLORS['white'])
    img = Image.new('RGB', (width, height), color)
    return img

def create_startup_frame(width=320, height=240):
    """Create a startup frame with text"""
    img = Image.new('RGB', (width, height), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    try:
        font = ImageFont.load_default()
    except:
        font = None
    
    text = "TRICORDER\nSTARTUP"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (width - text_width) // 2
    y = (height - text_height) // 2
    
    draw.text((x, y), text, fill=(0, 255, 0), font=font, align='center')
    return img

def create_static_test_frame(width=320, height=240):
    """Create a test pattern frame"""
    img = Image.new('RGB', (width, height), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    grid_size = 20
    for x in range(0, width, grid_size):
        for y in range(0, height, grid_size):
            if (x // grid_size + y // grid_size) % 2 == 0:
                color = (128, 128, 128)
            else:
                color = (64, 64, 64)
            draw.rectangle([x, y, x + grid_size, y + grid_size], fill=color)
    
    try:
        font = ImageFont.load_default()
    except:
        font = None
    
    text = "TEST PATTERN"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    x = (width - text_width) // 2
    y = height // 2
    
    draw.text((x, y), text, fill=(255, 255, 255), font=font)
    return img

def create_animated_test_frame(frame_num, total_frames=30, width=320, height=240):
    """Create an animated test frame"""
    img = Image.new('RGB', (width, height), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    center_x, center_y = width // 2, height // 2
    radius = min(width, height) // 3
    
    angle_offset = (frame_num / total_frames) * 360
    
    num_sectors = 8
    for i in range(num_sectors):
        angle = (i * 360 / num_sectors + angle_offset) % 360
        hue = angle / 360
        rgb = colorsys.hsv_to_rgb(hue, 1.0, 1.0)
        color = tuple(int(c * 255) for c in rgb)
        
        start_angle = i * 360 / num_sectors
        end_angle = (i + 1) * 360 / num_sectors
        
        draw.pieslice([center_x - radius, center_y - radius, 
                      center_x + radius, center_y + radius], 
                     start_angle, end_angle, fill=color)
    
    try:
        font = ImageFont.load_default()
    except:
        font = None
    
    text = f"Frame {frame_num:03d}"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    x = (width - text_width) // 2
    y = height - 30
    
    draw.text((x, y), text, fill=(255, 255, 255), font=font)
    return img

class TricorderServer:
    def __init__(self):
        self.udp_socket = None
        self.running = False
        
    def start_udp_listener(self):
        """Start UDP listener for device communication"""
        try:
            self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_socket.bind(('', CONFIG["udp_port"]))
            self.udp_socket.settimeout(1.0)
            self.running = True
            
            print(f"UDP listener started on port {CONFIG['udp_port']}")
            
            while self.running:
                try:
                    data, addr = self.udp_socket.recvfrom(4096)  # Increased buffer size like original
                    self.handle_device_message(data.decode('utf-8'), addr)
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"❌ UDP error: {e}")
                    
        except Exception as e:
            print(f"Failed to start UDP listener: {e}")
    
    def handle_device_message(self, message: str, addr: tuple):
        """Handle incoming device messages"""
        try:
            data = json.loads(message)
            print(f"📡 Received from {addr}: {message}")
            
            # Skip messages from the server itself
            if addr[0] == get_server_ip():
                print(f"🚫 Ignoring message from server IP: {addr[0]}")
                return
            
            # Handle both 'deviceId' (from ESP32) and 'device_id' formats
            device_id = data.get('deviceId') or data.get('device_id', f'UNKNOWN_{addr[0]}')
            
            # Only process messages from actual ESP32 devices (they should have deviceId starting with TRICORDER_)
            if not device_id.startswith('TRICORDER_'):
                print(f"🚫 Ignoring non-tricorder device: {device_id} from {addr[0]}")
                return
            
            # Update device registry with comprehensive info
            devices[device_id] = {
                'device_id': device_id,
                'ip_address': addr[0],
                'port': addr[1],
                'last_seen': datetime.now().isoformat(),
                'status': 'online',
                # ESP32-specific fields
                'firmware_version': data.get('firmwareVersion'),
                'wifi_connected': data.get('wifiConnected'),
                'free_heap': data.get('freeHeap'),
                'uptime': data.get('uptime'),
                'sd_card_initialized': data.get('sdCardInitialized'),
                'video_playing': data.get('videoPlaying'),
                'current_video': data.get('currentVideo'),
                'video_looping': data.get('videoLooping'),
                'current_frame': data.get('currentFrame'),
                **data  # Include any additional fields
            }
            
            print(f"✓ Updated device: {device_id} at {addr[0]}")
            
            # Broadcast to web clients
            socketio.emit('device_update', devices[device_id])
            print(f"📡 Emitted device_update for {device_id}")
            
            # Also broadcast the raw response for command handling
            socketio.emit('device_response', {
                'device_id': device_id,
                'response': data
            })
            print(f"📡 Emitted device_response for {device_id}: {data.get('result', 'No result')}")
            
        except json.JSONDecodeError:
            print(f"⚠️ Invalid JSON from {addr}: {message}")
        except Exception as e:
            print(f"❌ Error handling message from {addr}: {e}")

# Initialize server
server = TricorderServer()

@app.route('/')
def index():
    """Main web interface"""
    return '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Tricorder Control System</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
            .container { max-width: 1200px; margin: 0 auto; }
            .status { background: white; padding: 20px; margin: 10px 0; border-radius: 8px; }
            .devices { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
            .device { background: white; padding: 15px; border-radius: 8px; border-left: 4px solid #4CAF50; }
            .device.offline { border-left-color: #f44336; }
            button { background: #2196F3; color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; margin: 2px; }
            button:hover { background: #1976D2; }
            .simulator { text-align: center; margin: 20px 0; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🔺 Tricorder Control System</h1>
            
            <div class="status">
                <h2>Server Status</h2>
                <p>🖥️ Server IP: <span id="server-ip">''' + get_server_ip() + '''</span></p>
                <p>✅ UDP Server running on port ''' + str(CONFIG['udp_port']) + '''</p>
                <p>✅ Web Server running on port ''' + str(CONFIG['web_port']) + '''</p>
                <p>📱 Connected Devices: <span id="device-count">0</span></p>
                <button onclick="refreshServerInfo()" style="background: #2196F3; margin-top: 10px;">🔄 Refresh Server Info</button>
            </div>

            <div class="status">
                <h2>ESP32 Simulator</h2>
                <p>The simulator shows what appears on the ESP32 TFT display (320x240 ST7789)</p>
                <div class="simulator">
                    <button onclick="window.open('/simulator', '_blank')">Open Simulator</button>
                </div>
            </div>
            
            <div class="status">
                <h2>Connected Devices</h2>
                <div style="margin-bottom: 15px;">
                    <button onclick="startDiscovery()" style="background: #4CAF50;">🔍 Search for Devices</button>
                    <input type="text" id="deviceIP" placeholder="192.168.1.xxx" style="padding: 8px; margin: 0 10px; border: 1px solid #ccc; border-radius: 4px;">
                    <button onclick="addDevice()" style="background: #FF9800;">➕ Add Device Manually</button>
                </div>
                <div id="devices" class="devices">
                    <p>No devices connected. Use the buttons above to discover or add devices.</p>
                </div>
            </div>
        </div>

        <script src="https://cdnjs.cloudflare.com/ajax/libs/socket.io/4.0.0/socket.io.js"></script>
        <script>
            const socket = io();
            const devicesDiv = document.getElementById('devices');
            const deviceCount = document.getElementById('device-count');
            
            socket.on('device_update', function(device) {
                updateDeviceDisplay();
            });
            
            // Store devices data globally so updateDeviceDisplay can access it
            window.devicesData = {};
            
            socket.on('device_update', function(device) {
                window.devicesData[device.device_id] = device;
                updateDeviceDisplay();
            });
            
            function updateDeviceDisplay() {
                const deviceList = Object.values(window.devicesData || {});
                deviceCount.textContent = deviceList.length;
                
                if (deviceList.length === 0) {
                    devicesDiv.innerHTML = '<p>No devices connected.</p>';
                    return;
                }
                
                devicesDiv.innerHTML = deviceList.map(device => `
                    <div class="device ${device.status}">
                        <h3>${device.device_id}</h3>
                        <p><strong>IP:</strong> ${device.ip_address}</p>
                        <p><strong>Status:</strong> ${device.status}</p>
                        <p><strong>Last seen:</strong> ${new Date(device.last_seen).toLocaleString()}</p>
                        ${device.firmware_version ? `<p><strong>Firmware:</strong> ${device.firmware_version}</p>` : ''}
                        ${device.free_heap ? `<p><strong>Free Heap:</strong> ${device.free_heap} bytes</p>` : ''}
                        ${device.video_playing ? `<p><strong>Video:</strong> ${device.current_video || 'Playing'} ${device.video_looping ? '(Looping)' : ''}</p>` : ''}
                        <div style="margin-top: 10px;">
                            <strong>📺 Image Controls:</strong><br>
                            <button onclick="sendCommand('${device.device_id}', 'play_video', 'startup.jpg')">Play Startup</button>
                            <button onclick="sendImageCommand('${device.device_id}', 'greenscreen.jpg')" style="background: #00ff00; color: black; margin: 2px;">🟩 Green Screen</button>
                            <button onclick="sendImageCommand('${device.device_id}', 'test.jpg')" style="background: #ffcc00; color: black; margin: 2px;">📺 Test Card</button>
                            <br><br>
                            <strong>💡 LED Controls:</strong><br>
                            <button onclick="sendLEDColor('${device.device_id}', 255, 0, 0)" style="background: #ff4444; margin: 2px;">🔴 Red</button>
                            <button onclick="sendLEDColor('${device.device_id}', 0, 255, 0)" style="background: #44ff44; margin: 2px;">🟢 Green</button>
                            <button onclick="sendLEDColor('${device.device_id}', 0, 0, 255)" style="background: #4444ff; margin: 2px;">🔵 Blue</button>
                            <button onclick="sendLEDColor('${device.device_id}', 255, 255, 0)" style="background: #ffff44; color: black; margin: 2px;">🟡 Yellow</button>
                            <button onclick="sendLEDColor('${device.device_id}', 255, 0, 255)" style="background: #ff44ff; margin: 2px;">🟣 Magenta</button>
                            <button onclick="sendLEDColor('${device.device_id}', 0, 255, 255)" style="background: #44ffff; color: black; margin: 2px;">🔵 Cyan</button>
                            <button onclick="sendLEDColor('${device.device_id}', 255, 255, 255)" style="background: #ffffff; color: black; margin: 2px;">⚪ White</button>
                            <button onclick="sendLEDColor('${device.device_id}', 0, 0, 0)" style="background: #666666; margin: 2px;">⚫ Off</button>
                            <br><br>
                            <button onclick="sendCommand('${device.device_id}', 'stop_video', '')">Stop Video</button>
                        </div>
                    </div>
                `).join('');
            }
            
            function sendCommand(deviceId, action, data) {
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_id: deviceId,
                        action: action,
                        data: data
                    })
                });
            }
            
            function sendImageCommand(deviceId, filename) {
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_id: deviceId,
                        action: 'display_image',
                        parameters: {
                            filename: filename
                        }
                    })
                });
            }
            
            function sendLEDColor(deviceId, r, g, b) {
                // Simple debouncing to prevent rapid clicks
                const now = Date.now();
                if (window.lastLEDCommand && (now - window.lastLEDCommand) < 200) {
                    console.log('⏳ LED command ignored (too fast)');
                    return;
                }
                window.lastLEDCommand = now;
                
                console.log(`Setting LED color for ${deviceId} to RGB(${r}, ${g}, ${b})`);
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_id: deviceId,
                        action: 'set_builtin_led',
                        parameters: {
                            r: r,
                            g: g,
                            b: b
                        }
                    })
                })
                .then(response => response.json())
                .then(data => {
                    console.log('LED command response:', data);
                    if (data.status === 'sent') {
                        console.log(`✅ LED color command sent to ${deviceId}`);
                    }
                })
                .catch(error => {
                    console.error('LED command error:', error);
                    alert('Failed to send LED color command');
                });
            }
            
            function startDiscovery() {
                console.log('Starting device discovery...');
                fetch('/api/discovery', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'}
                })
                .then(response => response.json())
                .then(data => {
                    console.log('Discovery response:', data);
                    if (data.status) {
                        alert('Device discovery started! Check console for details.');
                    } else if (data.error) {
                        alert('Discovery failed: ' + data.error);
                    }
                })
                .catch(error => {
                    console.error('Discovery error:', error);
                    alert('Failed to start discovery scan');
                });
            }
            
            function refreshServerInfo() {
                console.log('Refreshing server info...');
                fetch('/api/server_info')
                .then(response => response.json())
                .then(data => {
                    console.log('Server info:', data);
                    document.getElementById('server-ip').textContent = data.server_ip;
                    document.getElementById('device-count').textContent = data.device_count.toString();
                    alert(`Server Info Updated!\\nIP: ${data.server_ip}\\nDevices: ${data.device_count}\\nUptime: ${Math.round(data.uptime)}s`);
                })
                .catch(error => {
                    console.error('Server info error:', error);
                    alert('Failed to refresh server info');
                });
            }
            
            function addDevice() {
                const ipInput = document.getElementById('deviceIP');
                const ipAddress = ipInput.value.trim();
                
                if (!ipAddress) {
                    alert('Please enter an IP address');
                    return;
                }
                
                console.log('Adding device:', ipAddress);
                fetch('/api/add_device', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        ip_address: ipAddress
                    })
                })
                .then(response => response.json())
                .then(data => {
                    console.log('Add device response:', data);
                    if (data.status) {
                        alert('Device add initiated! Check if device appears in the list.');
                        ipInput.value = ''; // Clear input
                    } else if (data.error) {
                        alert('Add device failed: ' + data.error);
                    }
                })
                .catch(error => {
                    console.error('Add device error:', error);
                    alert('Failed to add device');
                });
            }
            
            // Load existing devices when page loads
            function loadDevices() {
                fetch('/api/devices')
                .then(response => response.json())
                .then(devices => {
                    console.log('Loaded devices:', devices);
                    devices.forEach(device => {
                        window.devicesData[device.device_id] = device;
                    });
                    updateDeviceDisplay();
                })
                .catch(error => {
                    console.error('Failed to load devices:', error);
                });
            }
            
            // Load devices on page load
            window.addEventListener('load', loadDevices);
        </script>
    </body>
    </html>
    '''

@app.route('/simulator')
def simulator():
    """ESP32 simulator page"""
    return '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32 Display Simulator</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; text-align: center; }
            .simulator { background: #000; padding: 20px; border-radius: 10px; display: inline-block; margin: 20px; }
            canvas { border: 2px solid #333; transform: scale(2); transform-origin: center; }
            .controls { margin: 20px; }
            button { background: #2196F3; color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; margin: 2px; }
            select { padding: 8px; margin: 5px; }
        </style>
    </head>
    <body>
        <h1>ESP32 Display Simulator (ST7789 320x240)</h1>
        
        <div class="simulator">
            <canvas id="display" width="320" height="240"></canvas>
        </div>
        
        <div class="controls">
            <select id="videoSelect">
                <option value="">Select video...</option>
                <option value="startup.jpg">Startup</option>
                <option value="static_test.jpg">Test Pattern</option>
                <option value="color_red.jpg">Red</option>
                <option value="color_green.jpg">Green</option>
                <option value="color_blue.jpg">Blue</option>
                <option value="color_cyan.jpg">Cyan</option>
                <option value="color_magenta.jpg">Magenta</option>
                <option value="color_yellow.jpg">Yellow</option>
                <option value="color_white.jpg">White</option>
                <option value="animated_test">Animated Test (30 frames)</option>
            </select>
            <br>
            <button onclick="playVideo()">Play</button>
            <button onclick="stopVideo()">Stop</button>
            <button onclick="clearScreen()">Clear</button>
        </div>
        
        <p>This simulator shows exactly what appears on the ESP32 TFT display.</p>
        
        <script>
            const canvas = document.getElementById('display');
            const ctx = canvas.getContext('2d');
            const videoSelect = document.getElementById('videoSelect');
            let animationInterval = null;
            let currentFrame = 0;
            
            // Initialize with black screen
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, 320, 240);
            ctx.fillStyle = '#00ff00';
            ctx.font = '16px monospace';
            ctx.textAlign = 'center';
            ctx.fillText('ESP32 Simulator Ready', 160, 120);
            
            function playVideo() {
                const video = videoSelect.value;
                if (!video) return;
                
                stopVideo(); // Stop any current animation
                
                if (video === 'animated_test') {
                    playAnimatedSequence();
                } else {
                    displayFrame(video);
                }
            }
            
            function displayFrame(filename) {
                const img = new Image();
                img.onload = function() {
                    ctx.drawImage(img, 0, 0, 320, 240);
                };
                img.onerror = function() {
                    // Fallback for missing images
                    ctx.fillStyle = '#ff0000';
                    ctx.fillRect(0, 0, 320, 240);
                    ctx.fillStyle = '#ffffff';
                    ctx.font = '16px monospace';
                    ctx.textAlign = 'center';
                    ctx.fillText('Frame Missing', 160, 120);
                };
                img.src = '/api/simulator/frames/' + filename;
            }
            
            function playAnimatedSequence() {
                currentFrame = 0;
                animationInterval = setInterval(() => {
                    const filename = 'animated_test_frame_' + String(currentFrame + 1).padStart(3, '0') + '.jpg';
                    displayFrame(filename);
                    currentFrame = (currentFrame + 1) % 30;
                }, 66); // ~15 FPS
            }
            
            function stopVideo() {
                if (animationInterval) {
                    clearInterval(animationInterval);
                    animationInterval = null;
                }
            }
            
            function clearScreen() {
                stopVideo();
                ctx.fillStyle = '#000000';
                ctx.fillRect(0, 0, 320, 240);
            }
        </script>
    </body>
    </html>
    '''

@app.route('/api/simulator/frames/<filename>')
def serve_frame(filename):
    """Serve a frame image for the simulator"""
    try:
        width, height = 320, 240
        
        if filename.startswith('color_'):
            color_name = filename.replace('color_', '').replace('.jpg', '')
            img = create_color_frame(color_name, width, height)
            
        elif filename == 'startup.jpg':
            img = create_startup_frame(width, height)
            
        elif filename == 'startup_mid.jpg':
            img = create_startup_frame(width, height)
            img = Image.eval(img, lambda x: int(x * 0.7))
            
        elif filename == 'static_test.jpg':
            img = create_static_test_frame(width, height)
            
        elif filename.startswith('animated_test_frame_'):
            frame_str = filename.replace('animated_test_frame_', '').replace('.jpg', '')
            frame_num = int(frame_str)
            img = create_animated_test_frame(frame_num, 30, width, height)
            
        else:
            img = Image.new('RGB', (width, height), (32, 32, 32))
            draw = ImageDraw.Draw(img)
            
            try:
                font = ImageFont.load_default()
            except:
                font = None
            
            text = f"Missing:\n{filename}"
            bbox = draw.textbbox((0, 0), text, font=font)
            text_width = bbox[2] - bbox[0]
            text_height = bbox[3] - bbox[1]
            x = (width - text_width) // 2
            y = (height - text_height) // 2
            
            draw.text((x, y), text, fill=(255, 128, 128), font=font, align='center')
        
        img_io = io.BytesIO()
        img.save(img_io, 'JPEG', quality=85)
        img_io.seek(0)
        
        return send_file(img_io, mimetype='image/jpeg')
        
    except Exception as e:
        print(f"Error serving frame {filename}: {e}")
        abort(404)

@app.route('/api/devices')
def get_devices():
    """Get all connected devices"""
    return jsonify(list(devices.values()))

@app.route('/api/server_info')
def get_server_info():
    """Get server information"""
    return jsonify({
        'server_ip': get_server_ip(),
        'udp_port': CONFIG['udp_port'],
        'web_port': CONFIG['web_port'],
        'device_count': len(devices),
        'uptime': time.time() - server_start_time if 'server_start_time' in globals() else 0,
        'status': 'running'
    })

@app.route('/api/discovery', methods=['POST'])
def start_discovery():
    """Manually trigger device discovery"""
    try:
        print("🔍 Discovery scan initiated")
        
        # Send discovery command to IP range
        discovery_command = {
            'id': str(uuid.uuid4()),
            'action': 'discovery',
            'timestamp': datetime.now().isoformat()
        }
        
        # Scan common IP ranges for ESP32 devices
        ip_ranges = [
            "192.168.1.{}", # Common home router range
            "192.168.0.{}", # Alternative home router range
            "10.0.0.{}",    # Some router configurations
        ]
        
        devices_found = 0
        
        if server.udp_socket:
            command_json = json.dumps(discovery_command)
            
            # For each IP range, scan common device IPs
            for ip_template in ip_ranges:
                # Scan from .2 to .254 (skip .1 as it's usually the router, and .255 is broadcast)
                for i in range(2, 255):
                    target_ip = ip_template.format(i)
                    try:
                        # Send discovery command
                        server.udp_socket.sendto(
                            command_json.encode('utf-8'),
                            (target_ip, CONFIG['udp_port'])
                        )
                        devices_found += 1
                        
                        # Add small delay to prevent network flooding
                        if devices_found % 50 == 0:
                            import time
                            time.sleep(0.1)
                            
                    except Exception as e:
                        # Skip IPs that can't be reached
                        continue
            
            print(f"📤 Sent discovery to {devices_found} IP addresses across multiple ranges")
            return jsonify({'status': f'Discovery scan initiated - scanned {devices_found} IP addresses'})
        else:
            return jsonify({'error': 'UDP socket not available'}), 500
            
    except Exception as e:
        print(f"❌ Discovery error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/add_device', methods=['POST'])
def add_device():
    """Manually add a device by IP address"""
    try:
        data = request.get_json()
        ip_address = data.get('ip_address')
        
        if not ip_address:
            return jsonify({'error': 'IP address is required'}), 400
        
        print(f"➕ Adding device at {ip_address}")
        
        # Send a ping command to the device
        ping_command = {
            'id': str(uuid.uuid4()),
            'action': 'ping',
            'timestamp': datetime.now().isoformat()
        }
        
        if server.udp_socket:
            command_json = json.dumps(ping_command)
            server.udp_socket.sendto(
                command_json.encode('utf-8'),
                (ip_address, CONFIG['udp_port'])
            )
            print(f"📤 Sent ping to {ip_address}")
            return jsonify({'status': f'Device add initiated - sent ping to {ip_address}'})
        else:
            return jsonify({'error': 'UDP socket not available'}), 500
            
    except Exception as e:
        print(f"❌ Add device error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/command', methods=['POST'])
def send_command():
    """Send command to device"""
    try:
        data = request.get_json()
        device_id = data.get('device_id')
        action = data.get('action')
        command_data = data.get('data', '')
        parameters = data.get('parameters', {})
        
        if not device_id or not action:
            return jsonify({'error': 'Missing device_id or action'}), 400
        
        # Create command in format ESP32 expects
        esp32_command = {
            'commandId': str(uuid.uuid4()),
            'action': action,
            'timestamp': datetime.now().isoformat()
        }
        
        # Add parameters if provided (for LED commands)
        if parameters:
            esp32_command['parameters'] = parameters
        
        # Add data if provided (for other commands)
        if command_data:
            esp32_command['parameters'] = {'filename': command_data}
        
        # Store command for tracking
        command_record = {
            'id': esp32_command['commandId'],
            'device_id': device_id,
            'action': action,
            'data': command_data,
            'parameters': parameters,
            'timestamp': datetime.now().isoformat()
        }
        active_commands[command_record['id']] = command_record
        command_history.append(command_record)
        
        # Send to device (if connected)
        if device_id in devices:
            device = devices[device_id]
            try:
                # Send UDP command in ESP32 format
                command_json = json.dumps(esp32_command)
                if server.udp_socket:
                    server.udp_socket.sendto(
                        command_json.encode('utf-8'),
                        (device['ip_address'], CONFIG['udp_port'])
                    )
                print(f"Sent command to {device_id}: {action}")
                print(f"Command JSON: {command_json}")
                
            except Exception as e:
                print(f"Failed to send command to {device_id}: {e}")
                return jsonify({'error': f'Failed to send command: {e}'}), 500
        
        return jsonify({'status': 'sent', 'command_id': esp32_command['commandId']})
        
    except Exception as e:
        print(f"Command error: {e}")
        return jsonify({'error': str(e)}), 500

@socketio.on('connect')
def handle_connect():
    """Handle client connection"""
    print('Client connected')
    emit('devices', list(devices.values()))

@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection"""
    print('Client disconnected')

def run_udp_server():
    """Run UDP server in background thread"""
    server.start_udp_listener()

if __name__ == '__main__':
    print("Starting Enhanced Tricorder Control Server with ESP32 Simulator...")
    print(f"Web interface: http://localhost:{CONFIG['web_port']}")
    print(f"ESP32 Simulator: http://localhost:{CONFIG['web_port']}/simulator")
    print(f"UDP listener: port {CONFIG['udp_port']}")
    
    # Start UDP server in background
    udp_thread = threading.Thread(target=run_udp_server, daemon=True)
    udp_thread.start()
    
    # Run Flask app
    socketio.run(app, host='0.0.0.0', port=CONFIG['web_port'], debug=False)
