#!/usr/bin/env python3
"""
Simple test using localhost - testing the Playbook system
"""

import requests
import json
import os

def test_localhost():
    print("🧪 Testing Playbook System via Localhost")
    print("=" * 45)
    
    # Try different localhost addresses
    base_urls = [
        "http://127.0.0.1:8080",
        "http://localhost:8080"
    ]
    
    for base_url in base_urls:
        print(f"\n🔍 Trying {base_url}...")
        try:
            response = requests.get(f"{base_url}/api/props", timeout=3)
            if response.status_code == 200:
                devices = response.json()
                print(f"✅ Connected! Found {len(devices)} devices")
                
                tricorders = [d for d in devices if d.get('type') == 'tricorder']
                print(f"📱 Tricorders: {len(tricorders)}")
                for device in tricorders:
                    print(f"   • {device['deviceId']} - {device.get('deviceLabel', 'Unknown')}")
                
                return base_url
            else:
                print(f"❌ HTTP {response.status_code}")
        except Exception as e:
            print(f"❌ Connection failed: {str(e)[:100]}...")
    
    print("\n❌ Could not connect to server on any URL")
    return None

def test_file_upload(server_url):
    print(f"\n📤 Testing file upload to {server_url}")
    
    test_file = "test_playbook_image.jpg"
    if not os.path.exists(test_file):
        print(f"❌ Test file {test_file} not found!")
        return False
    
    upload_url = f"{server_url}/api/props/tricorder/files/upload"
    print(f"📡 Upload URL: {upload_url}")
    
    try:
        with open(test_file, 'rb') as f:
            files = {'file': (test_file, f, 'image/jpeg')}
            response = requests.post(upload_url, files=files, timeout=30)
        
        print(f"📊 Response: {response.status_code}")
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Upload successful: {result}")
            return True
        else:
            print(f"❌ Upload failed: {response.text}")
            return False
    except Exception as e:
        print(f"❌ Upload error: {e}")
        return False

if __name__ == "__main__":
    server_url = test_localhost()
    if server_url:
        test_file_upload(server_url)
    else:
        print("\n🔍 Checking if server is running...")
        print("   Make sure the Python server is running:")
        print("   Ctrl+Shift+P → Tasks: Run Task → 'Start Python Server'")
