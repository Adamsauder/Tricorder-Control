#!/usr/bin/env python3

import socket
import json
import time

def test_battery_reading():
    """Test battery reading from the ESP32 device"""
    
    # ESP32 device IP (adjust if needed)
    esp32_ip = "192.168.1.48"
    esp32_port = 8888
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)  # 5 second timeout
    
    try:
        # Send battery request command
        command = {
            "action": "get_battery",
            "commandId": "battery_test_001"
        }
        
        command_json = json.dumps(command)
        print(f"Sending battery request to {esp32_ip}:{esp32_port}")
        print(f"Command: {command_json}")
        
        # Send the command
        sock.sendto(command_json.encode(), (esp32_ip, esp32_port))
        
        # Wait for response
        print("Waiting for battery response...")
        data, addr = sock.recvfrom(1024)
        response = json.loads(data.decode())
        
        print(f"\n=== BATTERY RESPONSE ===")
        print(f"Response from: {addr}")
        print(f"Device ID: {response.get('deviceId', 'Unknown')}")
        print(f"Battery Voltage: {response.get('batteryVoltage', 'N/A')}V")
        print(f"Battery Percentage: {response.get('batteryPercentage', 'N/A')}%")
        print(f"Battery Status: {response.get('batteryStatus', 'Unknown')}")
        print(f"Command ID: {response.get('commandId', 'N/A')}")
        print("========================\n")
        
        # Check if we got valid battery data
        voltage = response.get('batteryVoltage', 0)
        percentage = response.get('batteryPercentage', 0)
        status = response.get('batteryStatus', 'Unknown')
        
        if voltage > 0:
            print(f"✓ Battery monitoring is working!")
            print(f"  Voltage: {voltage}V")
            print(f"  Percentage: {percentage}%")
            print(f"  Status: {status}")
            
            if voltage < 3.0:
                print("⚠️  WARNING: Voltage seems very low - may need calibration")
            elif voltage > 4.5:
                print("⚠️  WARNING: Voltage seems very high - may need calibration")
            else:
                print("✓ Voltage reading seems reasonable")
        else:
            print("❌ Battery voltage reading is 0V - ADC not working properly")
            print("   Possible issues:")
            print("   - ADC pin not configured correctly")
            print("   - No voltage present on GPIO34")
            print("   - Voltage divider circuit issue")
        
    except socket.timeout:
        print("❌ Timeout waiting for response from ESP32")
        print("   Check that:")
        print("   - ESP32 is powered on and connected to WiFi")
        print("   - IP address is correct (currently using 192.168.1.48)")
        print("   - UDP port 8888 is accessible")
        
    except json.JSONDecodeError as e:
        print(f"❌ Failed to parse JSON response: {e}")
        print(f"Raw response: {data}")
        
    except Exception as e:
        print(f"❌ Error testing battery: {e}")
        
    finally:
        sock.close()

if __name__ == "__main__":
    print("ESP32 Battery Monitor Test")
    print("=========================")
    test_battery_reading()
