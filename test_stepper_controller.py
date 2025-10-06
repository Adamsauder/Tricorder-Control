#!/usr/bin/env python3
"""
TMC2209 Stepper Controller Test Script

Tests the stepper controller firmware with various commands.
"""

import socket
import json
import time
import sys

class StepperController:
    def __init__(self, ip="255.255.255.255", port=8888):
        self.ip = ip
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.settimeout(2.0)
        
    def send_command(self, action, **kwargs):
        """Send a command to the stepper controller"""
        command = {
            "action": action,
            "commandId": f"{action}_{int(time.time() * 1000)}",
            **kwargs
        }
        
        message = json.dumps(command)
        print(f"Sending: {message}")
        
        self.sock.sendto(message.encode(), (self.ip, self.port))
        
        try:
            response, addr = self.sock.recvfrom(1024)
            response_data = json.loads(response.decode())
            print(f"Response from {addr}: {response_data}")
            return response_data
        except socket.timeout:
            print("No response received")
            return None
    
    def move_to(self, position):
        """Move to absolute position"""
        return self.send_command("move_to", position=position)
    
    def move_relative(self, steps):
        """Move relative number of steps"""
        return self.send_command("move_relative", steps=steps)
    
    def set_speed(self, speed):
        """Set movement speed in steps/second"""
        return self.send_command("set_speed", speed=speed)
    
    def set_current(self, current):
        """Set motor current in mA"""
        return self.send_command("set_current", current=current)
    
    def set_microsteps(self, microsteps):
        """Set microstepping (1,2,4,8,16,32,64,128,256)"""
        return self.send_command("set_microsteps", microsteps=microsteps)
    
    def stop(self):
        """Stop motor movement"""
        return self.send_command("stop")
    
    def enable(self):
        """Enable motor"""
        return self.send_command("enable")
    
    def disable(self):
        """Disable motor"""
        return self.send_command("disable")
    
    def home(self):
        """Start homing sequence"""
        return self.send_command("home")
    
    def status(self):
        """Get motor status"""
        return self.send_command("status")

def main():
    print("=== TMC2209 Stepper Controller Test ===")
    
    # Create controller instance
    stepper = StepperController()
    
    # Test sequence
    tests = [
        ("Get Status", lambda: stepper.status()),
        ("Enable Motor", lambda: stepper.enable()),
        ("Set Speed to 500", lambda: stepper.set_speed(500)),
        ("Set Current to 800mA", lambda: stepper.set_current(800)),
        ("Set Microsteps to 16", lambda: stepper.set_microsteps(16)),
        ("Move Forward 1600 steps", lambda: stepper.move_relative(1600)),
        ("Wait 3 seconds", lambda: time.sleep(3)),
        ("Move Back 800 steps", lambda: stepper.move_relative(-800)),
        ("Wait 2 seconds", lambda: time.sleep(2)),
        ("Move to Position 0", lambda: stepper.move_to(0)),
        ("Wait 2 seconds", lambda: time.sleep(2)),
        ("Stop Motor", lambda: stepper.stop()),
        ("Get Final Status", lambda: stepper.status()),
    ]
    
    try:
        for test_name, test_func in tests:
            print(f"\n--- {test_name} ---")
            result = test_func()
            if result is not None:
                print(f"Success: {result.get('message', 'OK')}")
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        stepper.stop()
    except Exception as e:
        print(f"Error: {e}")
    
    print("\n=== Test Complete ===")

def interactive_mode():
    """Interactive command mode"""
    print("=== Interactive Mode ===")
    print("Commands: move_to <pos>, move_rel <steps>, speed <sps>, current <ma>,")
    print("          microsteps <ms>, stop, enable, disable, home, status, quit")
    
    stepper = StepperController()
    
    while True:
        try:
            cmd = input("\nstepper> ").strip().split()
            if not cmd:
                continue
                
            action = cmd[0].lower()
            
            if action == "quit" or action == "exit":
                break
            elif action == "move_to" and len(cmd) > 1:
                stepper.move_to(int(cmd[1]))
            elif action == "move_rel" and len(cmd) > 1:
                stepper.move_relative(int(cmd[1]))
            elif action == "speed" and len(cmd) > 1:
                stepper.set_speed(int(cmd[1]))
            elif action == "current" and len(cmd) > 1:
                stepper.set_current(int(cmd[1]))
            elif action == "microsteps" and len(cmd) > 1:
                stepper.set_microsteps(int(cmd[1]))
            elif action == "stop":
                stepper.stop()
            elif action == "enable":
                stepper.enable()
            elif action == "disable":
                stepper.disable()
            elif action == "home":
                stepper.home()
            elif action == "status":
                stepper.status()
            else:
                print("Unknown command or missing parameter")
                
        except KeyboardInterrupt:
            break
        except ValueError:
            print("Invalid parameter (must be integer)")
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "interactive":
        interactive_mode()
    else:
        main()