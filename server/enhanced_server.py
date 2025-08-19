#!/usr/bin/env python3
"""
Enhanced Simple Server with sACN Data Viewer
Prop control server with sACN monitoring capabilities
"""

import asyncio
import socket
import json
import time
import uuid
import threading
import ipaddress
import requests
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
        
    # Configure tricorder to respond to RGB channels for 4 total LEDs:
    # Channels 1-3: Front NeoPixel strip LED #1 (RGB)
    # Channels 4-6: Front NeoPixel strip LED #2 (RGB) 
    # Channels 7-9: Front NeoPixel strip LED #3 (RGB)
    # Channels 10-12: Onboard LED (RGB)
    try:
        # Remove any existing configuration
        sacn_receiver.remove_device(device_id)
        
        # Add device with 4 LEDs total (3 NeoPixel strip + 1 onboard)
        success = sacn_receiver.add_device(
            device_id=device_id,
            ip_address=device_info['ip_address'],
            universe=CONFIG['sacn_universe'],  # Use configured universe
            start_channel=1,  # Start at channel 1 for first LED
            num_leds=4,  # 4 total LEDs (3 NeoPixel + 1 onboard)
            builtin_led_channels=None,  # Onboard LED is now part of the 4-LED array
            device_type="tricorder"  # Mark as tricorder
        )
        
        if success:
            print(f"✅ Auto-configured {device_id} for sACN RGB control (4 LEDs: 3 NeoPixel strip + 1 onboard)")
        else:
            print(f"⚠️ Failed to auto-configure {device_id} for sACN")
            
    except Exception as e:
        print(f"❌ Error auto-configuring {device_id} for sACN: {e}")

def auto_configure_polyinoculator_for_sacn(device_id: str, device_info: dict):
    """Automatically configure a polyinoculator for sACN LED control when it connects"""
    if not SACN_AVAILABLE:
        return
        
    sacn_receiver = get_sacn_receiver()
    if not sacn_receiver:
        return
    
    # Ensure command callback is set up for sACN processing
    def sacn_command_callback(device_id: str, action: str, params: dict):
        command_id = str(uuid.uuid4())
        send_udp_command_to_device(device_id, action, params, command_id)
    set_command_callback(sacn_command_callback)
        
    # Get device's sACN universe and LED count
    universe = device_info.get('sacn_universe', 1)
    num_leds = device_info.get('num_leds', 12)
    
    try:
        # Remove any existing configuration
        sacn_receiver.remove_device(device_id)
        
        # Add polyinoculator device with command-based control like tricorders
        # Each LED gets 3 channels (RGB), starting from channel 1
        success = sacn_receiver.add_device(
            device_id=device_id,
            ip_address=device_info['ip_address'],
            universe=universe,
            start_channel=1,  # Start at channel 1
            num_leds=num_leds,  # 12 LEDs by default
            builtin_led_channels=None,  # No built-in LED control
            device_type="polyinoculator"  # Mark as polyinoculator
        )
        
        if success:
            print(f"✅ Auto-configured {device_id} for sACN control (universe {universe}, {num_leds} LEDs)")
        else:
            print(f"⚠️ Failed to auto-configure {device_id} for sACN")
            
    except Exception as e:
        print(f"❌ Error auto-configuring {device_id} for sACN: {e}")
    
    print(f"📡 {device_id} configured for command-based sACN processing on universe {universe}")

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
            
            # Process messages from ESP32 devices
            # Accept legacy format (TRICORDER_, POLYINOCULATOR_) and new format (TRIC, POLY)
            is_tricorder = (device_id.startswith('TRICORDER_') or device_id.startswith('TRIC'))
            is_polyinoculator = (device_id.startswith('POLYINOCULATOR_') or device_id.startswith('POLY'))
            
            if not (is_tricorder or is_polyinoculator):
                print(f"🚫 Ignoring unsupported device: {device_id} from {addr[0]}")
                return
            
            # Determine device type
            device_type = 'tricorder' if is_tricorder else 'polyinoculator'
            
            # Update device registry with comprehensive info
            devices[device_id] = {
                'device_id': device_id,
                'device_type': device_type,
                'device_label': data.get('deviceLabel', device_id),  # Use deviceLabel if available, fallback to device_id
                'fixture_number': data.get('fixtureNumber', 1),  # Default to fixture 1 if not specified
                'ip_address': addr[0],
                'port': addr[1],
                'last_seen': datetime.now().isoformat(),
                'status': 'online',
                # Common ESP32 fields
                'firmware_version': data.get('firmwareVersion'),
                'wifi_connected': data.get('wifiConnected'),
                'free_heap': data.get('freeHeap'),
                'uptime': data.get('uptime'),
                **data  # Include any additional fields
            }
            
            # Add device-specific fields
            if device_type == 'tricorder':
                devices[device_id].update({
                    'sd_card_initialized': data.get('sdCardInitialized'),
                    'video_playing': data.get('videoPlaying'),
                    'current_video': data.get('currentVideo'),
                    'video_looping': data.get('videoLooping'),
                    'current_frame': data.get('currentFrame'),
                    'battery_voltage': data.get('batteryVoltage'),
                    'battery_percentage': data.get('batteryPercentage'),
                    'battery_status': data.get('batteryStatus'),
                })
            elif device_type == 'polyinoculator':
                devices[device_id].update({
                    'num_leds': data.get('numLeds', 12),
                    'brightness': data.get('brightness', 128),
                    'sacn_enabled': data.get('sacnEnabled', True),
                    'sacn_universe': data.get('sacnUniverse', 1),
                })
            
            print(f"✓ Updated device: {device_id} ({device_type}) at {addr[0]}")
            
            # Auto-configure device for sACN control (only if not already configured)
            if device_id not in devices or 'sacn_configured' not in devices[device_id]:
                print(f"🔧 Configuring {device_id} for sACN...")
                if device_type == 'tricorder':
                    auto_configure_tricorder_for_sacn(device_id, devices[device_id])
                elif device_type == 'polyinoculator':
                    auto_configure_polyinoculator_for_sacn(device_id, devices[device_id])
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

