#!/usr/bin/env python3
"""
Test script to send commands to Tricorder and check file system
"""

import socket
import json
import time

def send_udp_command(ip, port, command):
    """Send UDP command to device"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)
        
        command_json = json.dumps(command)
        print(f"Sending to {ip}:{port}: {command_json}")
        
        sock.sendto(command_json.encode(), (ip, port))
        
        # Wait for response
        try:
            response, addr = sock.recvfrom(1024)
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
    # Tricorder IP from serial output
    tricorder_ip = "192.168.1.28"
    port = 8888
    
    print("Testing Tricorder file system...")
    print("=" * 50)
    
    # Test 1: List available videos
    print("\n1. Checking available videos...")
    command = {
        "action": "list_videos",
        "commandId": "test_list"
    }
    response = send_udp_command(tricorder_ip, port, command)
    
    if response:
        if "videos" in response:
            print(f"Found {len(response['videos'])} videos:")
            for video in response['videos']:
                print(f"  - {video}")
        else:
            print("No videos found or no video list in response")
    
    time.sleep(1)
    
    # Test 2: Try to display a simple test image
    print("\n2. Testing image display...")
    command = {
        "action": "display_image",
        "filename": "boot.jpg",  # We know this exists from serial output
        "commandId": "test_display"
    }
    response = send_udp_command(tricorder_ip, port, command)
    
    time.sleep(1)
    
    # Test 3: Try to list SD card contents
    print("\n3. Checking SD card status...")
    command = {
        "action": "status",
        "commandId": "test_status"
    }
    response = send_udp_command(tricorder_ip, port, command)
    
    print("\nTest complete!")

if __name__ == "__main__":
    main()
