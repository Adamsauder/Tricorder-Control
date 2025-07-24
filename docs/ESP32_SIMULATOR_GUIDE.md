# ESP32 Display Simulator Guide

This guide explains how to use the ESP32 display simulator for developing and testing your Tricorder GUI without needing the physical hardware.

## What is the ESP32 Simulator?

The ESP32 simulator replicates the exact behavior of the ESP32-2432S032C-I display that's used in your Tricorder project. It shows a virtual 320x240 pixel ST7789 TFT display that matches what would appear on the physical device.

## Features

### 🖥️ Accurate Display Simulation
- **Resolution**: 320x240 pixels (matching ESP32-2432S032C-I)
- **Display Type**: ST7789 TFT simulation
- **Color Depth**: Full RGB color support
- **Scaling**: Adjustable scale from 0.5x to 3x for easy viewing

### 🎬 Video Playback Simulation
- **Static Images**: Test single frame displays
- **Animated Sequences**: Play frame-by-frame animations
- **Color Patterns**: Solid color screens for testing
- **Frame Rate Control**: Adjustable FPS from 5-30 fps

### 🎛️ Interactive Controls
- **Play/Pause/Stop**: Standard media controls
- **Brightness Control**: Simulate backlight adjustment (10-100%)
- **Video Selection**: Choose from available test patterns
- **Real-time Updates**: Live display updates

### 📊 Development Tools
- **Command Logging**: See all commands sent to the simulator
- **Status Display**: Monitor current video, frame numbers, etc.
- **Hardware Info**: View simulated device specifications

## How to Use

### Method 1: Standalone Simulator (Simple)

1. **Start the Enhanced Server**:
   ```bash
   # From the Tricorder-Control directory
   python server/enhanced_server.py
   ```
   Or double-click: `start_enhanced_server.bat`

2. **Open the Simulator**:
   - Go to: http://localhost:5000/simulator
   - You'll see a black 320x240 screen with controls below

3. **Test Display Functions**:
   - Select a video/pattern from the dropdown
   - Click "Play" to start
   - Adjust brightness, scale, and FPS as needed
   - Use "Stop" to clear the screen

### Method 2: Integrated with React App (Advanced)

1. **Start Both Servers**:
   ```bash
   # Terminal 1: Enhanced server
   python server/enhanced_server.py
   
   # Terminal 2: React dev server
   cd web
   npm run dev
   ```

2. **Open the React Interface**:
   - Go to: http://localhost:3000
   - Click the "ESP32 Simulator" tab
   - Use the full-featured simulator component

## Available Test Content

### Static Images
- `startup.jpg` - Boot screen with "TRICORDER STARTUP" text
- `static_test.jpg` - Grid test pattern
- Various color screens (red, green, blue, cyan, magenta, yellow, white)

### Animated Sequences
- `animated_test` - 30-frame color wheel animation
- `color_cycle` - Cycling through solid colors

### Custom Content
The simulator can display any 320x240 JPEG image. Add your own test images to test specific UI elements.

## Development Workflow

### 1. Design Phase
- Use the simulator to mockup your interface layouts
- Test different color schemes and brightness levels
- Verify text readability at 320x240 resolution

### 2. Implementation Phase
- Create your graphics at exactly 320x240 pixels
- Test animations frame-by-frame
- Validate color accuracy and contrast

### 3. Testing Phase
- Simulate different environmental conditions with brightness control
- Test user interaction flows
- Validate performance with different frame rates

## Integration with Real Hardware

The simulator uses the same command structure as the real ESP32 devices:

```json
{
  "action": "play_video",
  "device_id": "SIMULATOR_001", 
  "video": "startup.jpg",
  "fps": 15
}
```

Commands tested in the simulator will work identically on physical devices.

## Technical Details

### Display Specifications
- **Controller**: ST7789 TFT (simulated)
- **Resolution**: 320 x 240 pixels
- **Color Format**: RGB565 (16-bit) 
- **Refresh Rate**: Up to 30 FPS
- **Brightness**: PWM controlled backlight simulation

### File Formats
- **Images**: JPEG format preferred
- **Animations**: Sequential JPEG frames
- **Naming**: Use descriptive names (e.g., `startup.jpg`, `menu_main.jpg`)

### Performance Considerations
- The simulator processes images in real-time
- Frame rates above 20 FPS may stress the system
- Large animations (>50 frames) should be tested carefully

## Troubleshooting

### Common Issues

**Simulator shows "Frame Missing"**:
- The requested image file doesn't exist
- Check the filename in your commands
- Verify the file is accessible by the server

**Animation not playing**:
- Ensure FPS is set appropriately (5-30)
- Check that the animation sequence exists
- Verify the play command was sent successfully

**Blank or black screen**:
- Click "Clear" and try again
- Check server logs for errors
- Verify the enhanced server is running

### Debug Information

The simulator provides real-time debug information:
- Current resolution and scale
- Active video/animation name
- Frame numbers for animations
- Brightness and FPS settings

## API Endpoints

If you're developing custom clients:

- `GET /api/simulator/frames/{filename}` - Get a frame image
- `POST /api/command` - Send commands to simulator
- `GET /api/simulator/status` - Get simulator status

## Future Enhancements

Planned features for the simulator:
- Multi-device simulation (multiple screens)
- Touch input simulation
- Sound effect integration  
- Network latency simulation
- Recording and playback of test sessions

## Tips for GUI Development

### Best Practices
1. **Design for 320x240**: Always design at native resolution
2. **High Contrast**: Ensure good visibility in different lighting
3. **Large Text**: Use at least 12pt fonts for readability
4. **Simple Layouts**: Avoid cluttered interfaces
5. **Test Animations**: Verify smooth playback at target FPS

### Color Guidelines
- Use web-safe colors for consistency
- Test with brightness at 50% to simulate battery saving
- Avoid pure white backgrounds (hard on eyes)
- Consider color-blind accessibility

### Performance Tips
- Keep animations under 2 seconds when possible
- Use solid colors for backgrounds to reduce processing
- Test frame rates - 15 FPS is often sufficient
- Optimize JPEG quality vs file size

The ESP32 simulator is a powerful tool for developing your Tricorder GUI efficiently. It saves time by allowing you to iterate on designs quickly without flashing firmware or handling physical devices.
