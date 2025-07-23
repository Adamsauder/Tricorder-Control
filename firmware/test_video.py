#!/usr/bin/env python3
"""
Tricorder Video Test Script
Tests video playback functionality via UDP commands
"""

import socket
import json
import time
import sys

# Configuration
TRICORDER_IP = "192.168.1.100"  # Update with your tricorder's IP
UDP_PORT = 8888
TIMEOUT = 5.0

def test_connection():
    """Test basic network connectivity to the tricorder"""
    import subprocess
    import platform
    
    print(f"Testing connection to {TRICORDER_IP}...")
    
    # Use ping to test basic connectivity
    param = "-n" if platform.system().lower() == "windows" else "-c"
    command = ["ping", param, "2", TRICORDER_IP]
    
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            print("✓ Device is reachable via ping")
            return True
        else:
            print("✗ Device is not responding to ping")
            print("Check that the device is powered on and connected to the network")
            return False
    except subprocess.TimeoutExpired:
        print("✗ Ping timed out")
        return False
    except Exception as e:
        print(f"✗ Ping test failed: {e}")
        return False

def send_command(command_data, wait_for_response=True):
    """Send a UDP command to the tricorder and optionally wait for response"""
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(TIMEOUT)
    
    try:
        # Send command
        command_json = json.dumps(command_data)
        print(f"Sending: {command_json}")
        
        sock.sendto(command_json.encode('utf-8'), (TRICORDER_IP, UDP_PORT))
        
        if wait_for_response:
            # Wait for response - increased buffer size for large video lists
            response, addr = sock.recvfrom(4096)  # Increased from 1024 to 4096 bytes
            response_data = json.loads(response.decode('utf-8'))
            print(f"Response: {json.dumps(response_data, indent=2)}")
            return response_data
        
        return None
        
    except socket.timeout:
        print("Timeout waiting for response")
        print("Troubleshooting tips:")
        print("1. Check that the tricorder is powered on and connected to WiFi")
        print("2. Verify the IP address is correct")
        print("3. Ensure both devices are on the same network")
        print("4. Check if UDP port 8888 is blocked by firewall")
        return None
    except socket.gaierror as e:
        print(f"DNS/Address error: {e}")
        print("Check that the IP address format is correct (e.g., 192.168.1.48)")
        return None
    except ConnectionRefusedError:
        print("Connection refused - device may not be listening on UDP port 8888")
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None
    finally:
        sock.close()

def test_video_commands():
    """Test various video-related commands"""
    
    print("=== Tricorder Video Test ===")
    print(f"Target IP: {TRICORDER_IP}:{UDP_PORT}")
    print()
    
    # Test basic connectivity first
    if not test_connection():
        print("Cannot reach device. Please check network connection.")
        return
    
    # Test 1: Get status
    print("1. Getting device status...")
    status = send_command({
        "action": "status",
        "commandId": "test_status"
    })
    
    if status:
        print(f"Device ID: {status.get('deviceId', 'Unknown')}")
        print(f"SD Card: {'✓' if status.get('sdCardInitialized') else '✗'}")
        print(f"Video Playing: {'✓' if status.get('videoPlaying') else '✗'}")
        if status.get('currentVideo'):
            print(f"Current Video: {status.get('currentVideo')}")
    
    print()
    
    # Test 2: List videos
    print("2. Listing available videos...")
    send_command({
        "action": "list_videos",
        "commandId": "test_list"
    })
    print("Check serial output for video list")
    print()
    
    # Test 3: Play a test video (if available)
    test_video = input("Enter video filename to test (or press Enter to skip): ").strip()
    
    if test_video:
        print(f"3. Playing video: {test_video}")
        response = send_command({
            "action": "play_video",
            "commandId": "test_play",
            "parameters": {
                "filename": test_video,
                "loop": True
            }
        })
        
        if response:
            print("Video command sent successfully")
            
            # Wait a bit, then check status
            time.sleep(2)
            print("Checking playback status...")
            status = send_command({
                "action": "status",
                "commandId": "test_status2"
            })
            
            if status and status.get('videoPlaying'):
                print(f"✓ Video is playing: {status.get('currentVideo')}")
                print(f"Current frame: {status.get('currentFrame', 0)}")
                print(f"Looping: {'✓' if status.get('videoLooping') else '✗'}")
                
                # Ask if user wants to stop
                stop = input("Stop video? (y/N): ").strip().lower()
                if stop == 'y':
                    print("Stopping video...")
                    send_command({
                        "action": "stop_video",
                        "commandId": "test_stop"
                    })
            else:
                print("✗ Video is not playing")
    
    # Test 4: Test LED controls (to verify other functionality still works)
    print()
    print("4. Testing LED controls...")
    
    print("Setting LEDs to red...")
    send_command({
        "action": "set_led_color",
        "commandId": "test_led_red",
        "parameters": {"r": 255, "g": 0, "b": 0}
    })
    
    time.sleep(1)
    
    print("Setting LEDs to green...")
    send_command({
        "action": "set_led_color",
        "commandId": "test_led_green",
        "parameters": {"r": 0, "g": 255, "b": 0}
    })
    
    time.sleep(1)
    
    print("Setting LEDs to blue...")
    send_command({
        "action": "set_led_color",
        "commandId": "test_led_blue",
        "parameters": {"r": 0, "g": 0, "b": 255}
    })
    
    print()
    print("=== Test Complete ===")

