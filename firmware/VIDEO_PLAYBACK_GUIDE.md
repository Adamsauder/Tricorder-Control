# Tricorder Video Playback Guide

## Overview
The tricorder firmware now supports video playback from SD card using JPEG image sequences. This provides a simple but effective way to display animations and video content on the ESP32's TFT display.

## Hardware Requirements
- ESP32-2432S032C board with TFT display
- MicroSD card (formatted as FAT32)
- SD card inserted into the board's SD slot

## SD Card Setup

### 1. Format SD Card
Format your microSD card as FAT32.

### 2. Create Video Directory
Create a folder named `videos` in the root of the SD card:
```
/videos/
```

### 3. Prepare Video Content
Videos should be prepared as JPEG image sequences. You can convert video files using FFmpeg:

```bash
# Convert MP4 to JPEG sequence (15 FPS, 320x240 resolution)
ffmpeg -i input.mp4 -vf "scale=320:240" -r 15 -q:v 2 frame_%04d.jpg

# For smaller file sizes, use lower quality
ffmpeg -i input.mp4 -vf "scale=320:240" -r 15 -q:v 8 frame_%04d.jpg
```

### 4. Organize Files (Simplified Approach)
Place JPEG files in the `/videos/` directory using simple names:
- Use single representative images instead of frame sequences
- Name files with simple, descriptive names
- No frame numbers or complex suffixes needed

Example structure:
```
/videos/
├── static_test.jpg      # Test pattern
├── color_red.jpg        # Solid red
├── color_blue.jpg       # Solid blue
├── startup.jpg          # Startup animation frame
└── animated_test.jpg    # Animation frame
```

### 5. Simple Commands
With the simplified approach, use base names without extensions:
```bash
# Play videos using simple names
play static_test     # Displays static_test.jpg
play color_red       # Displays color_red.jpg
play startup         # Displays startup.jpg or startup_frame_001.jpg
```

## Usage

### UDP Commands

#### Play Video (Simplified)
```json
{
  "action": "play_video",
  "commandId": "cmd123",
  "parameters": {
    "filename": "color_red",
    "loop": true
  }
}
```

**Note:** Use simple base names without file extensions or frame numbers. The firmware automatically finds the correct file.

Examples:
- `"filename": "static_test"` → finds `static_test.jpg`
- `"filename": "color_red"` → finds `color_red.jpg`  
- `"filename": "startup"` → finds `startup_frame_001.jpg` or `startup.jpg`

#### Stop Video
```json
{
  "action": "stop_video",
  "commandId": "cmd124"
}
```

#### List Available Videos
```json
{
  "action": "list_videos",
  "commandId": "cmd125"
}
```

**Note:** This returns simplified base names, not individual frame files.

#### Get Status (includes video info)
```json
{
  "action": "status",
  "commandId": "cmd126"
}
```

### Status Response
The status response now includes video-related information:
```json
{
  "commandId": "cmd126",
  "deviceId": "TRICORDER_001",
  "sdCardInitialized": true,
  "videoPlaying": true,
  "currentVideo": "startup_animation.jpg",
  "videoLooping": true,
  "currentFrame": 42
}
```

## Performance Notes

### Frame Rate
- Default: ~15 FPS (66ms per frame)
- Adjust `FRAME_DELAY_MS` in firmware for different rates
- Lower frame rates = smoother WiFi/LED performance

### Memory Usage
- Video buffer: 8KB by default
- Larger files may require multiple reads per frame
- Monitor free heap in status responses

### SD Card Performance
- Use high-quality, fast SD cards (Class 10 or better)
- Avoid very large files (>100KB per frame)
- Sequential reads are fastest

## Troubleshooting

### Video Won't Play
1. Check SD card is properly inserted and detected
2. Verify file exists in `/videos/` directory
3. Ensure file is valid JPEG format
4. Check serial output for error messages

### Poor Performance
1. Reduce JPEG quality/file size
2. Lower frame rate
3. Use smaller resolution
4. Check available heap memory

### SD Card Issues
1. Reformat SD card as FAT32
2. Use a different/newer SD card
3. Check SD card connections
4. Verify pin definitions match your board

## Pin Configuration
Current SD card pins (adjust if different on your board):
```cpp
#define SD_CS 5            // SD card chip select
#define SD_MOSI 23         // SD card MOSI
#define SD_MISO 19         // SD card MISO
#define SD_SCLK 18         // SD card SCLK
```

## Future Enhancements
- Support for AVI/MP4 containers
- Audio playback capability
- Better compression/streaming
- Video thumbnail generation
- Playlist support
