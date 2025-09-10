#!/usr/bin/env python3
"""
Test script to explore SD card structure on Tricorder
"""

import socket
import json
import time

def send_udp_command(ip, port, command):
    """Send UDP command to device"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(10)  # Longer timeout for complex commands
        
        command_json = json.dumps(command)
        print(f"Sending to {ip}:{port}: {command_json}")
        
        sock.sendto(command_json.encode(), (ip, port))
        
        # Wait for response
        try:
            response, addr = sock.recvfrom(2048)  # Larger buffer for file lists
            response_data = response.decode()
            print(f"Response from {addr}: {response_data}")
            return json.loads(response_data)
        except socket.timeout:
            print("No response received (timeout)")
            return None
        except Exception as e:
            print(f"Error receiving response: {e}")
            return None
            
    except Exception as e:
        print(f"Error sending command: {e}")
        return None
    finally:
        sock.close()

def main():
    tricorder_ip = "192.168.1.28"
    port = 8888
    
    print("Testing Tricorder SD card structure...")
    print("=" * 50)
    
    # Test 1: Show boot image (we know this exists)
    print("\n1. Testing boot image display...")
    command = {
        "action": "display_image",
        "filename": "boot.jpg",
        "commandId": "test_boot"
    }
    send_udp_command(tricorder_ip, port, command)
    
    time.sleep(2)
    
    # Test 2: Try to create and display a simple folder
    print("\n2. Testing folder creation and static image...")
    
    # For now, let's try displaying some common image names that might exist
    test_images = [
        "boot.jpg",
        "default.jpg", 
        "test.jpg",
        "image.jpg",
        "tricorder.jpg"
    ]
    
    for img in test_images:
        print(f"\nTrying to display: {img}")
        command = {
            "action": "display_image", 
            "filename": img,
            "commandId": f"test_{img.replace('.', '_')}"
        }
        response = send_udp_command(tricorder_ip, port, command)
        time.sleep(1)
    
    # Test 3: Try folder mode
    print("\n3. Testing folder mode...")
    
    test_folders = [
        "test",
        "images", 
        "photos",
        "static",
        "folder1"
    ]
    
    for folder in test_folders:
        print(f"\nTrying to play folder: {folder}")
        command = {
            "action": "play_video",
            "filename": folder,
            "loop": False,
            "commandId": f"test_folder_{folder}"
        }
        response = send_udp_command(tricorder_ip, port, command)
        time.sleep(1)

if __name__ == "__main__":
    main()
