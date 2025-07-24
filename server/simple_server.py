#!/usr/bin/env python3
"""
Simplified Tricorder Control Server
Streamlined server for testing ESP32 tricorder functionality
"""

import asyncio
import socket
import json
import time
import uuid
from datetime import datetime
from typing import Dict, List, Optional
from flask import Flask, render_template, request, jsonify, send_file
from flask_socketio import SocketIO, emit
import threading
import ipaddress
import requests
import os
from werkzeug.utils import secure_filename

# Flask app setup
app = Flask(__name__)
app.config['SECRET_KEY'] = 'tricorder_control_secret'
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size
socketio = SocketIO(app, cors_allowed_origins="*")

# Create uploads directory if it doesn't exist
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

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

class TricorderServer:
    def __init__(self):
        self.udp_socket = None
        self.running = False
        
    def start_udp_listener(self):
        """Start UDP listener for device communication"""
        try:
            self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_socket.bind(('', CONFIG["udp_port"]))
            self.udp_socket.settimeout(1.0)  # Non-blocking with timeout
            self.running = True
            
            print(f"UDP listener started on port {CONFIG['udp_port']}")
            
            while self.running:
                try:
                    data, addr = self.udp_socket.recvfrom(4096)
                    self.handle_udp_message(data.decode('utf-8'), addr)
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"UDP listener error: {e}")
                    
        except Exception as e:
            print(f"Failed to start UDP listener: {e}")
    
    def handle_udp_message(self, message: str, addr: tuple):
        """Handle incoming UDP message from tricorder device"""
        try:
            data = json.loads(message)
            ip_address = addr[0]
            
            # Check if this is a response to a command
            if 'commandId' in data:
                command_id = data['commandId']
                device_id = data.get('deviceId', 'UNKNOWN')
                
                # Update device info
                if device_id != 'UNKNOWN':
                    self.update_device_info(device_id, ip_address, data)
                
                # Handle command response
                if command_id in active_commands:
                    active_commands[command_id]['response'] = data
                    active_commands[command_id]['completed'] = True
                    active_commands[command_id]['response_time'] = time.time()
                
                # Add to command history
                history_entry = {
                    'timestamp': datetime.now().isoformat(),
                    'device_id': device_id,
                    'command_id': command_id,
                    'response': data,
                    'ip_address': ip_address
                }
                command_history.append(history_entry)
                
                # Keep only last 100 entries
                if len(command_history) > 100:
                    command_history.pop(0)
                
                # Broadcast to web clients
                socketio.emit('device_response', history_entry)
                
                print(f"Received response from {device_id} ({ip_address}): {data}")
            
        except json.JSONDecodeError:
            print(f"Invalid JSON from {addr}: {message}")
        except Exception as e:
            print(f"Error handling UDP message: {e}")
    
    def update_device_info(self, device_id: str, ip_address: str, data: Dict):
        """Update device information"""
        if device_id not in devices:
            devices[device_id] = {
                'device_id': device_id,
                'first_seen': datetime.now().isoformat(),
                'command_count': 0
            }
        
        devices[device_id].update({
            'ip_address': ip_address,
            'last_seen': datetime.now().isoformat(),
            'firmware_version': data.get('firmwareVersion'),
            'wifi_connected': data.get('wifiConnected'),
            'free_heap': data.get('freeHeap'),
            'uptime': data.get('uptime'),
            'sd_card_initialized': data.get('sdCardInitialized'),
            'video_playing': data.get('videoPlaying'),
            'current_video': data.get('currentVideo'),
            'video_looping': data.get('videoLooping'),
            'current_frame': data.get('currentFrame')
        })
        
        # Broadcast device update to web clients
        socketio.emit('device_update', devices[device_id])
    
    def send_command(self, device_id: str, action: str, parameters: Dict = None) -> str:
        """Send command to tricorder device"""
        if device_id not in devices:
            raise Exception(f"Device {device_id} not found")
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': action,
            'parameters': parameters or {}
        }
        
        # Store active command
        active_commands[command_id] = {
            'device_id': device_id,
            'command': command,
            'sent_time': time.time(),
            'completed': False,
            'response': None
        }
        
        # Send UDP command
        message = json.dumps(command)
        self.udp_socket.sendto(message.encode('utf-8'), (ip_address, CONFIG["udp_port"]))
        
        devices[device_id]['command_count'] += 1
        
        print(f"Sent command to {device_id} ({ip_address}): {action}")
        return command_id
    
    def stop(self):
        """Stop the server"""
        self.running = False
        if self.udp_socket:
            self.udp_socket.close()
    
    def discover_devices(self):
        """Scan local network for tricorder devices"""
        if not self.udp_socket:
            return
        
        # Get local IP address
        try:
            # Connect to a remote address to get local IP
            temp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            temp_socket.connect(("8.8.8.8", 80))
            local_ip = temp_socket.getsockname()[0]
            temp_socket.close()
            
            # Create network range (assuming /24 subnet)
            network = ipaddress.IPv4Network(f"{local_ip}/24", strict=False)
            
            print(f"Scanning network {network} for tricorder devices...")
            
            # Send status commands to common IP ranges
            for ip in network.hosts():
                ip_str = str(ip)
                try:
                    # Send a status command to each IP
                    command_id = str(uuid.uuid4())
                    command = {
                        'commandId': command_id,
                        'action': 'status',
                        'parameters': {}
                    }
                    
                    message = json.dumps(command)
                    self.udp_socket.sendto(message.encode('utf-8'), (ip_str, CONFIG["udp_port"]))
                    
                except Exception:
                    # Ignore errors for unreachable IPs
                    pass
            
            print("Discovery scan completed")
            
        except Exception as e:
            print(f"Discovery error: {e}")
    
    def start_discovery_timer(self):
        """Start periodic device discovery"""
        def discovery_loop():
            while self.running:
                self.discover_devices()
                time.sleep(30)  # Discover every 30 seconds
        
        discovery_thread = threading.Thread(target=discovery_loop, daemon=True)
        discovery_thread.start()

