#!/usr/bin/env python3

import socket
import json
import time

def test_led_colors():
    """Test multiple LED colors to verify the command system is working"""
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.settimeout(5)

    colors = [
        {'r': 0, 'g': 255, 'b': 0, 'name': 'GREEN'},
        {'r': 0, 'g': 0, 'b': 255, 'name': 'BLUE'},
        {'r': 255, 'g': 255, 'b': 0, 'name': 'YELLOW'},
        {'r': 255, 'g': 255, 'b': 255, 'name': 'WHITE'},
        {'r': 255, 'g': 0, 'b': 255, 'name': 'MAGENTA'}
    ]

    print("Testing LED colors...")
    print("Watch the ESP32 serial output and physical LEDs")
    print("=" * 50)

    for i, color in enumerate(colors):
        command = {
            'action': 'set_led_color',
            'r': color['r'], 
            'g': color['g'], 
            'b': color['b'],
            'commandId': f'test_{color["name"].lower()}_{int(time.time())}'
        }
        
        print(f'{i+1}. Testing {color["name"]} (R:{color["r"]}, G:{color["g"]}, B:{color["b"]})')
        udp_socket.sendto(json.dumps(command).encode(), ('192.168.1.48', 8888))
        
        try:
            response, addr = udp_socket.recvfrom(1024)
            print(f'   ✓ Response: {response.decode()}')
        except socket.timeout:
            print('   ✗ No response received')
        
        time.sleep(3)  # Wait 3 seconds between colors

    udp_socket.close()
    print("\nColor test complete!")
    print("If LEDs didn't change, check:")
    print("1. LED strip power connection")
    print("2. Data wire connection to pin 13")
    print("3. LED strip type (WS2812B)")

if __name__ == "__main__":
    test_led_colors()
