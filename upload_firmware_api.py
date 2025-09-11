#!/usr/bin/env python3

import requests
import os

def upload_firmware_via_api(prop_type, firmware_path, server_url="http://localhost:8080"):
    """Upload firmware using the server's API endpoint"""
    
    if not os.path.exists(firmware_path):
        print(f"❌ Firmware file not found: {firmware_path}")
        return False
    
    # API endpoint for firmware updates
    api_url = f"{server_url}/api/props/{prop_type}/firmware/update"
    
    print(f"📡 Uploading firmware via API to {api_url}")
    print(f"📦 Firmware file: {firmware_path}")
    
    try:
        with open(firmware_path, 'rb') as f:
            files = {'firmware': f}
            
            print("🚀 Sending firmware update request...")
            response = requests.post(api_url, files=files, timeout=30)
            
            if response.status_code == 200:
                result = response.json()
                print("✅ API call successful!")
                print(f"📊 Response: {result}")
                return True
            else:
                print(f"❌ API call failed: {response.status_code}")
                print(f"📝 Response: {response.text}")
                return False
                
    except Exception as e:
        print(f"❌ Error uploading firmware: {e}")
        return False

if __name__ == "__main__":
    # Use the correct firmware file from the build
    firmware_path = r"C:\Prop Control\Prop-Control\firmware\tricorder\.pio\build\tricorder\firmware.bin"
    
    print("=== Tricorder Firmware Update via Server API ===")
    success = upload_firmware_via_api("tricorder", firmware_path)
    
    if success:
        print("✅ Firmware update initiated successfully!")
    else:
        print("❌ Failed to initiate firmware update")
