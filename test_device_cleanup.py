#!/usr/bin/env python3
"""
Test script for device timeout and cleanup functionality
"""

import requests
import time
import json

def test_device_cleanup():
    """Test the device cleanup functionality"""
    base_url = "http://localhost:8080"
    
    print("🧪 Testing Device Cleanup Functionality")
    print("=" * 50)
    
    try:
        # Check current devices
        print("\n1. Checking current devices...")
        response = requests.get(f"{base_url}/api/devices")
        if response.ok:
            devices = response.json()
            print(f"   Found {len(devices)} devices currently registered")
            for device in devices:
                print(f"   - {device['device_id']} ({device.get('ip_address', 'unknown IP')}) - Last seen: {device.get('last_seen', 'unknown')}")
        else:
            print(f"   ❌ Failed to get devices: {response.status_code}")
            return
        
        # Manual cleanup test
        print("\n2. Testing manual cleanup...")
        response = requests.post(f"{base_url}/api/devices/cleanup")
        if response.ok:
            result = response.json()
            print(f"   ✅ Manual cleanup successful")
            print(f"   📊 Active devices after cleanup: {result['active_devices']}")
            print(f"   ⏱️ Device timeout setting: {result['device_timeout']}s")
        else:
            print(f"   ❌ Manual cleanup failed: {response.status_code}")
        
        # Check devices after cleanup
        print("\n3. Checking devices after cleanup...")
        response = requests.get(f"{base_url}/api/devices")
        if response.ok:
            devices_after = response.json()
            print(f"   Found {len(devices_after)} devices after cleanup")
            if len(devices_after) < len(devices):
                print(f"   🧹 Cleanup removed {len(devices) - len(devices_after)} offline devices")
            else:
                print("   ℹ️ No devices were removed (all still online)")
        
        # Server info
        print("\n4. Checking server info...")
        response = requests.get(f"{base_url}/api/server_info")
        if response.ok:
            server_info = response.json()
            print(f"   Server IP: {server_info['server_ip']}")
            print(f"   Device count: {server_info['device_count']}")
            print(f"   Uptime: {server_info['uptime']:.1f}s")
        
        print("\n✅ Device cleanup test completed!")
        print("\nℹ️ To test automatic cleanup:")
        print("   1. Turn off a device (unplug or disconnect)")
        print("   2. Wait 30 seconds (configured timeout)")
        print("   3. Check the dashboard - device should be removed")
        
    except Exception as e:
        print(f"❌ Test failed: {e}")

if __name__ == "__main__":
    test_device_cleanup()
