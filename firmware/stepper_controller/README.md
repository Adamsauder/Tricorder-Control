# TMC2209 Stepper Controller Firmware

ESP32-C6 XIAO firmware for controlling a TMC2209 stepper driver with network connectivity and advanced features.

## Features

- **TMC2209 UART Control**: Full configuration via UART interface
- **Network Control**: WiFi + UDP command interface  
- **OTA Updates**: Remote firmware updates
- **StealthChop Mode**: Silent operation at low speeds
- **SpreadCycle Mode**: High torque at higher speeds
- **Microstepping**: Configurable from 1/1 to 1/256
- **Current Control**: Adjustable motor current
- **Position Tracking**: Absolute position control
- **Status LED**: Visual feedback via NeoPixel
- **Homing**: Sensorless homing using StallGuard

## Hardware Connections

### ESP32-C6 XIAO to TMC2209 Wiring

```
ESP32-C6 Pin  →  TMC2209 Pin    Description
D0 (GPIO15)   →  STEP           Step pulse input
D1 (GPIO16)   →  DIR            Direction input  
D2 (GPIO17)   →  EN             Enable (active low)
D3 (GPIO18)   →  MS1            Microstep config 1
D4 (GPIO19)   →  MS2            Microstep config 2
D5 (GPIO20)   →  PDN_UART       UART communication
D6 (GPIO21)   →  PDN_UART       UART communication
D10 (GPIO5)   →  -              Status LED (WS2812B)
VCC           →  VIO            Logic voltage (3.3V)
GND           →  GND            Ground
```

### TMC2209 to Stepper Motor

```
TMC2209 Pin   →  Stepper Motor
A1            →  Coil A+
A2            →  Coil A-
B1            →  Coil B+
B2            →  Coil B-
VM            →  Motor voltage (12-24V)
GND           →  Motor ground
```

### Power Supply

- **Logic Power**: 3.3V from ESP32-C3 (VCC to VIO)
- **Motor Power**: 12-24V DC to VM pin
- **Current**: Depends on motor, typically 0.6-2A RMS

## Software Configuration

### WiFi Settings

Edit these lines in main.cpp:
```cpp
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";
```

### TMC2209 Settings

Key parameters in `configureTMC2209()`:
```cpp
driver.rms_current(600);        // Motor current in mA
driver.microsteps(16);          // Microstepping (1,2,4,8,16,32,64,128,256)
driver.en_spreadCycle(false);   // StealthChop mode (true = SpreadCycle)
```

## Web Interface

The controller hosts a web interface on port 80 for easy testing and control.

### Accessing the Web Interface

1. Connect the ESP32-C6 to your WiFi network
2. Check serial output for the IP address
3. Open a web browser and navigate to: `http://[ESP32_IP_ADDRESS]`

### Web Interface Features

- **Real-time Status Display**: Current position, target, speed, and motor state
- **Motion Controls**: 
  - Absolute positioning (move to specific position)
  - Relative movement (move by number of steps)
  - Quick move buttons (-1000, -100, -10, +10, +100, +1000 steps)
- **High-Speed Control**: Supports speeds up to 20,000 steps/sec with automatic mode switching
- **Current Control**: Set motor current (mA)
- **Motor Control**: Enable/disable, emergency stop, homing
- **Auto-refresh**: Status updates every 2 seconds

### Performance Optimizations

**Optimized for StepperOnline 17HS08-1004S Motors:**
- **Current Setting**: 600mA (60% of 1.0A rating) for optimal thermal performance
- **Microstepping**: **FULL STEPS (1:1)** - Maximum speed configuration
- **Mode**: SpreadCycle (better for high-inductance motors)
- **Realistic Speed Range**: 1000-8000 steps/sec
- **Maximum Speed**: ~15000 steps/sec (with load dependency)

**Motor Characteristics Addressed:**
- **High Inductance (4.5mH)**: SpreadCycle mode compensates for this limitation
- **Low Current (1.0A)**: Optimized current setting prevents overheating
- **Compact Size (20mm)**: Limited thermal mass requires conservative current
- **3.5Ω Resistance**: Good for TMC2209 voltage range
- **Full Steps**: **4x-8x speed increase** by eliminating microstepping overhead

### Web API Endpoints

- **GET `/`**: Main control interface (HTML)
- **GET `/api`**: Get current motor status (JSON)
- **POST `/api`**: Send motor commands (JSON)

The web interface uses the same command format as the UDP interface, making it easy to test commands before implementing them in your control system.

## UDP Command Interface

The controller also listens on UDP port 8888 for JSON commands:

### Move Commands

**Absolute Move:**
```json
{
  "action": "move_to",
  "position": 1600,
  "commandId": "move1"
}
```

**Relative Move:**
```json
{
  "action": "move_relative", 
  "steps": 800,
  "commandId": "move2"
}
```

### Configuration Commands

**Set Speed:**
```json
{
  "action": "set_speed",
  "speed": 1000,
  "commandId": "speed1"
}
```

**Set Current:**
```json
{
  "action": "set_current",
  "current": 800,
  "commandId": "current1"
}
```

**Set Microstepping:**
```json
{
  "action": "set_microsteps",
  "microsteps": 32,
  "commandId": "microstep1"
}
```

### Control Commands

**Stop Motor:**
```json
{
  "action": "stop",
  "commandId": "stop1"
}
```

**Enable/Disable:**
```json
{
  "action": "enable",
  "commandId": "enable1"
}
```

**Home Motor:**
```json
{
  "action": "home",
  "commandId": "home1"
}
```

**Get Status:**
```json
{
  "action": "status",
  "commandId": "status1"
}
```

## Status LED Indicators

- **Red**: WiFi disconnected
- **Green**: Ready/Idle
- **Blinking Blue**: Motor moving
- **Purple**: OTA update in progress

## Serial Output

Connect to serial monitor at 115200 baud for:
- Startup diagnostics
- Command responses
- Motor status
- Error messages

## Advanced Features

### StallGuard Homing

The firmware supports sensorless homing using TMC2209's StallGuard feature:
- Motor moves in negative direction
- StallGuard detects when motor hits endstop
- Position is reset to zero

### Automatic Mode Switching

- **StealthChop**: Silent operation below TPWMTHRS velocity
- **SpreadCycle**: High torque above TPWMTHRS velocity

### Network Discovery

The device registers via mDNS as `stepper_[MAC].local` for easy discovery.

## Compilation

1. Install PlatformIO
2. Install TMCStepper library
3. Build and upload:
```bash
cd firmware/stepper_controller
pio run -t upload
```

## Integration with Prop Control System

This firmware follows the same patterns as other prop devices:
- UDP heartbeat broadcasts
- JSON command protocol
- OTA update support
- mDNS registration

The main server can discover and control stepper motors just like other props.

## Troubleshooting

### Motor Not Moving

1. Check wiring connections
2. Verify power supply voltage
3. Check enable pin (should be LOW to enable)
4. Verify UART communication with TMC2209

### Communication Issues

1. Check UART wiring (RX/TX crossed)
2. Verify driver address settings
3. Check resistor value (R_SENSE)

### Overheating

1. Reduce motor current
2. Improve cooling
3. Check for mechanical binding

## Example Applications

- **Camera Sliders**: Smooth linear motion
- **Pan/Tilt Systems**: Precise angular positioning  
- **Focus Pullers**: Smooth focus control
- **Prop Animation**: Automated prop movement
- **Turntables**: Rotating displays