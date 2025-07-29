#!/usr/bin/env python3
"""
Convert PNG images to JPEG for tricorder compatibility
"""

from PIL import Image
import os

# List of images to convert (without extension)
images_to_convert = [
    "greenscreen",
    "Test Card"
]

def convert_png_to_jpg(input_path, output_path):
    """Convert a PNG image to JPEG"""
    try:
        # Open the PNG image
        with Image.open(input_path) as img:
            # Convert RGBA to RGB if necessary (JPEG doesn't support transparency)
            if img.mode in ('RGBA', 'LA'):
                # Create a white background
                background = Image.new('RGB', img.size, (255, 255, 255))
                if img.mode == 'RGBA':
                    background.paste(img, mask=img.split()[-1])  # Use alpha channel as mask
                else:
                    background.paste(img)
                img = background
            elif img.mode != 'RGB':
                img = img.convert('RGB')
            
            # Save as JPEG with high quality
            img.save(output_path, 'JPEG', quality=95, optimize=True)
            print(f"✅ Converted: {input_path} → {output_path}")
            return True
    except Exception as e:
        print(f"❌ Error converting {input_path}: {e}")
        return False

def main():
    print("🖼️  PNG to JPEG Converter for Tricorder")
    print("=" * 50)
    
    current_dir = os.getcwd()
    print(f"Working directory: {current_dir}")
    
    for image_name in images_to_convert:
        # Try different PNG file locations
        png_paths = [
            f"{image_name}.png",
            f"uploads/{image_name}.png",
            f"firmware/{image_name}.png",
            f"firmware/simple_videos/{image_name}.png"
        ]
        
        png_found = False
        for png_path in png_paths:
            if os.path.exists(png_path):
                jpg_path = f"{image_name}.jpg"
                if convert_png_to_jpg(png_path, jpg_path):
                    png_found = True
                    print(f"   📁 Output: {os.path.abspath(jpg_path)}")
                break
        
        if not png_found:
            print(f"❌ PNG file not found for: {image_name}")
            print(f"   Searched in: {', '.join(png_paths)}")
    
    print("\n📋 Instructions:")
    print("1. Copy the generated .jpg files to your SD card root directory")
    print("2. Test the buttons in the web interface")
    print("3. The tricorder should now display the images!")

if __name__ == "__main__":
    main()
