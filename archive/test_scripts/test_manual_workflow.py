"""
Simple script to test the full workflow manually
"""
import socket
import json
import time
import threading

# Global flag to track responses
received_responses = []

def listen_for_responses():
    """Listen for UDP responses from server"""
    global received_responses
    
    try:
        # Listen on a different port to avoid conflicts
        listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        listen_sock.bind(('', 8889))  # Different port
        listen_sock.settimeout(1.0)
        
        print("🎧 Listening for server responses on port 8889...")
        
        while True:
            try:
                data, addr = listen_sock.recvfrom(4096)
                message = data.decode('utf-8')
                received_responses.append((addr, message))
                print(f"📡 Received from {addr}: {message}")
            except socket.timeout:
                continue
                
    except Exception as e:
        print(f"❌ Listener error: {e}")

def test_manual_workflow():
    """Test the manual workflow step by step"""
    
    print("🧪 Testing Manual Workflow")
    print("="*50)
    
    device_ip = "192.168.1.48"
    device_port = 8888
    
    # Step 1: Send status command (like "Add Device by IP" does)
    print(f"\n📡 Step 1: Sending status command to {device_ip}")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        command = {
            'action': 'status',
            'commandId': f'manual_status_{int(time.time())}'
        }
        message = json.dumps(command)
        print(f"   Sending: {message}")
        
        sock.sendto(message.encode(), (device_ip, device_port))
        
        # Listen for response
        sock.settimeout(3)
        response, addr = sock.recvfrom(1024)
        response_str = response.decode()
        print(f"   ✅ Status Response: {response_str}")
        
        sock.close()
        
    except Exception as e:
        print(f"   ❌ Status Error: {e}")
        return False
    
    # Step 2: Send list_videos command
    print(f"\n🎬 Step 2: Sending list_videos command to {device_ip}")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        command = {
            'action': 'list_videos',
            'commandId': f'manual_list_{int(time.time())}'
        }
        message = json.dumps(command)
        print(f"   Sending: {message}")
        
        sock.sendto(message.encode(), (device_ip, device_port))
        
        # Listen for response
        sock.settimeout(3)
        response, addr = sock.recvfrom(1024)
        response_str = response.decode()
        print(f"   ✅ List Response: {response_str}")
        
        # Parse and show available animations
        response_data = json.loads(response_str)
        result = response_data.get('result', '')
        
        if 'animations:' in result:
            animations_part = result.split('animations:')[1]
            animations = [name.strip() for name in animations_part.strip().split(',')]
            print(f"   🎬 Available animations: {animations}")
            
            # This is what should populate the dropdown
            print(f"   ✅ These should appear in dropdown: {', '.join(animations)}")
        else:
            print(f"   ⚠️ Unexpected result format: {result}")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"   ❌ List Error: {e}")
        return False

if __name__ == "__main__":
    # Start response listener in background
    # listener_thread = threading.Thread(target=listen_for_responses, daemon=True)
    # listener_thread.start()
    
    success = test_manual_workflow()
    
    print(f"\n{'✅ Manual workflow successful!' if success else '❌ Manual workflow failed!'}")
    print("\nIf this works but the web interface doesn't, the issue is in:")
    print("1. WebSocket connection between browser and server")
    print("2. Device not being added to server's devices dictionary")
    print("3. send_command WebSocket event handler")