def cleanup_offline_devices():
    """Remove devices that haven't been seen within the timeout period"""
    global devices
    current_time = datetime.now()
    timeout_seconds = CONFIG['device_timeout']
    
    offline_devices = []
    
    for device_id, device_info in list(devices.items()):
        try:
            last_seen_str = device_info.get('last_seen')
            if not last_seen_str:
                continue
                
            last_seen = datetime.fromisoformat(last_seen_str.replace('Z', '+00:00'))
            # Handle timezone-naive datetime by assuming local timezone
            if last_seen.tzinfo is None:
                last_seen = last_seen.replace(tzinfo=None)
                current_time_local = current_time.replace(tzinfo=None)
            else:
                current_time_local = current_time
            
            time_diff = (current_time_local - last_seen).total_seconds()
            
            if time_diff > timeout_seconds:
                offline_devices.append(device_id)
                
        except (ValueError, TypeError) as e:
            print(f"⚠️ Error parsing last_seen for {device_id}: {e}")
            # If we can't parse the timestamp, consider it old and remove it
            offline_devices.append(device_id)
    
    # Remove offline devices
    for device_id in offline_devices:
        device_info = devices.pop(device_id, {})
        print(f"🔌 Removed offline device: {device_id} (last seen: {device_info.get('last_seen', 'unknown')})")
        
        # Remove from sACN receiver if configured
        if SACN_AVAILABLE:
            sacn_receiver = get_sacn_receiver()
            if sacn_receiver:
                sacn_receiver.remove_device(device_id)
        
        # Notify web clients
        socketio.emit('device_removed', {
            'device_id': device_id,
            'reason': 'timeout',
            'last_seen': device_info.get('last_seen')
        })
    
    if offline_devices:
        print(f"🧹 Cleanup completed: removed {len(offline_devices)} offline devices")
        # Emit updated device list
        socketio.emit('devices_update', list(devices.values()))

