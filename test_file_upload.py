#!/usr/bin/env python3
"""
Test script for SD card file upload functionality
"""

import requests
import json
import time

# Server configuration
SERVER_HOST = "localhost"
SERVER_PORT = 8080
API_BASE = f"http://{SERVER_HOST}:{SERVER_PORT}"

def test_file_upload():
    print("🧪 Testing SD Card File Upload System")
    print("=" * 50)
    
    # First, check what devices are available
    print("📡 Checking available devices...")
    devices_response = requests.get(f"{API_BASE}/api/devices")
    
    if devices_response.status_code == 200:
        devices = devices_response.json()
        tricorder_devices = [d for d in devices if 'tric' in d.get('device_id', '').lower()]
        online_tricorders = [d for d in tricorder_devices if d.get('status') == 'online']
        
        print(f"📱 Found {len(tricorder_devices)} tricorder devices")
        print(f"🟢 {len(online_tricorders)} are online")
        
        if online_tricorders:
            for device in online_tricorders:
                print(f"   • {device['device_id']} ({device['ip_address']})")
        else:
            print("❌ No online tricorder devices found!")
            return
            
    else:
        print(f"❌ Failed to get devices: {devices_response.status_code}")
        return
    
    # Create a test file to upload
    test_content = "This is a test file for SD card upload functionality.\nCreated at: " + time.ctime()
    
    print("\n📄 Creating test file...")
    with open("test_upload.txt", "w") as f:
        f.write(test_content)
    
    print("📤 Uploading test file to tricorder devices...")
    
    # Upload the file using multipart form data
    with open("test_upload.txt", "rb") as f:
        files = {'files': ('test_upload.txt', f, 'text/plain')}
        
        upload_response = requests.post(
            f"{API_BASE}/api/props/tricorder/files/upload",
            files=files
        )
    
    if upload_response.status_code == 200:
        result = upload_response.json()
        print(f"✅ Upload command sent successfully!")
        print(f"   Success: {result.get('success')}")
        print(f"   Message: {result.get('message')}")
        print(f"   Successful uploads: {result.get('successful_uploads')}")
        print(f"   Total uploads: {result.get('total_uploads')}")
        print(f"   Files uploaded: {result.get('files_uploaded')}")
        
        # Show detailed results
        if 'results' in result:
            print("\n📋 Detailed results:")
            for device_result in result['results']:
                device_id = device_result['device_id']
                print(f"   📱 {device_id}:")
                for file_result in device_result['files']:
                    status = "✅" if file_result['success'] else "❌"
                    print(f"      {status} {file_result['filename']}: {file_result['message']}")
        
    else:
        print(f"❌ Upload failed: {upload_response.status_code}")
        print(f"Response: {upload_response.text}")
    
    # Clean up test file
    import os
    try:
        os.remove("test_upload.txt")
        print("\n🧹 Test file cleaned up")
    except:
        pass

if __name__ == "__main__":
    test_file_upload()
