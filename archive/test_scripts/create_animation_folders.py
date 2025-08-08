#!/usr/bin/env python3
"""
Create Animation Folders Script
Organizes existing JPEG frame sequences into folders for the tricorder video system.
"""

import os
import shutil
import re
from pathlib import Path

def create_animation_folders(source_dir, output_dir):
    """
    Organize JPEG frame sequences into folders.
    
    Args:
        source_dir: Directory containing frame sequence files
        output_dir: Directory where animation folders will be created
    """
    
    if not os.path.exists(source_dir):
        print(f"Source directory not found: {source_dir}")
        return
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Group files by base name
    animations = {}
    
    # Pattern to match frame sequences
    frame_patterns = [
        r'(.+)_(\d+)\.(jpg|jpeg)$',           # name_001.jpg
        r'(.+)_frame_(\d+)\.(jpg|jpeg)$',     # name_frame_001.jpg
        r'(.+)\.(\d+)\.(jpg|jpeg)$',          # name.001.jpg
    ]
    
    print(f"Scanning {source_dir} for frame sequences...")
    
    for filename in os.listdir(source_dir):
        if not filename.lower().endswith(('.jpg', '.jpeg')):
            continue
            
        base_name = None
        frame_num = None
        
        # Try each pattern
        for pattern in frame_patterns:
            match = re.match(pattern, filename, re.IGNORECASE)
            if match:
                base_name = match.group(1)
                frame_num = int(match.group(2))
                break
        
        if base_name:
            if base_name not in animations:
                animations[base_name] = []
            animations[base_name].append((frame_num, filename))
        else:
            # Single file without frame number
            base_name = os.path.splitext(filename)[0]
            if base_name not in animations:
                animations[base_name] = []
            animations[base_name].append((0, filename))
    
    # Create folders and organize files
    for anim_name, frames in animations.items():
        if len(frames) == 0:
            continue
            
        # Sort frames by frame number
        frames.sort(key=lambda x: x[0])
        
        # Create animation folder
        anim_folder = os.path.join(output_dir, anim_name)
        os.makedirs(anim_folder, exist_ok=True)
        
        print(f"\nCreating animation: {anim_name} ({len(frames)} frames)")
        
        # Copy and rename frames
        for i, (frame_num, filename) in enumerate(frames):
            source_path = os.path.join(source_dir, filename)
            
            # Create sequential filename (001.jpg, 002.jpg, etc.)
            new_filename = f"{i+1:03d}.jpg"
            dest_path = os.path.join(anim_folder, new_filename)
            
            shutil.copy2(source_path, dest_path)
            print(f"  {filename} -> {anim_name}/{new_filename}")
    
    print(f"\nAnimation folders created in: {output_dir}")

