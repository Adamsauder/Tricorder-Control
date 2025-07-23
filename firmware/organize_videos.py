#!/usr/bin/env python3
"""
Organize Test Videos for Simplified Tricorder Usage
Copies representative files from complex frame sequences to simple names
"""

import os
import shutil

def organize_test_videos():
    """Organize test videos into simple naming scheme"""
    
    source_dir = "test_videos"
    output_dir = "simple_videos"
    
    if not os.path.exists(source_dir):
        print(f"Error: {source_dir} directory not found.")
        print("Run 'python generate_test_patterns.py' first to create test videos.")
        return
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    print("Organizing test videos for simplified usage...")
    print(f"Source: {source_dir}/")
    print(f"Output: {output_dir}/")
    print()
    
    # Copy representative files with simple names
    file_mappings = [
        # Source file -> Simple name
        ("static_test.jpg", "static_test.jpg"),
        ("color_red.jpg", "color_red.jpg"),
        ("color_green.jpg", "color_green.jpg"),
        ("color_blue.jpg", "color_blue.jpg"),
        ("color_white.jpg", "color_white.jpg"),
        ("color_yellow.jpg", "color_yellow.jpg"),
        ("color_cyan.jpg", "color_cyan.jpg"),
        ("color_magenta.jpg", "color_magenta.jpg"),
        
        # Use first frame for animated sequences
        ("animated_test_frame_001.jpg", "animated_test.jpg"),
        ("startup_frame_001.jpg", "startup.jpg"),
        
        # Create some additional representative frames
        ("animated_test_frame_015.jpg", "animated_mid.jpg"),
        ("startup_frame_010.jpg", "startup_mid.jpg"),
    ]
    
    copied_count = 0
    
    for source_file, target_file in file_mappings:
        source_path = os.path.join(source_dir, source_file)
        target_path = os.path.join(output_dir, target_file)
        
        if os.path.exists(source_path):
            shutil.copy2(source_path, target_path)
            file_size = os.path.getsize(target_path)
            print(f"✓ {source_file} -> {target_file} ({file_size} bytes)")
            copied_count += 1
        else:
            print(f"✗ {source_file} not found")
    
    print()
    print(f"Organized {copied_count} video files for tricorder use.")
    print()
    print("📋 To use on your tricorder:")
    print("1. Copy all files from 'simple_videos/' to your SD card's '/videos/' directory")
    print("2. Use these simple commands:")
    print()
    print("   # Test basic display")
    print("   play static_test")
    print()
    print("   # Test colors") 
    print("   play color_red")
    print("   play color_blue")
    print("   play color_white")
    print()
    print("   # Test animations")
    print("   play animated_test")
    print("   play startup")
    print()
    print("3. All commands now use simple base names (no _001, _frame_, etc.)")

def create_video_readme():
    """Create a README for the simple videos"""
    
    readme_content = '''# Simple Tricorder Videos

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
'''
    
    with open("simple_videos/README.md", "w") as f:
        f.write(readme_content)
    
    print("✓ Created README.md in simple_videos/")

if __name__ == "__main__":
    try:
        organize_test_videos()
        create_video_readme()
        
        print()
        print("🎉 Video organization complete!")
        print()
        print("Next steps:")
        print("1. Copy 'simple_videos/*.jpg' to your SD card's /videos/ directory")
        print("2. Flash the updated firmware to your ESP32")
        print("3. Test with simple commands like 'play color_red'")
        
    except Exception as e:
        print(f"Error: {e}")
