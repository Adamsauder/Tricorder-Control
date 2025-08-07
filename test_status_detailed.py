#!/usr/bin/env python3
"""
Get detailed status from ESP32 including battery debug info
"""

import socket
import json
import time

def test_status():
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5)

    # ESP32 details
    esp32_ip = '192.168.1.48'
    esp32_port = 8888

    # Send status request
    command = {
        'action': 'status',
        'commandId': 'status_test_' + str(int(time.time()))
    }

    command_json = json.dumps(command)
    print(f'Sending status request to {esp32_ip}:{esp32_port}')
    print(f'Command: {command_json}')

    try:
        # Send command
        sock.sendto(command_json.encode(), (esp32_ip, esp32_port))
        
        # Wait for response
        print('Waiting for response...')
        data, addr = sock.recvfrom(1024)
        response = data.decode()
        
        print(f'Raw response from {addr}: {response}')
        
        # Parse and display status info
        try:
            status_data = json.loads(response)
            print('\n=== DEVICE STATUS ===')
            print(f'Device ID: {status_data.get("deviceId", "N/A")}')
            print(f'Firmware Version: {status_data.get("firmwareVersion", "N/A")}')
            print(f'WiFi Connected: {status_data.get("wifiConnected", "N/A")}')
            print(f'IP Address: {status_data.get("ipAddress", "N/A")}')
            print(f'Free Heap: {status_data.get("freeHeap", "N/A")} bytes')
            print(f'Uptime: {status_data.get("uptime", "N/A")} ms')
            print('\n=== BATTERY INFO ===')
            print(f'Battery Voltage: {status_data.get("batteryVoltage", "N/A")}V')
            print(f'Battery Percentage: {status_data.get("batteryPercentage", "N/A")}%')
            print(f'Battery Status: {status_data.get("batteryStatus", "N/A")}')
            print('====================')
            
            # Check if this is new firmware
            firmware_version = status_data.get("firmwareVersion", "")
            if firmware_version:
                print(f'\nFirmware version check: {firmware_version}')
                print('Expected version: 0.3 (with battery calibration)')
            
        except json.JSONDecodeError:
            print('Failed to parse JSON response')
            
    except socket.timeout:
        print('❌ Timeout waiting for response from ESP32')
        print('Make sure the device is powered on and connected to WiFi')
    except Exception as e:
        print(f'❌ Error: {e}')
    finally:
        sock.close()

if __name__ == "__main__":
    test_status()
