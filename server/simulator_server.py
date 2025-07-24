#!/usr/bin/env python3
"""
ESP32 Simulator Server
Serves static frame images for the ESP32 display simulator
"""

import os
from flask import Flask, send_file, abort
from PIL import Image, ImageDraw, ImageFont
import io
import colorsys

app = Flask(__name__)

# Color mappings
COLORS = {
    'red': (255, 0, 0),
    'green': (0, 255, 0),
    'blue': (0, 0, 255),
    'cyan': (0, 255, 255),
    'magenta': (255, 0, 255),
    'yellow': (255, 255, 0),
    'white': (255, 255, 255),
    'black': (0, 0, 0),
}

def create_color_frame(color_name, width=320, height=240):
    """Create a solid color frame"""
    color = COLORS.get(color_name, COLORS['white'])
    img = Image.new('RGB', (width, height), color)
    return img

def create_startup_frame(width=320, height=240):
    """Create a startup frame with text"""
    img = Image.new('RGB', (width, height), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    try:
        # Try to use a larger font if available
        font = ImageFont.load_default()
    except:
        font = None
    
    # Draw tricorder startup text
    text = "TRICORDER\nSTARTUP"
    
    # Calculate text position
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (width - text_width) // 2
    y = (height - text_height) // 2
    
    draw.text((x, y), text, fill=(0, 255, 0), font=font, align='center')
    
    return img

def create_static_test_frame(width=320, height=240):
    """Create a test pattern frame"""
    img = Image.new('RGB', (width, height), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Draw a grid pattern
    grid_size = 20
    for x in range(0, width, grid_size):
        for y in range(0, height, grid_size):
            # Checkerboard pattern
            if (x // grid_size + y // grid_size) % 2 == 0:
                color = (128, 128, 128)
            else:
                color = (64, 64, 64)
            draw.rectangle([x, y, x + grid_size, y + grid_size], fill=color)
    
    # Add text
    try:
        font = ImageFont.load_default()
    except:
        font = None
    
    text = "TEST PATTERN"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    x = (width - text_width) // 2
    y = height // 2
    
    draw.text((x, y), text, fill=(255, 255, 255), font=font)
    
    return img

def create_animated_test_frame(frame_num, total_frames=30, width=320, height=240):
    """Create an animated test frame"""
    img = Image.new('RGB', (width, height), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Create a rotating color wheel
    center_x, center_y = width // 2, height // 2
    radius = min(width, height) // 3
    
    # Calculate rotation angle
    angle_offset = (frame_num / total_frames) * 360
    
    # Draw colored sectors
    num_sectors = 8
    for i in range(num_sectors):
        angle = (i * 360 / num_sectors + angle_offset) % 360
        hue = angle / 360
        rgb = colorsys.hsv_to_rgb(hue, 1.0, 1.0)
        color = tuple(int(c * 255) for c in rgb)
        
        # Draw sector
        start_angle = i * 360 / num_sectors
        end_angle = (i + 1) * 360 / num_sectors
        
        draw.pieslice([center_x - radius, center_y - radius, 
                      center_x + radius, center_y + radius], 
                     start_angle, end_angle, fill=color)
    
    # Add frame number
    try:
        font = ImageFont.load_default()
    except:
        font = None
    
    text = f"Frame {frame_num:03d}"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    x = (width - text_width) // 2
    y = height - 30
    
    draw.text((x, y), text, fill=(255, 255, 255), font=font)
    
    return img

@app.route('/api/simulator/frames/<filename>')
def serve_frame(filename):
    """Serve a frame image for the simulator"""
    try:
        width, height = 320, 240
        
        if filename.startswith('color_'):
            # Extract color name
            color_name = filename.replace('color_', '').replace('.jpg', '')
            img = create_color_frame(color_name, width, height)
            
        elif filename == 'startup.jpg':
            img = create_startup_frame(width, height)
            
        elif filename == 'startup_mid.jpg':
            img = create_startup_frame(width, height)
            # Make it slightly different (dimmer)
            img = Image.eval(img, lambda x: int(x * 0.7))
            
        elif filename == 'static_test.jpg':
            img = create_static_test_frame(width, height)
            
        elif filename.startswith('animated_test_frame_'):
            # Extract frame number
            frame_str = filename.replace('animated_test_frame_', '').replace('.jpg', '')
            frame_num = int(frame_str)
            img = create_animated_test_frame(frame_num, 30, width, height)
            
        elif filename in ['animated_test.jpg', 'animated_mid.jpg']:
            # Default animated frame
            img = create_animated_test_frame(1, 30, width, height)
            
        else:
            # Default frame
            img = Image.new('RGB', (width, height), (32, 32, 32))
            draw = ImageDraw.Draw(img)
            
            try:
                font = ImageFont.load_default()
            except:
                font = None
            
            text = f"Missing:\n{filename}"
            bbox = draw.textbbox((0, 0), text, font=font)
            text_width = bbox[2] - bbox[0]
            text_height = bbox[3] - bbox[1]
            x = (width - text_width) // 2
            y = (height - text_height) // 2
            
            draw.text((x, y), text, fill=(255, 128, 128), font=font, align='center')
        
        # Convert to JPEG
        img_io = io.BytesIO()
        img.save(img_io, 'JPEG', quality=85)
        img_io.seek(0)
        
        return send_file(img_io, mimetype='image/jpeg')
        
    except Exception as e:
        print(f"Error serving frame {filename}: {e}")
        abort(404)

@app.route('/api/simulator/status')
def simulator_status():
    """Get simulator status"""
    return {
        'status': 'running',
        'available_frames': list(COLORS.keys()) + [
            'startup', 'startup_mid', 'static_test', 'animated_test', 'animated_mid'
        ],
        'resolution': {'width': 320, 'height': 240},
        'max_fps': 30
    }

if __name__ == '__main__':
    print("Starting ESP32 Simulator Server...")
    print("Available at: http://localhost:5001")
    app.run(host='0.0.0.0', port=5001, debug=True)
