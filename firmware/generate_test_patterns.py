#!/usr/bin/env python3
"""
Test Pattern Generator for Tricorder Video Testing
Generates simple test patterns as JPEG files for video testing
"""

from PIL import Image, ImageDraw, ImageFont
import os
import math

def create_test_pattern(width=320, height=240):
    """Create a simple test pattern image"""
    
    # Create image
    img = Image.new('RGB', (width, height), 'black')
    draw = ImageDraw.Draw(img)
    
    # Draw colorful gradient background
    for y in range(height):
        for x in range(width):
            r = int(255 * (x / width))
            g = int(255 * (y / height))
            b = int(255 * ((x + y) / (width + height)))
            draw.point((x, y), (r, g, b))
    
    # Draw test pattern elements
    # Corner markers
    corner_size = 20
    draw.rectangle([0, 0, corner_size, corner_size], fill='white')
    draw.rectangle([width-corner_size, 0, width, corner_size], fill='white')
    draw.rectangle([0, height-corner_size, corner_size, height], fill='white')
    draw.rectangle([width-corner_size, height-corner_size, width, height], fill='white')
    
    # Center crosshair
    center_x, center_y = width // 2, height // 2
    draw.line([center_x-20, center_y, center_x+20, center_y], fill='white', width=2)
    draw.line([center_x, center_y-20, center_x, center_y+20], fill='white', width=2)
    
    # Draw text
    try:
        # Try to use a font, fall back to default if not available
        font = ImageFont.truetype("arial.ttf", 16)
    except:
        font = ImageFont.load_default()
    
    draw.text((10, height-40), "TRICORDER TEST", fill='white', font=font)
    draw.text((10, height-25), f"{width}x{height}", fill='white', font=font)
    
    return img

def create_animated_test_pattern(width=320, height=240, frames=30):
    """Create an animated test pattern sequence"""
    
    patterns = []
    
    for frame in range(frames):
        # Create base image
        img = Image.new('RGB', (width, height), 'black')
        draw = ImageDraw.Draw(img)
        
        # Animated color wheel
        angle = (frame / frames) * 2 * math.pi
        
        for y in range(height):
            for x in range(width):
                # Distance from center
                dx = x - width/2
                dy = y - height/2
                distance = math.sqrt(dx*dx + dy*dy)
                
                # Angle from center
                pixel_angle = math.atan2(dy, dx)
                
                # Animated colors based on angle and time
                r = int(127 + 127 * math.sin(pixel_angle + angle))
                g = int(127 + 127 * math.sin(pixel_angle + angle + 2*math.pi/3))
                b = int(127 + 127 * math.sin(pixel_angle + angle + 4*math.pi/3))
                
                # Add radial pattern
                radial = int(127 + 127 * math.sin(distance * 0.1 + angle * 3))
                r = (r + radial) // 2
                g = (g + radial) // 2
                b = (b + radial) // 2
                
                draw.point((x, y), (r, g, b))
        
        # Add frame counter
        try:
            font = ImageFont.truetype("arial.ttf", 20)
        except:
            font = ImageFont.load_default()
        
        draw.text((10, 10), f"Frame {frame+1:02d}/{frames}", fill='white', font=font)
        
        # Add moving elements
        circle_x = int(width/2 + 80 * math.cos(angle))
        circle_y = int(height/2 + 60 * math.sin(angle))
        draw.ellipse([circle_x-10, circle_y-10, circle_x+10, circle_y+10], fill='white')
        
        patterns.append(img)
    
    return patterns

def save_test_patterns():
    """Save various test patterns"""
    
    # Create output directory
    os.makedirs('test_videos', exist_ok=True)
    
    print("Generating test patterns...")
    
    # Static test pattern
    print("1. Creating static test pattern...")
    static_pattern = create_test_pattern()
    static_pattern.save('test_videos/static_test.jpg', 'JPEG', quality=85)
    print("   Saved: test_videos/static_test.jpg")
    
    # Animated test pattern
    print("2. Creating animated test pattern (30 frames)...")
    animated_frames = create_animated_test_pattern(frames=30)
    
    # Save animated sequence as individual frames
    for i, frame in enumerate(animated_frames):
        filename = f'test_videos/animated_test_frame_{i+1:03d}.jpg'
        frame.save(filename, 'JPEG', quality=85)
    
    print(f"   Saved: test_videos/animated_test_frame_001.jpg to animated_test_frame_030.jpg")
    
    # Create a simple color cycle
    print("3. Creating color cycle pattern...")
    colors = [
        ('red', (255, 0, 0)),
        ('green', (0, 255, 0)),
        ('blue', (0, 0, 255)),
        ('yellow', (255, 255, 0)),
        ('cyan', (0, 255, 255)),
        ('magenta', (255, 0, 255)),
        ('white', (255, 255, 255))
    ]
    
    for i, (name, color) in enumerate(colors):
        img = Image.new('RGB', (320, 240), color)
        draw = ImageDraw.Draw(img)
        
        try:
            font = ImageFont.truetype("arial.ttf", 24)
        except:
            font = ImageFont.load_default()
        
        # Use contrasting text color
        text_color = 'black' if sum(color) > 384 else 'white'
        draw.text((120, 110), name.upper(), fill=text_color, font=font)
        
        img.save(f'test_videos/color_{name}.jpg', 'JPEG', quality=85)
    
    print("   Saved: test_videos/color_*.jpg (7 colors)")
    
    # Create startup animation
    print("4. Creating startup animation...")
    startup_frames = []
    
    for frame in range(20):
        img = Image.new('RGB', (320, 240), 'black')
        draw = ImageDraw.Draw(img)
        
        # Animated "TRICORDER" text
        alpha = frame / 19.0
        brightness = int(255 * alpha)
        
        try:
            font = ImageFont.truetype("arial.ttf", 32)
        except:
            font = ImageFont.load_default()
        
        # Fade in effect
        color = (brightness, brightness, brightness)
        draw.text((80, 100), "TRICORDER", fill=color, font=font)
        
        # Add scanning line
        if frame > 10:
            scan_y = int((frame - 10) * 24)
            if scan_y < 240:
                draw.line([0, scan_y, 320, scan_y], fill='cyan', width=2)
        
        img.save(f'test_videos/startup_frame_{frame+1:03d}.jpg', 'JPEG', quality=85)
    
    print("   Saved: test_videos/startup_frame_001.jpg to startup_frame_020.jpg")
    
    print("\nTest patterns generated successfully!")
    print("\nTo use these on your tricorder:")
    print("1. Copy the desired .jpg files to your SD card's /videos/ directory")
    print("2. Use UDP commands to play them:")
    print('   {"action": "play_video", "commandId": "test", "parameters": {"filename": "static_test.jpg", "loop": false}}')
    print('   {"action": "play_video", "commandId": "test", "parameters": {"filename": "color_red.jpg", "loop": true}}')

if __name__ == "__main__":
    try:
        save_test_patterns()
    except ImportError:
        print("Error: PIL (Pillow) library is required.")
        print("Install it with: pip install Pillow")
    except Exception as e:
        print(f"Error: {e}")
