#!/usr/bin/env python3
"""
sACN (E1.31) Receiver for Tricorder LED Control
Film set lighting protocol integration - receives sACN data and controls ESP32 tricorder LEDs
"""

import socket
import struct
import threading
import time
import json
from typing import Dict, List, Optional, Tuple, Callable
from dataclasses import dataclass

# sACN E1.31 Constants
E131_DEFAULT_PORT = 5568
E131_UNIVERSE_DISCOVERY_INTERVAL = 1.0
ACN_PACKET_IDENTIFIER = b"ASC-E1.17\x00\x00\x00"

@dataclass
class TricorderDevice:
    """Represents a device receiving sACN data (tricorder or polyinoculator)"""
    device_id: str
    ip_address: str
    universe: int
    start_channel: int  # DMX channel (1-512) 
    num_leds: int = 3
    device_type: str = "tricorder"  # "tricorder" or "polyinoculator"
    builtin_led_channels: Optional[Tuple[int, int, int]] = None  # RGB channels for built-in LED
    last_seen: float = 0
    online: bool = False
    # Track last sent values to prevent network flooding
    last_builtin_led_values: Optional[Tuple[int, int, int]] = None  # (R, G, B)
    last_led_values: Optional[List[Tuple[int, int, int]]] = None  # List of (R, G, B) for each LED

