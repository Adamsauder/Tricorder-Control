#!/usr/bin/env python3
"""
Send OTA update command to Tricorder devices via UDP
"""
import socket
import json
import time
import os
import requests

def get_devices_from_server(server_url="http://localhost:8080"):
    """Get device list from the Python server"""
    try:
        response = requests.get(f"{server_url}/api/devices", timeout=5)
        if response.status_code == 200:
            data = response.json()
            return data.get('devices', {})
        else:
            print(f"❌ Server returned status {response.status_code}")
            return {}
    except Exception as e:
        print(f"❌ Failed to connect to server: {e}")
        return {}

def send_ota_update(device_ip="192.168.1.100", udp_port=8888):
    """Send OTA update command to device"""
    
    # Build path to tricorder firmware binary
    firmware_path = os.path.join(os.path.dirname(__file__), 
                                 "firmware", "tricorder", ".pio", "build", "tricorder", "firmware.bin")
    
    if not os.path.exists(firmware_path):
        print(f"❌ Tricorder firmware file not found: {firmware_path}")
        return False
    
    firmware_size = os.path.getsize(firmware_path)
    print(f"📦 Tricorder firmware file: {firmware_path} ({firmware_size} bytes)")
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)  # 5 second timeout
    
    try:
        # Create OTA update command for tricorder
        command = {
            "action": "ota_update",
            "commandId": f"ota_{int(time.time())}",
            "parameters": {
                "firmware_url": f"http://192.168.1.24:8080/uploads/tricorder_firmware.bin"
            },
            "timestamp": int(time.time())
        }
        
        command_json = json.dumps(command)
        print(f"📡 Sending OTA command to {device_ip}:{udp_port}")
        print(f"Command: {command_json}")
        
        # Send command
        sock.sendto(command_json.encode('utf-8'), (device_ip, udp_port))
        print("✅ OTA command sent")
        
        # Wait for response
        try:
            response, addr = sock.recvfrom(1024)
            print(f"📨 Response from {addr}: {response.decode('utf-8')}")
            return True
        except socket.timeout:
            print("⏰ No response received (timeout)")
            return False
            
    except Exception as e:
        print(f"❌ Error sending OTA command: {e}")
        return False
    finally:
        sock.close()

def scan_ip_range(base_ip="192.168.0", start=1, end=254):
    """Scan IP range for tricorder devices"""
    print(f"🔍 Scanning {base_ip}.{start}-{end} for devices...")
    
    found_devices = []
    
    for i in range(start, end + 1):
        ip = f"{base_ip}.{i}"
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(1.0)  # Short timeout for scanning
        
        try:
            # Send ping command
            ping_command = {
                "action": "ping",
                "commandId": f"ping_{int(time.time())}",
                "timestamp": int(time.time())
            }
            
            message = json.dumps(ping_command)
            sock.sendto(message.encode('utf-8'), (ip, 8888))
            
            try:
                response, addr = sock.recvfrom(1024)
                data = json.loads(response.decode('utf-8'))
                device_type = data.get('type', 'Unknown')
                device_id = data.get('deviceId', 'Unknown')
                print(f"📍 Found device: {ip} - {device_id} ({device_type})")
                found_devices.append({
                    'ip': ip,
                    'device_id': device_id,
                    'device_type': device_type,
                    'data': data
                })
            except (socket.timeout, json.JSONDecodeError):
                pass  # No response or invalid JSON
                
        except Exception:
            pass  # Skip errors during scanning
        finally:
            sock.close()
    
    return found_devices

def discover_devices():
    """Discover devices on network"""
    print("🔍 Discovering devices...")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(3.0)
    
    try:
        # Send discovery broadcast
        discovery_command = {
            "action": "discover",
            "commandId": f"discover_{int(time.time())}",
            "timestamp": int(time.time())
        }
        
        message = json.dumps(discovery_command)
        
        # Try multiple broadcast addresses
        broadcast_addresses = [
            '255.255.255.255',  # Global broadcast
            '192.168.1.255',    # Common home network
            '192.168.4.255',    # AP mode network
            '192.168.0.255',    # Another common network
            '10.0.0.255'        # Another common network
        ]
        
        for broadcast in broadcast_addresses:
            try:
                sock.sendto(message.encode('utf-8'), (broadcast, 8888))
                print(f"📡 Discovery sent to {broadcast}")
            except Exception as e:
                print(f"⚠️  Failed to send to {broadcast}: {e}")
        
        devices = []
        start_time = time.time()
        
        while time.time() - start_time < 5:  # Wait longer for responses
            try:
                response, addr = sock.recvfrom(1024)
                data = json.loads(response.decode('utf-8'))
                if addr[0] not in [d['ip'] for d in devices]:
                    devices.append({
                        'ip': addr[0],
                        'data': data
                    })
                    device_type = data.get('type', 'Unknown')
                    device_id = data.get('deviceId', 'Unknown')
                    print(f"📍 Found device: {addr[0]} - {device_id} ({device_type})")
            except socket.timeout:
                break
            except Exception as e:
                print(f"Error parsing response: {e}")
        
        return devices
        
    finally:
        sock.close()

if __name__ == "__main__":
    print("=== Tricorder OTA Update Tool ===")
    
    # First, get devices from the server
    print("🔍 Getting device list from server...")
    server_devices = get_devices_from_server()
    
    # Filter for tricorder devices
    tricorder_devices = []
    for device_id, device_data in server_devices.items():
        device_type = device_data.get('device_type', device_data.get('type', 'unknown'))
        if device_type in ['tricorder', 'iv_station']:
            tricorder_devices.append({
                'device_id': device_id,
                'ip': device_data.get('ip_address', 'unknown'),
                'device_type': device_type,
                'status': device_data.get('status', 'unknown')
            })
    
    print(f"📋 Found {len(tricorder_devices)} tricorder/iv_station devices in server:")
    for device in tricorder_devices:
        status_icon = "🟢" if device['status'] == 'online' else "🔴"
        print(f"  {status_icon} {device['device_id']} ({device['device_type']}) - {device['ip']}")
    
    # Also scan the 192.168.0.x range for devices not in server
    print("\n🔍 Scanning 192.168.0.x range for additional devices...")
    scanned_devices = scan_ip_range("192.168.0", 1, 50)  # Scan first 50 IPs
    
    # Filter scanned devices for tricorders not already in server
    server_ips = [d['ip'] for d in tricorder_devices]
    new_tricorders = [d for d in scanned_devices 
                     if d['device_type'] in ['tricorder', 'iv_station'] 
                     and d['ip'] not in server_ips]
    
    if new_tricorders:
        print(f"📋 Found {len(new_tricorders)} additional tricorder devices:")
        for device in new_tricorders:
            print(f"  🆕 {device['device_id']} ({device['device_type']}) - {device['ip']}")
        tricorder_devices.extend([{
            'device_id': d['device_id'],
            'ip': d['ip'],
            'device_type': d['device_type'],
            'status': 'found'
        } for d in new_tricorders])
    
    # Send OTA updates to all found tricorder devices
    if not tricorder_devices:
        print("❌ No tricorder devices found")
    else:
        print(f"\n🚀 Sending OTA updates to {len(tricorder_devices)} devices...")
        
        success_count = 0
        for device in tricorder_devices:
            print(f"\n📡 Updating {device['device_id']} at {device['ip']}...")
            if send_ota_update(device['ip']):
                success_count += 1
                print(f"✅ {device['device_id']} update sent successfully")
            else:
                print(f"❌ Failed to update {device['device_id']}")
        
        print(f"\n📊 Summary: {success_count}/{len(tricorder_devices)} devices updated successfully")
