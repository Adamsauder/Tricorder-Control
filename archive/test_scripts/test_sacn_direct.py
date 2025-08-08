#!/usr/bin/env python3

import socket
import struct
import time

def send_sacn_data(universe=1, channels_data=None):
    """Send sACN E1.31 data to control LEDs properly"""
    
    if channels_data is None:
        # Default: Set first 3 channels to bright red
        channels_data = [255, 0, 0] + [0] * 509  # 512 channels total
    
    # sACN E1.31 packet header (simplified)
    packet = bytearray(638)  # E1.31 packet size
    
    # ACN Root Layer
    packet[0:2] = struct.pack(">H", 0x0010)  # Preamble Size
    packet[2:4] = struct.pack(">H", 0x0000)  # Post-amble Size  
    packet[4:16] = b"ASC-E1.17\x00\x00\x00"  # ACN Packet Identifier
    packet[16:18] = struct.pack(">H", 0x726e)  # Flags and Length
    packet[18:22] = struct.pack(">I", 0x00000004)  # Vector
    packet[22:38] = b"\x00" * 16  # CID (Component Identifier)
    
    # Framing Layer
    packet[38:40] = struct.pack(">H", 0x721b)  # Flags and Length
    packet[40:44] = struct.pack(">I", 0x00000002)  # Vector
    packet[44:108] = b"Tricorder sACN Test\x00" + b"\x00" * 43  # Source Name
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
    packet[123:125] = struct.pack(">H", 513)  # Property value count (512 + start code)
    packet[125] = 0  # Start Code
    
    # DMX Data (512 channels)
    for i, value in enumerate(channels_data[:512]):
        packet[126 + i] = value
    
    # Send to multicast address
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    
    multicast_addr = f"239.255.{(universe >> 8) & 0xFF}.{universe & 0xFF}"
    
    print(f"Sending sACN data to universe {universe} at {multicast_addr}:5568")
    print(f"Channel 1-3: R={channels_data[0]} G={channels_data[1]} B={channels_data[2]}")
    
    try:
        sock.sendto(packet, (multicast_addr, 5568))
        print("✓ sACN packet sent successfully")
    except Exception as e:
        print(f"❌ Error sending sACN: {e}")
    finally:
        sock.close()

def test_sacn_colors():
    """Test different colors via sACN"""
    
    colors = [
        ([255, 0, 0], "RED"),
        ([0, 255, 0], "GREEN"), 
        ([0, 0, 255], "BLUE"),
        ([255, 255, 0], "YELLOW"),
        ([255, 0, 255], "MAGENTA"),
        ([0, 255, 255], "CYAN"),
        ([255, 255, 255], "WHITE")
    ]
    
    print("Testing sACN LED control...")
    print("This should properly control the LEDs if sACN is the issue")
    print("=" * 60)
    
    for rgb, name in colors:
        channels = rgb + [0] * 509  # RGB + 509 zero channels
        print(f"\\nSetting LEDs to {name}...")
        send_sacn_data(universe=1, channels_data=channels)
        time.sleep(3)
        
    print("\\nsACN color test complete!")

if __name__ == "__main__":
    test_sacn_colors()
