#!/usr/bin/env python3
"""
Enhanced SACN Controller with Persistent Prop Configuration
Manages individual prop configurations and SACN addressing automatically
"""

import json
import socket
import struct
import threading
import time
import requests
from typing import Dict, List, Optional, Tuple, Callable
from dataclasses import dataclass, asdict
import sqlite3
from pathlib import Path

@dataclass
class PropDevice:
    """Enhanced device representation with persistent configuration"""
    device_id: str
    device_label: str  # User-friendly name stored on prop
    ip_address: str
    device_type: str  # "tricorder" or "polyinoculator"
    
    # SACN Configuration (stored on prop)
    sacn_universe: int
    dmx_start_address: int
    num_leds: int
    brightness: int
    
    # Network status
    last_seen: float = 0
    online: bool = False
    firmware_version: str = ""
    
    # Runtime state
    last_led_values: Optional[List[Tuple[int, int, int]]] = None

class PropConfigManager:
    """Manages prop configurations and database synchronization"""
    
    def __init__(self, db_path: str = "props.db"):
        self.db_path = db_path
        self.props: Dict[str, PropDevice] = {}
        self.init_database()
    
    def init_database(self):
        """Initialize SQLite database for prop configuration tracking"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS props (
                device_id TEXT PRIMARY KEY,
                device_label TEXT NOT NULL,
                ip_address TEXT,
                device_type TEXT NOT NULL,
                sacn_universe INTEGER NOT NULL,
                dmx_start_address INTEGER NOT NULL,
                num_leds INTEGER NOT NULL,
                brightness INTEGER DEFAULT 128,
                last_seen REAL,
                firmware_version TEXT,
                created_at REAL DEFAULT (julianday('now')),
                updated_at REAL DEFAULT (julianday('now'))
            )
        ''')
        
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_device_label ON props(device_label);
        ''')
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_universe_address ON props(sacn_universe, dmx_start_address);
        ''')
        
        conn.commit()
        conn.close()
    
    def discover_prop(self, ip_address: str) -> Optional[PropDevice]:
        """Discover a prop and load its stored configuration"""
        try:
            # Request configuration from prop
            response = requests.get(f"http://{ip_address}/api/config", timeout=5)
            if response.status_code == 200:
                config_data = response.json()
                
                prop = PropDevice(
                    device_id=config_data.get('deviceId', 'UNKNOWN'),
                    device_label=config_data.get('deviceLabel', 'Unknown Prop'),
                    ip_address=ip_address,
                    device_type=config_data.get('deviceType', 'unknown'),
                    sacn_universe=config_data.get('sacnUniverse', 1),
                    dmx_start_address=config_data.get('dmxStartAddress', 1),
                    num_leds=config_data.get('numLeds', 3),
                    brightness=config_data.get('brightness', 128),
                    firmware_version=config_data.get('firmwareVersion', ''),
                    last_seen=time.time(),
                    online=True
                )
                
                # Update database
                self.save_prop_to_db(prop)
                self.props[prop.device_id] = prop
                
                print(f"Discovered prop: {prop.device_label} ({prop.device_id}) at {ip_address}")
                print(f"  Universe: {prop.sacn_universe}, DMX: {prop.dmx_start_address}-{prop.dmx_start_address + prop.num_leds * 3 - 1}")
                
                return prop
                
        except Exception as e:
            print(f"Failed to discover prop at {ip_address}: {e}")
        
        return None
    
    def save_prop_to_db(self, prop: PropDevice):
        """Save prop configuration to database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO props 
            (device_id, device_label, ip_address, device_type, sacn_universe, 
             dmx_start_address, num_leds, brightness, last_seen, firmware_version, updated_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, julianday('now'))
        ''', (
            prop.device_id, prop.device_label, prop.ip_address, prop.device_type,
            prop.sacn_universe, prop.dmx_start_address, prop.num_leds, 
            prop.brightness, prop.last_seen, prop.firmware_version
        ))
        
        conn.commit()
        conn.close()
    
    def load_props_from_db(self):
        """Load all props from database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('SELECT * FROM props ORDER BY device_label')
        rows = cursor.fetchall()
        
        for row in rows:
            prop = PropDevice(
                device_id=row[0],
                device_label=row[1],
                ip_address=row[2] or "",
                device_type=row[3],
                sacn_universe=row[4],
                dmx_start_address=row[5],
                num_leds=row[6],
                brightness=row[7],
                last_seen=row[8] or 0,
                firmware_version=row[9] or "",
                online=False
            )
            self.props[prop.device_id] = prop
        
        conn.close()
        print(f"Loaded {len(self.props)} props from database")
    
    def update_prop_config(self, device_id: str, config_update: dict) -> bool:
        """Update prop configuration both on device and in database"""
        if device_id not in self.props:
            return False
        
        prop = self.props[device_id]
        
        # Update prop remotely
        try:
            response = requests.post(
                f"http://{prop.ip_address}/api/config",
                json=config_update,
                timeout=5
            )
            
            if response.status_code == 200:
                # Update local copy
                for key, value in config_update.items():
                    if hasattr(prop, key):
                        setattr(prop, key, value)
                
                # Save to database
                self.save_prop_to_db(prop)
                print(f"Updated config for {prop.device_label}: {config_update}")
                return True
                
        except Exception as e:
            print(f"Failed to update config for {device_id}: {e}")
        
        return False
    
    def get_prop_by_label(self, label: str) -> Optional[PropDevice]:
        """Find prop by device label"""
        for prop in self.props.values():
            if prop.device_label == label:
                return prop
        return None
    
    def get_props_by_universe(self, universe: int) -> List[PropDevice]:
        """Get all props in a specific SACN universe"""
        return [prop for prop in self.props.values() if prop.sacn_universe == universe]
    
    def check_address_conflicts(self) -> List[Tuple[PropDevice, PropDevice]]:
        """Check for DMX address conflicts within universes"""
        conflicts = []
        
        for universe in set(prop.sacn_universe for prop in self.props.values()):
            universe_props = self.get_props_by_universe(universe)
            
            for i, prop1 in enumerate(universe_props):
                for prop2 in universe_props[i+1:]:
                    # Calculate address ranges
                    prop1_end = prop1.dmx_start_address + (prop1.num_leds * 3) - 1
                    prop2_end = prop2.dmx_start_address + (prop2.num_leds * 3) - 1
                    
                    # Check for overlap
                    if not (prop1_end < prop2.dmx_start_address or prop2_end < prop1.dmx_start_address):
                        conflicts.append((prop1, prop2))
        
        return conflicts
    
    def suggest_address(self, universe: int, num_leds: int) -> int:
        """Suggest next available DMX address in universe"""
        universe_props = self.get_props_by_universe(universe)
        
        if not universe_props:
            return 1
        
        # Find the highest used address
        max_address = 0
        for prop in universe_props:
            prop_end = prop.dmx_start_address + (prop.num_leds * 3) - 1
            max_address = max(max_address, prop_end)
        
        # Suggest next available address (with some padding)
        suggested = max_address + 4  # Leave some gap
        
        # Make sure it fits in universe (512 channels max)
        if suggested + (num_leds * 3) - 1 <= 512:
            return suggested
        else:
            return 1  # Suggest new universe needed
    
    def get_status_summary(self) -> dict:
        """Get summary of all props and their status"""
        online_count = sum(1 for prop in self.props.values() if prop.online)
        conflicts = self.check_address_conflicts()
        
        return {
            "total_props": len(self.props),
            "online_props": online_count,
            "offline_props": len(self.props) - online_count,
            "address_conflicts": len(conflicts),
            "universes_in_use": len(set(prop.sacn_universe for prop in self.props.values())),
            "props": [asdict(prop) for prop in self.props.values()]
        }

class EnhancedSACNController:
    """Enhanced SACN controller with prop configuration management"""
    
    def __init__(self):
        self.prop_manager = PropConfigManager()
        self.running = False
        self.sacn_socket = None
        self.discovery_thread = None
        
    def start(self):
        """Start the enhanced SACN controller"""
        print("Starting Enhanced SACN Controller...")
        
        # Load existing props from database
        self.prop_manager.load_props_from_db()
        
        # Start discovery
        self.running = True
        self.discovery_thread = threading.Thread(target=self._discovery_loop)
        self.discovery_thread.daemon = True
        self.discovery_thread.start()
        
        print("Enhanced SACN Controller started")
    
    def stop(self):
        """Stop the controller"""
        self.running = False
        if self.discovery_thread:
            self.discovery_thread.join()
    
    def _discovery_loop(self):
        """Continuous discovery of props on network"""
        while self.running:
            # Simple network scan (you could use mDNS for better discovery)
            for i in range(1, 255):
                if not self.running:
                    break
                
                ip = f"192.168.1.{i}"  # Adjust network range as needed
                prop = self.prop_manager.discover_prop(ip)
                
                if prop:
                    # Check for conflicts after discovery
                    conflicts = self.prop_manager.check_address_conflicts()
                    if conflicts:
                        print(f"WARNING: DMX address conflicts detected:")
                        for prop1, prop2 in conflicts:
                            print(f"  {prop1.device_label} vs {prop2.device_label} in universe {prop1.sacn_universe}")
            
            # Wait before next discovery cycle
            time.sleep(30)
    
    def configure_prop(self, device_id: str, **config) -> bool:
        """Configure a specific prop"""
        return self.prop_manager.update_prop_config(device_id, config)
    
    def get_prop_status(self) -> dict:
        """Get status of all props"""
        return self.prop_manager.get_status_summary()

if __name__ == "__main__":
    controller = EnhancedSACNController()
    controller.start()
    
    try:
        while True:
            # Print status every 60 seconds
            time.sleep(60)
            status = controller.get_prop_status()
            print(f"\nStatus: {status['online_props']}/{status['total_props']} props online, "
                  f"{status['universes_in_use']} universes, {status['address_conflicts']} conflicts")
    except KeyboardInterrupt:
        print("\nShutting down...")
        controller.stop()


# Global controller instance for backwards compatibility
_enhanced_controller: Optional[EnhancedSACNController] = None

def initialize_sacn_receiver(interface_ip: str = "0.0.0.0") -> 'EnhancedSACNController':
    """Initialize the global enhanced sACN controller"""
    global _enhanced_controller
    _enhanced_controller = EnhancedSACNController()
    return _enhanced_controller

def get_sacn_receiver() -> Optional['EnhancedSACNController']:
    """Get the global enhanced sACN controller instance"""
    return _enhanced_controller

def set_command_callback(callback: Callable):
    """Set the command callback for the controller"""
    if _enhanced_controller:
        _enhanced_controller.command_callback = callback

# For backwards compatibility with enhanced_server.py
SACN_AVAILABLE = True
SACNReceiver = EnhancedSACNController  # Alias for compatibility
