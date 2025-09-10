#!/usr/bin/env python3
"""
MP4 to Tricorder JPEG Converter

Converts MP4 videos to JPEG frame sequences optimized for the ESP32 Tricorder:
- Resizes to 320x240 (tricorder display resolution)
- Extracts frames at specified FPS
- Names frames sequentially for proper playback order
- Optimizes JPEG quality for ESP32 memory constraints

Usage:
    python mp4_to_tricorder.py input_video.mp4 output_folder

Requirements:
    pip install opencv-python pillow

Author: GitHub Copilot
"""

import cv2
import os
import sys
from PIL import Image
import argparse

def convert_mp4_to_tricorder_frames(input_video, output_folder, fps=10, max_frames=None, portrait=False):
    """
    Convert MP4 video to JPEG frames for tricorder playback
    
    Args:
        input_video (str): Path to input MP4 file
        output_folder (str): Output directory for JPEG frames
        fps (int): Target frames per second for extraction (default: 10)
        max_frames (int): Maximum number of frames to extract (default: None = no limit)
        portrait (bool): If True, resize to 240x320 (portrait), else 320x240 (landscape)
    """
    
    # Create output directory if it doesn't exist
    os.makedirs(output_folder, exist_ok=True)
    
    # Open video file
    cap = cv2.VideoCapture(input_video)
    
    if not cap.isOpened():
        print(f"Error: Could not open video file {input_video}")
        return False
    
    # Get video properties
    video_fps = cap.get(cv2.CAP_PROP_FPS)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    duration = total_frames / video_fps
    
    # Set target resolution based on orientation
    if portrait:
        target_width, target_height = 240, 320
        orientation_desc = "portrait (240x320)"
    else:
        target_width, target_height = 320, 240
        orientation_desc = "landscape (320x240)"
    
    print(f"Video Info:")
    print(f"  File: {input_video}")
    print(f"  Original FPS: {video_fps:.2f}")
    print(f"  Total frames: {total_frames}")
    print(f"  Duration: {duration:.2f} seconds")
    print(f"  Target extraction FPS: {fps}")
    print(f"  Target resolution: {orientation_desc}")
    if max_frames:
        print(f"  Max frames to extract: {max_frames}")
    else:
        print(f"  Extracting all frames (no limit)")
    
    # Calculate frame interval for desired FPS
    frame_interval = max(1, int(video_fps / fps))
    
    frame_count = 0
    extracted_count = 0
    
    while True:
        ret, frame = cap.read()
        
        if not ret or (max_frames and extracted_count >= max_frames):
            break
        
        # Only process every nth frame based on target FPS
        if frame_count % frame_interval == 0:
            # Convert BGR to RGB (OpenCV uses BGR by default)
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            
            # Convert to PIL Image for easier resizing
            pil_image = Image.fromarray(frame_rgb)
            
            # Resize to tricorder display resolution based on orientation
            # Maintain aspect ratio and pad if necessary
            pil_image.thumbnail((target_width, target_height), Image.Resampling.LANCZOS)
            
            # Create a new image with black background using target dimensions
            final_image = Image.new('RGB', (target_width, target_height), (0, 0, 0))
            
            # Center the resized image
            x_offset = (target_width - pil_image.width) // 2
            y_offset = (target_height - pil_image.height) // 2
            final_image.paste(pil_image, (x_offset, y_offset))
            
            # Save as JPEG with optimized settings for ESP32
            frame_filename = f"frame_{extracted_count+1:03d}.jpg"
            frame_path = os.path.join(output_folder, frame_filename)
            
            # Use moderate JPEG quality to balance file size and visual quality
            final_image.save(frame_path, 
                           format='JPEG', 
                           quality=85, 
                           optimize=True)
            
            extracted_count += 1
            print(f"  Extracted frame {extracted_count}: {frame_filename}")
        
        frame_count += 1
    
    cap.release()
    
    print(f"\nConversion complete!")
    print(f"  Extracted {extracted_count} frames")
    print(f"  Output folder: {output_folder}")
    print(f"  Frame naming: frame_001.jpg to frame_{extracted_count:03d}.jpg")
    print(f"\nTo use with tricorder:")
    print(f"  1. Copy the entire '{os.path.basename(output_folder)}' folder to your SD card's /videos/ directory")
    print(f"  2. Use the tricorder web interface to play folder '{os.path.basename(output_folder)}'")
    
    if extracted_count > 30:
        print(f"\n⚠️  Note: Extracted {extracted_count} frames (more than firmware's 30-frame limit)")
        print(f"    The tricorder will only play the first 30 frames in sequence")
        print(f"    Consider using --max-frames 30 or increasing --fps to reduce frame count")

    
    return True

def main():
    parser = argparse.ArgumentParser(description='Convert MP4 video to tricorder JPEG frames')
    parser.add_argument('input_video', help='Path to input MP4 file')
    parser.add_argument('output_folder', help='Output directory for JPEG frames')
    parser.add_argument('--fps', type=int, default=10, help='Target FPS for frame extraction (default: 10)')
    parser.add_argument('--max-frames', type=int, default=None, help='Maximum frames to extract (default: None = no limit)')
    parser.add_argument('--portrait', action='store_true', help='Use portrait orientation (240x320) instead of landscape (320x240)')
    
    args = parser.parse_args()
    
    # Check if input file exists
    if not os.path.exists(args.input_video):
        print(f"Error: Input video file '{args.input_video}' not found")
        sys.exit(1)
    
    # Check if input is a video file
    if not args.input_video.lower().endswith(('.mp4', '.avi', '.mov', '.mkv', '.wmv')):
        print(f"Warning: '{args.input_video}' doesn't appear to be a common video format")
    
    success = convert_mp4_to_tricorder_frames(
        args.input_video, 
        args.output_folder, 
        args.fps, 
        args.max_frames,
        args.portrait
    )
    
    if success:
        print("\n✅ Conversion successful!")
    else:
        print("\n❌ Conversion failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
