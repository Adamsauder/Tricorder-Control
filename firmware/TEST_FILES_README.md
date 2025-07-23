# Tricorder Video Test Files

This directory contains scripts and tools for testing video playback functionality on your ESP32 tricorder.

## Test Scripts

### `generate_test_patterns.py`
Python script that creates various test pattern images for testing video display.

**Requirements:**
```bash
pip install Pillow
```

**Usage:**
```bash
python generate_test_patterns.py
```

**Generated Files:**
- `static_test.jpg` - Static test pattern with corner markers and crosshair
- `color_*.jpg` - Solid color test images (red, green, blue, etc.)
- `animated_test_frame_*.jpg` - 30-frame animated sequence
- `startup_frame_*.jpg` - 20-frame startup animation

### `test_video.py`
Python script for sending UDP commands to test video playback.

**Usage:**
```bash
# Run automated tests
python test_video.py

# Run with specific IP
python test_video.py 192.168.1.100

# Interactive mode
python test_video.py
# Then select 'i' for interactive mode
```

### `prepare_video.sh` / `prepare_video.bat`
Convert existing video files to ESP32-compatible JPEG format.

**Requirements:**
- FFmpeg installed and in PATH

**Usage:**
```bash
# Linux/Mac
./prepare_video.sh input_video.mp4

# Windows
prepare_video.bat input_video.mp4
```

## Testing Procedure

### 1. Prepare Test Files
```bash
# Generate test patterns
python generate_test_patterns.py

# Copy to SD card
# Copy files from test_videos/ to SD:/videos/
```

### 2. Test Basic Functionality
```bash
# Run the test script
python test_video.py YOUR_TRICORDER_IP

# Follow the prompts to:
# - Check device status
# - List available videos
# - Play test videos
# - Test LED controls
```

### 3. Manual Testing
Use the interactive mode for manual testing:
```bash
python test_video.py YOUR_TRICORDER_IP
# Select 'i' for interactive mode

# Try these commands:
status                    # Check device status
list                     # List videos on SD card
play static_test.jpg     # Play static test pattern
play color_red.jpg       # Play red color test
stop                     # Stop current video
led 255 0 0             # Set LEDs to red
```

## Expected Results

### Static Test Pattern
- Should display colorful gradient background
- White corner markers visible
- Center crosshair visible
- Text "TRICORDER TEST" and resolution info

### Color Tests
- Should display solid colors
- Color name should be visible in center
- No flickering or artifacts

### Animated Sequences
- Smooth color transitions
- Frame counter should increment
- Moving elements should be visible
- No significant lag or stuttering

## Troubleshooting

### Video Not Playing
1. Check SD card is inserted and detected
2. Verify files are in `/videos/` directory
3. Check file names match exactly (case sensitive)
4. Ensure files are valid JPEG format

### Poor Performance
1. Reduce file sizes (increase JPEG quality number)
2. Use smaller resolution images
3. Check available memory in status output

### UDP Communication Issues
1. Verify tricorder IP address
2. Check both devices are on same network
3. Ensure UDP port 8888 is not blocked
4. Try ping test first

### Display Issues
1. Check TFT display configuration
2. Verify pin connections
3. Test with simple color patterns first

## File Size Guidelines

For best performance:
- **Resolution**: 320x240 or smaller
- **File Size**: <50KB per image
- **Format**: JPEG with quality 5-10
- **Frame Rate**: 15 FPS or lower

## Custom Test Patterns

To create your own test patterns, modify `generate_test_patterns.py`:

```python
# Example: Create custom pattern
def create_custom_pattern():
    img = Image.new('RGB', (320, 240), 'blue')
    draw = ImageDraw.Draw(img)
    draw.text((100, 100), "CUSTOM", fill='white')
    return img
```
