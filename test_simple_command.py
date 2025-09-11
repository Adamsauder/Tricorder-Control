#!/usr/bin/env python3

import socket
import json
import time

def test_device_response(ip, port=8888):
    """Test if a device responds to UDP commands"""
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)  # 5 second timeout
        
        # Simple status command
        command = {
            "action": "get_status",
            "commandId": f"test_{int(time.time())}"
        }
        
        message = json.dumps(command)
        print(f"📡 Testing device at {ip}:{port}")
        print(f"Command: {message}")
        
        # Send command
        sock.sendto(message.encode('utf-8'), (ip, port))
        print("✅ Command sent")
        
        # Wait for response
        try:
            response, addr = sock.recvfrom(1024)
            print(f"✅ Response received from {addr}: {response.decode('utf-8')}")
            return True
        except socket.timeout:
            print("⏰ No response (timeout)")
            return False
            
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        sock.close()

if __name__ == "__main__":
    # Test a few devices
    test_devices = [
        "192.168.1.187",  # TRIC001
        "192.168.1.204",  # TRIC002
        "192.168.1.184",  # TRIC0275
    ]
    
    print("=== Simple Device Response Test ===")
    for device_ip in test_devices:
        test_device_response(device_ip)
        print()
