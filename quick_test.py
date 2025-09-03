#!/usr/bin/env python3
"""
Simple test for the file upload endpoint
"""

import requests
import time

print("🧪 Testing File Upload to Tricorders")
print("=" * 40)

# Test 1: Check if devices are available
print("📡 Getting device list...")
try:
    response = requests.get("http://localhost:8080/api/devices")
    if response.status_code == 200:
        devices = response.json()
        tricorders = [d for d in devices if 'tric' in d['device_id'].lower()]
        print(f"✅ Found {len(tricorders)} tricorders:")
        for t in tricorders:
            print(f"   • {t['device_id']} at {t['ip_address']} ({t['status']})")
    else:
        print(f"❌ Failed to get devices: {response.status_code}")
        exit(1)
except Exception as e:
    print(f"❌ Connection error: {e}")
    exit(1)

# Test 2: Create and upload a test file
print("\n📄 Creating test file...")
test_content = f"Test file created at {time.ctime()}\nThis is a test for SD card upload!"

with open("upload_test.txt", "w") as f:
    f.write(test_content)

print("📤 Uploading test file...")
try:
    with open("upload_test.txt", "rb") as f:
        files = {'files': ('upload_test.txt', f, 'text/plain')}
        response = requests.post("http://localhost:8080/api/props/tricorder/files/upload", files=files)
    
    if response.status_code == 200:
        result = response.json()
        print("✅ Upload successful!")
        print(f"   Message: {result.get('message')}")
        print(f"   Success count: {result.get('successful_uploads')}/{result.get('total_uploads')}")
        print(f"   Files: {result.get('files_uploaded')}")
    else:
        print(f"❌ Upload failed: {response.status_code}")
        print(f"   Response: {response.text}")
        
except Exception as e:
    print(f"❌ Upload error: {e}")

# Test 3: Test image display command
print("\n🖼️  Testing image display...")
try:
    image_cmd = {
        "device_id": tricorders[0]['device_id'] if tricorders else "TRIC001",
        "action": "display_image", 
        "parameters": {"filename": "test_image.jpg"}
    }
    
    response = requests.post("http://localhost:8080/api/command", json=image_cmd)
    
    if response.status_code == 200:
        result = response.json()
        print("✅ Display command sent!")
        print(f"   Status: {result.get('status')}")
        print(f"   Command ID: {result.get('command_id')}")
    else:
        print(f"❌ Display command failed: {response.status_code}")
        
except Exception as e:
    print(f"❌ Display command error: {e}")

# Cleanup
import os
try:
    os.remove("upload_test.txt")
    print("\n🧹 Test file cleaned up")
except:
    pass

print("\n🎉 Test complete!")
