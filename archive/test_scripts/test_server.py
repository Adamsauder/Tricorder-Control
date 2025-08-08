#!/usr/bin/env python3
"""
Minimal test server to debug discovery issues
"""

import socket
import json
import uuid
from flask import Flask, jsonify, request
import threading
import time

app = Flask(__name__)

# Global UDP socket
udp_socket = None
devices = {}

def init_udp():
    global udp_socket
    try:
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.bind(('', 8888))
        udp_socket.settimeout(1.0)
        print("UDP socket initialized on port 8888")
        return True
    except Exception as e:
        print(f"Failed to initialize UDP socket: {e}")
        return False

def listen_udp():
    global udp_socket
    while True:
        try:
            if udp_socket:
                data, addr = udp_socket.recvfrom(4096)
                message = data.decode('utf-8')
                print(f"Received from {addr}: {message}")
                
                # Parse and store device info
                try:
                    response = json.loads(message)
                    if 'deviceId' in response:
                        device_id = response['deviceId']
                        devices[device_id] = {
                            'device_id': device_id,
                            'ip_address': addr[0],
                            'last_response': response,
                            'last_seen': time.time()
                        }
                        print(f"Updated device: {device_id} at {addr[0]}")
                except:
                    pass
                    
        except socket.timeout:
            continue
        except Exception as e:
            print(f"UDP error: {e}")
            time.sleep(1)

@app.route('/api/discover')
def discover():
    print("Discovery endpoint called")
    if not udp_socket:
        return jsonify({'error': 'UDP socket not initialized'}), 500
    
    try:
        # Test with known device
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': 'status',
            'parameters': {}
        }
        
        message = json.dumps(command)
        udp_socket.sendto(message.encode('utf-8'), ('192.168.1.48', 8888))
        print(f"Sent discovery command: {message}")
        
        return jsonify({'status': 'Discovery command sent to 192.168.1.48'})
    except Exception as e:
        print(f"Discovery error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/add_device', methods=['POST'])
def add_device():
    print("Add device endpoint called")
    data = request.get_json()
    print(f"Request data: {data}")
    
    ip_address = data.get('ip_address')
    if not ip_address:
        return jsonify({'error': 'IP address required'}), 400
    
    if not udp_socket:
        return jsonify({'error': 'UDP socket not initialized'}), 500
    
    try:
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': 'status',
            'parameters': {}
        }
        
        message = json.dumps(command)
        udp_socket.sendto(message.encode('utf-8'), (ip_address, 8888))
        print(f"Sent command to {ip_address}: {message}")
        
        return jsonify({'status': f'Command sent to {ip_address}'})
    except Exception as e:
        print(f"Add device error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices')
def get_devices():
    return jsonify(list(devices.values()))

@app.route('/')
def index():
    return '''
<!DOCTYPE html>
<html>
<head><title>Test Server</title></head>
<body>
    <h1>Tricorder Test Server</h1>
    <button onclick="testDiscover()">Test Discover</button>
    <button onclick="testAddDevice()">Test Add Device (192.168.1.48)</button>
    <div id="result"></div>
    <div id="devices"></div>
    
    <script>
    function testDiscover() {
        fetch('/api/discover')
        .then(r => r.json())
        .then(data => {
            document.getElementById('result').innerHTML = JSON.stringify(data);
            updateDevices();
        });
    }
    
    function testAddDevice() {
        fetch('/api/add_device', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ip_address: '192.168.1.48'})
        })
        .then(r => r.json())
        .then(data => {
            document.getElementById('result').innerHTML = JSON.stringify(data);
            setTimeout(updateDevices, 1000);
        });
    }
    
    function updateDevices() {
        fetch('/api/devices')
        .then(r => r.json())
        .then(data => {
            document.getElementById('devices').innerHTML = '<h3>Devices:</h3>' + JSON.stringify(data, null, 2);
        });
    }
    
    setInterval(updateDevices, 2000);
    </script>
</body>
</html>
    '''

if __name__ == '__main__':
    print("Starting test server...")
    
    if init_udp():
        # Start UDP listener thread
        udp_thread = threading.Thread(target=listen_udp, daemon=True)
        udp_thread.start()
        print("UDP listener thread started")
        
        # Start web server
        print("Starting web server on port 5000...")
        app.run(host='0.0.0.0', port=5000, debug=True)
    else:
        print("Failed to start - UDP socket initialization failed")
