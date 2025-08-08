#!/usr/bin/env python3
"""
Test battery calibration after voltage divider correction
"""

import socket
import json
import time

def test_battery():
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5)

    # ESP32 details
    esp32_ip = '192.168.1.48'
    esp32_port = 8888

    # Send battery status request
    command = {
        'action': 'get_battery',
        'commandId': 'battery_test_' + str(int(time.time()))
    }

    command_json = json.dumps(command)
    print(f'Sending battery request to {esp32_ip}:{esp32_port}')
    print(f'Command: {command_json}')

    try:
        # Send command
        sock.sendto(command_json.encode(), (esp32_ip, esp32_port))
        
        # Wait for response
        print('Waiting for response...')
        data, addr = sock.recvfrom(1024)
        response = data.decode()
        
        print(f'Response from {addr}: {response}')
        
        # Parse and display battery info
        try:
            battery_data = json.loads(response)
            print('\n=== BATTERY STATUS ===')
            print(f'Voltage: {battery_data.get("batteryVoltage", "N/A")}V')
            print(f'Percentage: {battery_data.get("batteryPercentage", "N/A")}%')
            print(f'Status: {battery_data.get("batteryStatus", "N/A")}')
            print('=====================')
            
            # Calculate expected values based on your measurement
            measured_voltage = battery_data.get('batteryVoltage', 0)
            if measured_voltage > 0:
                print(f'\nCalibration check:')
                print(f'Your multimeter read: 4.1V on battery')
                print(f'GPIO34 was reading: 0.036V (4.1V ÷ 114 = 0.036V)')
                print(f'ESP32 now reports: {measured_voltage}V')
                if abs(measured_voltage - 4.1) < 0.2:
                    print('✅ Calibration appears correct!')
                else:
                    print('❌ Calibration may need adjustment')
                    
                # Show percentage calculation details
                percentage = battery_data.get('batteryPercentage', 0)
                print(f'\nPercentage calculation:')
                print(f'Voltage range: 3.0V (0%) to 4.2V (100%)')
                print(f'Current voltage: {measured_voltage}V')
                print(f'Calculated percentage: {percentage}%')
                
                # Manual calculation check
                manual_percent = ((measured_voltage - 3.0) / (4.2 - 3.0)) * 100
                manual_percent = max(0, min(100, manual_percent))
                print(f'Manual calculation: {manual_percent:.1f}%')
            
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
    test_battery()