def device_cleanup_task():
    """Background task to periodically clean up offline devices"""
    while True:
        try:
            cleanup_offline_devices()
            time.sleep(CONFIG['device_timeout'] // 2)  # Check every half timeout period
        except Exception as e:
            print(f"❌ Error in device cleanup task: {e}")
            time.sleep(10)  # Wait 10 seconds before retrying

@app.route('/')
def index():
    """Main web interface - serve enhanced dashboard"""
    try:
        with open('web/enhanced-prop-dashboard.html', 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        # Fallback to basic interface if enhanced dashboard not found
        return basic_interface()

def basic_interface():
    """Fallback basic interface"""
    return '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Prop Control System</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
            .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🔺 Prop Control System</h1>
            <p>Enhanced dashboard not found. Please check that web/enhanced-prop-dashboard.html exists.</p>
            <p>Server is running on port ''' + str(CONFIG['web_port']) + '''</p>
        </div>
    </body>
    </html>
    '''

@app.route('/api/devices')
def get_devices():
    """Get all connected devices"""
    return jsonify(list(devices.values()))

@app.route('/api/devices/cleanup', methods=['POST'])
def manual_device_cleanup():
    """Manually trigger device cleanup"""
    try:
        cleanup_offline_devices()
        return jsonify({
            'success': True,
            'message': 'Device cleanup completed',
            'active_devices': len(devices),
            'device_timeout': CONFIG['device_timeout']
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'message': str(e)
        })

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
    
    # Handle LED commands specially - ESP32 expects RGB values at top level
    if action in ['set_led_color', 'set_builtin_led'] and parameters:
        # Flatten LED parameters to top level for ESP32 compatibility
        if 'r' in parameters:
            esp32_command['r'] = parameters['r']
        if 'g' in parameters:
            esp32_command['g'] = parameters['g'] 
        if 'b' in parameters:
            esp32_command['b'] = parameters['b']
        print(f"sACN LED command to {device_id}: R={parameters.get('r')}, G={parameters.get('g')}, B={parameters.get('b')}")
    elif parameters:
        # For non-LED commands, use parameters object
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

@app.route('/api/battery/<device_id>', methods=['GET'])
def get_device_battery(device_id):
    """Get battery status for a specific device"""
    try:
        if device_id not in devices:
            return jsonify({'error': 'Device not found'}), 404
        
        device = devices[device_id]
        
        # Send battery command to device
        command_id = str(uuid.uuid4())
        esp32_command = {
            'commandId': command_id,
            'action': 'get_battery',
            'timestamp': datetime.now().isoformat()
        }
        
        # Send UDP command
        command_json = json.dumps(esp32_command)
        if server.udp_socket:
            server.udp_socket.sendto(
                command_json.encode('utf-8'),
                (device['ip_address'], CONFIG['udp_port'])
            )
        
        # Wait for response (simplified - in production you'd want better response handling)
        time.sleep(0.1)
        
        # Return battery info from device status if available
        if 'batteryPercentage' in device:
            return jsonify({
                'device_id': device_id,
                'battery_voltage': device.get('batteryVoltage', 0),
                'battery_percentage': device.get('batteryPercentage', 0),
                'battery_status': device.get('batteryStatus', 'Unknown'),
                'timestamp': datetime.now().isoformat()
            })
        else:
            return jsonify({
                'device_id': device_id,
                'error': 'Battery information not available',
                'timestamp': datetime.now().isoformat()
            })
        
    except Exception as e:
        print(f"Battery request error: {e}")
        return jsonify({'error': str(e)}), 500

# ==========================================
# Device Configuration API Endpoints
# ==========================================

@app.route('/api/config/<device_id>', methods=['GET'])
def get_device_config(device_id):
    """Get device configuration"""
    try:
        if device_id not in devices:
            return jsonify({'error': f'Device {device_id} not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        # Send HTTP request to device's /api/config endpoint
        response = requests.get(f"http://{ip_address}/api/config", timeout=5)
        
        if response.status_code == 200:
            config_data = response.json()
            return jsonify({
                'device_id': device_id,
                'config': config_data,
                'retrieved_at': datetime.now().isoformat()
            })
        else:
            return jsonify({'error': f'Device returned status {response.status_code}'}), 500
            
    except requests.RequestException as e:
        print(f"Config request error: {e}")
        return jsonify({'error': f'Failed to connect to device: {str(e)}'}), 500
    except Exception as e:
        print(f"Config request error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/config/<device_id>', methods=['POST'])
def set_device_config(device_id):
    """Update device configuration"""
    try:
        if device_id not in devices:
            return jsonify({'error': f'Device {device_id} not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        # Get configuration data from request
        config_data = request.get_json()
        if not config_data:
            return jsonify({'error': 'No configuration data provided'}), 400
        
        # Send HTTP POST request to device's /api/config endpoint
        response = requests.post(
            f"http://{ip_address}/api/config", 
            json=config_data,
            headers={'Content-Type': 'application/json'},
            timeout=10
        )
        
        if response.status_code == 200:
            result = response.json()
            
            # Broadcast config update to web clients
            socketio.emit('device_config_updated', {
                'device_id': device_id,
                'config': config_data,
                'result': result,
                'updated_at': datetime.now().isoformat()
            })
            
            return jsonify({
                'device_id': device_id,
                'config': config_data,
                'result': result,
                'updated_at': datetime.now().isoformat()
            })
        else:
            error_msg = f'Device returned status {response.status_code}'
            try:
                error_data = response.json()
                if 'error' in error_data:
                    error_msg = error_data['error']
            except:
                pass
            return jsonify({'error': error_msg}), 500
            
    except requests.RequestException as e:
        print(f"Config update error: {e}")
        return jsonify({'error': f'Failed to connect to device: {str(e)}'}), 500
    except Exception as e:
        print(f"Config update error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/config/<device_id>/factory_reset', methods=['POST'])
def factory_reset_device(device_id):
    """Factory reset device configuration"""
    try:
        if device_id not in devices:
            return jsonify({'error': f'Device {device_id} not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        # Send HTTP POST request to device's /api/factory-reset endpoint
        response = requests.post(f"http://{ip_address}/api/factory-reset", timeout=10)
        
        if response.status_code == 200:
            result = response.json()
            
            # Broadcast factory reset notification to web clients
            socketio.emit('device_factory_reset', {
                'device_id': device_id,
                'result': result,
                'reset_at': datetime.now().isoformat()
            })
            
            return jsonify({
                'device_id': device_id,
                'result': result,
                'reset_at': datetime.now().isoformat()
            })
        else:
            error_msg = f'Device returned status {response.status_code}'
            try:
                error_data = response.json()
                if 'error' in error_data:
                    error_msg = error_data['error']
            except:
                pass
            return jsonify({'error': error_msg}), 500
            
    except requests.RequestException as e:
        print(f"Factory reset error: {e}")
        return jsonify({'error': f'Failed to connect to device: {str(e)}'}), 500
    except Exception as e:
        print(f"Factory reset error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/config/<device_id>/restart', methods=['POST'])
def restart_device(device_id):
    """Restart device"""
    try:
        if device_id not in devices:
            return jsonify({'error': f'Device {device_id} not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        # Send HTTP POST request to device's /api/restart endpoint
        response = requests.post(f"http://{ip_address}/api/restart", timeout=5)
        
        if response.status_code == 200:
            result = response.json()
            
            # Broadcast restart notification to web clients
            socketio.emit('device_restart', {
                'device_id': device_id,
                'result': result,
                'restart_at': datetime.now().isoformat()
            })
            
            return jsonify({
                'device_id': device_id,
                'result': result,
                'restart_at': datetime.now().isoformat()
            })
        else:
            return jsonify({'error': f'Device returned status {response.status_code}'}), 500
            
    except requests.RequestException as e:
        print(f"Restart error: {e}")
        return jsonify({'error': f'Failed to connect to device: {str(e)}'}), 500
    except Exception as e:
        print(f"Restart error: {e}")
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
    print("Starting Prop Control Server with sACN Data Viewer...")
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
    
    # Start device cleanup task in background
    cleanup_thread = threading.Thread(target=device_cleanup_task, daemon=True)
    cleanup_thread.start()
    print(f"Device cleanup task started (timeout: {CONFIG['device_timeout']}s)")
    
    # Run Flask app
    socketio.run(app, host='0.0.0.0', port=CONFIG['web_port'], debug=False)