class SACNReceiver:
    """sACN E1.31 Receiver for controlling tricorder LEDs from lighting consoles"""
    
    def __init__(self, interface_ip: str = "0.0.0.0"):
        self.interface_ip = interface_ip
        self.running = False
        self.socket: Optional[socket.socket] = None
        self.receive_thread = None
        self.devices: Dict[str, TricorderDevice] = {}
        self.universe_callbacks: Dict[int, Callable] = {}
        self.last_data: Dict[int, List[int]] = {}  # universe -> dmx_data
        self.send_command_callback: Optional[Callable] = None
        
        # Statistics
        self.packets_received = 0
        self.packets_processed = 0  # Only count packets that caused device updates
        self.universes_seen = set()
        self.last_packet_time = 0
        
    def set_command_callback(self, callback: Callable):
        """Set callback function to send commands to tricorder devices"""
        self.send_command_callback = callback
        
    def add_device(self, device_id: str, ip_address: str, universe: int, 
                   start_channel: int, num_leds: int = 3, 
                   builtin_led_channels: Optional[Tuple[int, int, int]] = None,
                   device_type: str = "tricorder"):
        """Add a device to receive sACN control"""
        device = TricorderDevice(
            device_id=device_id,
            ip_address=ip_address,
            universe=universe,
            start_channel=start_channel,
            num_leds=num_leds,
            device_type=device_type,
            builtin_led_channels=builtin_led_channels,
            last_seen=time.time(),
            online=True,
            last_builtin_led_values=None,  # Initialize change tracking
            last_led_values=None  # Initialize LED strip change tracking
        )
        self.devices[device_id] = device
        print(f"📡 Added sACN device: {device_id} at {ip_address} (Universe {universe}, Ch {start_channel})")
        return True
        
    def remove_device(self, device_id: str):
        """Remove a tricorder device from sACN control"""
        if device_id in self.devices:
            del self.devices[device_id]
            print(f"📡 Removed sACN device: {device_id}")
            return True
        return False
        
    def start(self):
        """Start the sACN receiver"""
        if self.running:
            return True
            
        try:
            # Create multicast socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            # Bind to sACN port
            self.socket.bind(('', E131_DEFAULT_PORT))
            
            # Enable multicast
            mreq = struct.pack("4sl", socket.inet_aton("239.255.0.0"), socket.INADDR_ANY)
            self.socket.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            
            self.running = True
            self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.receive_thread.start()
            
            print(f"🎭 sACN Receiver started on port {E131_DEFAULT_PORT}")
            return True
            
        except Exception as e:
            print(f"❌ Failed to start sACN receiver: {e}")
            self.running = False
            return False
            
    def stop(self):
        """Stop the sACN receiver"""
        self.running = False
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        if self.receive_thread:
            self.receive_thread.join(timeout=1.0)
        print("📡 sACN Receiver stopped")
        
    def _receive_loop(self):
        """Main receive loop for sACN packets"""
        while self.running and self.socket:
            try:
                data, addr = self.socket.recvfrom(1144)  # E1.31 max packet size
                self._process_packet(data, addr)
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"❌ sACN receive error: {e}")
                    
    def _process_packet(self, data: bytes, addr: Tuple[str, int]):
        """Process received sACN packet"""
        try:
            # Validate packet structure
            if len(data) < 126:  # Minimum E1.31 packet size
                return
                
            # Check ACN packet identifier
            if data[4:16] != ACN_PACKET_IDENTIFIER:
                return
                
            # Extract universe (bytes 113-114, big endian)
            universe = struct.unpack(">H", data[113:115])[0]
            
            # Extract DMX data (starts at byte 126)
            dmx_data = list(data[126:])
            
            # Check if this universe data has actually changed before processing
            if universe in self.last_data:
                # Compare first 50 channels (most commonly used) for efficiency
                if self.last_data[universe][:50] == dmx_data[:50]:
                    # Data hasn't changed, skip processing devices but update stats
                    self.packets_received += 1
                    self.last_packet_time = time.time()
                    return
            
            # Update statistics
            self.packets_received += 1
            self.packets_processed += 1  # Only count when data changed
            self.universes_seen.add(universe)
            self.last_packet_time = time.time()
            self.last_data[universe] = dmx_data
            
            # Process for each device listening on this universe (only when data changed)
            for device_id, device in self.devices.items():
                if device.universe == universe:
                    self._update_device_from_dmx(device, dmx_data)
                    
        except Exception as e:
            print(f"❌ Error processing sACN packet: {e}")
            
    def _update_device_from_dmx(self, device: TricorderDevice, dmx_data: List[int]):
        """Update tricorder device LEDs based on DMX data - only send changes"""
        try:
            # Update device status
            device.last_seen = time.time()
            device.online = True
            
            if device.device_type == "tricorder":
                # Handle tricorder with built-in LED channels
                if device.builtin_led_channels:
                    r_ch, g_ch, b_ch = device.builtin_led_channels
                    if (r_ch - 1 < len(dmx_data) and 
                        g_ch - 1 < len(dmx_data) and 
                        b_ch - 1 < len(dmx_data)):
                        
                        current_builtin_values = (
                            dmx_data[r_ch - 1],
                            dmx_data[g_ch - 1],
                            dmx_data[b_ch - 1]
                        )
                        
                        # Only send if values changed AND are not just "empty" sACN data
                        if device.last_builtin_led_values != current_builtin_values:
                            # Check if this is meaningful sACN data (non-zero) or if we had previous non-zero values
                            is_meaningful_data = (
                                # Current values are non-zero (active lighting)
                                any(v > 0 for v in current_builtin_values) or
                                # Previous values were non-zero (we're turning off intentionally)
                                (device.last_builtin_led_values and any(v > 0 for v in device.last_builtin_led_values))
                            )
                            
                            if is_meaningful_data and self.send_command_callback:
                                print(f"📡 sACN sending LED update to {device.device_id}: R:{current_builtin_values[0]} G:{current_builtin_values[1]} B:{current_builtin_values[2]}")
                                
                                # Send tricorder commands
                                self.send_command_callback(
                                    device.device_id,
                                    'set_builtin_led',
                                    {
                                        'r': current_builtin_values[0],
                                        'g': current_builtin_values[1],
                                        'b': current_builtin_values[2]
                                    }
                                )
                                
                                # Also send LED strip command if configured
                                if device.num_leds > 0:
                                    self.send_command_callback(
                                        device.device_id,
                                        'set_led_color',
                                        {
                                            'r': current_builtin_values[0],
                                            'g': current_builtin_values[1],
                                            'b': current_builtin_values[2]
                                        }
                                    )
                            else:
                                # Skip sending "empty" sACN data that would interfere with manual control
                                print(f"📡 sACN skipping empty data for {device.device_id}: {current_builtin_values}")
                            
                            # Update stored values regardless of whether we sent commands
                            device.last_builtin_led_values = current_builtin_values
            
            elif device.device_type == "polyinoculator":
                # Handle polyinoculator with LED array
                # For polyinoculators, process multiple LEDs based on start channel
                # Each LED takes 3 channels (RGB)
                led_commands = []
                for led_idx in range(device.num_leds):
                    channel_offset = led_idx * 3
                    r_channel = device.start_channel + channel_offset - 1  # Convert to 0-based
                    g_channel = r_channel + 1
                    b_channel = r_channel + 2
                    
                    if r_channel < len(dmx_data) and g_channel < len(dmx_data) and b_channel < len(dmx_data):
                        r_val = dmx_data[r_channel]
                        g_val = dmx_data[g_channel]
                        b_val = dmx_data[b_channel]
                        led_commands.append([r_val, g_val, b_val])
                    else:
                        led_commands.append([0, 0, 0])
                
                # Only send if values changed AND are not just "empty" sACN data
                if device.last_led_values != led_commands:
                    # Check if this is meaningful sACN data (non-zero) or if we had previous non-zero values
                    has_active_leds = any(any(rgb) for rgb in led_commands)
                    had_active_leds = device.last_led_values and any(any(rgb) for rgb in device.last_led_values)
                    is_meaningful_data = has_active_leds or had_active_leds
                    
                    if is_meaningful_data and self.send_command_callback:
                        print(f"📡 sACN sending LED array update to {device.device_id}: {led_commands[:3]}...")  # Show first 3 LEDs
                        
                        # Send array command to polyinoculator
                        self.send_command_callback(
                            device.device_id,
                            'set_leds_array',
                            {
                                'leds': led_commands
                            }
                        )
                    else:
                        # Skip sending "empty" sACN data that would interfere with manual control
                        print(f"📡 sACN skipping empty data for {device.device_id}")
                    
                    # Update stored values regardless of whether we sent commands
                    device.last_led_values = led_commands
            
            # Note: LED strip is now handled above with built-in LED for uniform control
                
        except Exception as e:
            print(f"❌ Error updating device {device.device_id}: {e}")
            
    def get_status(self):
        """Get receiver status"""
        efficiency = 0
        if self.packets_received > 0:
            efficiency = (self.packets_processed / self.packets_received) * 100
            
        return {
            'running': self.running,
            'interface_ip': self.interface_ip,
            'devices': {device_id: {
                'ip': device.ip_address,
                'universe': device.universe,
                'start_channel': device.start_channel,
                'num_leds': device.num_leds,
                'online': device.online,
                'last_seen': device.last_seen
            } for device_id, device in self.devices.items()},
            'packets_received': self.packets_received,
            'packets_processed': self.packets_processed,
            'processing_efficiency': f"{efficiency:.1f}%",
            'universes_seen': list(self.universes_seen),
            'last_packet_time': self.last_packet_time
        }
        
    def get_universe_data(self, universe: int) -> Optional[List[int]]:
        """Get last DMX data for a universe"""
        return self.last_data.get(universe)


# Global receiver instance
_sacn_receiver: Optional[SACNReceiver] = None

def initialize_sacn_receiver(interface_ip: str = "0.0.0.0") -> SACNReceiver:
    """Initialize the global sACN receiver"""
    global _sacn_receiver
    _sacn_receiver = SACNReceiver(interface_ip)
    return _sacn_receiver

def get_sacn_receiver() -> Optional[SACNReceiver]:
    """Get the global sACN receiver instance"""
    return _sacn_receiver

def set_command_callback(callback: Callable):
    """Set the command callback for the receiver"""
    if _sacn_receiver:
        _sacn_receiver.set_command_callback(callback)

# For backwards compatibility, keep some aliases
SACN_AVAILABLE = True  # We implement our own receiver
initialize_sacn_controller = initialize_sacn_receiver
get_sacn_controller = get_sacn_receiver