def create_sample_animations():
    """Create some sample animation folders with test patterns."""
    
    from PIL import Image, ImageDraw, ImageFont
    import math
    
    animations_dir = "sample_animations"
    os.makedirs(animations_dir, exist_ok=True)
    
    # Animation 1: Color Cycle
    color_cycle_dir = os.path.join(animations_dir, "color_cycle")
    os.makedirs(color_cycle_dir, exist_ok=True)
    
    colors = [
        (255, 0, 0),    # Red
        (255, 128, 0),  # Orange
        (255, 255, 0),  # Yellow
        (0, 255, 0),    # Green
        (0, 255, 255),  # Cyan
        (0, 0, 255),    # Blue
        (128, 0, 255),  # Purple
        (255, 0, 255),  # Magenta
    ]
    
    for i, color in enumerate(colors):
        img = Image.new('RGB', (320, 240), color)
        draw = ImageDraw.Draw(img)
        
        # Add frame number
        try:
            # Try to use a larger font
            font = ImageFont.truetype("arial.ttf", 24)
        except:
            font = ImageFont.load_default()
        
        draw.text((10, 10), f"Frame {i+1}/8", fill=(255, 255, 255), font=font)
        draw.text((10, 200), f"Color: {color}", fill=(255, 255, 255), font=font)
        
        img.save(os.path.join(color_cycle_dir, f"{i+1:03d}.jpg"), 'JPEG', quality=85)
    
    print(f"Created color_cycle animation with {len(colors)} frames")
    
    # Animation 2: Spinning Circle
    spinner_dir = os.path.join(animations_dir, "spinner")
    os.makedirs(spinner_dir, exist_ok=True)
    
    frames = 12
    for i in range(frames):
        img = Image.new('RGB', (320, 240), (0, 0, 0))
        draw = ImageDraw.Draw(img)
        
        # Calculate rotation angle
        angle = (i / frames) * 360
        
        # Draw spinning circle
        center_x, center_y = 160, 120
        radius = 50
        
        # Draw main circle
        draw.ellipse([center_x - radius, center_y - radius, 
                     center_x + radius, center_y + radius], 
                     outline=(0, 255, 0), width=3)
        
        # Draw rotating dot
        dot_x = center_x + int(radius * 0.7 * math.cos(math.radians(angle)))
        dot_y = center_y + int(radius * 0.7 * math.sin(math.radians(angle)))
        draw.ellipse([dot_x - 8, dot_y - 8, dot_x + 8, dot_y + 8], 
                     fill=(255, 0, 0))
        
        # Add frame info
        try:
            font = ImageFont.truetype("arial.ttf", 16)
        except:
            font = ImageFont.load_default()
        
        draw.text((10, 10), f"Frame {i+1}/{frames}", fill=(255, 255, 255), font=font)
        draw.text((10, 220), f"Angle: {angle:.0f}°", fill=(255, 255, 255), font=font)
        
        img.save(os.path.join(spinner_dir, f"{i+1:03d}.jpg"), 'JPEG', quality=85)
    
    print(f"Created spinner animation with {frames} frames")
    
    # Animation 3: Text Scroll
    scroll_dir = os.path.join(animations_dir, "text_scroll")
    os.makedirs(scroll_dir, exist_ok=True)
    
    text_frames = 16
    text = "TRICORDER ONLINE - SCANNING... - "
    
    for i in range(text_frames):
        img = Image.new('RGB', (320, 240), (0, 0, 50))
        draw = ImageDraw.Draw(img)
        
        # Scrolling text
        scroll_offset = i * 20
        
        try:
            font = ImageFont.truetype("arial.ttf", 20)
        except:
            font = ImageFont.load_default()
        
        # Draw scrolling text
        x_pos = 320 - scroll_offset
        draw.text((x_pos, 100), text, fill=(0, 255, 255), font=font)
        
        # Draw border
        draw.rectangle([0, 0, 319, 239], outline=(0, 255, 0), width=2)
        
        # Frame info
        draw.text((10, 10), f"Frame {i+1}/{text_frames}", fill=(255, 255, 255))
        
        img.save(os.path.join(scroll_dir, f"{i+1:03d}.jpg"), 'JPEG', quality=85)
    
    print(f"Created text_scroll animation with {text_frames} frames")
    
    print(f"\nSample animations created in: {animations_dir}")
    print("Copy these folders to your SD card's /videos directory")

def main():
    print("Tricorder Animation Folder Creator")
    print("==================================")
    
    choice = input("\n1. Organize existing frame sequences into folders\n2. Create sample animations\n3. Both\n\nChoice (1-3): ")
    
    if choice in ['1', '3']:
        source = input("Enter source directory path (containing frame sequence files): ").strip()
        if not source:
            source = "simple_videos"  # Default from previous scripts
        
        output = input("Enter output directory path (where animation folders will be created): ").strip()
        if not output:
            output = "animation_folders"
        
        create_animation_folders(source, output)
    
    if choice in ['2', '3']:
        create_sample_animations()
    
    print("\nDone! Copy the animation folders to your SD card's /videos directory.")
    print("Then use commands like: play_video color_cycle loop")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nOperation cancelled.")
    except Exception as e:
        print(f"Error: {e}")
