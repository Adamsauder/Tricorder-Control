# Defragment### Required Components
- **Microcontroller**: Seeed Studio XIAO ESP32-C3
- **LEDs**: 2x RGBW LEDs (WS2812 or SK6812 compatible with white channel)
- **Servo Motor**: Standard 9g servo (SG90 or similar)
- **Switch**: Momentary push button or toggle switch
- **Power Management**: TPS61023 5V boost converter breakout board
- **Power Supply**: 3-5V power supply (TPS61023 input) or 5V direct (2-3A recommended)
- **Resistors**: 
  - 1x 330Ω resistor (LED data line)
  - *(No external pull-up needed for trigger - ESP32-C3 has built-in pull-up)*
- **Capacitor**: 1000µF electrolytic capacitor (power smoothing)Wiring Guide

## Overview
The Defragmentor is a film prop featuring:
- 2 individually controllable RGBW LEDs
- 1 servo motor for physical movement
- 1 trigger switch for state switching
- WiFi connectivity for lighting console control
- Support for both sACN E1.31 and UDP protocols

## Hardware Components

### Required Components
- **Microcontroller**: Seeed Studio XIAO ESP32-C3
- **LEDs**: 2x RGBW LEDs (WS2812 or SK6812 compatible with white channel)
- **Servo Motor**: Standard 9g servo (SG90 or similar)
- **Switch**: Momentary push button or toggle switch
- **Power Supply**: 5V power supply (2-3A recommended)
- **Resistors**: 
  - 1x 330Ω resistor (LED data line)
  - *(No external pull-up needed for trigger - ESP32-C3 has built-in pull-up)*
- **Capacitor**: 1000µF electrolytic capacitor (power smoothing)

### Optional Components
- **Status LED**: Single color LED for visual feedback
- **Enclosure**: 3D printed or custom housing
- **Connectors**: JST connectors for modular connections

## Pin Assignments

### Seeed Studio XIAO ESP32-C3 Pinout
```
Pin | GPIO | Function        | Connection
----|------|-----------------|------------------
D10 | 18   | LED Data        | RGBW LED strip data line
D4  | 6    | Trigger Input   | Trigger switch input
D3  | 21   | Servo Control   | Servo signal line
D9  | 8    | Power Enable    | TPS61023 boost converter enable
3V3 | -    | 3.3V Power      | Logic power (low current)
5V  | -    | 5V Power        | High current devices (or TPS61023 input)
GND | -    | Ground          | Common ground
```

## Wiring Diagrams

### Power Distribution (with TPS61023 Boost Converter)
```
3-5V Power Supply ──> TPS61023 IN
                     │
ESP32-C3 D9 (GPIO8) ──> TPS61023 EN (Enable Pin)
                     │
                     TPS61023 OUT (5V) ──┬─> ESP32-C3 (5V pin)
                                         ├─> RGBW LEDs (VCC)  
                                         └─> Servo Motor (Red wire)

Ground (Common)
├── Power Supply (GND)
├── TPS61023 (GND)
├── ESP32-C3 (GND)
├── RGBW LEDs (GND)
├── Servo Motor (Brown/Black wire)
└── Trigger Switch (one terminal)
```

**Alternative: Direct 5V Power (without TPS61023)**
```
5V Power Supply ──┬─> ESP32-C3 (5V pin)
                  ├─> RGBW LEDs (VCC)
                  └─> Servo Motor (Red wire)
                  
Note: D9 pin not used in direct 5V configuration
```

### LED Wiring (RGBW Strip)
```
ESP32-C3 D10 (GPIO18) ──[330Ω]──> LED Data In
5V Power ──────────────────────> LED VCC
Ground ────────────────────────> LED GND

LED Data Out ──> Next LED Data In (if chaining)
```

### Servo Motor Wiring
```
ESP32-C3 D3 (GPIO21) ──> Servo Signal (Orange/Yellow wire)
ESP32-C3 D9 (GPIO8)  ──> TPS61023 Enable Pin
TPS61023 5V Output ───> Servo VCC (Red wire)  
Ground ───────────────> Servo GND (Brown/Black wire)
```

**Note**: The servo power is controlled by the ESP32-C3 via the TPS61023 enable pin. This allows software control of servo power and helps with debugging.

