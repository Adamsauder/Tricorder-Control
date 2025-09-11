#!/usr/bin/env python3

import socket
import json
import time

def test_device_communication(ip):
    """Test basic UDP communication with a device"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)
        
        # Simple status command
        command = {
            "action": "get_status",
            "commandId": f"test_{int(time.time())}"
        }
        
        message = json.dumps(command)
        print(f"📡 Testing communication with {ip}:8888")
        print(f"Command: {message}")
        
        sock.sendto(message.encode(), (ip, 8888))
        print("✅ Command sent")
        
        try:
            response, addr = sock.recvfrom(1024)
            print(f"✅ Response: {response.decode()}")
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
    # Test a few tricorder devices
    test_devices = [
        "192.168.1.187",  # TRIC001
        "192.168.1.204",  # TRIC002
    ]
    
    print("=== Testing Basic UDP Communication ===")
    for device_ip in test_devices:
        success = test_device_communication(device_ip)
        if success:
            print(f"✅ {device_ip} is responding to UDP commands")
        else:
            print(f"❌ {device_ip} is NOT responding to UDP commands")
        print()
