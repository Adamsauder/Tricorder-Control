#!/usr/bin/env python3
"""
Test ADC debugging to see detailed ADC readings from all pins
"""

import socket
import json
import time

def test_adc_debug():
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(10)  # Longer timeout for debug response

    # ESP32 details
    esp32_ip = '192.168.1.48'
    esp32_port = 8888

    # Send ADC debug request
    command = {
        'action': 'debug_adc',
        'commandId': 'adc_debug_' + str(int(time.time()))
    }

    command_json = json.dumps(command)
    print(f'Sending ADC debug request to {esp32_ip}:{esp32_port}')
    print(f'Command: {command_json}')

    try:
        # Send command
        sock.sendto(command_json.encode(), (esp32_ip, esp32_port))
        
        # Wait for response
        print('Waiting for ADC debug response...')
        data, addr = sock.recvfrom(2048)  # Larger buffer for debug data
        response = data.decode()
        
        print(f'Raw response from {addr}:')
        print(response)
        print('\n' + '='*60)
        
        # Parse and display debug info
        try:
            debug_data = json.loads(response)
            print('\n=== ADC DEBUG ANALYSIS ===')
            print(f'Device: {debug_data.get("deviceId", "N/A")}')
            print(f'Primary Pin: GPIO{debug_data.get("primaryPin", "N/A")}')
            print(f'ADC Resolution: {debug_data.get("adcResolution", "N/A")} bits')
            print(f'ADC Attenuation: {debug_data.get("adcAttenuation", "N/A")}')
            print(f'Voltage Divider Ratio: {debug_data.get("voltageDivider", "N/A")}')
            
            print('\n--- All ADC Pin Readings ---')
            adc_readings = debug_data.get("adcReadings", [])
            for reading in adc_readings:
                pin = reading.get("pin")
                raw = reading.get("rawValue")
                voltage = reading.get("voltage")
                is_primary = reading.get("isPrimaryPin", False)
                status = " ← PRIMARY PIN" if is_primary else ""
                print(f'GPIO{pin}: Raw={raw:4d}/4095, Voltage={voltage:.3f}V{status}')
            
            print('\n--- Primary Pin Detailed Analysis ---')
            primary_raw = debug_data.get("primaryRawADC", 0)
            primary_voltage = debug_data.get("primaryVoltageADC", 0)
            calculated_battery = debug_data.get("calculatedBatteryVoltage", 0)
            
            print(f'Raw ADC Value: {primary_raw}/4095 ({primary_raw/4095*100:.1f}%)')
            print(f'ADC Voltage: {primary_voltage:.3f}V')
            print(f'Calculated Battery: {calculated_battery:.3f}V')
            
            # Analysis
            print('\n--- DIAGNOSTIC ANALYSIS ---')
            if primary_raw == 0:
                print('❌ PROBLEM: ADC reading is 0')
                print('   Possible causes:')
                print('   - No voltage present on GPIO34')
                print('   - GPIO34 not connected to battery circuit')
                print('   - ADC hardware failure')
                print('   - Wrong GPIO pin assignment')
            elif primary_raw >= 4095:
                print('❌ PROBLEM: ADC reading is maximum (4095)')
                print('   Possible causes:')
                print('   - Input voltage too high for current attenuation')
                print('   - Short circuit or wiring issue')
            else:
                print(f'✅ ADC is reading: {primary_raw} (expected ~{int(0.036/3.3*4095)} for 0.036V)')
                expected_raw = int(0.036 / 3.3 * 4095)
                print(f'   Expected for 0.036V: ~{expected_raw}')
                print(f'   Actual reading: {primary_raw}')
                if abs(primary_raw - expected_raw) < 50:
                    print('✅ Reading matches expected value!')
                else:
                    print('❌ Reading does not match expected value')
                    
        except json.JSONDecodeError:
            print('Failed to parse JSON response')
            
    except socket.timeout:
        print('❌ Timeout waiting for response from ESP32')
        print('Make sure the device is powered on and connected to WiFi')
    except Exception as e:
        print(f'❌ Error: {e}')
    finally:
        sock.close()

if __name__ == "__main__":
    test_adc_debug()