### Trigger Switch Wiring
```
ESP32-C3 D4 (GPIO6) ──> Switch Terminal 1
                         (Internal pull-up enabled in firmware)
                     
Ground ─────────────────> Switch Terminal 2
```

## Detailed Wiring Instructions

### Step 1: Prepare the ESP32-C3
1. Solder header pins to the XIAO ESP32-C3 if not pre-installed
2. Test the board by connecting to USB and uploading a simple blink sketch
3. Verify all pins are accessible and not damaged

### Step 2: Wire the Power System

**Option A: Using TPS61023 Boost Converter (Recommended)**
1. **Connect TPS61023 Boost Converter**:
   - TPS61023 IN to 3-5V power supply positive
   - TPS61023 GND to power supply ground
   - TPS61023 EN to ESP32-C3 D9 (GPIO8)
   - TPS61023 OUT to servo and LED 5V power

2. **ESP32-C3 Power Connections**:
   - ESP32-C3 5V pin to TPS61023 OUT (or direct 5V supply)
   - ESP32-C3 GND to common ground
   - Add 1000µF capacitor between 5V and GND near ESP32-C3

**Option B: Direct 5V Power Supply**
1. **Connect 5V Power Supply**:
   - Red wire from PSU to ESP32-C3 5V pin
   - Black wire from PSU to ESP32-C3 GND pin
   - Add 1000µF capacitor between 5V and GND near the ESP32

3. **Power Distribution**:
   - Create power rails for 5V and GND
   - Use breadboard or perfboard for clean connections
   - Ensure adequate wire gauge (20-22 AWG minimum)

### Step 3: Install RGBW LEDs
1. **LED Strip Preparation**:
   - Cut RGBW LED strip to 2 individual LEDs or use discrete LEDs
   - Identify Data In, VCC, and GND pads
   - Pre-tin all connection points

2. **Data Line Connection**:
   - Solder 330Ω resistor to ESP32-C3 D10 (GPIO18)
   - Connect resistor output to first LED Data In
   - If using 2 separate LEDs, connect Data Out of first to Data In of second

3. **Power Connections**:
   - Connect LED VCC to 5V rail
   - Connect LED GND to ground rail
   - Ensure solid connections to prevent voltage drops

### Step 4: Install Servo Motor
1. **Signal Connection**:
   - Connect servo signal wire (orange/yellow) to ESP32-C3 D3 (GPIO21)
   - Keep signal wire away from power lines to reduce noise

2. **Power Connections**:
   - **If using TPS61023**: Connect servo VCC (red) to TPS61023 OUT
   - **If using direct 5V**: Connect servo VCC (red) to 5V rail
   - Connect servo GND (brown/black) to ground rail
   - Consider adding a small capacitor (100µF) near servo for noise reduction

3. **Power Enable Connection** (if using TPS61023):
   - Connect ESP32-C3 D9 (GPIO8) to TPS61023 EN pin
   - This allows software control of servo power

### Step 5: Install Trigger Switch
1. **Switch Wiring**:
   - Connect one terminal to ESP32-C3 D4 (GPIO6)
   - Connect other terminal to ground
   - Internal pull-up resistor is enabled in firmware (no external resistor needed)

2. **Switch Mounting**:
   - Mount switch in accessible location on prop
   - Ensure reliable mechanical connection
   - Use debouncing in software (already implemented)

### Step 6: Optional Status LED
1. **LED Connection**:
   - Connect LED anode to ESP32-C3 D1 (GPIO2) through 220Ω resistor
   - Connect LED cathode to ground
   - This provides visual feedback for prop status

## Testing Procedures

### Basic Power Test
1. Connect power supply (DO NOT exceed 5.5V)
2. Verify ESP32-C3 powers up (onboard LED should illuminate)
3. Check voltage levels: 5V rail should be 4.8-5.2V, 3.3V should be 3.1-3.4V

### LED Test
1. Upload test firmware with simple LED patterns
2. Verify both LEDs can display all RGBW colors
3. Test individual LED control
4. Check for proper color mixing with white channel

### Servo Test
1. Upload servo test code
2. Verify servo moves to 0° and 180° positions
3. Check for smooth operation without binding
4. Ensure servo doesn't draw excessive current

