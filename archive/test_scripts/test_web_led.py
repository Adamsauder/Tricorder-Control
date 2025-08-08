#!/usr/bin/env python3

import requests
import json
import time

def test_led_via_web_interface():
    """Test LED control through the web interface instead of bypassing sACN"""
    
    try:
        # Test with web interface API
        base_url = "http://localhost:8080"
        
        print("Testing LED control via web interface...")
        
        # Test setting LED color via web API
        test_color = {
            "r": 255,
            "g": 100, 
            "b": 0  # Orange color
        }
        
        print(f"Sending orange color via web API: {test_color}")
        
        response = requests.post(
            f"{base_url}/api/device/TRICORDER_001/led_color",
            json=test_color,
            timeout=10
        )
        
        if response.status_code == 200:
            print(f"✓ Web API response: {response.json()}")
            print("Check if LEDs show ORANGE color now...")
        else:
            print(f"✗ Web API failed: {response.status_code} - {response.text}")
            
        # Also try the direct command endpoint
        print("\nTrying device command endpoint...")
        command = {
            "action": "set_led_color",
            "r": 0,
            "g": 255, 
            "b": 128  # Cyan-green
        }
        
        response2 = requests.post(
            f"{base_url}/api/device/TRICORDER_001/command",
            json=command,
            timeout=10
        )
        
        if response2.status_code == 200:
            print(f"✓ Command API response: {response2.json()}")
            print("Check if LEDs show CYAN-GREEN color now...")
        else:
            print(f"✗ Command API failed: {response2.status_code} - {response2.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Network error: {e}")
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    test_led_via_web_interface()
