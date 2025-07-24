#!/usr/bin/env python3
"""
Quick test script to check both ESP32 devices
"""
import socket
import json
import time

def test_device(ip_address):
    print(f"\n🔍 Testing device at {ip_address}...")
    
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(3.0)
        
        # Send discovery command
        command = {
            "commandId": f"test-{int(time.time())}",
            "action": "discovery",
            "parameters": {}
        }
        
        message = json.dumps(command)
        sock.sendto(message.encode('utf-8'), (ip_address, 8888))
        
        print(f"📤 Sent discovery command to {ip_address}")
        
        # Wait for response
        try:
            data, addr = sock.recvfrom(1024)
            response = json.loads(data.decode('utf-8'))
            
            print(f"✅ Response from {addr}:")
            print(f"   Device ID: {response.get('deviceId', 'Unknown')}")
            print(f"   Type: {response.get('type', 'Unknown')}")
            print(f"   Firmware: {response.get('firmwareVersion', 'Unknown')}")
            print(f"   IP: {response.get('ipAddress', 'Unknown')}")
            
            return response.get('deviceId', 'Unknown')
            
        except socket.timeout:
            print(f"❌ No response from {ip_address} (timeout)")
            return None
            
        except Exception as e:
            print(f"❌ Error parsing response from {ip_address}: {e}")
            return None
            
    except Exception as e:
        print(f"❌ Failed to test {ip_address}: {e}")
        return None
    finally:
        sock.close()

def main():
    print("🔍 Testing ESP32 Tricorder Devices")
    print("=" * 50)
    
    # Test common IP addresses - adjust these to match your devices
    test_ips = [
        "192.168.1.48",  # Common first device IP
        "192.168.1.49",  # Common second device IP
        "192.168.1.50",  # Alternative
        "192.168.1.51",  # Alternative
    ]
    
    found_devices = []
    
    for ip in test_ips:
        device_id = test_device(ip)
        if device_id:
            found_devices.append((ip, device_id))
    
    print(f"\n📋 Summary:")
    print(f"Found {len(found_devices)} device(s):")
    
    for ip, device_id in found_devices:
        print(f"  • {device_id} at {ip}")
    
    if len(found_devices) == 0:
        print("❌ No devices found. Check:")
        print("  1. Are both ESP32s powered on?")
        print("  2. Are they connected to WiFi?")
        print("  3. Are the IP addresses correct?")
        print("  4. Is the firmware flashed with UDP support?")
    
    elif len(found_devices) == 1:
        print("⚠️  Only one device found. Check:")
        print("  1. Is the second ESP32 powered on?")
        print("  2. Is it connected to the same WiFi network?")
        print("  3. Does it have a different IP address?")
    
    else:
        print("✅ Multiple devices found! The server should see both.")

if __name__ == "__main__":
    main()
