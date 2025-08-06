#!/usr/bin/env python3
"""
Enhanced Simple Server with sACN Data Viewer
Tricorder control server with sACN monitoring capabilities
"""

import asyncio
import socket
import json
import time
import uuid
import threading
import ipaddress
from datetime import datetime
from typing import Dict, List, Optional
from flask import Flask, render_template, request, jsonify, send_file, abort
from flask_socketio import SocketIO, emit

# Optional import for network interface detection
try:
    import psutil
except ImportError:
    psutil = None

# Import sACN controller
from sacn_controller import (
    SACNReceiver, set_command_callback,
    initialize_sacn_receiver, get_sacn_receiver, SACN_AVAILABLE
)

# Flask app setup
app = Flask(__name__)
app.config['SECRET_KEY'] = 'tricorder_control_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

# Configuration
CONFIG = {
    "udp_port": 8888,
    "web_port": 8080,  # Changed to match web frontend proxy configuration
    "device_timeout": 30,  # seconds
    "command_timeout": 5,  # seconds
    "sacn_enabled": True,  # Enable sACN integration
    "sacn_universe": 1,  # Default sACN universe
    "sacn_fps": 30,  # sACN update rate
}

# Global state
devices: Dict[str, Dict] = {}
active_commands: Dict[str, Dict] = {}
command_history: List[Dict] = []
server_ip: Optional[str] = None
server_start_time = time.time()

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

