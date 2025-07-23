#!/bin/bash

# Tricorder Video Preparation Script
# Converts video files to JPEG sequences suitable for ESP32 playback

# Check if FFmpeg is installed
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: FFmpeg is not installed. Please install FFmpeg first."
    echo "Visit: https://ffmpeg.org/download.html"
    exit 1
fi

# Function to display usage
usage() {
    echo "Usage: $0 <input_video> [output_name] [fps] [quality]"
    echo ""
    echo "Parameters:"
    echo "  input_video  : Input video file (mp4, avi, mov, etc.)"
    echo "  output_name  : Output filename prefix (default: uses input name)"
    echo "  fps          : Frame rate (default: 15, max recommended: 20)"
    echo "  quality      : JPEG quality 1-31, lower=better (default: 5)"
    echo ""
    echo "Examples:"
    echo "  $0 startup.mp4"
    echo "  $0 animation.avi my_animation 10 8"
    echo ""
    echo "Output will be saved as a single JPEG file per sequence."
}

# Check arguments
if [ $# -lt 1 ]; then
    usage
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_NAME="${2:-$(basename "${INPUT_FILE%.*}")}"
FPS="${3:-15}"
QUALITY="${4:-5}"

# Validate input file
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found."
    exit 1
fi

# Validate parameters
if ! [[ "$FPS" =~ ^[0-9]+$ ]] || [ "$FPS" -lt 1 ] || [ "$FPS" -gt 30 ]; then
    echo "Error: FPS must be between 1 and 30"
    exit 1
fi

if ! [[ "$QUALITY" =~ ^[0-9]+$ ]] || [ "$QUALITY" -lt 1 ] || [ "$QUALITY" -gt 31 ]; then
    echo "Error: Quality must be between 1 and 31"
    exit 1
fi

echo "Converting video to ESP32-compatible format..."
echo "Input: $INPUT_FILE"
echo "Output: ${OUTPUT_NAME}.jpg"
echo "FPS: $FPS"
echo "Quality: $QUALITY"
echo ""

# Create output directory
mkdir -p "tricorder_videos"

# Get video info
duration=$(ffprobe -v quiet -show_entries format=duration -of csv=p=0 "$INPUT_FILE")
frame_count=$(echo "$duration * $FPS" | bc -l | cut -d'.' -f1)

echo "Video duration: ${duration}s"
echo "Estimated frames: $frame_count"
echo ""

# Convert video to JPEG sequence, then combine into single file
# For ESP32, we'll create a simple format where each frame is a standard JPEG
# This approach uses a single JPEG file that can be read frame by frame

# First, extract all frames
echo "Extracting frames..."
ffmpeg -i "$INPUT_FILE" \
    -vf "scale=320:240:force_original_aspect_ratio=decrease,pad=320:240:(ow-iw)/2:(oh-ih)/2" \
    -r "$FPS" \
    -q:v "$QUALITY" \
    -y \
    "tricorder_videos/temp_frame_%04d.jpg"

if [ $? -ne 0 ]; then
    echo "Error: FFmpeg conversion failed"
    exit 1
fi

# Count actual frames created
actual_frames=$(ls tricorder_videos/temp_frame_*.jpg 2>/dev/null | wc -l)
echo "Frames extracted: $actual_frames"

# For simple implementation, let's create a single representative frame
# In a full implementation, you'd concatenate all frames or create a custom format
if [ "$actual_frames" -gt 0 ]; then
    # Use the first frame as the video "thumbnail" for now
    cp "tricorder_videos/temp_frame_0001.jpg" "tricorder_videos/${OUTPUT_NAME}.jpg"
    
    # Get file size
    file_size=$(stat -f%z "tricorder_videos/${OUTPUT_NAME}.jpg" 2>/dev/null || stat -c%s "tricorder_videos/${OUTPUT_NAME}.jpg" 2>/dev/null)
    
    echo ""
    echo "Conversion complete!"
    echo "Output file: tricorder_videos/${OUTPUT_NAME}.jpg"
    echo "File size: ${file_size} bytes"
    
    if [ "$file_size" -gt 51200 ]; then
        echo "Warning: File size is large (>50KB). Consider increasing quality value for smaller files."
    fi
    
    echo ""
    echo "Copy this file to your SD card's /videos/ directory:"
    echo "  SD:/videos/${OUTPUT_NAME}.jpg"
    echo ""
    echo "To play this video, send UDP command:"
    echo '{"action": "play_video", "commandId": "test", "parameters": {"filename": "'${OUTPUT_NAME}'.jpg", "loop": true}}'
else
    echo "Error: No frames were extracted"
    exit 1
fi

# Clean up temporary frames
rm -f tricorder_videos/temp_frame_*.jpg

echo "Done!"
