# External LED Enhancements - IO21 Configuration

## Overview
Updated the Tricorder firmware and web API to support 3 external NeoPixel LEDs connected to GPIO pin 21 (IO21) instead of the previous pin 2 configuration.

## Hardware Changes

### Pin Configuration
- **Previous**: GPIO 2 for external LEDs
- **Updated**: GPIO 21 (IO21) for external LEDs  
- **LED Count**: 3 NeoPixel LEDs (front-facing)

### Firmware Changes (main.cpp)

#### Pin Definition Updates
```cpp
#define LED_PIN 21         // NeoPixel data pin (external connection) - IO21
#define NUM_LEDS 3         // Number of NeoPixels in strip (3 front LEDs)
```

#### New LED Functions Added
1. **Individual LED Control**
   ```cpp
   void setIndividualLED(int ledIndex, int r, int g, int b)
   ```
   - Control individual LEDs (0, 1, 2)
   - Allows different colors per LED

2. **Scanner Effect** 
   ```cpp
   void scannerEffect(int r, int g, int b, int delayMs = 100)
   ```
   - Kitt/Cylon-style scanning effect
   - Configurable delay between LED transitions

3. **Pulse Effect**
   ```cpp
   void pulseEffect(int r, int g, int b, int duration = 2000)
   ```
   - Breathing/pulsing effect
   - Configurable duration

#### New UDP Commands
- `set_individual_led` - Control single LED
- `scanner_effect` - Run scanner animation  
- `pulse_effect` - Run pulse animation

### API Changes (tricorderAPI.ts)

#### New TypeScript Interfaces
```typescript
export interface IndividualLEDParameters {
  ledIndex: number;
  r: number;
  g: number; 
  b: number;
}

export interface LEDEffectParameters {
  r: number;
  g: number;
  b: number;
  delay?: number;
  duration?: number;
}
```

#### New API Functions
- `setIndividualLED()` - Control individual LED
- `scannerEffect()` - Trigger scanner effect
- `pulseEffect()` - Trigger pulse effect
- Bulk operation variants for all new functions

## Usage Examples

### Individual LED Control
```javascript
// Set LED 0 to red, LED 1 to green, LED 2 to blue
await tricorderAPI.setIndividualLED("TRICORDER_001", {
  ledIndex: 0, r: 255, g: 0, b: 0
});
await tricorderAPI.setIndividualLED("TRICORDER_001", {
  ledIndex: 1, r: 0, g: 255, b: 0  
});
await tricorderAPI.setIndividualLED("TRICORDER_001", {
  ledIndex: 2, r: 0, g: 0, b: 255
});
```

### Scanner Effect
```javascript
// Red scanner effect with 150ms delay
await tricorderAPI.scannerEffect("TRICORDER_001", {
  r: 255, g: 0, b: 0, delay: 150
});
```

### Pulse Effect  
```javascript
// Blue pulse for 3 seconds
await tricorderAPI.pulseEffect("TRICORDER_001", {
  r: 0, g: 0, b: 255, duration: 3000
});
```

## Testing

### Manual Testing Script
Run `test_led_functions.py` to test all LED functions:

```bash
python test_led_functions.py [ESP32_IP_ADDRESS]
```

The test script will:
1. Test basic color changes (Red, Green, Blue, etc.)
2. Test brightness levels
3. Test individual LED control  
4. Test scanner and pulse effects
5. Cleanup by turning off all LEDs

### UDP Command Examples
```json
// Set all LEDs to white
{
  "commandId": "test1",
  "action": "set_led_color", 
  "parameters": {"r": 255, "g": 255, "b": 255}
}

// Set LED 1 to red
{
  "commandId": "test2",
  "action": "set_individual_led",
  "parameters": {"ledIndex": 1, "r": 255, "g": 0, "b": 0}
}

// Run scanner effect
{
  "commandId": "test3", 
  "action": "scanner_effect",
  "parameters": {"r": 255, "g": 0, "b": 0, "delay": 100}
}
```

## Hardware Connection

Connect your 3 NeoPixel LEDs to:
- **Data Pin**: GPIO 21 (IO21) 
- **Power**: 3.3V or 5V (depending on LED specifications)
- **Ground**: GND

Make sure to include appropriate current limiting and decoupling capacitors as per NeoPixel best practices.

## Benefits

1. **More Control**: Individual LED addressing allows complex patterns
2. **Visual Effects**: Scanner and pulse effects for enhanced prop realism  
3. **API Integration**: All functions available through web interface
4. **Bulk Operations**: Control multiple devices simultaneously
5. **Testing Tools**: Comprehensive test script for validation

The 3 external LEDs on IO21 now provide much more flexibility for creating engaging visual effects on your Tricorder props!
