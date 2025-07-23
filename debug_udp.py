#!/usr/bin/env python3
"""
Debug UDP listener to test if the server can receive messages
"""

import socket
import json
import threading
import time

def test_udp_server():
    """Test UDP server functionality"""
    
    print("Testing UDP server functionality...")
    
    # Create UDP socket
    try:
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.bind(('', 8888))
        udp_socket.settimeout(1.0)
        print("✓ UDP socket created and bound to port 8888")
    except Exception as e:
        print(f"✗ Failed to create UDP socket: {e}")
        return
    
    # Start listener thread
    def listen():
        print("UDP listener started...")
        while True:
            try:
                data, addr = udp_socket.recvfrom(4096)
                message = data.decode('utf-8')
                print(f"📡 Received from {addr}: {message}")
                
                # Try to parse as JSON
                try:
                    parsed = json.loads(message)
                    print(f"📋 Parsed JSON: {parsed}")
                    
                    if 'deviceId' in parsed:
                        print(f"🎯 Device ID found: {parsed['deviceId']}")
                    
                except json.JSONDecodeError:
                    print("⚠️  Message is not valid JSON")
                    
            except socket.timeout:
                continue
            except Exception as e:
                print(f"❌ UDP error: {e}")
                break
    
    listener_thread = threading.Thread(target=listen, daemon=True)
    listener_thread.start()
    
    # Send test command to known device
    print("\nSending test command to 192.168.1.48...")
    
    try:
        command = {
            'commandId': 'test-123',
            'action': 'status',
            'parameters': {}
        }
        message = json.dumps(command)
        udp_socket.sendto(message.encode('utf-8'), ('192.168.1.48', 8888))
        print(f"📤 Sent: {message}")
        
        # Wait for response
        print("⏳ Waiting 5 seconds for response...")
        time.sleep(5)
        
    except Exception as e:
        print(f"❌ Failed to send test command: {e}")
    
    print("\nTest complete. Check output above for any received messages.")
    udp_socket.close()

if __name__ == "__main__":
    test_udp_server()
