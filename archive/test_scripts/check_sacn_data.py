#!/usr/bin/env python3

import requests
import json

def check_sacn_data():
    """Check current sACN/DMX data values"""
    try:
        # Check current universe data
        response = requests.get('http://localhost:8080/api/sacn/universe/1', timeout=5)
        if response.status_code == 200:
            data = response.json()
            print('Universe 1 DMX Data:')
            # Show first 10 channels
            dmx_data = data.get('dmx_data', [])
            if dmx_data:
                print(f'Channels 1-10: {dmx_data[:10]}')
                print(f'Channel 1 (R): {dmx_data[0] if len(dmx_data) > 0 else "N/A"}')
                print(f'Channel 2 (G): {dmx_data[1] if len(dmx_data) > 1 else "N/A"}')
                print(f'Channel 3 (B): {dmx_data[2] if len(dmx_data) > 2 else "N/A"}')
                
                # Check if all RGB channels are zero (black)
                if len(dmx_data) >= 3:
                    if dmx_data[0] == 0 and dmx_data[1] == 0 and dmx_data[2] == 0:
                        print("⚠️  RGB channels are all ZERO (BLACK) - this is overriding LED commands!")
                    else:
                        print(f"✓ RGB channels have values: R={dmx_data[0]} G={dmx_data[1]} B={dmx_data[2]}")
            else:
                print('No DMX data available')
            print(f'Full response: {json.dumps(data, indent=2)}')
        else:
            print(f'Failed to get universe data: {response.status_code} - {response.text}')
            
    except Exception as e:
        print(f'Error: {e}')

if __name__ == "__main__":
    check_sacn_data()
