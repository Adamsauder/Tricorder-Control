import socket
import json
import time

def test_device_communication():
    """Test direct communication with the tricorder device"""
    
    print("🧪 Testing Tricorder Device Communication")
    print("="*50)
    
    device_ip = "192.168.1.48"
    device_port = 8888
    
    # Test 1: Send status command
    print("\n📡 Test 1: Sending status command...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        command = {
            'action': 'status',
            'command_id': f'test_status_{int(time.time())}'
        }
        message = json.dumps(command)
        print(f"   Sending: {message}")
        
        sock.sendto(message.encode(), (device_ip, device_port))
        
        # Listen for response
        sock.settimeout(5)
        response, addr = sock.recvfrom(1024)
        response_str = response.decode()
        print(f"   ✅ Response: {response_str}")
        
        # Parse JSON
        response_data = json.loads(response_str)
        device_id = response_data.get('deviceId', 'Unknown')
        print(f"   📱 Device ID: {device_id}")
        
        sock.close()
        
    except Exception as e:
        print(f"   ❌ Error: {e}")
        return False
    
    # Test 2: Send list_videos command
    print("\n📺 Test 2: Sending list_videos command...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        command = {
            'action': 'list_videos',
            'command_id': f'test_list_{int(time.time())}'
        }
        message = json.dumps(command)
        print(f"   Sending: {message}")
        
        sock.sendto(message.encode(), (device_ip, device_port))
        
        # Listen for response
        sock.settimeout(5)
        response, addr = sock.recvfrom(1024)
        response_str = response.decode()
        print(f"   ✅ Response: {response_str}")
        
        # Parse the response
        response_data = json.loads(response_str)
        result = response_data.get('result', '')
        
        if 'animations:' in result:
            animations_part = result.split('animations:')[1]
            animations = [name.strip() for name in animations_part.strip().split(',')]
            print(f"   🎬 Available animations: {animations}")
        else:
            print(f"   ⚠️ Unexpected result format: {result}")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"   ❌ Error: {e}")
        return False

if __name__ == "__main__":
    success = test_device_communication()
    print(f"\n{'✅ All tests passed!' if success else '❌ Some tests failed!'}")
