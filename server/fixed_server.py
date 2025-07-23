#!/usr/bin/env python3
"""
Fixed Tricorder Control Server
Working version with proper UDP handling
"""

import socket
import json
import uuid
import time
import threading
from datetime import datetime
from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit

# Flask app setup
app = Flask(__name__)
app.config['SECRET_KEY'] = 'tricorder_control_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

# Global state
devices = {}
udp_socket = None
running = False

def start_udp_listener():
    """Start UDP listener for device communication"""
    global udp_socket, running
    
    try:
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.bind(('', 8888))
        udp_socket.settimeout(1.0)
        running = True
        
        print("✓ UDP listener started on port 8888")
        
        while running:
            try:
                data, addr = udp_socket.recvfrom(4096)
                message = data.decode('utf-8')
                print(f"📡 Received from {addr}: {message}")
                
                # Parse JSON response
                try:
                    response = json.loads(message)
                    if 'deviceId' in response:
                        device_id = response['deviceId']
                        ip_address = addr[0]
                        
                        # Update device info
                        devices[device_id] = {
                            'device_id': device_id,
                            'ip_address': ip_address,
                            'last_seen': datetime.now().isoformat(),
                            'firmware_version': response.get('firmwareVersion'),
                            'wifi_connected': response.get('wifiConnected'),
                            'free_heap': response.get('freeHeap'),
                            'uptime': response.get('uptime'),
                            'sd_card_initialized': response.get('sdCardInitialized'),
                            'video_playing': response.get('videoPlaying'),
                            'current_video': response.get('currentVideo'),
                            'video_looping': response.get('videoLooping'),
                            'current_frame': response.get('currentFrame')
                        }
                        
                        print(f"✓ Updated device: {device_id} at {ip_address}")
                        
                        # Broadcast device update to web clients
                        socketio.emit('device_update', devices[device_id])
                        print(f"📡 Emitted device_update for {device_id}")
                        
                        # Also broadcast the raw response for command handling
                        socketio.emit('device_response', {
                            'device_id': device_id,
                            'response': response
                        })
                        print(f"📡 Emitted device_response for {device_id}: {response.get('result', 'No result')}")
                        
                except json.JSONDecodeError:
                    print(f"⚠️ Invalid JSON from {addr}: {message}")
                    
            except socket.timeout:
                continue
            except Exception as e:
                print(f"❌ UDP error: {e}")
                
    except Exception as e:
        print(f"❌ Failed to start UDP listener: {e}")

def send_command_to_device(ip_address, command):
    """Send UDP command to device"""
    try:
        if udp_socket:
            message = json.dumps(command)
            udp_socket.sendto(message.encode('utf-8'), (ip_address, 8888))
            print(f"📤 Sent to {ip_address}: {message}")
            return True
    except Exception as e:
        print(f"❌ Failed to send command: {e}")
        return False

# Web routes
@app.route('/')
def index():
    """Main web interface"""
    return render_template('index.html')

@app.route('/debug')
def debug():
    """Debug web interface"""
    return render_template('debug.html')

@app.route('/api/devices')
def get_devices():
    """Get list of discovered devices"""
    device_list = list(devices.values())
    print(f"📋 API: Returning {len(device_list)} devices")
    return jsonify(device_list)

@app.route('/api/discover')
def discover_devices():
    """Manually trigger device discovery"""
    try:
        print("🔍 Discovery scan initiated")
        
        # Send status command to known device
        command = {
            'commandId': str(uuid.uuid4()),
            'action': 'status',
            'parameters': {}
        }
        
        # Try the known IP
        if send_command_to_device('192.168.1.48', command):
            return jsonify({'status': 'Discovery scan initiated - sent command to 192.168.1.48'})
        else:
            return jsonify({'error': 'Failed to send discovery command'}), 500
            
    except Exception as e:
        print(f"❌ Discovery error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/add_device', methods=['POST'])
def add_device():
    """Manually add a device by IP address"""
    data = request.get_json()
    ip_address = data.get('ip_address')
    
    if not ip_address:
        return jsonify({'error': 'IP address required'}), 400
    
    try:
        print(f"➕ Adding device at {ip_address}")
        
        command = {
            'commandId': str(uuid.uuid4()),
            'action': 'status',
            'parameters': {}
        }
        
        if send_command_to_device(ip_address, command):
            return jsonify({'status': f'Status command sent to {ip_address}'})
        else:
            return jsonify({'error': f'Failed to contact {ip_address}'}), 500
            
    except Exception as e:
        print(f"❌ Add device error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices/<device_id>/command', methods=['POST'])
def send_device_command(device_id):
    """Send command to specific device"""
    if device_id not in devices:
        return jsonify({'error': 'Device not found'}), 404
    
    data = request.get_json()
    action = data.get('action')
    parameters = data.get('parameters', {})
    
    device = devices[device_id]
    ip_address = device['ip_address']
    
    command = {
        'commandId': str(uuid.uuid4()),
        'action': action,
        'parameters': parameters
    }
    
    if send_command_to_device(ip_address, command):
        return jsonify({'status': f'Command sent to {device_id}'})
    else:
        return jsonify({'error': 'Failed to send command'}), 500

# WebSocket events
@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    print(f"🌐 Web client connected")
    emit('devices', list(devices.values()))

@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print(f"🌐 Web client disconnected")

@socketio.on('send_command')
def handle_send_command(data):
    """Handle command from web interface"""
    try:
        device_id = data['device_id']
        action = data['action']
        parameters = data.get('parameters', {})
        
        print(f"🌐 WebSocket command: {action} for {device_id}")
        print(f"🌐 Available devices: {list(devices.keys())}")
        
        if device_id not in devices:
            print(f"❌ Device {device_id} not found in devices list")
            emit('error', {'message': f'Device {device_id} not found'})
            return
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        command = {
            'commandId': str(uuid.uuid4()),
            'action': action,
            'parameters': parameters
        }
        
        print(f"📤 Sending {action} command to {ip_address}")
        
        if send_command_to_device(ip_address, command):
            emit('command_sent', {'command_id': command['commandId']})
            print(f"✅ Command sent successfully")
        else:
            emit('error', {'message': 'Failed to send command'})
            print(f"❌ Failed to send command")
            
    except Exception as e:
        print(f"❌ WebSocket command error: {e}")
        emit('error', {'message': str(e)})

if __name__ == '__main__':
    print("🚀 Starting Tricorder Control Server...")
    
    # Start UDP listener in background thread
    udp_thread = threading.Thread(target=start_udp_listener, daemon=True)
    udp_thread.start()
    
    # Wait a moment for UDP to start
    time.sleep(1)
    
    try:
        print("🌐 Starting web server on port 5000...")
        socketio.run(app, host='0.0.0.0', port=5000, debug=False)
    except KeyboardInterrupt:
        print("\n🛑 Shutting down server...")
        running = False
        if udp_socket:
            udp_socket.close()
