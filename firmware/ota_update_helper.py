#!/usr/bin/env python3
"""
OTA Update Helper for Tricorder ESP32 Firmware
Helps discover devices and provides OTA update instructions
"""

import socket
import json
import time
import subprocess
import platform
import sys
from typing import List, Dict, Optional

class TricorderOTAHelper:
    def __init__(self):
        self.discovered_devices = []
        
    def discover_devices(self, timeout: int = 5) -> List[Dict]:
        """Discover Tricorder devices on the network"""
        print("Discovering Tricorder devices...")
        
        # Create UDP socket for discovery
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.settimeout(1)
        
        # Send discovery broadcast
        discovery_msg = {
            "action": "discovery",
            "commandId": f"discovery_{int(time.time())}"
        }
        
        message = json.dumps(discovery_msg).encode()
        
        try:
            # Broadcast to common networks
            broadcast_addresses = [
                ('255.255.255.255', 8888),
                ('192.168.1.255', 8888),
                ('192.168.0.255', 8888),
                ('10.0.0.255', 8888)
            ]
            
            for addr in broadcast_addresses:
                try:
                    sock.sendto(message, addr)
                except Exception as e:
                    print(f"Failed to send to {addr}: {e}")
            
            # Listen for responses
            devices = []
            start_time = time.time()
            
            while time.time() - start_time < timeout:
                try:
                    data, addr = sock.recvfrom(1024)
                    response = json.loads(data.decode())
                    
                    if response.get('type') == 'tricorder':
                        device_info = {
                            'deviceId': response.get('deviceId', 'Unknown'),
                            'firmwareVersion': response.get('firmwareVersion', 'Unknown'),
                            'ipAddress': response.get('ipAddress', addr[0]),
                            'actualIP': addr[0]  # IP we received response from
                        }
                        
                        # Avoid duplicates
                        if not any(d['ipAddress'] == device_info['ipAddress'] for d in devices):
                            devices.append(device_info)
                            print(f"Found: {device_info['deviceId']} at {device_info['ipAddress']} (v{device_info['firmwareVersion']})")
                
                except socket.timeout:
                    continue
                except json.JSONDecodeError:
                    continue
                except Exception as e:
                    print(f"Error processing response: {e}")
                    continue
        
        finally:
            sock.close()
        
        self.discovered_devices = devices
        print(f"\nDiscovered {len(devices)} Tricorder device(s)")
        return devices
    
    def get_ota_info(self, ip_address: str) -> Optional[Dict]:
        """Get OTA information from a specific device"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(3)
        
        try:
            ota_msg = {
                "action": "ota_info",
                "commandId": f"ota_info_{int(time.time())}"
            }
            
            message = json.dumps(ota_msg).encode()
            sock.sendto(message, (ip_address, 8888))
            
            data, addr = sock.recvfrom(1024)
            response = json.loads(data.decode())
            return response
            
        except Exception as e:
            print(f"Failed to get OTA info from {ip_address}: {e}")
            return None
        finally:
            sock.close()
    
    def show_platformio_instructions(self, device_ip: str, device_id: str):
        """Show PlatformIO OTA upload instructions"""
        print(f"\n{'='*60}")
        print(f"PlatformIO OTA Instructions for {device_id}")
        print(f"{'='*60}")
        print(f"Device IP: {device_ip}")
        print(f"OTA Password: tricorder123")
        print()
        print("Method 1: Upload via PlatformIO CLI")
        print("-" * 40)
        print(f"pio run --target upload --upload-port {device_ip}")
        print()
        print("Method 2: Add to platformio.ini")
        print("-" * 40)
        print("[env:esp32_ota]")
        print("platform = espressif32")
        print("board = esp32dev")
        print("framework = arduino")
        print(f"upload_protocol = espota")
        print(f"upload_port = {device_ip}")
        print(f"upload_flags = --auth=tricorder123")
        print()
        print("Then run: pio run -e esp32_ota --target upload")
        print()
        print("Method 3: Arduino IDE")
        print("-" * 40)
        print("1. Go to Tools -> Port")
        print(f"2. Select '{device_id} at {device_ip}'")
        print("3. Upload normally (password will be prompted)")
        print("4. Enter password: tricorder123")
        print()
        
    def show_arduino_ide_instructions(self, device_ip: str, device_id: str):
        """Show Arduino IDE OTA instructions"""
        print(f"\n{'='*60}")
        print(f"Arduino IDE OTA Instructions for {device_id}")
        print(f"{'='*60}")
        print("1. Make sure the device is powered on and connected to WiFi")
        print("2. Open Arduino IDE")
        print("3. Go to Tools -> Port")
        print(f"4. Look for '{device_id} at {device_ip}' in the network ports section")
        print("5. Select that port")
        print("6. Click Upload")
        print("7. When prompted, enter password: tricorder123")
        print("8. Wait for upload to complete")
        print()
        print("Note: If the device doesn't appear in network ports:")
        print("- Check that both computer and ESP32 are on same network")
        print("- Try restarting Arduino IDE")
        print("- Verify the device IP is accessible via ping")
        print()

    def ping_device(self, ip_address: str) -> bool:
        """Ping a device to check connectivity"""
        try:
            if platform.system().lower() == "windows":
                result = subprocess.run(['ping', '-n', '1', ip_address], 
                                      capture_output=True, timeout=5)
            else:
                result = subprocess.run(['ping', '-c', '1', ip_address], 
                                      capture_output=True, timeout=5)
            return result.returncode == 0
        except Exception:
            return False

    def run_interactive(self):
        """Run interactive OTA helper"""
        print("Tricorder OTA Update Helper")
        print("=" * 40)
        
        while True:
            print("\nOptions:")
            print("1. Discover devices")
            print("2. Show OTA instructions for device")
            print("3. Test device connectivity")
            print("4. Get device OTA info")
            print("5. Exit")
            
            choice = input("\nSelect option (1-5): ").strip()
            
            if choice == '1':
                devices = self.discover_devices()
                if not devices:
                    print("No devices found. Make sure devices are powered on and connected to WiFi.")
                    
            elif choice == '2':
                if not self.discovered_devices:
                    print("Please discover devices first (option 1)")
                    continue
                    
                print("\nDiscovered devices:")
                for i, device in enumerate(self.discovered_devices):
                    print(f"{i+1}. {device['deviceId']} at {device['ipAddress']}")
                
                try:
                    device_num = int(input("Select device number: ")) - 1
                    if 0 <= device_num < len(self.discovered_devices):
                        device = self.discovered_devices[device_num]
                        
                        print("\nChoose upload method:")
                        print("1. PlatformIO")
                        print("2. Arduino IDE")
                        
                        method = input("Select method (1-2): ").strip()
                        
                        if method == '1':
                            self.show_platformio_instructions(device['ipAddress'], device['deviceId'])
                        elif method == '2':
                            self.show_arduino_ide_instructions(device['ipAddress'], device['deviceId'])
                        else:
                            print("Invalid selection")
                    else:
                        print("Invalid device number")
                except ValueError:
                    print("Invalid input")
                    
            elif choice == '3':
                if not self.discovered_devices:
                    print("Please discover devices first (option 1)")
                    continue
                    
                print("\nTesting connectivity to discovered devices...")
                for device in self.discovered_devices:
                    ip = device['ipAddress']
                    print(f"Pinging {device['deviceId']} at {ip}...", end=" ")
                    if self.ping_device(ip):
                        print("✓ Reachable")
                    else:
                        print("✗ Not reachable")
                        
            elif choice == '4':
                if not self.discovered_devices:
                    print("Please discover devices first (option 1)")
                    continue
                    
                print("\nDiscovered devices:")
                for i, device in enumerate(self.discovered_devices):
                    print(f"{i+1}. {device['deviceId']} at {device['ipAddress']}")
                
                try:
                    device_num = int(input("Select device number: ")) - 1
                    if 0 <= device_num < len(self.discovered_devices):
                        device = self.discovered_devices[device_num]
                        print(f"\nGetting OTA info from {device['deviceId']}...")
                        
                        ota_info = self.get_ota_info(device['ipAddress'])
                        if ota_info:
                            print(f"OTA Status: {'Enabled' if ota_info.get('otaEnabled') else 'Disabled'}")
                            print(f"Hostname: {ota_info.get('hostname', 'Unknown')}")
                            print(f"Message: {ota_info.get('result', 'No message')}")
                        else:
                            print("Failed to get OTA info")
                    else:
                        print("Invalid device number")
                except ValueError:
                    print("Invalid input")
                    
            elif choice == '5':
                print("Goodbye!")
                break
                
            else:
                print("Invalid option")

if __name__ == "__main__":
    helper = TricorderOTAHelper()
    
    if len(sys.argv) > 1 and sys.argv[1] == "--discover":
        # Quick discovery mode
        devices = helper.discover_devices()
        if devices:
            print(f"\nFound {len(devices)} device(s):")
            for device in devices:
                print(f"  {device['deviceId']} - {device['ipAddress']} (v{device['firmwareVersion']})")
        sys.exit(0)
    
    # Interactive mode
    helper.run_interactive()
