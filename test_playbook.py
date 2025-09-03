#!/usr/bin/env python3
"""
Test script for the Playbook display_image functionality
"""

import requests
import json
import time

# Configuration
SERVER_URL = "http://localhost:8080"
DEVICE_ID = "TRIC001"  # Update this to match your device
IMAGE_FILENAME = "test_image.jpg"  # Test image filename

def test_display_image():
    """Test sending a display_image command to a tricorder"""
    
    print(f"🧪 Testing display_image command for device: {DEVICE_ID}")
    print(f"📱 Image filename: {IMAGE_FILENAME}")
    
    # Prepare the command
    command_data = {
        "device_id": DEVICE_ID,
        "action": "display_image",
        "parameters": {
            "filename": IMAGE_FILENAME
        }
    }
    
    try:
        # Send the command
        print(f"📤 Sending command to {SERVER_URL}/api/command")
        response = requests.post(
            f"{SERVER_URL}/api/command",
            json=command_data,
            headers={'Content-Type': 'application/json'},
            timeout=10
        )
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Command sent successfully!")
            print(f"📋 Response: {json.dumps(result, indent=2)}")
            print(f"🆔 Command ID: {result.get('command_id', 'N/A')}")
        else:
            print(f"❌ Failed to send command")
            print(f"📋 Status: {response.status_code}")
            print(f"📋 Response: {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Network error: {e}")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")

def test_get_devices():
    """Get list of available devices"""
    
    print(f"\n🔍 Getting device list from {SERVER_URL}/api/props")
    
    try:
        response = requests.get(f"{SERVER_URL}/api/props", timeout=10)
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Retrieved device data")
            
            # Find tricorder devices
            tricorder_devices = []
            for prop_type in result.get('prop_types', []):
                if prop_type['type'] == 'tricorder':
                    tricorder_devices = prop_type['devices']
                    break
            
            if tricorder_devices:
                print(f"📱 Found {len(tricorder_devices)} tricorder device(s):")
                for device in tricorder_devices:
                    status = "🟢 online" if device.get('status') == 'online' else "🔴 offline"
                    sd_status = "💾 ready" if device.get('sd_card_initialized') else "💾 not ready"
                    print(f"  - {device.get('device_id')} ({device.get('ip_address')}) - {status}, {sd_status}")
            else:
                print("❌ No tricorder devices found")
                
        else:
            print(f"❌ Failed to get devices: {response.status_code}")
            
    except Exception as e:
        print(f"❌ Error getting devices: {e}")

if __name__ == "__main__":
    print("🎬 Prop Control System - Playbook Test")
    print("=" * 50)
    
    # First get device list
    test_get_devices()
    
    # Wait a moment
    time.sleep(2)
    
    # Test display command
    test_display_image()
    
    print("\n✅ Test completed!")
