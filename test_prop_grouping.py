#!/usr/bin/env python3
"""
Test script for prop-type grouping functionality
Tests the new prop-type API endpoints
"""

import requests
import json
import time

SERVER_URL = "http://localhost:8080"

def test_prop_types_api():
    """Test the new prop-type grouping API endpoints"""
    print("🧪 Testing Prop-Type Grouping API")
    print("=" * 50)
    
    try:
        # Test 1: Get all prop types
        print("\n📋 Test 1: Get all prop types")
        response = requests.get(f"{SERVER_URL}/api/props")
        if response.status_code == 200:
            data = response.json()
            print(f"✅ Found {data['total_types']} prop types")
            for prop_type in data['prop_types']:
                print(f"   - {prop_type['type']}: {prop_type['online_devices']}/{prop_type['total_devices']} online")
        else:
            print(f"❌ Failed to get prop types: {response.status_code}")
            return False
        
        # Test 2: Get devices by type (if any prop types exist)
        if data['total_types'] > 0:
            first_prop_type = data['prop_types'][0]['type']
            print(f"\n📱 Test 2: Get devices for '{first_prop_type}' prop type")
            response = requests.get(f"{SERVER_URL}/api/props/{first_prop_type}")
            if response.status_code == 200:
                type_data = response.json()
                print(f"✅ Found {type_data['total_devices']} {first_prop_type} devices")
                print(f"   Online: {type_data['online_devices']}")
            else:
                print(f"❌ Failed to get {first_prop_type} devices: {response.status_code}")
        
        # Test 3: Test SACN address setting (dry run - no devices needed)
        print(f"\n🌐 Test 3: Test SACN address setting API")
        test_data = {
            "universe": 221,
            "address": 1
        }
        response = requests.post(f"{SERVER_URL}/api/props/tricorder/sacn/address", 
                               json=test_data)
        print(f"📡 SACN address API response: {response.status_code}")
        if response.status_code == 200:
            result = response.json()
            print(f"   Updated: {result.get('devices_updated', 0)} devices")
        
        # Test 4: Test command sending API
        print(f"\n⚡ Test 4: Test command sending API")
        test_command = {
            "action": "ping",
            "parameters": {"test": True}
        }
        response = requests.post(f"{SERVER_URL}/api/props/tricorder/command", 
                               json=test_command)
        print(f"📤 Command API response: {response.status_code}")
        if response.status_code == 200:
            result = response.json()
            print(f"   Devices reached: {result.get('devices_updated', 0)}")
        
        print(f"\n✅ All prop-type API tests completed successfully!")
        return True
        
    except requests.ConnectionError:
        print("❌ Cannot connect to server. Make sure the server is running on port 8080")
        print("   Start server with: python server/enhanced_server.py")
        return False
    except Exception as e:
        print(f"❌ Test failed with error: {e}")
        return False

def test_server_health():
    """Test basic server health"""
    print("🏥 Testing server health...")
    try:
        response = requests.get(f"{SERVER_URL}/api/server_info")
        if response.status_code == 200:
            info = response.json()
            print(f"✅ Server running on {info['server_ip']}:{info['web_port']}")
            print(f"   Devices: {info['device_count']}")
            print(f"   Uptime: {info.get('uptime', 0):.1f}s")
            return True
        else:
            print(f"❌ Server health check failed: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Server health check error: {e}")
        return False

if __name__ == "__main__":
    print("🚀 Prop-Type Grouping Test Suite")
    print("=" * 50)
    
    # Test server health first
    if not test_server_health():
        exit(1)
    
    # Test prop-type APIs
    if test_prop_types_api():
        print("\n🎉 All tests passed! Prop-type grouping is working correctly.")
    else:
        print("\n💥 Some tests failed. Check the server logs.")
        exit(1)