def interactive_mode():
    """Interactive command mode"""
    print("=== Interactive Mode ===")
    print("Available commands:")
    print("  status - Get device status")
    print("  list - List available videos")
    print("  play <video_name> - Play video (use base name without _001 etc.)")
    print("  stop - Stop current video")
    print("  led <r> <g> <b> - Set LED color")
    print("  quit - Exit")
    print()
    print("Example video names:")
    print("  play static_test")
    print("  play color_red") 
    print("  play startup")
    print("  play animated_test")
    print()
    
    command_id = 1
    
    while True:
        try:
            user_input = input("Command: ").strip()
            
            if not user_input:
                continue
                
            if user_input == "quit":
                break
                
            parts = user_input.split()
            cmd = parts[0].lower()
            
            if cmd == "status":
                send_command({
                    "action": "status",
                    "commandId": f"interactive_{command_id}"
                })
                
            elif cmd == "list":
                send_command({
                    "action": "list_videos",
                    "commandId": f"interactive_{command_id}"
                })
                
            elif cmd == "play" and len(parts) > 1:
                video_name = parts[1]
                loop = input("Loop video? (Y/n): ").strip().lower() != 'n'
                send_command({
                    "action": "play_video",
                    "commandId": f"interactive_{command_id}",
                    "parameters": {
                        "filename": video_name,  # Use base name, firmware will find the file
                        "loop": loop
                    }
                })
                
            elif cmd == "stop":
                send_command({
                    "action": "stop_video",
                    "commandId": f"interactive_{command_id}"
                })
                
            elif cmd == "led" and len(parts) == 4:
                try:
                    r, g, b = int(parts[1]), int(parts[2]), int(parts[3])
                    send_command({
                        "action": "set_led_color",
                        "commandId": f"interactive_{command_id}",
                        "parameters": {"r": r, "g": g, "b": b}
                    })
                except ValueError:
                    print("Invalid color values. Use numbers 0-255.")
                    
            else:
                print("Unknown command. Type 'quit' to exit.")
                print("Available: status, list, play <name>, stop, led <r> <g> <b>, quit")
            
            command_id += 1
            
        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        TRICORDER_IP = sys.argv[1]
    
    print(f"Using tricorder IP: {TRICORDER_IP}")
    
    mode = input("Run (t)est or (i)nteractive mode? (t/i): ").strip().lower()
    
    if mode == 'i':
        interactive_mode()
    else:
        test_video_commands()
