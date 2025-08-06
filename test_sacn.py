#!/usr/bin/env python3
"""
Test sACN functionality for the Tricorder Control System
Tests API endpoints and basic functionality
"""

import requests
import json
import time
import sys

# Test configuration
SERVER_URL = "http://localhost:8080"
TEST_DEVICE_IP = "192.168.1.100"

def test_api_endpoint(endpoint, method="GET", data=None):
    """Test an API endpoint and return the result"""
    try:
        url = f"{SERVER_URL}{endpoint}"
        
        if method == "GET":
            response = requests.get(url, timeout=5)
        elif method == "POST":
            response = requests.post(url, json=data, timeout=5)
        elif method == "DELETE":
            response = requests.delete(url, timeout=5)
        else:
            return False, f"Unsupported method: {method}"
        
        if response.status_code == 200:
            try:
                result = response.json()
                return True, result
            except:
                return True, response.text
        else:
            return False, f"HTTP {response.status_code}: {response.text}"
            
    except requests.exceptions.ConnectionError:
        return False, "Connection refused - is the server running?"
    except requests.exceptions.Timeout:
        return False, "Request timeout"
    except Exception as e:
        return False, f"Error: {str(e)}"

def run_sacn_tests():
    """Run comprehensive sACN API tests"""
    print("🎬 Testing sACN Lighting Control System")
    print("=" * 50)
    
    # Test 1: Check if server is running
    print("\n1. Server Status Check")
    success, result = test_api_endpoint("/api/server_info")
    if success:
        print(f"✅ Server is running: {result.get('server_ip', 'Unknown IP')}")
    else:
        print(f"❌ Server check failed: {result}")
        return False
    
    # Test 2: sACN Status
    print("\n2. sACN Status Check")
    success, result = test_api_endpoint("/api/sacn/status")
    if success:
        print(f"✅ sACN Status: {json.dumps(result, indent=2)}")
        connected = result.get('connected', False)
        if connected:
            print("🟢 sACN is connected and ready")
        else:
            print("🟡 sACN is initialized but not connected")
    else:
        print(f"❌ sACN status failed: {result}")
    
    # Test 3: Add a test device
    print("\n3. Add Test Device")
    success, result = test_api_endpoint("/api/sacn/device", "POST", {"ip": TEST_DEVICE_IP})
    if success:
        print(f"✅ Device add response: {json.dumps(result, indent=2)}")
    else:
        print(f"❌ Device add failed: {result}")
    
    # Test 4: Set global color
    print("\n4. Set Global Color (Red)")
    success, result = test_api_endpoint("/api/sacn/color", "POST", {"red": 255, "green": 0, "blue": 0})
    if success:
        print(f"✅ Color set response: {json.dumps(result, indent=2)}")
    else:
        print(f"❌ Color set failed: {result}")
    
    # Test 5: Set individual LED
    print("\n5. Set Individual LED (LED 0 to Blue)")
    success, result = test_api_endpoint("/api/sacn/led", "POST", {"led_index": 0, "red": 0, "green": 0, "blue": 255})
    if success:
        print(f"✅ LED set response: {json.dumps(result, indent=2)}")
    else:
        print(f"❌ LED set failed: {result}")
    
    # Test 6: Start an effect
    print("\n6. Start Rainbow Effect")
    success, result = test_api_endpoint("/api/sacn/effect", "POST", {"effect": "rainbow", "speed": 5})
    if success:
        print(f"✅ Effect start response: {json.dumps(result, indent=2)}")
        print("🌈 Rainbow effect should be running...")
        time.sleep(3)  # Let it run for a bit
    else:
        print(f"❌ Effect start failed: {result}")
    
    # Test 7: Stop effects
    print("\n7. Stop All Effects")
    success, result = test_api_endpoint("/api/sacn/effect", "DELETE")
    if success:
        print(f"✅ Effect stop response: {json.dumps(result, indent=2)}")
    else:
        print(f"❌ Effect stop failed: {result}")
    
    # Test 8: Turn off all lights
    print("\n8. Turn Off All Lights")
    success, result = test_api_endpoint("/api/sacn/color", "POST", {"red": 0, "green": 0, "blue": 0})
    if success:
        print(f"✅ Lights off response: {json.dumps(result, indent=2)}")
    else:
        print(f"❌ Lights off failed: {result}")
    
    # Test 9: Final status check
    print("\n9. Final Status Check")
    success, result = test_api_endpoint("/api/sacn/status")
    if success:
        print(f"✅ Final status: {json.dumps(result, indent=2)}")
    else:
        print(f"❌ Final status failed: {result}")
    
    print("\n" + "=" * 50)
    print("🎭 sACN API Test Complete!")
    print("\nTo test the web interface, open:")
    print(f"🔗 Main Dashboard: {SERVER_URL}/")
    print(f"🎬 sACN Control: {SERVER_URL}/sacn-control")
    
    return True

def test_web_pages():
    """Test that web pages load correctly"""
    print("\n🌐 Testing Web Pages")
    print("-" * 30)
    
    pages = [
        ("/", "Main Dashboard"),
        ("/simulator", "ESP32 Simulator"),
        ("/sacn-control", "sACN Control Dashboard")
    ]
    
    for path, name in pages:
        success, result = test_api_endpoint(path)
        if success and "<!DOCTYPE html>" in str(result):
            print(f"✅ {name}: Loads correctly")
        else:
            print(f"❌ {name}: Failed to load")

if __name__ == "__main__":
    print("🚀 Starting sACN Test Suite")
    
    # Run API tests
    if run_sacn_tests():
        # Test web pages
        test_web_pages()
        
        print("\n🎉 All tests completed!")
        print("\nNext steps:")
        print("1. Open the sACN control dashboard in your browser")
        print("2. Connect real tricorder devices for testing")
        print("3. Configure your lighting network settings")
        print("4. Test with actual sACN/DMX lighting equipment")
    else:
        print("\n❌ Tests failed - check server status")
        sys.exit(1)