def auto_configure_tricorder_for_sacn(device_id: str, device_info: dict):
    """Automatically configure a tricorder for sACN LED control when it connects"""
    if not SACN_AVAILABLE:
        return
        
    sacn_receiver = get_sacn_receiver()
    if not sacn_receiver:
        return
        
    # Configure tricorder to respond to RGB channels 1, 2, 3 for built-in LED
    # and LED strip (if present)
    # TEMPORARILY DISABLED FOR LED TESTING
    # try:
    #     # Remove any existing configuration
    #     sacn_receiver.remove_device(device_id)
    #     
    #     # Add device with RGB channels 1, 2, 3 for both built-in LED and LED strip
    #     success = sacn_receiver.add_device(
    #         device_id=device_id,
    #         ip_address=device_info['ip_address'],
    #         universe=CONFIG['sacn_universe'],  # Use configured universe
    #         start_channel=1,  # Not used for uniform strip control
    #         num_leds=3,  # Enable LED strip with 3 LEDs
    #         builtin_led_channels=(1, 2, 3)  # RGB on channels 1, 2, 3 for both built-in and strip
    #     )
        
    #     if success:
    #         print(f"✅ Auto-configured {device_id} for sACN RGB control (channels 1,2,3 - built-in LED + LED strip)")
    #     else:
    #         print(f"⚠️ Failed to auto-configure {device_id} for sACN")
    #         
    # except Exception as e:
    #     print(f"❌ Error auto-configuring {device_id} for sACN: {e}")
    
    print(f"⚠ sACN auto-configuration temporarily disabled for {device_id} - testing direct LED control")

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
            
            # Auto-configure tricorder for sACN LED control (only if not already configured)
            if device_id not in devices or 'sacn_configured' not in devices[device_id]:
                print(f"🔧 Configuring {device_id} for sACN...")
                auto_configure_tricorder_for_sacn(device_id, devices[device_id])
                devices[device_id]['sacn_configured'] = True
                print(f"✅ {device_id} sACN configuration complete")
            else:
                print(f"📝 {device_id} already configured for sACN")
            
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
                <h2>📡 sACN Data Viewer</h2>
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 15px;">
                    <div>
                        <label for="sacnInterface"><strong>Network Interface:</strong></label>
                        <select id="sacnInterface" style="width: 100%; padding: 8px; margin-top: 5px;" onchange="updateSACNInterface()">
                            <option value="">Select Network Interface...</option>
                        </select>
                    </div>
                    <div>
                        <label for="sacnUniverse"><strong>sACN Universe:</strong></label>
                        <input type="number" id="sacnUniverse" min="1" max="63999" value="1" style="width: 100%; padding: 8px; margin-top: 5px;" onchange="updateSACNUniverse()">
                    </div>
                </div>
                <div style="margin-bottom: 15px;">
                    <span id="sacnStatus" style="padding: 5px 10px; border-radius: 15px; background: #ff4444; color: white; font-size: 0.9em;">sACN Disconnected</span>
                    <button onclick="toggleSACN()" id="sacnToggle" style="background: #4CAF50; margin-left: 10px;">Enable sACN</button>
                    <button onclick="refreshSACNData()" style="background: #2196F3; margin-left: 10px;">Refresh Data</button>
                </div>
                <div id="sacnDataTable" style="max-height: 400px; overflow-y: auto; border: 1px solid #ddd; border-radius: 4px;">
                    <p style="text-align: center; color: #666; padding: 20px;">Enable sACN and select a universe to view DMX data</p>
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
                        <p><strong>🎭 sACN:</strong> <span style="color: #4CAF50;">RGB Channels 1,2,3 (Built-in LED + LED Strip)</span></p>
                        <div style="margin-top: 10px;">
                            <strong>📺 Image Controls:</strong><br>
                            <button onclick="sendBootScreen('${device.device_id}')">Play Startup</button>
                            <button onclick="sendImageCommand('${device.device_id}', 'greenscreen.jpg')" style="background: #00ff00; color: black; margin: 2px;">🟩 Green Screen</button>
                            <button onclick="sendImageCommand('${device.device_id}', 'test.jpg')" style="background: #ffcc00; color: black; margin: 2px;">📺 Test Card</button>
                            <button onclick="sendImageCommand('${device.device_id}', 'test2.jpg')" style="background: #ff6600; color: white; margin: 2px;">📺 Test2</button>
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
                            <br>
                            <button onclick="sendDiagnostic('${device.device_id}')" style="background: #ff9900; margin: 2px;">🔧 LED Diagnostic</button>
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
            
            function sendBootScreen(deviceId) {
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_id: deviceId,
                        action: 'display_boot_screen',
                        parameters: {}
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
                
                // Send built-in LED command
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
                    console.log('Built-in LED command response:', data);
                    if (data.status === 'sent') {
                        console.log(`✅ Built-in LED color command sent to ${deviceId}`);
                    }
                })
                .catch(error => {
                    console.error('Built-in LED command error:', error);
                });
                
                // Also send LED strip command
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_id: deviceId,
                        action: 'set_led_color',
                        parameters: {
                            r: r,
                            g: g,
                            b: b
                        }
                    })
                })
                .then(response => response.json())
                .then(data => {
                    console.log('LED strip command response:', data);
                    if (data.status === 'sent') {
                        console.log(`✅ LED strip color command sent to ${deviceId}`);
                    }
                })
                .catch(error => {
                    console.error('LED strip command error:', error);
                });
            }
            
            function sendDiagnostic(deviceId) {
                console.log(`Running LED diagnostic for ${deviceId}`);
                
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_id: deviceId,
                        action: 'led_diagnostic',
                        parameters: {}
                    })
                })
                .then(response => response.json())
                .then(data => {
                    console.log('LED diagnostic response:', data);
                    if (data.status === 'sent') {
                        console.log(`✅ LED diagnostic command sent to ${deviceId}`);
                        alert('LED diagnostic started! Check the device LEDs and server console for details.');
                    }
                })
                .catch(error => {
                    console.error('LED diagnostic error:', error);
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
            
            // sACN Control Functions
            let sacnEnabled = false;
            
            // Load network interfaces when page loads
            window.addEventListener('load', function() {
                loadDevices();
                loadNetworkInterfaces();
                updateSACNStatus();
                setInterval(updateSACNStatus, 5000); // Update sACN status every 5 seconds
                setInterval(() => {
                    if (sacnEnabled) {
                        refreshSACNData();
                    }
                }, 2000); // Refresh sACN data every 2 seconds when enabled
            });
            
            function loadNetworkInterfaces() {
                fetch('/api/sacn/interfaces')
                .then(response => response.json())
                .then(data => {
                    const select = document.getElementById('sacnInterface');
                    select.innerHTML = '<option value="">Select Network Interface...</option>';
                    
                    if (data.interfaces) {
                        data.interfaces.forEach(iface => {
                            const option = document.createElement('option');
                            option.value = iface.name;
                            option.textContent = `${iface.name} (${iface.ip})`;
                            if (iface.default) {
                                option.selected = true;
                            }
                            select.appendChild(option);
                        });
                    }
                })
                .catch(error => {
                    console.error('Failed to load network interfaces:', error);
                });
            }
            
            function updateSACNStatus() {
                fetch('/api/sacn/status')
                .then(response => response.json())
                .then(data => {
                    const statusElement = document.getElementById('sacnStatus');
                    const toggleButton = document.getElementById('sacnToggle');
                    
                    sacnEnabled = data.running || false;
                    
                    if (sacnEnabled) {
                        statusElement.textContent = `sACN Receiver Running`;
                        statusElement.style.background = '#4CAF50';
                        toggleButton.textContent = 'Disable sACN';
                        toggleButton.style.background = '#f44336';
                    } else {
                        statusElement.textContent = 'sACN Receiver Stopped';
                        statusElement.style.background = '#ff4444';
                        toggleButton.textContent = 'Enable sACN';
                        toggleButton.style.background = '#4CAF50';
                    }
                })
                .catch(error => {
                    console.error('Failed to update sACN status:', error);
                });
            }
            
            function refreshSACNData() {
                const universe = parseInt(document.getElementById('sacnUniverse').value) || 1;
                
                fetch(`/api/sacn/universe/${universe}/data`)
                .then(response => response.json())
                .then(data => {
                    if (data.error) {
                        document.getElementById('sacnDataTable').innerHTML = 
                            `<p style="text-align: center; color: #ff4444; padding: 20px;">${data.error}</p>`;
                        return;
                    }
                    
                    let tableHTML = `
                        <div style="margin-bottom: 15px; padding: 10px; background: #f8f9fa; border-radius: 4px;">
                            <strong>🎭 Tricorder RGB Control:</strong>
                            <div style="display: inline-block; margin-left: 15px;">
                                <span style="background: #ff4444; color: white; padding: 2px 8px; border-radius: 3px; margin-right: 5px;">
                                    Ch1 (Red): ${data.data[0] || 0}
                                </span>
                                <span style="background: #44ff44; color: black; padding: 2px 8px; border-radius: 3px; margin-right: 5px;">
                                    Ch2 (Green): ${data.data[1] || 0}
                                </span>
                                <span style="background: #4444ff; color: white; padding: 2px 8px; border-radius: 3px;">
                                    Ch3 (Blue): ${data.data[2] || 0}
                                </span>
                            </div>
                        </div>
                        <table style="width: 100%; border-collapse: collapse; font-family: monospace; font-size: 12px;">
                            <thead>
                                <tr style="background: #f5f5f5; position: sticky; top: 0;">
                                    <th style="border: 1px solid #ddd; padding: 8px;">Channel</th>
                                    <th style="border: 1px solid #ddd; padding: 8px;">Value</th>
                                    <th style="border: 1px solid #ddd; padding: 8px;">Hex</th>
                                    <th style="border: 1px solid #ddd; padding: 8px;">%</th>
                                    <th style="border: 1px solid #ddd; padding: 8px;">Color</th>
                                </tr>
                            </thead>
                            <tbody>
                    `;
                    
                    for (let i = 0; i < Math.min(data.data.length, 100); i++) {
                        const value = data.data[i];
                        const percentage = Math.round((value / 255) * 100);
                        const hex = value.toString(16).padStart(2, '0').toUpperCase();
                        const grayValue = Math.round((value / 255) * 255);
                        
                        // Highlight RGB control channels (1, 2, 3)
                        const isRgbChannel = (i + 1) <= 3;
                        const channelBg = isRgbChannel ? 
                            (i === 0 ? '#ffe6e6' : i === 1 ? '#e6ffe6' : '#e6e6ff') : 
                            (value > 0 ? '#f0f8ff' : '');
                        const channelLabel = isRgbChannel ? 
                            (i === 0 ? ' (R)' : i === 1 ? ' (G)' : ' (B)') : '';
                        
                        tableHTML += `
                            <tr style="background: ${channelBg};">
                                <td style="border: 1px solid #ddd; padding: 4px; text-align: center; font-weight: ${isRgbChannel ? 'bold' : 'normal'};">${i + 1}${channelLabel}</td>
                                <td style="border: 1px solid #ddd; padding: 4px; text-align: center; font-weight: ${value > 0 ? 'bold' : 'normal'};">${value}</td>
                                <td style="border: 1px solid #ddd; padding: 4px; text-align: center;">${hex}</td>
                                <td style="border: 1px solid #ddd; padding: 4px; text-align: center;">${percentage}%</td>
                                <td style="border: 1px solid #ddd; padding: 4px;">
                                    <div style="width: 20px; height: 16px; background: rgb(${grayValue},${grayValue},${grayValue}); border: 1px solid #ccc;"></div>
                                </td>
                            </tr>
                        `;
                    }
                    
                    tableHTML += `
                            </tbody>
                        </table>
                        <p style="text-align: center; margin: 10px; color: #666; font-size: 11px;">
                            Universe ${universe} - Showing channels 1-${Math.min(data.data.length, 100)} of ${data.channels} total
                        </p>
                    `;
                    
                    document.getElementById('sacnDataTable').innerHTML = tableHTML;
                })
                .catch(error => {
                    console.error('Failed to refresh sACN data:', error);
                    document.getElementById('sacnDataTable').innerHTML = 
                        '<p style="text-align: center; color: #ff4444; padding: 20px;">Failed to load sACN data</p>';
                });
            }
            
            function updateSACNInterface() {
                const interface = document.getElementById('sacnInterface').value;
                if (!interface) return;
                
                fetch('/api/sacn/interface', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ interface: interface })
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        console.log('sACN interface updated:', interface);
                        updateSACNStatus();
                    } else {
                        alert('Failed to update sACN interface: ' + data.message);
                    }
                })
                .catch(error => {
                    console.error('Failed to update sACN interface:', error);
                    alert('Failed to update sACN interface');
                });
            }
            
            function updateSACNUniverse() {
                const universe = parseInt(document.getElementById('sacnUniverse').value);
                if (isNaN(universe) || universe < 1 || universe > 63999) {
                    alert('Universe must be between 1 and 63999');
                    return;
                }
                
                fetch('/api/sacn/universe', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ universe: universe })
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        console.log('sACN universe updated:', universe);
                        updateSACNStatus();
                    } else {
                        alert('Failed to update sACN universe: ' + data.message);
                    }
                })
                .catch(error => {
                    console.error('Failed to update sACN universe:', error);
                    alert('Failed to update sACN universe');
                });
            }
            
            function toggleSACN() {
                const networkInterface = document.getElementById('networkInterface').value;
                const universe = parseInt(document.getElementById('sacnUniverse').value) || 1;
                
                if (!networkInterface) {
                    alert('Please select a network interface');
                    return;
                }
                
                if (universe < 1 || universe > 63999) {
                    alert('Universe must be between 1 and 63999');
                    return;
                }
                
                fetch('/api/sacn/toggle', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        interface: networkInterface,
                        universe: universe
                    })
                })
                .then(response => response.json())
                .then(data => {
                    updateSACNStatus();
                    if (data.running) {
                        refreshSACNData();
                        console.log('✅ sACN enabled - Tricorders will respond to RGB channels 1,2,3');
                    } else {
                        document.getElementById('sacnDataTable').innerHTML = 
                            '<p style="text-align: center; color: #666; padding: 20px;">sACN receiver stopped</p>';
                    }
                })
                .catch(error => console.error('sACN toggle error:', error));
            }
        </script>
    </body>
    </html>
    '''

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

def send_udp_command_to_device(device_id: str, action: str, parameters: dict, command_id: Optional[str] = None):
    """
    Utility function to send UDP command to a device
    """
    if command_id is None:
        command_id = str(uuid.uuid4())
        
    if device_id not in devices:
        return False
        
    device = devices[device_id]
    
    # Create command in format ESP32 expects
    esp32_command = {
        'commandId': command_id,
        'action': action,
        'timestamp': datetime.now().isoformat()
    }
    
    if parameters:
        esp32_command['parameters'] = parameters
        
    try:
        # Send UDP command
        command_json = json.dumps(esp32_command)
        if server.udp_socket:
            server.udp_socket.sendto(
                command_json.encode('utf-8'),
                (device['ip_address'], CONFIG['udp_port'])
            )
        print(f"Sent UDP command to {device_id}: {action}")
        return True
    except Exception as e:
        print(f"Failed to send UDP command to {device_id}: {e}")
        return False

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
        
        # Handle LED commands specially - ESP32 expects RGB values at top level
        if action in ['set_led_color', 'set_builtin_led'] and parameters:
            # Flatten LED parameters to top level for ESP32 compatibility
            if 'r' in parameters:
                esp32_command['r'] = parameters['r']
            if 'g' in parameters:
                esp32_command['g'] = parameters['g'] 
            if 'b' in parameters:
                esp32_command['b'] = parameters['b']
            print(f"LED command flattened: R={parameters.get('r')}, G={parameters.get('g')}, B={parameters.get('b')}")
        elif parameters:
            # For non-LED commands, use parameters object
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

# ==========================================
# sACN (E1.31) API Endpoints
# ==========================================

@app.route('/api/sacn/status', methods=['GET'])
def sacn_status():
    """Get sACN receiver status"""
    sacn_receiver = get_sacn_receiver()
    if not sacn_receiver:
        return jsonify({
            'enabled': False,
            'available': SACN_AVAILABLE,
            'error': 'sACN receiver not initialized'
        })
    
    return jsonify({
        'enabled': True,
        'available': SACN_AVAILABLE,
        **sacn_receiver.get_status()
    })

@app.route('/api/sacn/device', methods=['POST'])
def sacn_add_device():
    """Add device to sACN control"""
    sacn_receiver = get_sacn_receiver()
    if not sacn_receiver:
        return jsonify({'error': 'sACN receiver not available'}), 400
    
    try:
        data = request.get_json()
        device_id = data.get('device_id')
        ip_address = data.get('ip_address')
        universe = data.get('universe', CONFIG['sacn_universe'])
        start_channel = data.get('start_channel', 1)
        num_leds = data.get('num_leds', 3)
        builtin_led_channels = data.get('builtin_led_channels')  # [r, g, b] channels
        
        if not device_id or not ip_address:
            return jsonify({'error': 'Missing device_id or ip_address'}), 400
        
        # Convert to tuple if provided
        if builtin_led_channels and len(builtin_led_channels) == 3:
            builtin_led_channels = tuple(builtin_led_channels)
        else:
            builtin_led_channels = None
            
        success = sacn_receiver.add_device(
            device_id, ip_address, universe, start_channel, 
            num_leds, builtin_led_channels
        )
        
        if success:
            return jsonify({'message': f'Device {device_id} added to sACN control'})
        else:
            return jsonify({'error': 'Failed to add device to sACN'}), 400
            
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/sacn/device/<device_id>', methods=['DELETE'])
def sacn_remove_device(device_id):
    """Remove device from sACN control"""
    sacn_receiver = get_sacn_receiver()
    if not sacn_receiver:
        return jsonify({'error': 'sACN receiver not available'}), 400
    
    success = sacn_receiver.remove_device(device_id)
    if success:
        return jsonify({'message': f'Device {device_id} removed from sACN control'})
    else:
        return jsonify({'error': 'Device not found'}), 404

@app.route('/api/sacn/universe/<int:universe>/data', methods=['GET'])
def sacn_get_universe_data(universe):
    """Get current DMX data for a universe"""
    sacn_receiver = get_sacn_receiver()
    if not sacn_receiver:
        return jsonify({'error': 'sACN receiver not available'}), 400
    
    dmx_data = sacn_receiver.get_universe_data(universe)
    if dmx_data is None:
        return jsonify({'error': f'No data received for universe {universe}'}), 404
    
    return jsonify({
        'universe': universe,
        'channels': len(dmx_data),
        'data': dmx_data[:50]  # Return first 50 channels to avoid huge responses
    })

# Note: Color and effect control endpoints removed since we are now a receiver
# Colors and effects are controlled by lighting consoles sending sACN data
# The receiver automatically forwards received DMX data to tricorder devices

# === Main Application Routes ===

# Additional sACN API endpoints for integrated control
@app.route('/api/sacn/interfaces')
def get_network_interfaces():
    """Get available network interfaces for sACN"""
    try:
        if psutil is None:
            # Fallback if psutil is not available
            return jsonify({
                'success': True,
                'interfaces': [{
                    'name': 'Default',
                    'ip': get_server_ip(),
                    'default': True
                }]
            })
            
        interfaces = []
        
        for interface_name, addresses in psutil.net_if_addrs().items():
            for addr in addresses:
                if addr.family == socket.AF_INET and not addr.address.startswith('127.'):
                    interfaces.append({
                        'name': interface_name,
                        'ip': addr.address,
                        'default': addr.address == get_server_ip()
                    })
                    break
        
        return jsonify({
            'success': True,
            'interfaces': interfaces
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

@app.route('/api/sacn/interface', methods=['POST'])
def set_sacn_interface():
    """Set the network interface for sACN receiver"""
    try:
        data = request.get_json()
        interface = data.get('interface')
        
        if not interface:
            return jsonify({'success': False, 'message': 'Interface name required'})
        
        # For sACN receiver, interface is set during initialization
        # This would require restarting the receiver with new interface
        # For now, just acknowledge the request
        return jsonify({
            'success': True,
            'message': f'Interface setting noted: {interface} (requires receiver restart)'
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

@app.route('/api/sacn/universe', methods=['POST'])
def set_sacn_universe():
    """Set the sACN universe"""
    try:
        data = request.get_json()
        universe = data.get('universe', 1)
        
        if not isinstance(universe, int) or universe < 1 or universe > 63999:
            return jsonify({'success': False, 'message': 'Universe must be between 1 and 63999'})
        
        # Update configuration
        CONFIG['sacn_universe'] = universe
        
        # For receiver, devices can be configured for different universes
        # No need to restart receiver, just note the configuration
        return jsonify({
            'success': True,
            'message': f'Default universe set to {universe}'
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

@app.route('/api/sacn/toggle', methods=['POST'])
def toggle_sacn():
    """Toggle sACN receiver with interface and universe configuration"""
    try:
        data = request.get_json()
        interface = data.get('interface')
        universe = data.get('universe', 1)
        
        if not SACN_AVAILABLE:
            return jsonify({
                'success': False,
                'message': 'sACN receiver not available'
            })
        
        receiver = get_sacn_receiver()
        
        # If receiver is running, stop it
        if receiver and receiver.running:
            receiver.stop()
            return jsonify({
                'success': True,
                'running': False,
                'message': 'sACN receiver stopped'
            })
        else:
            # Start or restart receiver
            if not receiver:
                receiver = initialize_sacn_receiver("0.0.0.0")
                # Set command callback
                def sacn_command_callback(device_id: str, action: str, params: dict):
                    command_id = str(uuid.uuid4())
                    send_udp_command_to_device(device_id, action, params, command_id)
                set_command_callback(sacn_command_callback)
            
            # Update universe configuration
            CONFIG['sacn_universe'] = universe
            
            if receiver.start():
                # Re-configure all connected tricorders for the new universe
                for device_id, device_info in devices.items():
                    auto_configure_tricorder_for_sacn(device_id, device_info)
                
                return jsonify({
                    'success': True,
                    'running': True,
                    'universe': universe,
                    'interface': interface,
                    'message': f'sACN receiver started on universe {universe}'
                })
            else:
                return jsonify({
                    'success': False,
                    'running': False,
                    'message': 'Failed to start sACN receiver'
                })
                
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

@app.route('/api/sacn/enable', methods=['POST'])
def enable_sacn():
    """Enable sACN receiver"""
    try:
        if not SACN_AVAILABLE:
            return jsonify({
                'success': False,
                'message': 'sACN receiver not available'
            })
        
        receiver = get_sacn_receiver()
        if not receiver:
            # Initialize receiver if not already done
            receiver = initialize_sacn_receiver("0.0.0.0")
            # Set command callback
            def sacn_command_callback(device_id: str, action: str, params: dict):
                command_id = str(uuid.uuid4())
                send_udp_command_to_device(device_id, action, params, command_id)
            set_command_callback(sacn_command_callback)
        
        if receiver:
            receiver.start()
            return jsonify({
                'success': True,
                'message': 'sACN receiver enabled'
            })
        else:
            return jsonify({
                'success': False,
                'message': 'Failed to initialize sACN receiver'
            })
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

@app.route('/api/sacn/disable', methods=['POST'])
def disable_sacn():
    """Disable sACN receiver"""
    try:
        if SACN_AVAILABLE:
            receiver = get_sacn_receiver()
            if receiver:
                receiver.stop()
        
        return jsonify({
            'success': True,
            'message': 'sACN receiver disabled'
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

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
    print("Starting Tricorder Control Server with sACN Data Viewer...")
    print(f"Web interface: http://localhost:{CONFIG['web_port']}")
    print(f"UDP listener: port {CONFIG['udp_port']}")
    
    # Initialize sACN receiver if enabled
    if CONFIG['sacn_enabled'] and SACN_AVAILABLE:
        sacn_receiver = initialize_sacn_receiver("0.0.0.0")  # Listen on all interfaces
        
        # Set command callback to send commands to devices
        def sacn_command_callback(device_id: str, action: str, params: dict):
            """Send sACN-received commands to tricorder devices via UDP"""
            command_id = str(uuid.uuid4())
            send_udp_command_to_device(device_id, action, params, command_id)
        
        set_command_callback(sacn_command_callback)
        
        if sacn_receiver.start():
            print(f"sACN receiver started on port 5568")
        else:
            print("Failed to start sACN receiver - using UDP only")
    else:
        if CONFIG['sacn_enabled']:
            print("sACN library not available - install with: pip install sacn")
        print("sACN disabled - using UDP only")
    
    # Start UDP server in background
    udp_thread = threading.Thread(target=run_udp_server, daemon=True)
    udp_thread.start()
    
    # Run Flask app
    socketio.run(app, host='0.0.0.0', port=CONFIG['web_port'], debug=False)
