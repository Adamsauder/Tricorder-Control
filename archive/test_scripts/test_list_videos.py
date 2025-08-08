#!/usr/bin/env python3
"""
Test list_videos command to see raw response
"""

import socket
import json
import uuid
import time

def test_list_videos():
    """Test the list_videos command"""
    
    # Create UDP socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind(('', 8889))  # Use different port to avoid conflict
    udp_socket.settimeout(5.0)
    
    print("Testing list_videos command...")
    
    try:
        # Send list_videos command
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': 'list_videos',
            'parameters': {}
        }
        
        message = json.dumps(command)
        print(f"Sending: {message}")
        
        # Send to device (use port 8888 for device)
        udp_socket.sendto(message.encode('utf-8'), ('192.168.1.48', 8888))
        
        print("Waiting for response...")
        
        # Wait for response
        data, addr = udp_socket.recvfrom(4096)
        response = data.decode('utf-8')
        print(f"\nRaw response from {addr}:")
        print(response)
        
        # Parse JSON
        try:
            parsed = json.loads(response)
            print(f"\nParsed JSON:")
            print(json.dumps(parsed, indent=2))
            
            if 'videos' in parsed:
                print(f"\nVideo list found:")
                for video in parsed['videos']:
                    print(f"  - {video}")
            else:
                print("\nNo 'videos' key in response")
                
        except json.JSONDecodeError as e:
            print(f"JSON parse error: {e}")
            
    except Exception as e:
        print(f"Error: {e}")
    finally:
        udp_socket.close()

if __name__ == "__main__":
    test_list_videos()
