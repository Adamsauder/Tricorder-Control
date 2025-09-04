#!/usr/bin/env python3
"""
Test script to check tricorder image display functionality
Sends display_image commands and monitors responses
"""

import socket
import json
import time
import uuid

# Server settings
SERVER_IP = "192.168.1.24"
UDP_PORT = 8888

# Test image files to try
TEST_IMAGES = [
    "greenscreen.jpg",
    "SFA2_202_211_Med_Tricorder_TRACTS.jpg", 
    "SFA2_202_211_Med_Tricorder_BODY.jpg"
]

def send_udp_command(device_ip, command):
    """Send UDP command to device and wait for response"""
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5.0)  # 5 second timeout
        
        # Send command
        message = json.dumps(command)
        print(f"Sending to {device_ip}: {message}")
        sock.sendto(message.encode(), (device_ip, UDP_PORT))
        
        # Wait for response
        response_data, addr = sock.recvfrom(1024)
        response = json.loads(response_data.decode())
        print(f"Response from {addr[0]}: {response}")
        return response
        
    except socket.timeout:
        print(f"No response from {device_ip} (timeout)")
        return None
    except Exception as e:
        print(f"Error communicating with {device_ip}: {e}")
        return None
    finally:
        sock.close()

def test_tricorder_image_display(device_ip):
    """Test image display on a specific tricorder"""
    print(f"\n=== Testing Tricorder at {device_ip} ===")
    
    # First test status to make sure device is responsive
    print("\n1. Testing device status...")
    status_cmd = {
        "action": "status",
        "commandId": str(uuid.uuid4()),
        "timestamp": int(time.time() * 1000)
    }
    
    response = send_udp_command(device_ip, status_cmd)
    if not response:
        print(f"Device {device_ip} is not responding to status commands")
        return False
        
    print(f"Device is online: {response.get('deviceId', 'Unknown')}")
    print(f"SD Card: {'Initialized' if response.get('sdCardInitialized', False) else 'NOT INITIALIZED'}")
    print(f"Free Heap: {response.get('freeHeap', 'Unknown')} bytes")
    
    # Test each image file
    for image_file in TEST_IMAGES:
        print(f"\n2. Testing image display: {image_file}")
        
        display_cmd = {
            "action": "display_image",
            "commandId": str(uuid.uuid4()),
            "timestamp": int(time.time() * 1000),
            "parameters": {
                "filename": image_file
            }
        }
        
        response = send_udp_command(device_ip, display_cmd)
        if response:
            print(f"Command result: {response.get('result', 'No result')}")
        else:
            print("No response to display_image command")
            
        # Wait a bit between tests
        time.sleep(2)
        
    return True

def main():
    print("Tricorder Image Display Test")
    print("=" * 40)
    
    # Known tricorder IPs from the server logs
    tricorder_ips = ["192.168.1.48", "192.168.1.234"]
    
    for ip in tricorder_ips:
        success = test_tricorder_image_display(ip)
        if not success:
            print(f"Failed to test tricorder at {ip}")
            
    print("\nTest completed!")

if __name__ == "__main__":
    main()
