#!/usr/bin/env python3
"""
Direct OTA update test - sends OTA command directly to device via UDP
"""

import socket
import json
import sys

def send_ota_command(device_ip, firmware_url, device_id):
    """Send OTA update command directly to device"""
    
    # Create OTA command in the format the device expects
    ota_command = {
        'action': 'ota_update',
        'device_id': device_id,  # Add device_id so device knows command is for it
        'commandId': 'test-ota-12345',
        'parameters': {
            'firmware_url': firmware_url
        }
    }
    
    message = json.dumps(ota_command)
    print(f"📦 Sending OTA command to {device_ip}:8888")
    print(f"📋 Command: {message}")
    
    try:
        # Send UDP command
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(message.encode('utf-8'), (device_ip, 8888))
        sock.close()
        
        print(f"✅ OTA command sent successfully!")
        return True
        
    except Exception as e:
        print(f"❌ Failed to send OTA command: {e}")
        return False

if __name__ == "__main__":
    # Test with both devices
    devices = [
        ("192.168.1.233", "IV9CD4"),  # Device we're monitoring
        ("192.168.0.236", "IV20D4")   # Other device
    ]
    
    # Use existing firmware URL from server
    firmware_url = "http://192.168.1.24:8080/api/firmware/download/iv_injector"
    
    print("🚀 Starting direct OTA update test...")
    print(f"🔗 Firmware URL: {firmware_url}")
    
    for ip, device_id in devices:
        print(f"\n🎯 Targeting {device_id} at {ip}")
        success = send_ota_command(ip, firmware_url, device_id)
        if success:
            print(f"✅ Command sent to {device_id}")
        else:
            print(f"❌ Failed to send to {device_id}")
    
    print("\n🔚 OTA test complete. Check device serial output for results.")
