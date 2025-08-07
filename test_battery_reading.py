#!/usr/bin/env python3
"""
Test battery reading from ESP32 device
"""

import socket
import json
import time
import sys

def test_battery_reading():
    """Test battery reading from ESP32"""
    
    # ESP32 IP address
    esp32_ip = "192.168.1.48"
    esp32_port = 8888
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)  # 5 second timeout
    
    try:
        print(f"Testing battery reading from ESP32 at {esp32_ip}:{esp32_port}")
        print("=" * 60)
        
        # Test battery command
        battery_command = {
            "action": "get_battery",
            "commandId": f"battery_test_{int(time.time())}"
        }
        
        print(f"Sending battery command: {json.dumps(battery_command)}")
        
        # Send command
        message = json.dumps(battery_command).encode('utf-8')
        sock.sendto(message, (esp32_ip, esp32_port))
        
        # Wait for response
        print("Waiting for battery response...")
        data, addr = sock.recvfrom(1024)
        response_str = data.decode('utf-8')
        
        print(f"Raw response: {response_str}")
        
        try:
            response = json.loads(response_str)
            print("\nParsed Battery Response:")
            print(f"  Device ID: {response.get('deviceId', 'N/A')}")
            print(f"  Command ID: {response.get('commandId', 'N/A')}")
            print(f"  Battery Voltage: {response.get('batteryVoltage', 'N/A')} V")
            print(f"  Battery Percentage: {response.get('batteryPercentage', 'N/A')}%")
            print(f"  Battery Status: {response.get('batteryStatus', 'N/A')}")
            
            # Analyze the voltage
            voltage = response.get('batteryVoltage', 0)
            if voltage > 0:
                print(f"\nVoltage Analysis:")
                print(f"  Raw voltage reading: {voltage:.3f}V")
                if voltage < 1.0:
                    print("  ⚠️  Voltage very low - likely no voltage divider or wrong pin")
                elif voltage < 2.0:
                    print("  ⚠️  Voltage low - might need voltage divider adjustment")
                elif voltage > 5.0:
                    print("  ⚠️  Voltage too high - voltage divider may be wrong")
                else:
                    print("  ✓ Voltage in reasonable range")
            
        except json.JSONDecodeError as e:
            print(f"Failed to parse JSON response: {e}")
            print(f"Response was: {response_str}")
            
    except socket.timeout:
        print("❌ Timeout waiting for response from ESP32")
        print("   - Check if ESP32 is connected to WiFi")
        print("   - Verify IP address is correct")
        
    except Exception as e:
        print(f"❌ Error during battery test: {e}")
        
    finally:
        sock.close()

if __name__ == "__main__":
    test_battery_reading()
