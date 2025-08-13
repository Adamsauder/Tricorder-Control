# LED Strip Troubleshooting Guide - Polyinoculator

## Current Issue: "Off white color on one strip, other strip not working"

This suggests multiple potential issues. Let's troubleshoot systematically:

## 1. Hardware Wiring Issues

### Check Power Connections:
- **VCC**: All strips should connect to **3.3V or 5V** (consistent across all strips)
- **GND**: All strips must share **common ground** with ESP32
- **Data**: Each strip connects to its designated GPIO pin

### Current Pin Assignment:
```
Strip 1 (7 LEDs): GPIO10 → Working (reference)
Strip 2 (4 LEDs): GPIO6  → Check wiring
Strip 3 (4 LEDs): GPIO7  → Check wiring
```

## 2. LED Strip Color Order Issues

### "Off white" color symptoms:
- **Wrong color order**: Strip expects RGB but firmware sends GRB
- **Voltage level mismatch**: 3.3V signal to 5V strip (dim/wrong colors)
- **Partial connection**: Loose wire causing color channel loss

### Test Different Color Orders:
If strips show wrong colors, try these alternatives in firmware:
```cpp
// Current (GRB):
FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds2, NUM_LEDS_2);

// Alternative 1 (RGB):
FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds2, NUM_LEDS_2);

// Alternative 2 (BRG):
FastLED.addLeds<WS2812B, LED_PIN_2, BRG>(leds2, NUM_LEDS_2);
```

## 3. Power Supply Issues

### Common Problems:
- **Insufficient current**: USB can't power multiple LED strips
- **Voltage drop**: Long wires cause voltage loss
- **Ground loops**: Poor grounding causes erratic behavior

### Solutions:
- Use **external 5V power supply** for LED strips
- Keep **data signal** from ESP32 (3.3V)
- Use **short, thick wires** for power
- **Common ground** between ESP32 and LED power supply

## 4. ESP32-C3 Specific Issues

### GPIO Pin Conflicts (Already Fixed):
- ❌ GPIO0/1: Boot/UART conflicts
- ❌ GPIO2/8/9: Strapping pins  
- ❌ GPIO4/5: ADC conflicts
- ✅ GPIO6/7: Clean pins (current choice)

## 5. Diagnostic Firmware Features

The updated firmware includes individual strip testing:
```
Boot sequence:
1. Strip 1 (GPIO10) → RED for 2 seconds
2. Strip 2 (GPIO6)  → GREEN for 2 seconds  
3. Strip 3 (GPIO7)  → BLUE for 2 seconds
```

### What to Look For:
- **Strip 1 RED**: Should work (reference)
- **Strip 2 GREEN**: Check if GREEN appears correctly
- **Strip 3 BLUE**: Check if BLUE appears correctly
- **Wrong colors**: Color order issue
- **No light**: Wiring or power issue
- **Dim/flickering**: Power supply issue

## 6. Quick Hardware Checks

### Step 1: Visual Inspection
- [ ] All solder joints solid
- [ ] No short circuits between pins
- [ ] Correct GPIO pins used (6 & 7)
- [ ] Data pin connected to DIN on LED strip

### Step 2: Multimeter Tests
- [ ] 3.3V between ESP32 VCC and GND
- [ ] 0V between ESP32 GND and LED strip GND
- [ ] Continuity from GPIO6 to Strip 2 data pin
- [ ] Continuity from GPIO7 to Strip 3 data pin

### Step 3: Power Supply Test
- [ ] Try external 5V supply for LED strips
- [ ] Keep ESP32 data pins connected
- [ ] Ensure common ground

## 7. Serial Monitor Debugging

Connect to serial monitor (115200 baud) to see:
```
Starting Polyinoculator Control System...
Pin assignments: GPIO10=7 LEDs, GPIO6=4 LEDs, GPIO7=4 LEDs
Testing strips individually...
Testing Strip 1 (GPIO10) - RED
Testing Strip 2 (GPIO6) - GREEN  
Testing Strip 3 (GPIO7) - BLUE
Strip testing complete. Check which colors appeared.
```

## 8. Common Solutions by Symptom

### "Off white" color:
1. **Try RGB color order** instead of GRB
2. **Check power supply voltage** (should be 5V for WS2812B)
3. **Verify data wire connection**

### No light at all:
1. **Check power connections** (VCC, GND)
2. **Verify GPIO pin assignment**
3. **Test with known working strip**

### Flickering/erratic:
1. **Add external power supply**
2. **Shorter data wires**
3. **Add 100Ω resistor** in series with data line

## 9. Next Steps

1. **Flash diagnostic firmware** and observe boot sequence
2. **Check serial output** for error messages
3. **Test each strip individually** with multimeter
4. **Try alternative color orders** if wrong colors appear
5. **Consider external power supply** if dim/flickering

Report back which colors appear during the diagnostic sequence!
