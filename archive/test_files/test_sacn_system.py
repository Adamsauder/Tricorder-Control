#!/usr/bin/env python3
"""
Test script for the hybrid sACN + UDP system
Tests both tricorders and polyinoculators with direct sACN and fallback UDP
"""

import socket
import struct
import time
import threading
import json
from typing import List, Tuple

class SACNTester:
    def __init__(self):
        self.devices = [
            {"id": "TRIC001", "universe": 1, "start_address": 1, "leds": 3, "type": "tricorder"},
            {"id": "POLY001", "universe": 1, "start_address": 10, "leds": 15, "type": "polyinoculator"},
            # Add more devices as needed for universe testing
        ]
        
    def send_sacn_packet(self, universe: int, dmx_data: List[int]):
        """Send sACN E1.31 packet to devices"""
        # Calculate multicast address
        subnet = (universe >> 8) & 0xFF
        host = universe & 0xFF
        if subnet == 0:
            subnet = 0
            host = universe
        multicast_addr = f"239.255.{subnet}.{host}"
        
        # Build E1.31 packet
        packet = bytearray(638)
        
        # ACN Root Layer
        packet[0:2] = struct.pack(">H", 0x0010)  # Preamble Size
        packet[2:4] = struct.pack(">H", 0x0000)  # Post-amble Size
        packet[4:16] = b"ASC-E1.17\\x00\\x00\\x00"  # ACN Packet Identifier
        packet[16:18] = struct.pack(">H", 0x726e)  # Flags and Length
        packet[18:22] = struct.pack(">I", 0x00000004)  # Vector
        packet[22:38] = b"\\x00" * 16  # CID
        
        # Framing Layer
        packet[38:40] = struct.pack(">H", 0x721b)  # Flags and Length
        packet[40:44] = struct.pack(">I", 0x00000002)  # Vector
        packet[44:108] = b"Tricorder sACN Test\\x00" + b"\\x00" * 43  # Source Name
        packet[108] = 100  # Priority
        packet[109:111] = struct.pack(">H", 0x0000)  # Reserved
        packet[111] = 0  # Sequence Number
        packet[112] = 0  # Options
        packet[113:115] = struct.pack(">H", universe)  # Universe
        
        # DMP Layer
        packet[115:117] = struct.pack(">H", 0x720b)  # Flags and Length
        packet[117] = 0x02  # Vector
        packet[118] = 0xa1  # Address Type & Data Type
        packet[119:121] = struct.pack(">H", 0x0000)  # First Property Address
        packet[121:123] = struct.pack(">H", 0x0001)  # Address Increment
        packet[123:125] = struct.pack(">H", 513)  # Property value count
        packet[125] = 0  # Start Code
        
        # DMX Data
        for i, value in enumerate(dmx_data[:512]):
            packet[126 + i] = value
        
        # Send packet
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.sendto(packet, (multicast_addr, 5568))
            print(f"✓ Sent sACN packet to {multicast_addr}:5568 for universe {universe}")
            return True
        except Exception as e:
            print(f"❌ Failed to send sACN packet: {e}")
            return False
        finally:
            sock.close()
    
    def send_udp_command(self, device_ip: str, command: dict):
        """Send UDP command to device"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            message = json.dumps(command)
            sock.sendto(message.encode(), (device_ip, 8888))
            print(f"✓ Sent UDP command to {device_ip}: {command['action']}")
            return True
        except Exception as e:
            print(f"❌ Failed to send UDP command: {e}")
            return False
        finally:
            sock.close()
    
    def test_sacn_colors(self):
        """Test sACN color changes"""
        print("\\n=== Testing sACN Color Control ===")
        
        colors = [
            {"name": "Red", "dmx": [255, 0, 0]},
            {"name": "Green", "dmx": [0, 255, 0]},
            {"name": "Blue", "dmx": [0, 0, 255]},
            {"name": "White", "dmx": [255, 255, 255]},
            {"name": "Purple", "dmx": [255, 0, 255]},
            {"name": "Yellow", "dmx": [255, 255, 0]},
            {"name": "Cyan", "dmx": [0, 255, 255]},
        ]
        
        for color in colors:
            print(f"\\nTesting {color['name']} via sACN...")
            
            # Create DMX data for universe 1
            dmx_data = [0] * 512
            
            # Set colors for each device
            for device in self.devices:
                if device["universe"] == 1:
                    start = device["start_address"] - 1  # Convert to 0-based
                    for led in range(device["leds"]):
                        dmx_data[start + (led * 3) + 0] = color["dmx"][0]  # R
                        dmx_data[start + (led * 3) + 1] = color["dmx"][1]  # G
                        dmx_data[start + (led * 3) + 2] = color["dmx"][2]  # B
            
            # Send sACN packet
            self.send_sacn_packet(1, dmx_data)
            time.sleep(1.5)
    
    def test_udp_fallback(self):
        """Test UDP command fallback when sACN is disabled"""
        print("\\n=== Testing UDP Fallback Commands ===")
        
        device_ips = ["192.168.1.48"]  # Add your device IPs here
        
        for ip in device_ips:
            print(f"\\nTesting UDP commands to {ip}...")
            
            # Disable sACN first
            self.send_udp_command(ip, {
                "action": "disable_sacn",
                "commandId": "test_disable_sacn"
            })
            time.sleep(0.5)
            
            # Test LED color via UDP
            self.send_udp_command(ip, {
                "action": "set_led_color",
                "r": 255, "g": 128, "b": 0,  # Orange
                "commandId": "test_udp_color"
            })
            time.sleep(1)
            
            # Re-enable sACN
            self.send_udp_command(ip, {
                "action": "enable_sacn",
                "commandId": "test_enable_sacn"
            })
            time.sleep(0.5)
    
    def test_sacn_priority(self):
        """Test that sACN overrides UDP commands"""
        print("\\n=== Testing sACN Priority System ===")
        
        device_ips = ["192.168.1.48"]  # Add your device IPs here
        
        # Set LED to orange via UDP
        for ip in device_ips:
            self.send_udp_command(ip, {
                "action": "set_led_color", 
                "r": 255, "g": 165, "b": 0,  # Orange
                "commandId": "test_udp_before_sacn"
            })
        
        time.sleep(1)
        print("LEDs should be orange from UDP command")
        
        # Override with sACN (blue)
        dmx_data = [0] * 512
        dmx_data[0:9] = [0, 0, 255] * 3  # Blue for tricorder (3 LEDs)
        dmx_data[9:54] = [0, 0, 255] * 15  # Blue for polyinoculator (15 LEDs)
        
        self.send_sacn_packet(1, dmx_data)
        time.sleep(1)
        print("LEDs should now be blue from sACN (overriding UDP)")
        
        # Try UDP command while sACN is active (should be ignored)
        for ip in device_ips:
            self.send_udp_command(ip, {
                "action": "set_led_color",
                "r": 255, "g": 0, "b": 0,  # Red
                "commandId": "test_udp_during_sacn"
            })
        
        time.sleep(1)
        print("LEDs should still be blue (UDP ignored during sACN)")
        
        # Stop sACN and wait for timeout
        print("Stopping sACN, waiting for timeout...")
        time.sleep(3)
        
        # Try UDP command after sACN timeout
        for ip in device_ips:
            self.send_udp_command(ip, {
                "action": "set_led_color",
                "r": 0, "g": 255, "b": 0,  # Green
                "commandId": "test_udp_after_sacn"
            })
        
        time.sleep(1)
        print("LEDs should now be green (UDP working after sACN timeout)")
    
    def test_universe_separation(self):
        """Test multiple universe separation"""
        print("\\n=== Testing Universe Separation ===")
        
        # Universe 1: Red
        dmx_data_u1 = [255, 0, 0] * 170 + [0] * 2  # Fill most of universe with red
        self.send_sacn_packet(1, dmx_data_u1)
        
        time.sleep(0.5)
        
        # Universe 2: Blue (should not affect universe 1 devices)
        dmx_data_u2 = [0, 0, 255] * 170 + [0] * 2  # Fill most of universe with blue
        self.send_sacn_packet(2, dmx_data_u2)
        
        time.sleep(2)
        print("Universe 1 devices should be red, Universe 2 devices should be blue")
    
    def run_full_test(self):
        """Run complete test suite"""
        print("🎭 Starting Hybrid sACN + UDP System Test")
        print("=" * 50)
        
        try:
            self.test_sacn_colors()
            time.sleep(2)
            
            self.test_udp_fallback()
            time.sleep(2)
            
            self.test_sacn_priority()
            time.sleep(2)
            
            self.test_universe_separation()
            time.sleep(2)
            
            print("\\n✅ All tests completed!")
            
        except KeyboardInterrupt:
            print("\\n⏹️ Test interrupted by user")
        except Exception as e:
            print(f"\\n❌ Test failed: {e}")

if __name__ == "__main__":
    tester = SACNTester()
    tester.run_full_test()
