# Tricorder Firmware v3.0 - MPEG Video Edition

## 🎬 Major New Features

### MPEG Video Playback
- **Full MPEG-1/MPEG-2 support** via ESP32_MJPEG library
- **Audio playback** with synchronized video (optional)
- **Advanced buffering** with configurable buffer sizes
- **Video seeking** and position control
- **Quality settings** from 1-10 for performance tuning
- **Streaming mode** for large video files
- **Video scaling** to fit display perfectly

### Performance Enhancements
- **CPU frequency control** (80/160/240 MHz)
- **Memory optimization** with heap monitoring
- **Multi-core processing** with dedicated video task
- **Preloading system** for smoother playback
- **Background processing** for uninterrupted operation

### Enhanced Configuration
- **Extended settings** for video quality and performance
- **Audio enable/disable** control
- **Frame rate configuration** up to 60 FPS
- **Buffer size optimization** (8KB to 128KB)
- **Video looping** and scaling controls

## 📋 Hardware Requirements

- **ESP32-2432S032C-I** board with TFT display
- **MicroSD card** (Class 10 or better recommended)
- **Minimum 4MB** flash memory
- **External power** recommended for optimal performance

## 📁 File Structure

### Video Files
Place MPEG files in `/videos/` directory on SD card:
```
/videos/
├── startup.mpeg          # Startup animation
├── tricorder_scan.mpg     # Scanning effect
├── data_analysis.mjpeg    # Data processing animation
└── alert_red.mpeg         # Alert sequence
```

### Audio Files (Optional)
Place corresponding MP3 files in `/audio/` directory:
```
/audio/
├── startup.mp3            # Audio for startup.mpeg
├── tricorder_scan.mp3     # Audio for tricorder_scan.mpg
└── alert_red.mp3          # Audio for alert_red.mpeg
```

## 🎮 Usage

### Web Interface
Access the web interface at the device's IP address:
- **Play/Pause/Stop** video controls
- **File browser** for available videos
- **Quality settings** adjustment
- **Audio toggle** control
- **Real-time status** monitoring

### API Endpoints

#### Video Control
```bash
# Play video with options
POST /play-video
{
  "filename": "startup",
  "loop": true,
  "audio": true
}

# Pause/Resume/Stop
POST /pause-video
POST /resume-video
POST /stop-video

# Seek to position (seconds)
POST /seek-video
{
  "position": 30
}
```

#### Configuration
```bash
# Get current configuration
GET /config

# Update configuration
POST /config
{
  "videoQuality": 8,
  "videoFrameRate": 30,
  "videoBufferSize": 65536,
  "videoAudioEnabled": true
}
```

#### LED Control
```bash
# Set LED colors
POST /led
{
  "r": 255,
  "g": 0,
  "b": 0
}

# Set brightness
POST /brightness?brightness=128
```

### UDP Commands (Compatible with Server)
```json
{
  "action": "play_video",
  "commandId": "cmd123",
  "parameters": {
    "filename": "startup",
    "loop": true,
    "enableAudio": true
  }
}
```

## 🔧 Configuration Options

### Video Settings
- **videoQuality**: 1-10 (higher = better quality, more processing)
- **videoFrameRate**: 1-60 FPS (target frame rate)
- **videoBufferSize**: 8192-131072 bytes (streaming buffer)
- **videoAudioEnabled**: true/false (enable audio playback)
- **videoLooping**: true/false (default loop behavior)
- **videoScaling**: true/false (scale to fit display)

### Performance Settings
- **cpuFrequency**: 80/160/240 MHz (CPU speed)
- **heapThreshold**: Minimum free heap before warnings
- **videoPreloading**: true/false (preload next frame)

## 📊 Performance Guidelines

### Optimal Settings by Use Case

#### High Quality (240MHz CPU)
```json
{
  "videoQuality": 9,
  "videoFrameRate": 30,
  "videoBufferSize": 131072,
  "cpuFrequency": 240
}
```

#### Balanced (160MHz CPU)
```json
{
  "videoQuality": 7,
  "videoFrameRate": 25,
  "videoBufferSize": 65536,
  "cpuFrequency": 160
}
```

#### Power Saving (80MHz CPU)
```json
{
  "videoQuality": 5,
  "videoFrameRate": 15,
  "videoBufferSize": 32768,
  "cpuFrequency": 80
}
```

## 🎥 Video Preparation

### Converting Videos to MPEG
```bash
# High quality MPEG-1 (recommended)
ffmpeg -i input.mp4 -vf "scale=320:240" -c:v mpeg1video -q:v 2 -r 25 output.mpeg

# Smaller file size
ffmpeg -i input.mp4 -vf "scale=240:180" -c:v mpeg1video -q:v 5 -r 15 output.mpeg

# MJPEG format (best compatibility)
ffmpeg -i input.mp4 -vf "scale=320:240" -c:v mjpeg -q:v 3 -r 25 output.mjpeg
```

### Audio Extraction
```bash
# Extract audio as MP3
ffmpeg -i input.mp4 -vn -c:a mp3 -b:a 96k output.mp3
```

## 🚀 Deployment

### Building and Flashing
```bash
cd firmware/tricorder_v3
pio run -t upload
```

### Monitoring
```bash
pio device monitor
```

## 📈 Monitoring and Debugging

### Status Information
- **Real-time performance** metrics
- **Memory usage** monitoring
- **Frame rate** statistics
- **Video position** and duration
- **Error reporting** and diagnostics

### Debug Output
Enable debug mode in configuration for detailed logging:
```json
{
  "debugMode": true
}
```

## 🔄 Migration from v2

### Key Differences
1. **File format**: MPEG instead of JPEG sequences
2. **Audio support**: Optional audio playback
3. **Performance**: Better CPU utilization
4. **Memory**: Improved memory management
5. **Configuration**: Extended video settings

### Compatibility
- **Web interface**: Enhanced with new controls
- **UDP protocol**: Backward compatible
- **LED control**: Same as v2
- **sACN support**: Unchanged

## 🛠️ Troubleshooting

### Common Issues

#### Video Won't Play
1. Check file format (MPEG-1, MPEG-2, MJPEG)
2. Verify file is in `/videos/` directory
3. Ensure SD card is properly mounted
4. Check available memory (>32KB recommended)

#### Poor Performance
1. Reduce video quality setting
2. Lower frame rate
3. Increase CPU frequency
4. Check heap memory usage

#### Audio Issues
1. Verify audio file exists in `/audio/`
2. Check audio format (MP3 only)
3. Ensure audio is enabled in configuration
4. Monitor memory usage during playback

### Memory Optimization
```cpp
// Monitor heap usage
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Largest block: %d bytes\n", ESP.getMaxAllocHeap());
```

## 📝 Version History

### v3.0 (Current)
- Full MPEG video support
- Audio playback capability
- Performance optimizations
- Enhanced web interface
- Multi-core task processing

### v2.x
- JPEG sequence playback
- Basic web interface
- UDP control protocol
- sACN integration

## 🎯 Future Enhancements

- **Hardware video decoding** for better performance
- **Streaming from network** sources
- **Video effects** and transitions
- **Playlist support** for multiple videos
- **Real-time video** capture and processing
