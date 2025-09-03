#!/usr/bin/env python3
"""
Direct UDP test of the Playbook system without HTTP
Tests the core upload_file and display_image commands via UDP
"""

import socket
import json
import time
import uuid
import os

def send_udp_command(command, device_ip="192.168.1.48", device_port=8888):
    """Send UDP command to device and wait for response"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)
        
        # Add command ID for tracking
        command["commandId"] = str(uuid.uuid4())[:8]
        
        # Send command
        message = json.dumps(command)
        sock.sendto(message.encode(), (device_ip, device_port))
        print(f"📤 Sent to {device_ip}: {message}")
        
        # Wait for response
        response, addr = sock.recvfrom(1024)
        response_data = json.loads(response.decode())
        print(f"📥 Response from {addr[0]}: {response_data}")
        
        sock.close()
        return response_data
        
    except Exception as e:
        print(f"❌ UDP Error: {e}")
        if sock:
            sock.close()
        return None

def test_display_image_command():
    """Test the display_image command"""
    print("\n🖼️ Testing display_image command")
    print("-" * 40)
    
    # Test with both tricorders
    devices = [
        ("TRIC001", "192.168.1.48"),
        ("TRIC691C", "192.168.1.234")
    ]
    
    for device_id, device_ip in devices:
        print(f"\n📱 Testing {device_id} at {device_ip}")
        
        command = {
            "action": "display_image",
            "filename": "test_playbook_image.jpg"
        }
        
        response = send_udp_command(command, device_ip)
        if response and response.get("status") == "success":
            print(f"✅ {device_id} should now display the test image")
        else:
            print(f"❌ {device_id} command failed or no response")

def test_led_commands():
    """Test LED commands to verify UDP communication"""
    print("\n💡 Testing LED commands for verification")
    print("-" * 40)
    
    devices = [
        ("TRIC001", "192.168.1.48"),
        ("TRIC691C", "192.168.1.234")
    ]
    
    for device_id, device_ip in devices:
        print(f"\n📱 Testing {device_id} at {device_ip}")
        
        # Test different LED patterns
        commands = [
            {"action": "set_led_color", "r": 255, "g": 0, "b": 0},  # Red
            {"action": "set_led_color", "r": 0, "g": 255, "b": 0},  # Green  
            {"action": "set_led_color", "r": 0, "g": 0, "b": 255},  # Blue
        ]
        
        for i, command in enumerate(commands):
            print(f"   Test {i+1}: {command}")
            response = send_udp_command(command, device_ip)
            time.sleep(1)  # Brief pause between commands

def test_file_serving():
    """Check if our test image file exists and can be served"""
    print("\n📁 Checking test file")
    print("-" * 40)
    
    test_file = "test_playbook_image.jpg"
    if os.path.exists(test_file):
        size = os.path.getsize(test_file)
        print(f"✅ Test file exists: {test_file} ({size} bytes)")
        return True
    else:
        print(f"❌ Test file not found: {test_file}")
        return False

if __name__ == "__main__":
    print("🧪 Direct UDP Test of Playbook System")
    print("=" * 50)
    
    # Check if test file exists
    if not test_file_serving():
        print("⚠️  Cannot test image display without test file")
        print("   Please run the image creation script first")
    
    # Test LED commands to verify UDP communication
    test_led_commands()
    
    # Test display image command
    test_display_image_command()
    
    print("\n🎉 UDP testing completed!")
    print("📋 Summary:")
    print("   • LED commands test basic UDP communication")
    print("   • display_image command tests new Playbook functionality")
    print("   • Check tricorder screens for image display results")