# Global server instance
server = TricorderServer()

# Web routes
@app.route('/')
def index():
    """Main web interface"""
    return render_template('index.html')

@app.route('/api/devices')
def get_devices():
    """Get list of discovered devices"""
    return jsonify(list(devices.values()))

@app.route('/api/devices/<device_id>/status')
def get_device_status(device_id):
    """Get device status"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    try:
        command_id = server.send_command(device_id, 'status')
        return jsonify({'command_id': command_id})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/play_video', methods=['POST'])
def play_video(device_id):
    """Play video on device"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    data = request.get_json()
    filename = data.get('filename')
    loop = data.get('loop', False)
    
    if not filename:
        return jsonify({'error': 'Filename required'}), 400
    
    try:
        command_id = server.send_command(device_id, 'play_video', {
            'filename': filename,
            'loop': loop
        })
        return jsonify({'command_id': command_id})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/stop_video', methods=['POST'])
def stop_video(device_id):
    """Stop video on device"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    try:
        command_id = server.send_command(device_id, 'stop_video')
        return jsonify({'command_id': command_id})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/list_videos')
def list_videos(device_id):
    """List videos on device"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    try:
        command_id = server.send_command(device_id, 'list_videos')
        return jsonify({'command_id': command_id})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/led_color', methods=['POST'])
def set_led_color(device_id):
    """Set LED color on device"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    data = request.get_json()
    r = data.get('r', 0)
    g = data.get('g', 0)
    b = data.get('b', 0)
    
    try:
        command_id = server.send_command(device_id, 'set_led_color', {
            'r': r, 'g': g, 'b': b
        })
        return jsonify({'command_id': command_id})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/led_brightness', methods=['POST'])
def set_led_brightness(device_id):
    """Set LED brightness on device"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    data = request.get_json()
    brightness = data.get('brightness', 128)
    
    try:
        command_id = server.send_command(device_id, 'set_led_brightness', {
            'brightness': brightness
        })
        return jsonify({'command_id': command_id})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/discover')
def discover_devices():
    """Manually trigger device discovery"""
    try:
        server.discover_devices()
        return jsonify({'status': 'Discovery scan initiated'})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/add_device', methods=['POST'])
def add_device():
    """Manually add a device by IP address"""
    data = request.get_json()
    ip_address = data.get('ip_address')
    
    if not ip_address:
        return jsonify({'error': 'IP address required'}), 400
    
    try:
        # Send a status command to the device to register it
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': 'status',
            'parameters': {}
        }
        
        message = json.dumps(command)
        server.udp_socket.sendto(message.encode('utf-8'), (ip_address, CONFIG["udp_port"]))
        
        return jsonify({'status': f'Attempted to contact device at {ip_address}'})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/commands')
def get_command_history():
    """Get command history"""
    return jsonify(command_history[-50:])  # Last 50 commands

@app.route('/api/commands/<command_id>')
def get_command_status(command_id):
    """Get command status"""
    if command_id not in active_commands:
        return jsonify({'error': 'Command not found'}), 404
    
    return jsonify(active_commands[command_id])

