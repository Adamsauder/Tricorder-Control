#!/usr/bin/env python3
"""
Test script for Polyinoculator devices
Tests UDP control commands for ESP32-C3 based polyinoculators
"""

import socket
import json
import time
import uuid

# Configuration
POLYINOCULATOR_IP = "192.168.1.49"  # Update with actual IP
UDP_PORT = 8888

def send_command(ip, command_data):
    """Send UDP command to polyinoculator"""
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5.0)
        
        # Add command ID for tracking
        command_data["commandId"] = str(uuid.uuid4())
        
        # Convert to JSON
        json_data = json.dumps(command_data)
        print(f"Sending to {ip}: {json_data}")
        
        # Send command
        sock.sendto(json_data.encode(), (ip, UDP_PORT))
        
        # Wait for response
        response, addr = sock.recvfrom(1024)
        response_data = json.loads(response.decode())
        print(f"Response from {addr}: {response_data}")
        
        sock.close()
        return response_data
        
    except Exception as e:
        print(f"Error sending command: {e}")
        return None

def test_polyinoculator_discovery():
    """Test device discovery"""
    print("🔍 Testing polyinoculator discovery...")
    command = {
        "action": "discovery"
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_set_led_color(r, g, b):
    """Test setting all LEDs to a color"""
    print(f"🎨 Testing set LED color: R={r}, G={g}, B={b}")
    command = {
        "action": "set_led_color",
        "r": r,
        "g": g,
        "b": b
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_set_brightness(brightness):
    """Test setting LED brightness"""
    print(f"💡 Testing set brightness: {brightness}")
    command = {
        "action": "set_brightness",
        "brightness": brightness
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_set_individual_led(led_index, r, g, b):
    """Test setting individual LED"""
    print(f"🎯 Testing individual LED {led_index}: R={r}, G={g}, B={b}")
    command = {
        "action": "set_individual_led",
        "ledIndex": led_index,
        "r": r,
        "g": g,
        "b": b
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_rainbow():
    """Test rainbow effect"""
    print("🌈 Testing rainbow effect...")
    command = {
        "action": "rainbow"
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_scanner(r=255, g=0, b=0):
    """Test scanner effect"""
    print(f"📡 Testing scanner effect: R={r}, G={g}, B={b}")
    command = {
        "action": "scanner",
        "r": r,
        "g": g,
        "b": b
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_pulse(r=255, g=255, b=255):
    """Test pulse effect"""
    print(f"💓 Testing pulse effect: R={r}, G={g}, B={b}")
    command = {
        "action": "pulse",
        "r": r,
        "g": g,
        "b": b
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_toggle_sacn():
    """Test toggling SACN"""
    print("🔄 Testing SACN toggle...")
    command = {
        "action": "toggle_sacn"
    }
    return send_command(POLYINOCULATOR_IP, command)

def test_status():
    """Test status request"""
    print("📊 Testing status request...")
    command = {
        "action": "status"
    }
    return send_command(POLYINOCULATOR_IP, command)

def main():
    """Run all polyinoculator tests"""
    print("🧪 Starting Polyinoculator Test Suite")
    print("=" * 50)
    
    # Test discovery
    test_polyinoculator_discovery()
    time.sleep(1)
    
    # Test status
    test_status()
    time.sleep(1)
    
    # Test basic LED control
    test_set_led_color(255, 0, 0)  # Red
    time.sleep(2)
    
    test_set_led_color(0, 255, 0)  # Green
    time.sleep(2)
    
    test_set_led_color(0, 0, 255)  # Blue
    time.sleep(2)
    
    # Test brightness
    test_set_brightness(64)  # Dim
    time.sleep(1)
    
    test_set_brightness(255)  # Bright
    time.sleep(1)
    
    # Test individual LEDs
    test_set_led_color(0, 0, 0)  # Clear all
    time.sleep(1)
    
    for i in range(3):  # Test first 3 LEDs
        test_set_individual_led(i, 255, 0, 255)  # Magenta
        time.sleep(0.5)
    
    time.sleep(1)
    
    # Test effects
    test_rainbow()
    time.sleep(3)
    
    test_scanner(255, 255, 0)  # Yellow scanner
    time.sleep(3)
    
    test_pulse(0, 255, 255)  # Cyan pulse
    time.sleep(3)
    
    # Test SACN toggle
    test_toggle_sacn()
    time.sleep(1)
    
    test_toggle_sacn()  # Toggle back
    time.sleep(1)
    
    # Clear LEDs
    test_set_led_color(0, 0, 0)
    
    print("✅ All tests completed!")

if __name__ == "__main__":
    main()
