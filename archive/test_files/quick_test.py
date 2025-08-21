#!/usr/bin/env python3
"""
Quick Server Test Script
Simple command-line tool to test tricorder communication
"""

import socket
import json
import time
import uuid
import sys

def send_command(ip_address, action, parameters=None):
    """Send a single command to tricorder and wait for response"""
    
    command_id = str(uuid.uuid4())
    command = {
        'commandId': command_id,
        'action': action,
        'parameters': parameters or {}
    }
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)  # 5 second timeout
    
    try:
        # Send command
        message = json.dumps(command)
        sock.sendto(message.encode('utf-8'), (ip_address, 8888))
        print(f"Sent: {action} to {ip_address}")
        print(f"Command: {message}")
        
        # Wait for response
        response_data, addr = sock.recvfrom(4096)
        response = json.loads(response_data.decode('utf-8'))
        
        print(f"Response from {addr[0]}:")
        print(json.dumps(response, indent=2))
        return response
        
    except socket.timeout:
        print(f"Timeout waiting for response from {ip_address}")
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None
    finally:
        sock.close()

def interactive_mode():
    """Interactive command-line interface"""
    print("Tricorder Quick Test")
    print("===================")
    
    # Get IP address
    ip_address = input("Enter tricorder IP address: ").strip()
    if not ip_address:
        print("IP address required")
        return
    
    print(f"\nConnecting to {ip_address}:8888")
    print("Available commands:")
    print("  1. status - Get device status")
    print("  2. list - List available videos")
    print("  3. play <filename> - Play video")
    print("  4. loop <filename> - Play video with loop")
    print("  5. stop - Stop video")
    print("  6. led <r> <g> <b> - Set LED color")
    print("  7. bright <0-255> - Set LED brightness")
    print("  8. quit - Exit")
    print()
    
    while True:
        try:
            cmd_input = input(f"{ip_address}> ").strip().lower()
            
            if not cmd_input:
                continue
                
            parts = cmd_input.split()
            cmd = parts[0]
            
            if cmd == 'quit' or cmd == 'exit':
                break
            elif cmd == 'status':
                send_command(ip_address, 'status')
            elif cmd == 'list':
                send_command(ip_address, 'list_videos')
            elif cmd == 'play' and len(parts) > 1:
                filename = ' '.join(parts[1:])
                send_command(ip_address, 'play_video', {'filename': filename, 'loop': False})
            elif cmd == 'loop' and len(parts) > 1:
                filename = ' '.join(parts[1:])
                send_command(ip_address, 'play_video', {'filename': filename, 'loop': True})
            elif cmd == 'stop':
                send_command(ip_address, 'stop_video')
            elif cmd == 'led' and len(parts) >= 4:
                try:
                    r, g, b = int(parts[1]), int(parts[2]), int(parts[3])
                    send_command(ip_address, 'set_led_color', {'r': r, 'g': g, 'b': b})
                except ValueError:
                    print("LED color values must be integers (0-255)")
            elif cmd == 'bright' and len(parts) >= 2:
                try:
                    brightness = int(parts[1])
                    send_command(ip_address, 'set_led_brightness', {'brightness': brightness})
                except ValueError:
                    print("Brightness must be an integer (0-255)")
            else:
                print("Unknown command or missing parameters")
                
            print()  # Empty line for readability
            
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Error: {e}")
    
    print("Goodbye!")

def quick_test(ip_address):
    """Run a quick test sequence"""
    print(f"Running quick test on {ip_address}")
    print("=" * 40)
    
    # Test 1: Get status
    print("1. Getting device status...")
    response = send_command(ip_address, 'status')
    if response:
        print("✓ Status command successful")
    else:
        print("✗ Status command failed")
        return
    
    print()
    
    # Test 2: List videos
    print("2. Listing videos...")
    response = send_command(ip_address, 'list_videos')
    if response:
        print("✓ List videos successful")
    else:
        print("✗ List videos failed")
    
    print()
    
    # Test 3: LED test
    print("3. Testing LEDs...")
    colors = [
        (255, 0, 0),    # Red
        (0, 255, 0),    # Green
        (0, 0, 255),    # Blue
        (0, 0, 0)       # Off
    ]
    
    for i, (r, g, b) in enumerate(colors):
        color_name = ['Red', 'Green', 'Blue', 'Off'][i]
        print(f"   Setting LEDs to {color_name}...")
        response = send_command(ip_address, 'set_led_color', {'r': r, 'g': g, 'b': b})
        if response:
            print(f"   ✓ {color_name} LED successful")
        else:
            print(f"   ✗ {color_name} LED failed")
        time.sleep(1)
    
    print("\nQuick test complete!")

def main():
    if len(sys.argv) > 1:
        ip_address = sys.argv[1]
        if len(sys.argv) > 2 and sys.argv[2] == 'test':
            quick_test(ip_address)
        else:
            # Single command mode
            action = sys.argv[2] if len(sys.argv) > 2 else 'status'
            send_command(ip_address, action)
    else:
        interactive_mode()

if __name__ == '__main__':
    main()
