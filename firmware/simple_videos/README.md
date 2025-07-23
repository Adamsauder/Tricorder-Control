# Simple Tricorder Videos

This directory contains simplified video files for easy tricorder testing.

## Available Videos

### Static Test Patterns
- `static_test.jpg` - Gradient background with corner markers and crosshair
- `color_red.jpg` - Solid red background
- `color_green.jpg` - Solid green background  
- `color_blue.jpg` - Solid blue background
- `color_white.jpg` - Solid white background
- `color_yellow.jpg` - Solid yellow background
- `color_cyan.jpg` - Solid cyan background
- `color_magenta.jpg` - Solid magenta background

### Animation Frames
- `animated_test.jpg` - Colorful rotating pattern (first frame)
- `animated_mid.jpg` - Colorful rotating pattern (middle frame)
- `startup.jpg` - Startup animation (first frame)
- `startup_mid.jpg` - Startup animation (middle frame)

## Usage

Copy all files to your SD card's `/videos/` directory, then use simple commands:

```bash
# Basic test
play static_test

# Color tests  
play color_red
play color_blue

# Animation tests
play animated_test
play startup
```

## File Sizes
All files are optimized for ESP32:
- Resolution: 320x240 pixels
- Format: JPEG
- Size: 2-10KB each
- Compatible with ESP32 memory constraints

## Notes
- All files are single images (no frame sequences)
- Use base names without extensions in play commands
- Firmware automatically finds and plays the correct file
