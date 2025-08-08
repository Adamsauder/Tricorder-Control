#!/usr/bin/env python3
"""
Test script for the enhanced LED functions on IO21 with 3 external LEDs
"""

import json
import socket
import time
import sys

# Configuration
ESP32_IP = "192.168.1.100"  # Replace with your ESP32's IP
UDP_PORT = 8888
DEVICE_ID = "TRICORDER_001"

def send_udp_command(command):
    """Send a UDP command to the ESP32 and return the response"""
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5.0)  # 5 second timeout
        
        # Send command
        command_json = json.dumps(command)
        print(f"Sending: {command_json}")
        sock.sendto(command_json.encode(), (ESP32_IP, UDP_PORT))
        
        # Wait for response
        response, addr = sock.recvfrom(1024)
        response_data = json.loads(response.decode())
        print(f"Response: {response_data}")
        
        sock.close()
        return response_data
        
    except socket.timeout:
        print("❌ Command timed out - check ESP32 connection")
        return None
    except Exception as e:
        print(f"❌ Error sending command: {e}")
        return None

def test_basic_led_functions():
    """Test basic LED color and brightness functions"""
    print("\n=== Testing Basic LED Functions ===")
    
    # Test LED color changes
    colors = [
        {"r": 255, "g": 0, "b": 0},    # Red
        {"r": 0, "g": 255, "b": 0},    # Green  
        {"r": 0, "g": 0, "b": 255},    # Blue
        {"r": 255, "g": 255, "b": 0},  # Yellow
        {"r": 255, "g": 0, "b": 255},  # Magenta
        {"r": 0, "g": 255, "b": 255},  # Cyan
        {"r": 255, "g": 255, "b": 255} # White
    ]
    
    color_names = ["Red", "Green", "Blue", "Yellow", "Magenta", "Cyan", "White"]
    
    for i, (color, name) in enumerate(zip(colors, color_names)):
        print(f"\n🔴 Setting all LEDs to {name}")
        command = {
            "commandId": f"test_color_{i}",
            "action": "set_led_color",
            "parameters": color
        }
        send_udp_command(command)
        time.sleep(1)
    
    # Test brightness levels
    print(f"\n💡 Testing brightness levels")
    command = {
        "commandId": "test_brightness",
        "action": "set_led_color",
        "parameters": {"r": 255, "g": 255, "b": 255}  # White for brightness test
    }
    send_udp_command(command)
    
    brightness_levels = [255, 128, 64, 16, 255]  # Full, half, quarter, dim, back to full
    brightness_names = ["Full", "Half", "Quarter", "Dim", "Full again"]
    
    for brightness, name in zip(brightness_levels, brightness_names):
        print(f"💡 Setting brightness to {name} ({brightness})")
        command = {
            "commandId": f"test_brightness_{brightness}",
            "action": "set_led_brightness", 
            "parameters": {"brightness": brightness}
        }
        send_udp_command(command)
        time.sleep(1)

def test_individual_led_control():
    """Test individual LED control (0, 1, 2)"""
    print("\n=== Testing Individual LED Control ===")
    
    # Turn off all LEDs first
    command = {
        "commandId": "clear_all",
        "action": "set_led_color",
        "parameters": {"r": 0, "g": 0, "b": 0}
    }
    send_udp_command(command)
    time.sleep(0.5)
    
    # Test each LED individually
    colors = [
        {"r": 255, "g": 0, "b": 0},    # Red
        {"r": 0, "g": 255, "b": 0},    # Green
        {"r": 0, "g": 0, "b": 255}     # Blue
    ]
    
    for led_index in range(3):
        print(f"🔆 Setting LED {led_index} to color {led_index}")
        command = {
            "commandId": f"test_individual_{led_index}",
            "action": "set_individual_led",
            "parameters": {
                "ledIndex": led_index,
                **colors[led_index]
            }
        }
        send_udp_command(command)
        time.sleep(1)
    
    time.sleep(2)  # Show the pattern

def test_led_effects():
    """Test scanner and pulse effects"""
    print("\n=== Testing LED Effects ===")
    
    # Test scanner effect
    print("🎯 Testing Scanner Effect (Kitt/Cylon style)")
    command = {
        "commandId": "test_scanner",
        "action": "scanner_effect",
        "parameters": {
            "r": 255,
            "g": 0, 
            "b": 0,
            "delay": 150  # 150ms delay between LEDs
        }
    }
    send_udp_command(command)
    time.sleep(3)  # Let the effect complete
    
    # Test pulse effect
    print("💓 Testing Pulse Effect (breathing)")
    command = {
        "commandId": "test_pulse",
        "action": "pulse_effect",
        "parameters": {
            "r": 0,
            "g": 0,
            "b": 255,
            "duration": 3000  # 3 second pulse
        }
    }
    send_udp_command(command)
    time.sleep(4)  # Let the effect complete

def test_ping():
    """Test basic connectivity"""
    print("\n=== Testing Connectivity ===")
    command = {
        "commandId": "ping_test",
        "action": "ping"
    }
    response = send_udp_command(command)
    if response and response.get("result") == "pong":
        print("✅ ESP32 is responding to commands")
        return True
    else:
        print("❌ ESP32 is not responding")
        return False

def main():
    """Run all LED tests"""
    if len(sys.argv) > 1:
        global ESP32_IP
        ESP32_IP = sys.argv[1]
        print(f"Using ESP32 IP: {ESP32_IP}")
    
    print(f"🔌 Testing 3 External LEDs on IO21")
    print(f"📡 Target ESP32: {ESP32_IP}:{UDP_PORT}")
    
    # Test connectivity first
    if not test_ping():
        print("\n❌ Cannot connect to ESP32. Check:")
        print("   1. ESP32 is powered and connected to WiFi")
        print("   2. IP address is correct")
        print("   3. Firewall allows UDP traffic")
        return
    
    try:
        # Run all tests
        test_basic_led_functions()
        test_individual_led_control()
        test_led_effects()
        
        # Final cleanup - turn off all LEDs
        print("\n🔄 Cleanup - turning off all LEDs")
        command = {
            "commandId": "cleanup",
            "action": "set_led_color",
            "parameters": {"r": 0, "g": 0, "b": 0}
        }
        send_udp_command(command)
        
        print("\n✅ All LED tests completed!")
        print("💡 Your 3 external LEDs on IO21 should be working properly")
        
    except KeyboardInterrupt:
        print("\n⚠️  Test interrupted by user")
        # Cleanup on interrupt
        command = {
            "commandId": "interrupt_cleanup", 
            "action": "set_led_color",
            "parameters": {"r": 0, "g": 0, "b": 0}
        }
        send_udp_command(command)

if __name__ == "__main__":
    main()