### Trigger Test
1. Monitor serial output while pressing trigger
2. Verify trigger state changes are detected
3. Test debouncing functionality
4. Confirm pull-up resistor is working

### Communication Test
1. Connect to WiFi network
2. Test UDP command reception
3. Verify sACN E1.31 data processing
4. Check web interface accessibility

## Troubleshooting

### Power Issues
- **ESP32 won't boot**: Check 5V supply, ensure good connections
- **LEDs dim or flickering**: Insufficient power supply current, check connections
- **Servo jittering**: Add capacitor near servo, check power supply stability

### LED Issues
- **No LED response**: Check data line connection, verify 330Ω resistor
- **Wrong colors**: Check RGBW order in firmware, verify wiring
- **Only first LED works**: Check data connection between LEDs

### Servo Issues
- **No servo movement**: Check signal connection to D3, verify power
- **Erratic movement**: Check for electrical noise, add filter capacitor
- **Servo doesn't reach endpoints**: Adjust servo limits in firmware

### Trigger Issues
- **No trigger response**: Check switch connection, verify internal pull-up is enabled
- **False triggers**: Check for electrical noise, verify debouncing
- **Inverted logic**: Check switch wiring polarity, verify pull-up configuration

### Communication Issues
- **No WiFi connection**: Check credentials, verify network compatibility
- **No sACN data**: Check universe/address settings, verify network
- **Web interface not accessible**: Check IP address, verify web server startup

## Safety Considerations

### Electrical Safety
- Always disconnect power when making wiring changes
- Use proper wire gauge for current requirements
- Insulate all connections to prevent shorts
- Add fuse protection to power supply input

### Mechanical Safety
- Secure all wiring to prevent movement damage
- Ensure servo has adequate clearance for movement
- Mount components securely in enclosure
- Protect connections from mechanical stress

### Environmental Protection
- Use conformal coating on PCB if needed
- Protect connections from moisture
- Consider heat dissipation for enclosed installations
- Use appropriate enclosure rating for environment

## Performance Optimization

### Power Efficiency
- Use efficient 5V power supply
- Minimize LED brightness when full output not needed
- Consider sleep modes for battery operation
- Monitor current consumption

### Response Time
- Keep wire runs short for servo signals
- Use appropriate pull-up resistor values
- Optimize firmware for fast trigger response
- Minimize network latency

### Reliability
- Use quality connectors for removable connections
- Strain relief all cable connections
- Regular inspection of connections
- Keep spare components available

## Firmware Configuration

The defragmentor firmware includes several configuration parameters:

### DMX Channel Mapping
- **Channels 1-4**: Low state LED 1 (RGBW)
- **Channels 5-8**: Low state LED 2 (RGBW)  
- **Channels 9-12**: High state LED 1 (RGBW)
- **Channels 13-16**: High state LED 2 (RGBW)

### Default Settings
- **DMX Universe**: 1
- **Start Address**: 1
- **Fixture Number**: 3
- **WiFi SSID**: "Rigging Electric"
- **WiFi Password**: "academy123"

### Trigger Behavior
- **Debounce Time**: 100ms
- **Servo Move Time**: 2000ms (2 seconds)
- **State Switch**: Toggle between low and high states

## Maintenance

### Regular Checks
- Verify all connections remain secure
- Test trigger responsiveness
- Check servo operation for wear
- Monitor LED brightness consistency

### Firmware Updates
- Keep firmware updated for bug fixes
- Backup configuration before updates
- Test all functions after updates
- Document any custom modifications

### Component Replacement
- Keep spare LEDs, servo, and switch
- Document wire colors and connections
- Take photos before disassembly
- Test thoroughly after component replacement

## Support and Resources

### Documentation
- ESP32-C3 datasheet and reference manual
- WS2812/SK6812 LED specifications
- Servo motor technical specifications
- sACN E1.31 protocol documentation

### Development Tools
- PlatformIO IDE for firmware development
- Logic analyzer for signal debugging
- Multimeter for electrical testing
- Oscilloscope for advanced debugging

### Community Resources
- ESP32 community forums
- Lighting control protocol groups
- Film/theatre technical communities
- Open source hardware projects

---

*This wiring guide is designed for the Defragmentor prop control system. Always follow proper electrical safety procedures and test thoroughly before deployment.*
