#!/usr/bin/env python3
"""
Test device communication while server is running
"""

import socket
import json
import uuid

def send_test_command():
    """Send a command to the device while the server is running"""
    
    print("Sending command to device while server is running...")
    
    # Create a separate UDP socket for sending (not binding to port 8888)
    send_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': 'status',
            'parameters': {}
        }
        
        message = json.dumps(command)
        print(f"Sending: {message}")
        
        # Send to device
        send_socket.sendto(message.encode('utf-8'), ('192.168.1.48', 8888))
        print("Command sent successfully!")
        print("Check the web interface devices list and server console output.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        send_socket.close()

if __name__ == "__main__":
    send_test_command()
