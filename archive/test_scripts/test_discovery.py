#!/usr/bin/env python3
"""
Test script to debug device discovery
"""

import socket
import json
import uuid
import ipaddress

def test_discovery():
    """Test the discovery functionality"""
    
    # Create UDP socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind(('', 8888))
    udp_socket.settimeout(1.0)
    
    print("UDP socket created and bound to port 8888")
    
    # Get local IP
    temp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    temp_socket.connect(("8.8.8.8", 80))
    local_ip = temp_socket.getsockname()[0]
    temp_socket.close()
    
    print(f"Local IP: {local_ip}")
    
    # Create network range
    network = ipaddress.IPv4Network(f"{local_ip}/24", strict=False)
    print(f"Scanning network: {network}")
    
    # Send status commands to known device
    test_ip = "192.168.1.48"
    command_id = str(uuid.uuid4())
    command = {
        'commandId': command_id,
        'action': 'status',
        'parameters': {}
    }
    
    message = json.dumps(command)
    print(f"Sending test command to {test_ip}: {message}")
    
    try:
        udp_socket.sendto(message.encode('utf-8'), (test_ip, 8888))
        print("Command sent successfully")
        
        # Listen for response
        print("Waiting for response...")
        try:
            data, addr = udp_socket.recvfrom(4096)
            response = data.decode('utf-8')
            print(f"Received response from {addr}: {response}")
        except socket.timeout:
            print("No response received (timeout)")
            
    except Exception as e:
        print(f"Error sending command: {e}")
    
    udp_socket.close()

if __name__ == "__main__":
    test_discovery()
