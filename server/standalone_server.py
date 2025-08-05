#!/usr/bin/env python3
"""
Standalone Tricorder Control Server
Command-line server application that runs independently of the web GUI
Version 0.1 - "Command Center"
"""

import asyncio
import socket
import json
import time
import uuid
import threading
import signal
import sys
import os
from datetime import datetime
from typing import Dict, List, Optional
import ipaddress
from pathlib import Path

# Configuration
CONFIG = {
    "udp_port": 8888,
    "device_timeout": 30,  # seconds
    "command_timeout": 5,  # seconds
    "discovery_interval": 30,  # seconds between auto-discovery scans
    "log_file": "tricorder_server.log",
    "max_log_size": 10 * 1024 * 1024,  # 10MB
}

class TricorderStandaloneServer:
    def __init__(self, enable_signal_handlers=True):
        self.udp_socket = None
        self.running = False
        self.devices: Dict[str, Dict] = {}
        self.command_history: List[Dict] = []
        self.active_commands: Dict[str, Dict] = {}
        self.server_start_time = time.time()
        self.log_file = None
        self.enable_signal_handlers = enable_signal_handlers
        self.stats = {
            "commands_sent": 0,
            "responses_received": 0,
            "devices_discovered": 0,
            "uptime_start": datetime.now()
        }
        
        # Setup logging
        self.setup_logging()
        
    def setup_logging(self):
        """Setup logging to file and console"""
        try:
            # Rotate log file if it's too large
            if os.path.exists(CONFIG["log_file"]):
                if os.path.getsize(CONFIG["log_file"]) > CONFIG["max_log_size"]:
                    backup_name = f"{CONFIG['log_file']}.old"
                    if os.path.exists(backup_name):
                        os.remove(backup_name)
                    os.rename(CONFIG["log_file"], backup_name)
            
            self.log_file = open(CONFIG["log_file"], 'a', encoding='utf-8')
            self.log(f"Tricorder Standalone Server v0.1 starting at {datetime.now()}")
        except Exception as e:
            print(f"Warning: Could not setup log file: {e}")
    
    def log(self, message: str, level: str = "INFO"):
        """Log message to both console and file"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        formatted_msg = f"[{timestamp}] {level}: {message}"
        
        print(formatted_msg)
        
        if self.log_file:
            try:
                self.log_file.write(formatted_msg + "\n")
                self.log_file.flush()
            except Exception:
                pass
    
    def start_udp_listener(self):
        """Start UDP listener for device communication"""
        try:
            self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_socket.bind(('', CONFIG["udp_port"]))
            self.udp_socket.settimeout(1.0)
            self.running = True
            
            self.log(f"UDP listener started on port {CONFIG['udp_port']}")
            
            while self.running:
                try:
                    data, addr = self.udp_socket.recvfrom(4096)
                    self.handle_device_message(data.decode('utf-8'), addr)
                except socket.timeout:
                    continue
                except Exception as e:
                    self.log(f"UDP listener error: {e}", "ERROR")
                    
        except Exception as e:
            self.log(f"Failed to start UDP listener: {e}", "ERROR")
            return False
        
        return True
    
    def handle_device_message(self, message: str, addr: tuple):
        """Handle incoming device messages"""
        try:
            data = json.loads(message)
            ip_address = addr[0]
            
            # Handle different message types
            if 'deviceId' in data:
                device_id = data['deviceId']
                
                # Check if this is a new device
                is_new_device = device_id not in self.devices
                
                # Update device information
                self.devices[device_id] = {
                    'device_id': device_id,
                    'type': data.get('type', 'tricorder'),
                    'ip_address': ip_address,
                    'port': addr[1],
                    'last_seen': datetime.now().isoformat(),
                    'status': 'online',
                    'firmware_version': data.get('firmwareVersion'),
                    'wifi_connected': data.get('wifiConnected'),
                    'free_heap': data.get('freeHeap'),
                    'uptime': data.get('uptime'),
                    'sd_card_initialized': data.get('sdCardInitialized'),
                    'video_playing': data.get('videoPlaying'),
                    'current_video': data.get('currentVideo'),
                    'video_looping': data.get('videoLooping'),
                    'current_frame': data.get('currentFrame'),
                    **data
                }
                
                if is_new_device:
                    self.stats["devices_discovered"] += 1
                    self.log(f"NEW DEVICE DISCOVERED: {device_id} at {ip_address}")
                    self.log(f"  Firmware: {data.get('firmwareVersion', 'Unknown')}")
                    self.log(f"  Type: {data.get('type', 'tricorder')}")
                else:
                    self.log(f"Device update: {device_id} at {ip_address}", "DEBUG")
            
            # Handle command responses
            if 'commandId' in data:
                command_id = data['commandId']
                device_id = data.get('deviceId', 'UNKNOWN')
                
                # Update active command status
                if command_id in self.active_commands:
                    self.active_commands[command_id]['response'] = data
                    self.active_commands[command_id]['completed'] = True
                    self.active_commands[command_id]['response_time'] = time.time()
                    
                    # Calculate response time
                    response_time = self.active_commands[command_id]['response_time'] - self.active_commands[command_id]['sent_time']
                    self.log(f"Command response received: {command_id} from {device_id} ({response_time:.3f}s)")
                
                # Add to history
                history_entry = {
                    'timestamp': datetime.now().isoformat(),
                    'device_id': device_id,
                    'command_id': command_id,
                    'response': data,
                    'ip_address': ip_address
                }
                
                self.command_history.append(history_entry)
                self.stats["responses_received"] += 1
                
                # Keep history manageable
                if len(self.command_history) > 1000:
                    self.command_history = self.command_history[-500:]
                
                # Log important responses
                if 'result' in data:
                    self.log(f"  Result: {data['result']}")
                    
        except json.JSONDecodeError:
            self.log(f"Invalid JSON from {addr}: {message}", "WARN")
        except Exception as e:
            self.log(f"Error handling message from {addr}: {e}", "ERROR")
    
    def send_command(self, device_id: str, action: str, parameters: Dict = None) -> Optional[str]:
        """Send command to specific device"""
        if not self.udp_socket or not self.running:
            self.log("Cannot send command: UDP socket not available", "ERROR")
            return None
        
        device = self.devices.get(device_id)
        if not device:
            self.log(f"Device not found: {device_id}", "ERROR")
            return None
        
        command_id = str(uuid.uuid4())
        command = {
            'commandId': command_id,
            'action': action,
            'parameters': parameters or {}
        }
        
        try:
            message = json.dumps(command)
            ip_address = device['ip_address']
            
            self.udp_socket.sendto(message.encode('utf-8'), (ip_address, CONFIG["udp_port"]))
            
            # Track the command
            self.active_commands[command_id] = {
                'command': command,
                'device_id': device_id,
                'sent_time': time.time(),
                'completed': False,
                'response': None
            }
            
            self.stats["commands_sent"] += 1
            self.log(f"Command sent to {device_id} ({ip_address}): {action}")
            
            return command_id
            
        except Exception as e:
            self.log(f"Failed to send command to {device_id}: {e}", "ERROR")
            return None
    
    def broadcast_command(self, action: str, parameters: Dict = None) -> List[str]:
        """Send command to all devices"""
        command_ids = []
        
        for device_id in self.devices:
            command_id = self.send_command(device_id, action, parameters)
            if command_id:
                command_ids.append(command_id)
        
        if command_ids:
            self.log(f"Broadcast command '{action}' sent to {len(command_ids)} devices")
        else:
            self.log("No devices available for broadcast", "WARN")
        
        return command_ids
    
    def discover_devices(self):
        """Scan for devices on the network"""
        if not self.udp_socket:
            return
        
        self.log("Starting device discovery scan...")
        
        try:
            # Get local IP address
            temp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            temp_socket.connect(("8.8.8.8", 80))
            local_ip = temp_socket.getsockname()[0]
            temp_socket.close()
            
            # Create network range (assuming /24 subnet)
            network = ipaddress.IPv4Network(f"{local_ip}/24", strict=False)
            
            # Send discovery commands
            discovery_command = {
                'commandId': str(uuid.uuid4()),
                'action': 'discovery',
                'parameters': {}
            }
            
            message = json.dumps(discovery_command)
            addresses_scanned = 0
            
            for ip in network.hosts():
                ip_str = str(ip)
                try:
                    self.udp_socket.sendto(message.encode('utf-8'), (ip_str, CONFIG["udp_port"]))
                    addresses_scanned += 1
                except Exception:
                    continue
            
            self.log(f"Discovery scan sent to {addresses_scanned} addresses")
            
        except Exception as e:
            self.log(f"Discovery error: {e}", "ERROR")
    
    def cleanup_old_devices(self):
        """Remove devices that haven't been seen recently"""
        now = datetime.now()
        offline_devices = []
        
        for device_id, device in list(self.devices.items()):
            try:
                last_seen = datetime.fromisoformat(device['last_seen'])
                if (now - last_seen).seconds > CONFIG["device_timeout"]:
                    offline_devices.append(device_id)
            except Exception:
                offline_devices.append(device_id)
        
        for device_id in offline_devices:
            self.log(f"Device went offline: {device_id}")
            del self.devices[device_id]
    
    def cleanup_old_commands(self):
        """Clean up old commands"""
        now = time.time()
        old_commands = []
        
        for command_id, command_info in self.active_commands.items():
            age = now - command_info['sent_time']
            if age > CONFIG["command_timeout"] * 2:  # Keep completed commands a bit longer
                old_commands.append(command_id)
        
        for command_id in old_commands:
            del self.active_commands[command_id]
    
    def get_server_stats(self):
        """Get server statistics"""
        uptime = datetime.now() - self.stats["uptime_start"]
        return {
            **self.stats,
            "uptime": str(uptime),
            "devices_online": len(self.devices),
            "active_commands": len(self.active_commands),
            "command_history_size": len(self.command_history)
        }
    
    def start_background_tasks(self):
        """Start background maintenance tasks"""
        def background_loop():
            discovery_counter = 0
            
            while self.running:
                try:
                    # Discovery every 30 seconds
                    if discovery_counter % CONFIG["discovery_interval"] == 0:
                        self.discover_devices()
                    
                    # Cleanup every 60 seconds
                    if discovery_counter % 60 == 0:
                        self.cleanup_old_devices()
                        self.cleanup_old_commands()
                    
                    discovery_counter += 1
                    time.sleep(1)
                    
                except Exception as e:
                    self.log(f"Background task error: {e}", "ERROR")
                    time.sleep(5)
        
        bg_thread = threading.Thread(target=background_loop, daemon=True)
        bg_thread.start()
        self.log("Background tasks started")
    
    def run_interactive_mode(self):
        """Run interactive command-line interface"""
        self.log("Starting interactive mode - type 'help' for commands")
        
        while self.running:
            try:
                command = input("\nTricorder> ").strip()
                
                if not command:
                    continue
                
                parts = command.split()
                cmd = parts[0].lower()
                
                if cmd in ['quit', 'exit', 'q']:
                    break
                elif cmd == 'help':
                    self.show_help()
                elif cmd == 'status':
                    self.show_status()
                elif cmd == 'devices' or cmd == 'list':
                    self.show_devices()
                elif cmd == 'discover':
                    self.discover_devices()
                elif cmd == 'stats':
                    self.show_stats()
                elif cmd == 'history':
                    self.show_command_history()
                elif cmd == 'send':
                    self.handle_send_command(parts[1:])
                elif cmd == 'broadcast':
                    self.handle_broadcast_command(parts[1:])
                elif cmd == 'ping':
                    if len(parts) > 1:
                        self.ping_device(parts[1])
                    else:
                        self.broadcast_command('ping')
                elif cmd == 'clear':
                    os.system('cls' if os.name == 'nt' else 'clear')
                else:
                    print(f"Unknown command: {cmd}. Type 'help' for available commands.")
                    
            except KeyboardInterrupt:
                break
            except EOFError:
                break
            except Exception as e:
                self.log(f"Interactive mode error: {e}", "ERROR")
        
        self.log("Exiting interactive mode")
    
    def show_help(self):
        """Show available commands"""
        help_text = """
Available Commands:
==================
  help                 - Show this help message
  status               - Show server status
  devices, list        - List all discovered devices
  discover             - Scan for new devices
  stats                - Show server statistics
  history              - Show recent command history
  ping [device_id]     - Ping device (or all devices)
  send <device_id> <action> [params] - Send command to specific device
  broadcast <action> [params]        - Send command to all devices
  clear                - Clear screen
  quit, exit, q        - Exit server

Examples:
  send TRICORDER_001 set_led_color r=255 g=0 b=0
  send TRICORDER_001 display_image filename=test.jpg
  broadcast set_led_brightness brightness=100
  ping TRICORDER_001
        """
        print(help_text)
    
    def show_status(self):
        """Show server status"""
        uptime = datetime.now() - self.stats["uptime_start"]
        print(f"\nTricorder Standalone Server Status:")
        print(f"  Version: 0.1")
        print(f"  Uptime: {uptime}")
        print(f"  UDP Port: {CONFIG['udp_port']}")
        print(f"  Devices Online: {len(self.devices)}")
        print(f"  Server Running: {'Yes' if self.running else 'No'}")
        print(f"  Log File: {CONFIG['log_file']}")
    
    def show_devices(self):
        """Show discovered devices"""
        if not self.devices:
            print("\nNo devices discovered yet. Try 'discover' command.")
            return
        
        print(f"\nDiscovered Devices ({len(self.devices)}):")
        print("=" * 80)
        
        for device_id, device in self.devices.items():
            last_seen = device.get('last_seen', 'Unknown')
            ip_address = device.get('ip_address', 'Unknown')
            firmware = device.get('firmware_version', 'Unknown')
            status = device.get('status', 'Unknown')
            
            print(f"  {device_id}")
            print(f"    IP: {ip_address}")
            print(f"    Firmware: {firmware}")
            print(f"    Status: {status}")
            print(f"    Last Seen: {last_seen}")
            
            # Show additional device info if available
            if device.get('video_playing'):
                print(f"    Video: {device.get('current_video', 'Unknown')} (Playing)")
            if device.get('free_heap'):
                print(f"    Free Heap: {device['free_heap']} bytes")
            
            print()
    
    def show_stats(self):
        """Show server statistics"""
        stats = self.get_server_stats()
        print(f"\nServer Statistics:")
        print(f"  Commands Sent: {stats['commands_sent']}")
        print(f"  Responses Received: {stats['responses_received']}")
        print(f"  Devices Discovered: {stats['devices_discovered']}")
        print(f"  Devices Online: {stats['devices_online']}")
        print(f"  Active Commands: {stats['active_commands']}")
        print(f"  Command History: {stats['command_history_size']} entries")
        print(f"  Uptime: {stats['uptime']}")
    
    def show_command_history(self, count=10):
        """Show recent command history"""
        if not self.command_history:
            print("\nNo command history available.")
            return
        
        print(f"\nRecent Command History (last {count}):")
        print("=" * 80)
        
        recent = self.command_history[-count:]
        for entry in recent:
            timestamp = entry['timestamp']
            device_id = entry['device_id']
            response = entry.get('response', {})
            result = response.get('result', 'No result')
            
            print(f"  [{timestamp}] {device_id}: {result}")
    
    def handle_send_command(self, args):
        """Handle send command from CLI"""
        if len(args) < 2:
            print("Usage: send <device_id> <action> [param=value ...]")
            return
        
        device_id = args[0]
        action = args[1]
        
        # Parse parameters
        parameters = {}
        for arg in args[2:]:
            if '=' in arg:
                key, value = arg.split('=', 1)
                # Try to convert to appropriate type
                try:
                    if value.lower() in ['true', 'false']:
                        parameters[key] = value.lower() == 'true'
                    elif value.isdigit():
                        parameters[key] = int(value)
                    else:
                        parameters[key] = value
                except:
                    parameters[key] = value
        
        command_id = self.send_command(device_id, action, parameters)
        if command_id:
            print(f"Command sent: {command_id}")
            
            # Wait for response for a short time
            time.sleep(2)
            if command_id in self.active_commands and self.active_commands[command_id]['completed']:
                response = self.active_commands[command_id]['response']
                print(f"Response: {response.get('result', 'No result')}")
        else:
            print("Failed to send command")
    
    def handle_broadcast_command(self, args):
        """Handle broadcast command from CLI"""
        if len(args) < 1:
            print("Usage: broadcast <action> [param=value ...]")
            return
        
        action = args[0]
        
        # Parse parameters
        parameters = {}
        for arg in args[1:]:
            if '=' in arg:
                key, value = arg.split('=', 1)
                try:
                    if value.lower() in ['true', 'false']:
                        parameters[key] = value.lower() == 'true'
                    elif value.isdigit():
                        parameters[key] = int(value)
                    else:
                        parameters[key] = value
                except:
                    parameters[key] = value
        
        command_ids = self.broadcast_command(action, parameters)
        if command_ids:
            print(f"Broadcast sent to {len(command_ids)} devices")
        else:
            print("No devices available for broadcast")
    
    def ping_device(self, device_id):
        """Ping specific device"""
        command_id = self.send_command(device_id, 'ping')
        if command_id:
            print(f"Ping sent to {device_id}")
        else:
            print(f"Failed to ping {device_id}")
    
    def stop(self):
        """Stop the server"""
        self.log("Stopping server...")
        self.running = False
        
        if self.udp_socket:
            try:
                self.udp_socket.close()
            except:
                pass
        
        if self.log_file:
            try:
                self.log_file.close()
            except:
                pass
    
    def start(self, interactive=True):
        """Start the server"""
        self.log("Starting Tricorder Standalone Server v0.1")
        
        # Setup signal handlers only if enabled (disabled when used as GUI backend)
        if self.enable_signal_handlers:
            try:
                signal.signal(signal.SIGINT, lambda s, f: self.stop())
                signal.signal(signal.SIGTERM, lambda s, f: self.stop())
            except ValueError as e:
                # Signal handling only works in main thread - ignore if not available
                self.log(f"Signal handling not available: {e}", "WARNING")
        
        # Start UDP listener in background
        udp_thread = threading.Thread(target=self.start_udp_listener, daemon=True)
        udp_thread.start()
        
        # Give UDP listener time to start
        time.sleep(1)
        
        if not self.running:
            self.log("Failed to start UDP listener", "ERROR")
            return False
        
        # Start background tasks
        self.start_background_tasks()
        
        # Initial device discovery
        self.discover_devices()
        
        self.log("Server started successfully")
        
        if interactive:
            try:
                self.run_interactive_mode()
            except KeyboardInterrupt:
                pass
        else:
            # Non-interactive mode - just run until stopped
            try:
                while self.running:
                    time.sleep(1)
            except KeyboardInterrupt:
                pass
        
        self.stop()
        return True

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Tricorder Standalone Server v0.1')
    parser.add_argument('--non-interactive', action='store_true', 
                       help='Run in non-interactive mode (no CLI)')
    parser.add_argument('--port', type=int, default=8888,
                       help='UDP port to listen on (default: 8888)')
    parser.add_argument('--log-file', type=str, default='tricorder_server.log',
                       help='Log file path (default: tricorder_server.log)')
    
    args = parser.parse_args()
    
    # Update config with command line arguments
    CONFIG['udp_port'] = args.port
    CONFIG['log_file'] = args.log_file
    
    print("=" * 60)
    print("Tricorder Control System - Standalone Server v0.1")
    print("=" * 60)
    print(f"UDP Port: {CONFIG['udp_port']}")
    print(f"Log File: {CONFIG['log_file']}")
    print(f"Interactive Mode: {'No' if args.non_interactive else 'Yes'}")
    print("=" * 60)
    
    server = TricorderStandaloneServer()
    
    try:
        server.start(interactive=not args.non_interactive)
    except Exception as e:
        print(f"Server error: {e}")
        return 1
    
    print("Server shutdown complete.")
    return 0

if __name__ == '__main__':
    sys.exit(main())
