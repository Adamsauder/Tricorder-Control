#!/usr/bin/env python3
"""
Test script for firmware upload functionality
"""

import os
import requests
import time

# Test configuration
SERVER_URL = "http://localhost:5000"
TEST_DEVICE_ID = "TRICORDER_001"

def test_firmware_upload():
    """Test uploading a firmware file"""
    print("Testing firmware upload...")
    
    # Create a dummy firmware file for testing
    test_firmware_path = "test_firmware.bin"
    with open(test_firmware_path, "wb") as f:
        f.write(b"DUMMY_FIRMWARE_DATA" * 100)  # Create a small test file
    
    try:
        # Upload firmware file
        with open(test_firmware_path, "rb") as f:
            files = {"firmware": ("test_firmware.bin", f, "application/octet-stream")}
            response = requests.post(f"{SERVER_URL}/api/firmware/upload", files=files)
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Firmware upload successful: {result}")
            return result["filename"]
        else:
            print(f"❌ Firmware upload failed: {response.text}")
            return None
    
    except Exception as e:
        print(f"❌ Upload error: {e}")
        return None
    
    finally:
        # Clean up test file
        if os.path.exists(test_firmware_path):
            os.remove(test_firmware_path)

def test_list_firmware():
    """Test listing firmware files"""
    print("Testing firmware list...")
    
    try:
        response = requests.get(f"{SERVER_URL}/api/firmware/list")
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Firmware list retrieved: {len(result.get('firmware_files', []))} files")
            for file in result.get('firmware_files', []):
                print(f"  - {file['filename']} ({file['size']} bytes)")
            return True
        else:
            print(f"❌ Failed to list firmware: {response.text}")
            return False
    
    except Exception as e:
        print(f"❌ List error: {e}")
        return False

def test_device_ota_status():
    """Test checking device OTA status"""
    print("Testing device OTA status...")
    
    try:
        response = requests.get(f"{SERVER_URL}/api/devices/{TEST_DEVICE_ID}/ota_status")
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ OTA status retrieved: {result}")
            return True
        else:
            print(f"❌ Failed to get OTA status: {response.text}")
            return False
    
    except Exception as e:
        print(f"❌ OTA status error: {e}")
        return False

def main():
    """Run all firmware tests"""
    print("🚀 Starting Firmware Update System Tests")
    print("=" * 50)
    
    # Test firmware upload
    firmware_file = test_firmware_upload()
    
    # Test firmware listing
    test_list_firmware()
    
    # Test device OTA status
    test_device_ota_status()
    
    print("=" * 50)
    print("✅ Firmware update system tests completed!")
    print("\nNext steps:")
    print("1. Start the Python server: python server/simple_server.py")
    print("2. Start the web interface: npm run dev (in web/ directory)")
    print("3. Flash firmware with OTA support to your ESP32 devices")
    print("4. Use the web interface to upload and install firmware updates")

if __name__ == "__main__":
    main()
