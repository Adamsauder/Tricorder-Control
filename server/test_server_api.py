import requests
import json
import time

def test_server_api():
    """Test the server API endpoints"""
    
    base_url = "http://localhost:5000"
    device_ip = "192.168.1.48"
    
    print("🧪 Testing Server API")
    print("="*40)
    
    # Test 1: Add device by IP
    print(f"\n📡 Test 1: Adding device at {device_ip}")
    
    response = requests.post(f"{base_url}/api/add_device", 
                           json={"ip_address": device_ip})
    
    print(f"   Status Code: {response.status_code}")
    print(f"   Response: {response.json()}")
    
    # Wait a moment for device to respond
    print("\n⏳ Waiting 3 seconds for device to respond...")
    time.sleep(3)
    
    # Test 2: Check devices list
    print("\n📋 Test 2: Checking devices list")
    
    response = requests.get(f"{base_url}/api/devices")
    
    print(f"   Status Code: {response.status_code}")
    devices = response.json()
    print(f"   Devices found: {len(devices)}")
    
    for device in devices:
        print(f"     - {device.get('device_id', 'Unknown')} at {device.get('ip_address', 'Unknown')}")
    
    if devices:
        print("\n✅ Device successfully added to server!")
        return True
    else:
        print("\n❌ Device not found in server list")
        return False

if __name__ == "__main__":
    test_server_api()
