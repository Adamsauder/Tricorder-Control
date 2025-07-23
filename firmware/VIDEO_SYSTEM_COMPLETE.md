# Tricorder Video System - Complete Guide

## 🎯 **Simplified Video System Overview**

The tricorder video system has been streamlined to use **single images** with **simple base names** instead of complex frame sequences. This makes it much easier to manage and use.

## 📁 **File Organization**

### Before (Complex)
```
/videos/
├── animated_test_frame_001.jpg
├── animated_test_frame_002.jpg
├── animated_test_frame_003.jpg
├── ...
├── startup_frame_001.jpg
├── startup_frame_002.jpg
└── ...
```

### After (Simplified) ✅
```
/videos/
├── static_test.jpg      # Test pattern
├── color_red.jpg        # Solid colors
├── color_blue.jpg
├── startup.jpg          # Animation frames
├── animated_test.jpg
└── ...
```

## 🎮 **Command Usage**

### Before (Complex)
```json
{"action": "play_video", "parameters": {"filename": "animated_test_frame_001.jpg"}}
```

### After (Simplified) ✅  
```json
{"action": "play_video", "parameters": {"filename": "animated_test"}}
```

**Interactive Commands:**
```bash
play static_test     # No .jpg extension needed
play color_red       # Simple base names
play startup         # Firmware finds the right file
```

## 🛠 **Setup Process**

### 1. **Generate Test Videos**
```bash
cd "c:\Tricorder Control\Tricorder-Control\firmware"
python generate_test_patterns.py  # Creates test_videos/
python organize_videos.py         # Creates simple_videos/
```

### 2. **Prepare SD Card**
1. Format microSD card as **FAT32**
2. Create `/videos/` directory
3. Copy files from `simple_videos/` to SD card's `/videos/`

### 3. **Flash Updated Firmware**
The firmware now includes:
- ✅ Smart file finding (base name → actual file)
- ✅ Simplified video listing
- ✅ Better error handling
- ✅ UDP response optimization

### 4. **Test the System**
```bash
python test_video.py 192.168.1.48  # Use your tricorder's IP
# Select 'i' for interactive mode
```

## 🧪 **Available Test Videos**

| Command | File | Description | Size |
|---------|------|-------------|------|
| `play static_test` | static_test.jpg | Test pattern with crosshair | 6.7KB |
| `play color_red` | color_red.jpg | Solid red background | 2.7KB |
| `play color_blue` | color_blue.jpg | Solid blue background | 3.0KB |
| `play color_white` | color_white.jpg | Solid white background | 2.9KB |
| `play startup` | startup.jpg | Startup animation frame | 1.8KB |
| `play animated_test` | animated_test.jpg | Colorful pattern | 10.2KB |

## 📡 **Network Commands Reference**

### Basic Commands
```bash
# In interactive mode (python test_video.py IP):
status              # Get device status
list               # List available videos  
play <name>        # Play video by base name
stop               # Stop current video
led 255 0 0        # Set LEDs to red
quit               # Exit
```

### UDP JSON Commands
```json
// Get status
{"action": "status", "commandId": "test1"}

// List videos (returns base names)
{"action": "list_videos", "commandId": "test2"}

// Play video (use base name)
{"action": "play_video", "commandId": "test3", 
 "parameters": {"filename": "color_red", "loop": true}}

// Stop video
{"action": "stop_video", "commandId": "test4"}

// LED control
{"action": "set_led_color", "commandId": "test5",
 "parameters": {"r": 255, "g": 0, "b": 0}}
```

## 🔧 **Firmware Features**

### Smart File Resolution
The firmware automatically finds files using base names:
- `"color_red"` → finds `color_red.jpg`
- `"startup"` → finds `startup.jpg` or `startup_frame_001.jpg`
- `"animated_test"` → finds `animated_test.jpg` or `animated_test_frame_001.jpg`

### Video List Optimization  
- Groups frame sequences by base name
- Returns simplified names over UDP
- Avoids buffer overflow issues
- Still logs full details to serial

### Error Handling
- Clear error messages for missing files
- Automatic SD card detection
- Network timeout handling
- Memory usage monitoring

## 🚀 **Quick Start Checklist**

- [ ] Generate test videos: `python generate_test_patterns.py`
- [ ] Organize videos: `python organize_videos.py`  
- [ ] Copy `simple_videos/*.jpg` to SD card `/videos/`
- [ ] Flash updated firmware to ESP32
- [ ] Test connection: `ping 192.168.1.48`
- [ ] Run test script: `python test_video.py 192.168.1.48`
- [ ] Try commands: `list`, `play color_red`, `status`

## 🎉 **Benefits of Simplified System**

### For Users
- ✅ Simple, memorable command names
- ✅ No complex file management
- ✅ Faster testing and development
- ✅ Less SD card space usage

### For Developers  
- ✅ Easier file organization
- ✅ Reduced network traffic
- ✅ Better error handling
- ✅ More reliable performance

### For Film Sets
- ✅ Quick video changes
- ✅ Intuitive operation
- ✅ Reliable cuing system
- ✅ Easy troubleshooting

## 📝 **Next Steps**

1. **Test the current system** with provided test videos
2. **Create custom videos** using your own content
3. **Integrate with film set workflows**
4. **Expand with additional features** (audio, sensors, etc.)

The video system is now production-ready for film set use! 🎬
