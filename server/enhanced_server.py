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
import os
from datetime import datetime
from typing import Dict, List, Optional
from flask import Flask, render_template, request, jsonify, send_file, abort
from flask_socketio import SocketIO, emit

# Optional import for network interface detection
try:
    import psutil
except ImportError:
    psutil = None

# sACN availability check
SACN_AVAILABLE = False  # sACN disabled for this configuration

def get_sacn_receiver():
    """Get the global sACN receiver instance"""
    return getattr(app, 'sacn_receiver', None)

# Flask app setup
app = Flask(__name__, static_folder='../web/dist', static_url_path='/static')
app.config['SECRET_KEY'] = 'tricorder_control_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

# Configuration
CONFIG = {
    "udp_port": 8888,
    "web_port": 8080,  # Changed to match web frontend proxy configuration
    "device_timeout": 30,  # seconds
    "command_timeout": 5,  # seconds
    "sacn_enabled": False,  # Disable sACN integration for basic use
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
    """Placeholder for sACN configuration - disabled"""
    print(f"📡 sACN auto-configuration disabled for {device_id}")

def auto_configure_polyinoculator_for_sacn(device_id: str, device_info: dict):
    """Placeholder for sACN configuration - disabled"""  
    print(f"📡 sACN auto-configuration disabled for {device_id}")

class TricorderServer:
    def __init__(self):
        self.udp_socket = None
        
    def get_reachable_server_ip(self, device_ip: str) -> str:
        """Get the server IP that can reach the device's subnet"""
        try:
            import subprocess
            import platform
            
            # Get all network interfaces
            if platform.system() == "Windows":
                result = subprocess.run(['ipconfig'], capture_output=True, text=True)
                lines = result.stdout.split('\n')
                
                # Parse Windows ipconfig output to find interface IPs
                interfaces = []
                current_interface = None
                for line in lines:
                    line = line.strip()
                    if "adapter" in line.lower():
                        current_interface = line
                    elif "IPv4 Address" in line and ":" in line:
                        ip = line.split(":")[-1].strip()
                        if ip and not ip.startswith("127."):
                            interfaces.append(ip)
            else:
                # Linux/Mac approach
                result = subprocess.run(['hostname', '-I'], capture_output=True, text=True)
                interfaces = [ip.strip() for ip in result.stdout.split() if ip.strip()]
            
            # Find the interface that's on the same subnet as the device
            device_subnet = ".".join(device_ip.split(".")[:-1])  # e.g., "192.168.0"
            
            for interface_ip in interfaces:
                if interface_ip.startswith(device_subnet):
                    return interface_ip
                    
            # If no matching subnet found, return the first non-loopback interface
            for interface_ip in interfaces:
                if not interface_ip.startswith("127."):
                    return interface_ip
                    
            # Fallback to the original method
            return get_server_ip()
            
        except Exception as e:
            print(f"⚠️ Error determining reachable server IP: {e}")
            return get_server_ip()
        
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
    
    def send_discovery_response(self, addr: tuple):
        """Send discovery response to device"""
        try:
            # Determine the best server IP to respond with based on the requesting device's subnet
            device_ip = addr[0]
            server_ip = self.get_reachable_server_ip(device_ip)
            
            response = {
                "action": "server_discovery_response",
                "server_ip": server_ip,
                "server_port": CONFIG["udp_port"],
                "timestamp": datetime.now().isoformat()
            }
            
            response_json = json.dumps(response)
            udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            udp_socket.sendto(response_json.encode(), (addr[0], 8888))
            udp_socket.close()
            
            print(f"📤 Discovery response sent to {addr[0]}: {response_json}")
            
        except Exception as e:
            print(f"❌ Failed to send discovery response to {addr[0]}: {e}")
    
    def handle_device_message(self, message: str, addr: tuple):
        """Handle incoming device messages"""
        try:
            data = json.loads(message)
            print(f"📡 Received from {addr}: {message}")
            
            # Skip messages from the server itself
            if addr[0] == get_server_ip():
                print(f"🚫 Ignoring message from server IP: {addr[0]}")
                return
            
            # Handle server discovery requests
            if data.get('action') == 'server_discovery':
                print(f"🔍 Server discovery request from {addr[0]}")
                self.send_discovery_response(addr)
                return
            
            # Handle both 'deviceId' (from ESP32) and 'device_id' formats
            device_id = data.get('deviceId') or data.get('device_id', f'UNKNOWN_{addr[0]}')
            
            # Process messages from ESP32 devices
            # First check if device explicitly provides its type
            device_type = data.get('type')
            
            # If no explicit type, use device ID pattern matching for legacy compatibility
            if not device_type:
                if device_id.startswith('TRICORDER_') or device_id.startswith('TRIC'):
                    device_type = 'tricorder'
                elif device_id.startswith('POLYINOCULATOR_') or device_id.startswith('POLY'):
                    device_type = 'polyinoculator'
                elif device_id.startswith('DEFRAGMENTOR_') or device_id.startswith('DEFRAG'):
                    device_type = 'defragmentor'
                elif device_id.startswith('IV_INJECTOR_') or device_id.startswith('INJECTOR'):
                    device_type = 'iv_injector'
                elif device_id.startswith('IVST'):  # New IV Station devices
                    device_type = 'iv_station'
                elif device_id.startswith('IV') and not device_id.startswith('IV_'):
                    device_type = 'iv_injector'  # Legacy IV devices
                elif device_id.startswith('IV_BLOOD_BAG_') or device_id.startswith('BLOOD_BAG'):
                    device_type = 'iv_blood_bag_station'
                elif device_id.startswith('POLY_CRADLE_') or device_id.startswith('CRADLE'):
                    device_type = 'polyinoculator_cradle'
            
            if not device_type:
                print(f"🚫 Ignoring unsupported device: {device_id} from {addr[0]}")
                return
            
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
                # Standardized sACN fields (normalize field names)
                'sacn_universe': data.get('sacnUniverse', data.get('sacn_universe', 1)),
                'dmx_address': data.get('dmxStartAddress', data.get('dmx_address', data.get('dmxAddress', 1))),
                'sacn_enabled': data.get('sacnEnabled', data.get('sacn_enabled', True)),
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
                    'num_leds': data.get('numLeds', 15),  # Updated for 3-strip configuration
                    'brightness': data.get('brightness', 128),
                    'sacn_enabled': data.get('sacnEnabled', True),
                    'sacn_universe': data.get('sacnUniverse', 1),
                })
            elif device_type == 'defragmentor':
                devices[device_id].update({
                    'num_leds': data.get('numLeds', 2),  # 2 RGBW LEDs
                    'servo_position': data.get('servoPosition', 0),
                    'trigger_state': data.get('triggerState', False),
                    'power_enabled': data.get('powerEnabled', False),
                    'sacn_enabled': data.get('sacnEnabled', True),
                    'sacn_universe': data.get('sacnUniverse', 1),
                })
            elif device_type == 'iv_station':
                devices[device_id].update({
                    'sd_card_initialized': data.get('sdCardInitialized'),
                    'video_playing': data.get('videoPlaying'),
                    'current_video': data.get('currentVideo'),
                    'video_looping': data.get('videoLooping'),
                    'current_frame': data.get('currentFrame'),
                    'battery_voltage': data.get('batteryVoltage'),
                    'battery_percentage': data.get('batteryPercentage'),
                    'battery_status': data.get('batteryStatus'),
                    'num_leds': data.get('numLeds', 1),  # Single RGBW LED
                    'brightness': data.get('brightness', 128),
                    'sacn_enabled': data.get('sacnEnabled', True),
                    'sacn_universe': data.get('sacnUniverse', 1),
                })
            elif device_type in ['iv_injector', 'iv_blood_bag_station', 'polyinoculator_cradle']:
                devices[device_id].update({
                    'num_leds': data.get('numLeds', 1),  # Single LED devices
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
                elif device_type == 'iv_station':
                    auto_configure_tricorder_for_sacn(device_id, devices[device_id])  # Use same config as tricorder
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

# Prop-type grouping helper functions
def get_devices_by_type(prop_type: str) -> List[Dict]:
    """Get all devices of a specific type"""
    return [device for device in devices.values() if device.get('device_type') == prop_type]

def get_online_devices_by_type(prop_type: str) -> List[Dict]:
    """Get all online devices of a specific type"""
    return [device for device in devices.values() 
            if device.get('device_type') == prop_type and device.get('status') == 'online']

def send_udp_command_to_device(device_id: str, action: str, parameters: dict, command_id: str):
    """Send UDP command to a specific device"""
    device = devices.get(device_id)
    if not device:
        print(f"❌ Device {device_id} not found")
        return False
    
    try:
        command = {
            'action': action,
            'commandId': command_id,
            **parameters
        }
        
        message = json.dumps(command)
        ip_address = device['ip_address']
        
        print(f"📤 UDP Command to {device_id}: {message}")
        
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.sendto(message.encode(), (ip_address, 8888))
        udp_socket.close()
        
        print(f"✓ Sent command to {device_id}: {action}")
        return True
    except Exception as e:
        print(f"❌ Failed to send command to {device_id}: {e}")
        return False

def send_bulk_command_to_type(prop_type: str, action: str, parameters: dict = None) -> dict:
    """Send command to all online devices of a specific type"""
    if parameters is None:
        parameters = {}
    
    online_devices = get_online_devices_by_type(prop_type)
    if not online_devices:
        return {'success': False, 'message': f'No online {prop_type} devices found'}
    
    command_id = str(uuid.uuid4())
    results = {'success': True, 'devices_updated': 0, 'total_devices': len(online_devices), 'errors': []}
    
    for device in online_devices:
        device_id = device['device_id']
        if send_udp_command_to_device(device_id, action, parameters, command_id):
            results['devices_updated'] += 1
        else:
            results['errors'].append(f"Failed to update {device_id}")
    
    if results['errors']:
        results['success'] = results['devices_updated'] > 0  # Partial success if some devices updated
    
    return results

@app.route('/')
def index():
    """Main web interface - serve enhanced dashboard"""
    try:
        index_path = os.path.join(os.getcwd(), 'web', 'dist', 'index.html')
        with open(index_path, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        # Fallback to older enhanced dashboard if React build not found
        try:
            fallback_path = os.path.join(os.getcwd(), 'web', 'enhanced-prop-dashboard.html')
            with open(fallback_path, 'r', encoding='utf-8') as f:
                return f.read()
        except FileNotFoundError:
            # Fallback to basic interface if enhanced dashboard not found
            return basic_interface()

@app.route('/assets/<path:filename>')
def serve_assets(filename):
    """Serve static assets from web/dist/assets"""
    try:
        asset_path = os.path.join(os.getcwd(), 'web', 'dist', 'assets', filename)
        return send_file(asset_path)
    except (FileNotFoundError, IsADirectoryError):
        abort(404)

@app.route('/registerSW.js')
def serve_register_sw():
    """Serve service worker registration"""
    try:
        sw_path = os.path.join(os.getcwd(), 'web', 'dist', 'registerSW.js')
        return send_file(sw_path)
    except FileNotFoundError:
        abort(404)

@app.route('/manifest.webmanifest')
def serve_manifest():
    """Serve web app manifest"""
    try:
        manifest_path = os.path.join(os.getcwd(), 'web', 'dist', 'manifest.webmanifest')
        return send_file(manifest_path)
    except FileNotFoundError:
        abort(404)

@app.route('/vite.svg')
def serve_vite_svg():
    """Serve vite icon"""
    try:
        icon_path = os.path.join(os.getcwd(), 'web', 'dist', 'vite.svg')
        return send_file(icon_path)
    except FileNotFoundError:
        abort(404)

@app.route('/sw.js')
def serve_sw():
    """Serve service worker"""
    try:
        sw_path = os.path.join(os.getcwd(), 'web', 'dist', 'sw.js')
        return send_file(sw_path)
    except FileNotFoundError:
        abort(404)

@app.route('/workbox-<filename>')
def serve_workbox(filename):
    """Serve workbox files"""
    try:
        workbox_path = os.path.join(os.getcwd(), 'web', 'dist', f'workbox-{filename}')
        return send_file(workbox_path)
    except FileNotFoundError:
        abort(404)

@app.route('/pwa-192x192.png')
def serve_pwa_icon():
    """Serve PWA icon"""
    try:
        icon_path = os.path.join(os.getcwd(), 'web', 'dist', 'pwa-192x192.png')
        return send_file(icon_path)
    except FileNotFoundError:
        abort(404)

@app.route('/uploads/<filename>')
def serve_firmware(filename):
    """Serve firmware files for OTA updates"""
    try:
        uploads_dir = os.path.join(os.getcwd(), 'uploads')
        firmware_path = os.path.join(uploads_dir, filename)
        
        # Security check - ensure file is within uploads directory
        if not os.path.commonpath([uploads_dir, firmware_path]) == uploads_dir:
            abort(403)
            
        if not os.path.exists(firmware_path):
            abort(404)
            
        print(f"📦 Serving firmware file: {filename}")
        return send_file(firmware_path, as_attachment=True, download_name=filename)
    except Exception as e:
        print(f"❌ Error serving firmware {filename}: {e}")
        abort(404)

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
    return jsonify({
        'devices': devices,  # Return as dictionary with device_id as keys
        'device_list': list(devices.values()),  # Also provide as list for compatibility
        'total_devices': len(devices),
        'online_devices': len([d for d in devices.values() if d.get('status') == 'online']),
        'last_updated': datetime.now().isoformat()
    })

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
        print(f"📤 UDP Command JSON to {device_id}: {command_json}")
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

@app.route('/api/device/<device_id>/command', methods=['POST'])
def send_device_command(device_id):
    """Send command to a specific device"""
    try:
        data = request.get_json()
        action = data.get('action')
        parameters = data.get('parameters', {})
        
        if not action:
            return jsonify({'error': 'Missing action'}), 400
        
        # Use existing command logic
        command_data = {
            'device_id': device_id,
            'action': action,
            'parameters': parameters
        }
        
        # Create command in format ESP32 expects
        esp32_command = {
            'commandId': str(uuid.uuid4()),
            'action': action,
            'timestamp': datetime.now().isoformat()
        }
        
        # Handle different command types
        if action == 'display_image' and 'filename' in parameters:
            esp32_command['parameters'] = {'filename': parameters['filename']}
        elif parameters:
            esp32_command['parameters'] = parameters
        
        # Store command for tracking
        command_record = {
            'id': esp32_command['commandId'],
            'device_id': device_id,
            'action': action,
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
                print(f"📱 Sent {action} command to {device_id}: {command_json}")
                
            except Exception as e:
                print(f"Failed to send command to {device_id}: {e}")
                return jsonify({'error': f'Failed to send command: {e}'}), 500
        else:
            return jsonify({'error': f'Device {device_id} not found or offline'}), 404
        
        return jsonify({'status': 'sent', 'command_id': esp32_command['commandId']})
        
    except Exception as e:
        print(f"Device command error: {e}")
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

@app.route('/api/config/<device_id>/network', methods=['POST'])
def set_device_network_config(device_id):
    """Update device network configuration"""
    try:
        if device_id not in devices:
            return jsonify({'error': f'Device {device_id} not found'}), 404
        
        device = devices[device_id]
        ip_address = device['ip_address']
        
        # Get network configuration data from request
        network_data = request.get_json()
        if not network_data:
            return jsonify({'error': 'No network configuration data provided'}), 400
        
        print(f"🌐 Updating network config for {device_id} at {ip_address}")
        print(f"Network data: {network_data}")
        
        # Send network configuration to device
        url = f"http://{ip_address}/network"
        headers = {'Content-Type': 'application/json'}
        
        response = requests.post(url, json=network_data, headers=headers, timeout=10)
        
        if response.status_code == 200:
            print(f"✅ Network configuration updated for {device_id}")
            # Note: Device will restart, so it may temporarily go offline
            return jsonify({
                'status': 'Network configuration updated - device restarting',
                'device_id': device_id,
                'message': 'Device will restart to apply network changes'
            })
        else:
            print(f"❌ Failed to update network config: {response.status_code} - {response.text}")
            return jsonify({'error': f'Failed to update network configuration: {response.text}'}), 500
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Network config request failed: {e}")
        return jsonify({'error': f'Failed to reach device: {str(e)}'}), 500
    except Exception as e:
        print(f"Network config update error: {e}")
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
# Prop-Type Grouping API Endpoints
# ==========================================

@app.route('/api/props', methods=['GET'])
def get_prop_types():
    """Get summary of all prop types with device counts"""
    prop_types = {}
    
    for device in devices.values():
        device_type = device.get('device_type', 'unknown')
        if device_type not in prop_types:
            prop_types[device_type] = {
                'type': device_type,
                'total_devices': 0,
                'online_devices': 0,
                'devices': []
            }
        
        prop_types[device_type]['total_devices'] += 1
        if device.get('status') == 'online':
            prop_types[device_type]['online_devices'] += 1
        prop_types[device_type]['devices'].append(device)
    
    return jsonify({
        'prop_types': list(prop_types.values()),
        'total_types': len(prop_types)
    })

@app.route('/api/props/<prop_type>', methods=['GET'])
def get_prop_type_devices(prop_type):
    """Get all devices of a specific prop type"""
    devices_of_type = get_devices_by_type(prop_type)
    online_count = sum(1 for d in devices_of_type if d.get('status') == 'online')
    
    return jsonify({
        'prop_type': prop_type,
        'total_devices': len(devices_of_type),
        'online_devices': online_count,
        'devices': devices_of_type
    })

@app.route('/api/props/<prop_type>/sacn/address', methods=['POST'])
def set_prop_type_sacn_address(prop_type):
    """Set SACN universe and address for all online devices of a prop type"""
    try:
        data = request.get_json()
        print(f"🔧 Received SACN address request for {prop_type}")
        print(f"📤 Raw request data: {data}")
        
        universe = data.get('universe')
        start_address = data.get('address')
        
        print(f"📊 Parsed values: universe={universe}, start_address={start_address}")
        
        if universe is None or start_address is None:
            return jsonify({'error': 'Universe and address are required'}), 400
        
        # Send universe command first
        universe_parameters = {'universe': universe}
        print(f"🚀 Sending universe command with parameters: {universe_parameters}")
        universe_result = send_bulk_command_to_type(prop_type, 'set_sacn_universe', universe_parameters)
        
        # Send address command second
        address_parameters = {'address': start_address}
        print(f"🚀 Sending address command with parameters: {address_parameters}")
        address_result = send_bulk_command_to_type(prop_type, 'set_sacn_address', address_parameters)
        
        # Combine results
        total_success = universe_result['success'] and address_result['success']
        combined_errors = universe_result['errors'] + address_result['errors']
        
        return jsonify({
            'success': total_success,
            'message': f"SACN address set to {universe}.{start_address} for {prop_type}",
            'devices_updated': address_result['devices_updated'],  # Use address result for final count
            'total_devices': address_result['total_devices'],
            'errors': combined_errors,
            'universe_result': universe_result,
            'address_result': address_result
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/props/<prop_type>/device/label', methods=['POST'])
def set_prop_type_device_labels(prop_type):
    """Set device labels for all online devices of a prop type"""
    try:
        data = request.get_json()
        if not data:
            return jsonify({'error': 'No data provided'}), 400
        
        # Check if it's bulk labeling with a pattern
        label_pattern = data.get('labelPattern', '')
        start_number = data.get('startNumber', 1)
        
        # Or individual device labeling
        device_labels = data.get('deviceLabels', {})
        
        # Get all online devices of the specified type
        prop_devices = []
        for device_id, device in devices.items():
            if device.get('device_type', '').lower().replace('_', '').replace('-', '') == prop_type.lower().replace('_', '').replace('-', ''):
                if device.get('status') == 'online':
                    prop_devices.append({
                        'device_id': device_id,
                        'ip_address': device['ip_address']
                    })
        
        if not prop_devices:
            return jsonify({'error': f'No online {prop_type} devices found'}), 404
        
        results = []
        errors = []
        
        for i, prop_device in enumerate(prop_devices):
            device_id = prop_device['device_id']
            ip_address = prop_device['ip_address']
            
            try:
                # Determine the label for this device
                if device_id in device_labels:
                    # Individual device label
                    new_label = device_labels[device_id]
                elif label_pattern:
                    # Pattern-based labeling
                    device_number = start_number + i
                    new_label = label_pattern.replace('{number}', str(device_number).zfill(3))
                else:
                    errors.append(f"No label specified for device {device_id}")
                    continue
                
                # Send label update to device
                response = requests.post(
                    f"http://{ip_address}/api/config",
                    json={'deviceLabel': new_label},
                    headers={'Content-Type': 'application/json'},
                    timeout=10
                )
                
                if response.status_code == 200:
                    results.append({
                        'device_id': device_id,
                        'new_label': new_label,
                        'status': 'success'
                    })
                    
                    # Update local device info
                    if device_id in devices:
                        devices[device_id]['device_label'] = new_label
                        
                    # Broadcast update to web clients
                    socketio.emit('device_updated', {
                        'device_id': device_id,
                        'device_label': new_label,
                        'updated_at': datetime.now().isoformat()
                    })
                else:
                    error_msg = f"Device {device_id} returned status {response.status_code}"
                    errors.append(error_msg)
                    results.append({
                        'device_id': device_id,
                        'new_label': new_label,
                        'status': 'failed',
                        'error': error_msg
                    })
                    
            except requests.RequestException as e:
                error_msg = f"Failed to connect to device {device_id}: {str(e)}"
                errors.append(error_msg)
                results.append({
                    'device_id': device_id,
                    'status': 'failed',
                    'error': error_msg
                })
            except Exception as e:
                error_msg = f"Error updating device {device_id}: {str(e)}"
                errors.append(error_msg)
                results.append({
                    'device_id': device_id,
                    'status': 'failed',
                    'error': error_msg
                })
        
        return jsonify({
            'prop_type': prop_type,
            'updated_devices': len([r for r in results if r.get('status') == 'success']),
            'total_devices': len(prop_devices),
            'results': results,
            'errors': errors
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/device/<device_id>/label', methods=['POST'])
def set_device_label(device_id):
    """Set device label for a specific device"""
    try:
        if device_id not in devices:
            return jsonify({'error': f'Device {device_id} not found'}), 404
        
        data = request.get_json()
        if not data or 'deviceLabel' not in data:
            return jsonify({'error': 'Device label not provided'}), 400
        
        device = devices[device_id]
        ip_address = device['ip_address']
        new_label = data['deviceLabel']
        
        # Send label update to device
        response = requests.post(
            f"http://{ip_address}/api/config",
            json={'deviceLabel': new_label},
            headers={'Content-Type': 'application/json'},
            timeout=10
        )
        
        if response.status_code == 200:
            # Update local device info
            devices[device_id]['device_label'] = new_label
            
            # Broadcast update to web clients
            socketio.emit('device_updated', {
                'device_id': device_id,
                'device_label': new_label,
                'updated_at': datetime.now().isoformat()
            })
            
            return jsonify({
                'device_id': device_id,
                'device_label': new_label,
                'status': 'success'
            })
        else:
            error_msg = f"Device returned status {response.status_code}"
            try:
                error_data = response.json()
                if 'error' in error_data:
                    error_msg = error_data['error']
            except:
                pass
            return jsonify({'error': error_msg}), 500
            
    except requests.RequestException as e:
        return jsonify({'error': f'Failed to connect to device: {str(e)}'}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/props/<prop_type>/firmware/update', methods=['POST'])
def update_prop_type_firmware(prop_type):
    """Update firmware for all online devices of a prop type"""
    try:
        if 'firmware' not in request.files:
            return jsonify({'error': 'No firmware file provided'}), 400
        
        firmware_file = request.files['firmware']
        if firmware_file.filename == '':
            return jsonify({'error': 'No firmware file selected'}), 400
        
        # Save firmware file to uploads directory
        uploads_dir = os.path.join(os.getcwd(), 'uploads')
        os.makedirs(uploads_dir, exist_ok=True)
        
        # Generate unique filename with timestamp
        timestamp = int(time.time())
        safe_filename = f"{prop_type}_{timestamp}_{firmware_file.filename}"
        firmware_path = os.path.join(uploads_dir, safe_filename)
        
        firmware_file.save(firmware_path)
        
        # Get server IP for firmware URL
        server_ip = get_server_ip()
        firmware_url = f"http://{server_ip}:{CONFIG['web_port']}/uploads/{safe_filename}"
        
        online_devices = get_online_devices_by_type(prop_type)
        if not online_devices:
            return jsonify({'error': f'No online {prop_type} devices found'}), 404
        
        # Send OTA update commands via UDP to all devices
        update_results = []
        for device in online_devices:
            device_id = device['device_id']
            ip_address = device['ip_address']
            
            try:
                # Create OTA update command
                command_id = str(uuid.uuid4())
                ota_command = {
                    'action': 'ota_update',
                    'device_id': device_id,  # Add device_id so device knows command is for it
                    'commandId': command_id,
                    'parameters': {
                        'firmware_url': firmware_url
                    }
                }
                
                # Send UDP command to device
                message = json.dumps(ota_command)
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                sock.sendto(message.encode(), (ip_address, CONFIG['udp_port']))
                sock.close()
                
                print(f"🔄 Sent OTA command to {device_id} at {ip_address}")
                print(f"📦 Firmware URL: {firmware_url}")
                
                update_results.append({
                    'device_id': device_id, 
                    'success': True, 
                    'message': 'OTA update command sent',
                    'firmware_url': firmware_url
                })
                    
            except Exception as e:
                print(f"❌ Failed to send OTA command to {device_id}: {e}")
                update_results.append({'device_id': device_id, 'success': False, 'message': str(e)})
        
        successful_updates = sum(1 for r in update_results if r['success'])
        
        return jsonify({
            'success': successful_updates > 0,
            'message': f"OTA update commands sent to {prop_type} devices",
            'successful_updates': successful_updates,
            'total_devices': len(online_devices),
            'firmware_url': firmware_url,
            'results': update_results
        })
        
    except Exception as e:
        print(f"❌ Firmware update error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/props/<prop_type>/files/upload', methods=['POST'])
def upload_files_to_prop_type(prop_type):
    """Upload files to SD cards of all online devices of a prop type"""
    try:
        if 'files' not in request.files:
            return jsonify({'error': 'No files provided'}), 400
        
        files = request.files.getlist('files')
        if not files or all(f.filename == '' for f in files):
            return jsonify({'error': 'No files selected'}), 400
        
        # Save files to uploads directory
        uploads_dir = os.path.join(os.getcwd(), 'uploads', 'sdcard_files')
        os.makedirs(uploads_dir, exist_ok=True)
        
        saved_files = []
        server_ip = get_server_ip()
        
        # Save each file
        for file in files:
            if file.filename != '':
                # Generate safe filename with timestamp
                timestamp = int(time.time())
                safe_filename = f"{timestamp}_{file.filename}"
                file_path = os.path.join(uploads_dir, safe_filename)
                
                file.save(file_path)
                file_url = f"http://{server_ip}:{CONFIG['web_port']}/uploads/sdcard_files/{safe_filename}"
                
                saved_files.append({
                    'original_name': file.filename,
                    'safe_name': safe_filename,
                    'url': file_url,
                    'path': file_path
                })
        
        if not saved_files:
            return jsonify({'error': 'No valid files to upload'}), 400
        
        online_devices = get_online_devices_by_type(prop_type)
        if not online_devices:
            return jsonify({'error': f'No online {prop_type} devices found'}), 404
        
        # Send file upload commands to all devices
        upload_results = []
        for device in online_devices:
            device_id = device['device_id']
            device_results = []
            
            for file_info in saved_files:
                try:
                    # Create file upload command
                    command_id = str(uuid.uuid4())
                    command = {
                        'action': 'upload_file',
                        'commandId': command_id,
                        'parameters': {
                            'filename': file_info['original_name'],
                            'url': file_info['url']
                        }
                    }
                    
                    # Send UDP command to device
                    message = json.dumps(command)
                    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    sock.sendto(message.encode(), (device['ip_address'], CONFIG['udp_port']))
                    sock.close()
                    
                    print(f"📁 Sent file upload command to {device_id} at {device['ip_address']}")
                    print(f"📄 File: {file_info['original_name']} -> {file_info['url']}")
                    
                    device_results.append({
                        'filename': file_info['original_name'],
                        'success': True,
                        'message': 'File upload command sent'
                    })
                    
                except Exception as e:
                    print(f"❌ Failed to send file upload command to {device_id}: {e}")
                    device_results.append({
                        'filename': file_info['original_name'],
                        'success': False,
                        'message': str(e)
                    })
            
            upload_results.append({
                'device_id': device_id,
                'files': device_results
            })
        
        successful_uploads = sum(1 for device in upload_results 
                               for file in device['files'] if file['success'])
        total_uploads = len(saved_files) * len(online_devices)
        
        return jsonify({
            'success': successful_uploads > 0,
            'message': f"File upload commands sent to {prop_type} devices",
            'successful_uploads': successful_uploads,
            'total_uploads': total_uploads,
            'files_uploaded': [f['original_name'] for f in saved_files],
            'results': upload_results
        })
        
    except Exception as e:
        print(f"❌ File upload error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/uploads/sdcard_files/<filename>')
def serve_sdcard_file(filename):
    """Serve SD card files for device download"""
    try:
        uploads_dir = os.path.join(os.getcwd(), 'uploads', 'sdcard_files')
        file_path = os.path.join(uploads_dir, filename)
        
        # Security check - ensure file is within uploads directory
        if not os.path.commonpath([uploads_dir, file_path]) == uploads_dir:
            abort(403)
            
        if not os.path.exists(file_path):
            abort(404)
            
        print(f"📁 Serving SD card file: {filename}")
        return send_file(file_path, as_attachment=True, download_name=filename)
    except Exception as e:
        print(f"❌ Error serving SD card file {filename}: {e}")
        abort(404)

@app.route('/api/props/<prop_type>/command', methods=['POST'])
def send_prop_type_command(prop_type):
    """Send a command to all online devices of a prop type"""
    try:
        data = request.get_json()
        action = data.get('action')
        parameters = data.get('parameters', {})
        
        if not action:
            return jsonify({'error': 'Action is required'}), 400
        
        result = send_bulk_command_to_type(prop_type, action, parameters)
        
        return jsonify({
            'success': result['success'],
            'message': f"Command '{action}' sent to {prop_type} devices",
            'devices_updated': result['devices_updated'],
            'total_devices': result['total_devices'],
            'errors': result['errors']
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/props/<prop_type>/save-current-as-default', methods=['POST'])
def save_current_as_default(prop_type):
    """Save current SACN LED state as default for all online devices of a prop type"""
    try:
        result = send_bulk_command_to_type(prop_type, 'save_current_as_default', {})
        
        return jsonify({
            'success': result['success'],
            'message': f"Saved current LED state as default for {prop_type} devices",
            'devices_updated': result['devices_updated'],
            'total_devices': result['total_devices'],
            'errors': result['errors']
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# ==========================================
# Firmware Management API Endpoints
# ==========================================

@app.route('/api/firmware/list', methods=['GET'])
def list_firmware():
    """List available firmware files"""
    try:
        firmware_dir = os.path.join(os.path.dirname(__file__), 'firmware_binaries')
        if not os.path.exists(firmware_dir):
            return jsonify({'firmware': []})
        
        firmware_files = []
        for filename in os.listdir(firmware_dir):
            if filename.endswith('.bin'):
                filepath = os.path.join(firmware_dir, filename)
                stat = os.stat(filepath)
                
                # Extract device type from filename
                device_type = filename.replace('_firmware.bin', '')
                
                firmware_files.append({
                    'device_type': device_type,
                    'filename': filename,
                    'size': stat.st_size,
                    'modified': datetime.fromtimestamp(stat.st_mtime).isoformat(),
                    'download_url': f'/api/firmware/download/{device_type}'
                })
        
        return jsonify({'firmware': firmware_files})
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/firmware/download/<device_type>', methods=['GET'])
def download_firmware(device_type):
    """Download firmware file for a specific device type"""
    try:
        firmware_dir = os.path.join(os.path.dirname(__file__), 'firmware_binaries')
        filename = f"{device_type}_firmware.bin"
        filepath = os.path.join(firmware_dir, filename)
        
        if not os.path.exists(filepath):
            return jsonify({'error': f'Firmware not found for {device_type}'}), 404
        
        return send_file(filepath, 
                        as_attachment=True, 
                        download_name=filename,
                        mimetype='application/octet-stream')
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/props/<prop_type>/firmware/update', methods=['POST'])
def update_prop_firmware(prop_type):
    """Update firmware for all online devices of a prop type"""
    try:
        # Check if firmware file exists
        firmware_dir = os.path.join(os.path.dirname(__file__), 'firmware_binaries')
        filename = f"{prop_type}_firmware.bin"
        filepath = os.path.join(firmware_dir, filename)
        
        if not os.path.exists(filepath):
            return jsonify({'error': f'Firmware not found for {prop_type}'}), 404
        
        # Get all online devices of this type
        online_devices = [
            device for device in devices.values()
            if device.get('online', False) and device.get('type') == prop_type
        ]
        
        if not online_devices:
            return jsonify({
                'success': False,
                'message': f'No online {prop_type} devices found',
                'devices_updated': 0,
                'total_devices': 0,
                'errors': []
            })
        
        # Read firmware file
        with open(filepath, 'rb') as f:
            firmware_data = f.read()
        
        # Send OTA update command to each device
        results = []
        errors = []
        
        for device in online_devices:
            device_id = device['deviceId']
            ip_address = device['ipAddress']
            
            try:
                # Create OTA update command
                command = {
                    'action': 'ota_update',
                    'commandId': str(uuid.uuid4()),
                    'timestamp': datetime.now().isoformat(),
                    'firmware_size': len(firmware_data),
                    'firmware_url': f'http://{get_server_ip()}:8080/api/firmware/download/{prop_type}'
                }
                
                # Send UDP command
                if send_udp_command_to_device(device_id, 'ota_update', 
                                            {'firmware_url': f'http://{get_server_ip()}:8080/api/firmware/download/{prop_type}',
                                             'firmware_size': len(firmware_data)}, 
                                            command['commandId']):
                    results.append(device_id)
                    print(f"✓ OTA update initiated for {device_id}")
                else:
                    errors.append(f"Failed to send OTA command to {device_id}")
                    
            except Exception as e:
                errors.append(f"Error updating {device_id}: {str(e)}")
        
        return jsonify({
            'success': len(results) > 0,
            'message': f"OTA update initiated for {len(results)} {prop_type} devices",
            'devices_updated': len(results),
            'total_devices': len(online_devices),
            'updated_devices': results,
            'errors': errors
        })
        
    except Exception as e:
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

@socketio.on('send_command')
def handle_send_command(data):
    """Handle command from web interface"""
    try:
        device_id = data['device_id']
        action = data['action']
        parameters = data.get('parameters', {})
        
        print(f"🌐 WebSocket command: {action} for {device_id}")
        
        if device_id not in devices:
            print(f"❌ Device {device_id} not found")
            emit('error', {'message': f'Device {device_id} not found'})
            return
        
        # Send command using existing send_udp_command_to_device function
        command_id = str(uuid.uuid4())
        success = send_udp_command_to_device(device_id, action, parameters, command_id)
        
        if success:
            emit('command_sent', {'command_id': command_id})
            # Broadcast to all clients
            socketio.emit('device_response', {
                'device_id': device_id,
                'command_id': command_id,
                'action': action,
                'status': 'sent'
            })
        else:
            emit('error', {'message': f'Failed to send command to {device_id}'})
            
    except Exception as e:
        print(f"WebSocket command error: {e}")
        emit('error', {'message': str(e)})

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
