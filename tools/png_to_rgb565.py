#!/usr/bin/env python3
"""Convert PNG image to RGB565 C array for TFT_eSPI."""

from PIL import Image
import sys
import os

def rgb_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_png_to_header(input_path, output_path, max_width=None, max_height=None, var_name=None):
    """Convert PNG to C header file with RGB565 array."""

    # Open and convert image
    img = Image.open(input_path).convert('RGB')

    # Scale if needed
    orig_width, orig_height = img.size
    if max_width and max_height:
        # Calculate scale factor to fit within bounds
        scale_w = max_width / orig_width
        scale_h = max_height / orig_height
        scale = min(scale_w, scale_h)

        if scale < 1.0:
            new_width = int(orig_width * scale)
            new_height = int(orig_height * scale)
            img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
            print(f"Scaled from {orig_width}x{orig_height} to {new_width}x{new_height}")

    width, height = img.size

    # Generate variable name from filename if not provided
    if var_name is None:
        base_name = os.path.splitext(os.path.basename(input_path))[0]
        var_name = base_name.replace('-', '_').replace(' ', '_')

    # Convert pixels to RGB565
    pixels = []
    for y in range(height):
        for x in range(width):
            r, g, b = img.getpixel((x, y))
            rgb565 = rgb_to_rgb565(r, g, b)
            pixels.append(rgb565)

    # Generate header file
    with open(output_path, 'w') as f:
        f.write(f"#pragma once\n\n")
        f.write(f"// Generated from {os.path.basename(input_path)}\n")
        f.write(f"// Original size: {orig_width}x{orig_height}\n")
        f.write(f"// Output size: {width}x{height}\n\n")
        f.write(f"#include <Arduino.h>\n\n")
        f.write(f"#define {var_name.upper()}_WIDTH {width}\n")
        f.write(f"#define {var_name.upper()}_HEIGHT {height}\n\n")
        f.write(f"const uint16_t {var_name}[{width * height}] PROGMEM = {{\n")

        # Write pixel data in rows for readability
        for y in range(height):
            f.write("    ")
            for x in range(width):
                idx = y * width + x
                f.write(f"0x{pixels[idx]:04X}")
                if idx < len(pixels) - 1:
                    f.write(", ")
            f.write("\n")

        f.write("};\n")

    print(f"Generated {output_path}")
    print(f"Array size: {width * height * 2} bytes")

if __name__ == "__main__":
    # Default paths for this project
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)

    input_file = os.path.join(project_dir, "images", "spartan_1.png")
    output_file = os.path.join(project_dir, "include", "spartan_image.h")

    # Scale to fit in body area (110x130 max, leaving some margin)
    convert_png_to_header(input_file, output_file, max_width=100, max_height=125, var_name="spartan_image")