# Firmware Update Routes
@app.route('/api/firmware/upload', methods=['POST'])
def upload_firmware():
    """Upload firmware file"""
    try:
        if 'firmware' not in request.files:
            return jsonify({'error': 'No firmware file provided'}), 400
        
        file = request.files['firmware']
        if file.filename == '':
            return jsonify({'error': 'No file selected'}), 400
        
        if not file.filename.endswith('.bin'):
            return jsonify({'error': 'File must be a .bin firmware file'}), 400
        
        # Save the uploaded file
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        return jsonify({
            'success': True,
            'filename': filename,
            'size': os.path.getsize(filepath),
            'path': filepath
        })
    
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/firmware/update', methods=['POST'])
def update_device_firmware(device_id):
    """Update firmware on a specific device"""
    try:
        if device_id not in devices:
            return jsonify({'error': 'Device not found'}), 404
        
        data = request.get_json()
        firmware_file = data.get('firmware_file')
        
        if not firmware_file:
            return jsonify({'error': 'No firmware file specified'}), 400
        
        firmware_path = os.path.join(app.config['UPLOAD_FOLDER'], firmware_file)
        if not os.path.exists(firmware_path):
            return jsonify({'error': 'Firmware file not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        # Upload firmware to device via HTTP
        try:
            with open(firmware_path, 'rb') as f:
                files = {'firmware': (firmware_file, f, 'application/octet-stream')}
                response = requests.post(f'http://{ip_address}/update', files=files, timeout=60)
                
                if response.status_code == 200:
                    return jsonify({
                        'success': True,
                        'message': 'Firmware update initiated successfully',
                        'device_id': device_id,
                        'firmware_file': firmware_file
                    })
                else:
                    return jsonify({
                        'error': f'Device returned error: {response.status_code}',
                        'message': response.text
                    }), 500
        
        except requests.exceptions.Timeout:
            return jsonify({'error': 'Firmware update timed out (device may be restarting)'}), 408
        except requests.exceptions.ConnectionError:
            return jsonify({'error': 'Could not connect to device for firmware update'}), 503
        except Exception as e:
            return jsonify({'error': f'Firmware update failed: {str(e)}'}), 500
    
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/firmware/list')
def list_firmware_files():
    """List available firmware files"""
    try:
        firmware_files = []
        upload_dir = app.config['UPLOAD_FOLDER']
        
        if os.path.exists(upload_dir):
            for filename in os.listdir(upload_dir):
                if filename.endswith('.bin'):
                    filepath = os.path.join(upload_dir, filename)
                    stat = os.stat(filepath)
                    firmware_files.append({
                        'filename': filename,
                        'size': stat.st_size,
                        'modified': datetime.fromtimestamp(stat.st_mtime).isoformat(),
                        'path': filepath
                    })
        
        return jsonify({'firmware_files': firmware_files})
    
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/ota_status')
def get_ota_status(device_id):
    """Get OTA update status for a device"""
    try:
        if device_id not in devices:
            return jsonify({'error': 'Device not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        try:
            # Try to connect to device's OTA web interface
            response = requests.get(f'http://{ip_address}/', timeout=5)
            ota_available = response.status_code == 200
        except:
            ota_available = False
        
        return jsonify({
            'device_id': device_id,
            'ota_available': ota_available,
            'ip_address': ip_address,
            'firmware_version': device.get('firmware_version', 'Unknown')
        })
    
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# WebSocket events
@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    print(f"Web client connected")
    emit('devices', list(devices.values()))
    emit('command_history', command_history[-20:])

@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print(f"Web client disconnected")

@socketio.on('send_command')
def handle_send_command(data):
    """Handle command from web interface"""
    try:
        device_id = data['device_id']
        action = data['action']
        parameters = data.get('parameters', {})
        
        command_id = server.send_command(device_id, action, parameters)
        emit('command_sent', {'command_id': command_id})
        
    except Exception as e:
        emit('error', {'message': str(e)})

def start_udp_server():
    """Start UDP server in background thread"""
    server.start_udp_listener()

def start_discovery():
    """Start device discovery in background thread"""
    time.sleep(5)  # Wait for UDP server to start
    server.start_discovery_timer()

if __name__ == '__main__':
    # Start UDP listener in background thread
    udp_thread = threading.Thread(target=start_udp_server, daemon=True)
    udp_thread.start()
    
    # Start device discovery in background thread
    discovery_thread = threading.Thread(target=start_discovery, daemon=True)
    discovery_thread.start()
    
    print(f"Starting Tricorder Control Server...")
    print(f"UDP listener: port {CONFIG['udp_port']}")
    print(f"Web interface: http://localhost:{CONFIG['web_port']}")
    print(f"Device discovery: scanning every 30 seconds")
    
    try:
        # Start web server
        socketio.run(app, host='0.0.0.0', port=CONFIG['web_port'], debug=True)
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.stop()
