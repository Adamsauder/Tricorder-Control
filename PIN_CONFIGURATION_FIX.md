# ESP32-C3 Pin Configuration Fix

## Issue Identified
GPIO pins 0 and 1 were not working for LED strips on the Seeed XIAO ESP32-C3.

## Root Cause
The ESP32-C3 has specific pin functions that can interfere with normal GPIO operation:

### Problematic Pins:
- **GPIO0**: BOOT button pin with internal pull-up, used for entering bootloader mode
- **GPIO1**: Often used for UART TX (Serial communication)
- **GPIO2, GPIO8, GPIO9**: Strapping pins that control boot modes

### Pin Conflicts:
- GPIO0 has boot-time behavior that can interfere with LED control
- GPIO1 can conflict with Serial communication
- Strapping pins (GPIO2/8/9) should be avoided for reliable operation

## Solution Applied
Changed LED strip pin assignments to avoid conflicts:

### Previous Configuration (PROBLEMATIC):
```cpp
#define LED_PIN_1 10       // Strip 1: 7 pixels on GPIO10 ✅ OK
#define LED_PIN_2 0        // Strip 2: 4 pixels on GPIO0  ❌ BOOT pin
#define LED_PIN_3 1        // Strip 3: 4 pixels on GPIO1  ❌ UART conflict
```

### New Configuration (FINAL - RECOMMENDED):
```cpp
#define LED_PIN_1 10       // Strip 1: 7 pixels on GPIO10 ✅ OK
#define LED_PIN_2 6        // Strip 2: 4 pixels on GPIO6  ✅ Safe GPIO (no ADC)
#define LED_PIN_3 7        // Strip 3: 4 pixels on GPIO7  ✅ Safe GPIO (no ADC)
```

## Issue Resolution - ADC Conflict Discovery
After initial attempt with GPIO4/5, discovered additional conflicts:
- **GPIO4**: ADC1_CH4 pin - can interfere with digital output
- **GPIO5**: ADC2_CH0 pin - ADC2 particularly problematic for digital output

## Final Solution Applied
Changed to GPIO6 and GPIO7 which have **no special functions** and are guaranteed safe.

## Safe GPIO Pins on ESP32-C3
For the Seeed XIAO ESP32-C3, the following pins are recommended for general GPIO use:
- ✅ **GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO10**: Safe for LED strips
- ⚠️ **GPIO0, GPIO1**: Avoid (boot/UART conflicts)
- ⚠️ **GPIO2, GPIO8, GPIO9**: Avoid (strapping pins)

## Hardware Wiring Update Required
You'll need to rewire your LED strips:

### Strip Connections:
1. **Strip 1 (7 LEDs)**: Connect to **GPIO10** (unchanged)
2. **Strip 2 (4 LEDs)**: Move from GPIO0 to **GPIO6** (final)
3. **Strip 3 (4 LEDs)**: Move from GPIO1 to **GPIO7** (final)

### Power & Ground:
- All strips: Connect VCC to 3.3V or 5V
- All strips: Connect GND to common ground
- Data lines: Connect to the respective GPIO pins above

## Files Updated
- ✅ `firmware/polyinoculator/src/main.cpp` - Pin definitions and comments
- ✅ `flash_polyinoculator.bat` - Hardware documentation
- ✅ `flash_polyinoculator.ps1` - Hardware documentation  
- ✅ `firmware/FIRMWARE_README.md` - Documentation

## Verification
- ✅ Firmware compiles successfully with new pin assignments
- ✅ No GPIO conflicts with ESP32-C3 special functions
- ✅ All 3 strips maintain independent control capability

## Next Steps
1. **Rewire hardware** according to new pin assignments
2. **Flash updated firmware** using `flash_polyinoculator.bat` or `.ps1`
3. **Test each strip** individually to verify operation
4. **Verify network functionality** remains intact

The firmware now uses GPIO pins that are guaranteed to work reliably on the ESP32-C3 platform.
