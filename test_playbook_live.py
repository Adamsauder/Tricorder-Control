#!/usr/bin/env python3
"""
Live test of the Playbook file upload system
"""

import requests
import json
import os
import time

# Server configuration
SERVER_URL = "http://192.168.1.24:8080"  # Use the actual server IP from logs
UPLOAD_ENDPOINT = f"{SERVER_URL}/api/props/tricorder/files/upload"
PROPS_ENDPOINT = f"{SERVER_URL}/api/props"
COMMAND_ENDPOINT = f"{SERVER_URL}/api/command"

def test_playbook_system():
    print("🎬 Live Playbook System Test")
    print("=" * 50)
    
    # Step 1: Check connected devices
    print(f"\n🔍 Checking devices at {PROPS_ENDPOINT}")
    try:
        response = requests.get(PROPS_ENDPOINT, timeout=5)
        if response.status_code == 200:
            devices = response.json()
            tricorders = [d for d in devices if d.get('type') == 'tricorder']
            print(f"✅ Found {len(tricorders)} tricorder(s):")
            for device in tricorders:
                print(f"   📱 {device['deviceId']} at {device.get('ipAddress', 'unknown IP')}")
            
            if not tricorders:
                print("❌ No tricorders found!")
                return False
        else:
            print(f"❌ Failed to get devices: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error checking devices: {e}")
        return False
    
    # Step 2: Test file upload
    test_file = "test_playbook_image.jpg"
    if not os.path.exists(test_file):
        print(f"❌ Test file {test_file} not found!")
        return False
    
    print(f"\n📤 Uploading {test_file} to tricorders...")
    try:
        with open(test_file, 'rb') as f:
            files = {'file': (test_file, f, 'image/jpeg')}
            response = requests.post(UPLOAD_ENDPOINT, files=files, timeout=30)
        
        if response.status_code == 200:
            result = response.json()
            print("✅ Upload successful!")
            print(f"   📄 Response: {result}")
        else:
            print(f"❌ Upload failed: {response.status_code}")
            print(f"   📄 Response: {response.text}")
            return False
    except Exception as e:
        print(f"❌ Upload error: {e}")
        return False
    
    # Step 3: Test display command
    print(f"\n🖼️ Testing display_image command...")
    for device in tricorders:
        device_id = device['deviceId']
        print(f"   📱 Sending to {device_id}...")
        
        command = {
            "deviceId": device_id,
            "action": "display_image",
            "filename": test_file
        }
        
        try:
            response = requests.post(COMMAND_ENDPOINT, json=command, timeout=10)
            if response.status_code == 200:
                result = response.json()
                print(f"   ✅ Command sent to {device_id}: {result}")
            else:
                print(f"   ❌ Command failed for {device_id}: {response.status_code}")
        except Exception as e:
            print(f"   ❌ Command error for {device_id}: {e}")
    
    print("\n🎉 Live test completed!")
    return True

if __name__ == "__main__":
    test_playbook_system()
