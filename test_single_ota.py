#!/usr/bin/env python3

import requests
import json

def test_single_device_ota():
    """Test OTA update on a single device"""
    
    # Test with one device
    device_ip = "192.168.1.187"  # TRIC001
    device_id = "TRIC001"
    
    # Direct UDP command (testing)
    import socket
    import time
    
    # Create OTA command
    command = {
        "action": "ota_update",
        "device_id": device_id,
        "commandId": f"test_ota_{int(time.time())}",
        "parameters": {
            "firmware_url": "http://192.168.1.24:8080/uploads/tricorder_firmware.bin"
        }
    }
    
    try:
        # Send direct UDP command
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(10)
        
        message = json.dumps(command)
        print(f"📡 Sending OTA command to {device_ip}:8888")
        print(f"Command: {message}")
        
        sock.sendto(message.encode(), (device_ip, 8888))
        print("✅ Command sent")
        
        # Wait for response
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
    print("=== Single Device OTA Test ===")
    test_single_device_ota()
