#!/usr/bin/env python3
"""
Simple Tricorder Control Server
Basic UDP command server for device management and image display control
"""

import asyncio
import socket
import json
import time
import uuid
import threading
from datetime import datetime
from typing import Dict, Optional
from flask import Flask, jsonify, request
from flask_socketio import SocketIO, emit

# Flask app setup
app = Flask(__name__)
app.config['SECRET_KEY'] = 'tricorder_control_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

# Configuration
CONFIG = {
    "udp_port": 8888,
    "web_port": 8080,
    "device_timeout": 30,  # seconds
    "command_timeout": 5,  # seconds
}

# Global state
devices: Dict[str, Dict] = {}
active_commands: Dict[str, Dict] = {}

class TricorderServer:
    def __init__(self):
        self.udp_socket = None
        self.running = False
        
    def start_udp_listener(self):
        """Start UDP listener for device communication"""
        try:
            self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_socket.bind(("0.0.0.0", CONFIG["udp_port"]))
            self.running = True
            print(f"UDP listener started on port {CONFIG['udp_port']}")
            
            while self.running:
                try:
                    data, addr = self.udp_socket.recvfrom(4096)
                    message = data.decode('utf-8')
                    print(f"📡 Received from {addr}: {message}")
                    
                    # Process the message
                    self.process_device_message(message, addr)
                    
                except Exception as e:
                    if self.running:
                        print(f"Error in UDP listener: {e}")
                        
        except Exception as e:
            print(f"Failed to start UDP listener: {e}")
    
    def process_device_message(self, message: str, addr: tuple):
        """Process incoming device messages"""
        try:
            data = json.loads(message)
            device_id = data.get('deviceId')
            
            if not device_id:
                return
            
            # Update device info
            device_info = {
                'device_id': device_id,
                'ip_address': addr[0],
                'last_seen': datetime.now().isoformat(),
                'type': data.get('type', 'unknown'),
                'status': 'online'
            }
            
            # Merge with existing data
            if device_id in devices:
                devices[device_id].update(device_info)
            else:
                devices[device_id] = device_info
            
            # Update with all received data
            devices[device_id].update(data)
            
            # Emit update to web clients
            socketio.emit('device_update', devices[device_id])
            
            # Handle command responses
            if 'commandId' in data:
                command_id = data['commandId']
                if command_id in active_commands:
                    print(f"✓ Command response: {data.get('result', 'No result')}")
                    socketio.emit('device_response', {
                        'device_id': device_id,
                        'command_id': command_id,
                        'result': data.get('result', 'No result')
                    })
                    
        except json.JSONDecodeError:
            print(f"Invalid JSON from {addr}: {message}")
        except Exception as e:
            print(f"Error processing message from {addr}: {e}")

# Initialize server
server = TricorderServer()

def cleanup_offline_devices():
    """Remove devices that haven't been seen within the timeout period"""
    current_time = datetime.now()
    timeout_seconds = CONFIG['device_timeout']
    
    offline_devices = []
    for device_id, device in list(devices.items()):
        try:
            last_seen = datetime.fromisoformat(device['last_seen'])
            if (current_time - last_seen).total_seconds() > timeout_seconds:
                offline_devices.append(device_id)
        except (KeyError, ValueError):
            offline_devices.append(device_id)
    
    for device_id in offline_devices:
        print(f"🔌 Removed offline device: {device_id}")
        del devices[device_id]
    
    if offline_devices:
        print(f"🧹 Cleanup completed: removed {len(offline_devices)} offline devices")

def device_cleanup_task():
    """Background task to periodically clean up offline devices"""
    while True:
        time.sleep(30)  # Check every 30 seconds
        cleanup_offline_devices()

def send_udp_command_to_device(device_id: str, action: str, parameters: dict, command_id: str):
    """Send UDP command to specific device"""
    if device_id not in devices:
        print(f"❌ Device {device_id} not found")
        return False
    
    device = devices[device_id]
    ip_address = device.get('ip_address')
    
    if not ip_address:
        print(f"❌ No IP address for device {device_id}")
        return False
    
    try:
        # Create command
        command = {
            'commandId': command_id,
            'action': action,
            'timestamp': datetime.now().isoformat(),
        }
        
        if parameters:
            command['parameters'] = parameters
        
        # Send via UDP
        command_json = json.dumps(command)
        print(f"Sent command to {device_id}: {action}")
        print(f"Command JSON: {command_json}")
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(command_json.encode(), (ip_address, CONFIG["udp_port"]))
        sock.close()
        
        # Track command
        active_commands[command_id] = {
            'device_id': device_id,
            'action': action,
            'timestamp': time.time()
        }
        
        return True
        
    except Exception as e:
        print(f"❌ Failed to send command to {device_id}: {e}")
        return False

@app.route('/')
def index():
    """Main web interface - serve enhanced dashboard"""
    try:
        with open('web/enhanced-prop-dashboard.html', 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        return jsonify({"error": "Web interface not found"}), 404

@app.route('/api/devices')
def get_devices():
    """Get all connected devices"""
    return jsonify(list(devices.values()))

@app.route('/api/command', methods=['POST'])
def send_command():
    """Send command to device"""
    try:
        data = request.get_json()
        device_id = data.get('device_id')
        action = data.get('action')
        parameters = data.get('parameters', {})
        
        if not device_id or not action:
            return jsonify({"error": "Missing device_id or action"}), 400
        
        command_id = str(uuid.uuid4())
        success = send_udp_command_to_device(device_id, action, parameters, command_id)
        
        if success:
            return jsonify({"success": True, "command_id": command_id})
        else:
            return jsonify({"error": "Failed to send command"}), 500
            
    except Exception as e:
        print(f"Error in send_command: {e}")
        return jsonify({"error": str(e)}), 500

@socketio.on('connect')
def handle_connect():
    """Handle client connection"""
    print(f"🔌 Client connected")
    emit('device_list', list(devices.values()))

@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection"""
    print(f"🔌 Client disconnected")

@socketio.on('send_command')
def handle_send_command(data):
    """Handle command from web interface"""
    try:
        device_id = data.get('device_id')
        action = data.get('action')
        parameters = data.get('parameters', {})
        
        if not device_id or not action:
            emit('error', {'message': 'Missing device_id or action'})
            return
        
        command_id = str(uuid.uuid4())
        success = send_udp_command_to_device(device_id, action, parameters, command_id)
        
        if success:
            emit('command_sent', {'command_id': command_id})
        else:
            emit('error', {'message': 'Failed to send command'})
            
    except Exception as e:
        print(f"Error in handle_send_command: {e}")
        emit('error', {'message': str(e)})

if __name__ == '__main__':
    print("Starting Tricorder Control Server...")
    print(f"Web interface: http://localhost:{CONFIG['web_port']}")
    print(f"UDP listener: port {CONFIG['udp_port']}")
    
    # Start UDP listener in background thread
    udp_thread = threading.Thread(target=server.start_udp_listener, daemon=True)
    udp_thread.start()
    
    # Start device cleanup task
    cleanup_thread = threading.Thread(target=device_cleanup_task, daemon=True)
    cleanup_thread.start()
    
    # Start Flask-SocketIO server
    socketio.run(app, host='0.0.0.0', port=CONFIG['web_port'], debug=False)
