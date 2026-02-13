#!/usr/bin/env python3
"""
Convert all non-PNG images in ASSETS/IMAGES to PNG format.
Supports common formats (JPEG, BMP, GIF, etc.) and RAW formats (ARW, CR2, NEF, etc.)
"""

import os
import shutil
from pathlib import Path
from PIL import Image

# Try to import rawpy for RAW format support
try:
    import rawpy
    HAS_RAWPY = True
except ImportError:
    HAS_RAWPY = False
    print("Warning: rawpy not installed. RAW formats (.ARW, .CR2, .NEF, etc.) won't be converted.")
    print("Install with: pip install rawpy")

# Common image extensions
COMMON_FORMATS = {'.jpg', '.jpeg', '.bmp', '.gif', '.tiff', '.tif', '.webp'}
RAW_FORMATS = {'.arw', '.cr2', '.nef', '.dng', '.orf', '.rw2', '.pef', '.srw'}

def convert_raw_to_png(input_path, output_path):
    """Convert RAW image format to PNG using rawpy."""
    if not HAS_RAWPY:
        print(f"Skipping {input_path.name}: rawpy not installed")
        return False

    try:
        with rawpy.imread(str(input_path)) as raw:
            # Process RAW image to RGB
            rgb = raw.postprocess()
            # Convert to PIL Image and save as PNG
            img = Image.fromarray(rgb)
            img.save(output_path, 'PNG')
            print(f"Converted {input_path.name} -> {output_path.name}")
            return True
    except Exception as e:
        print(f"Failed to convert {input_path.name}: {e}")
        return False

def convert_common_to_png(input_path, output_path):
    """Convert common image formats to PNG using PIL."""
    try:
        with Image.open(input_path) as img:
            # Convert to RGB if necessary (handles RGBA, P, etc.)
            if img.mode in ('RGBA', 'LA'):
                # Keep alpha channel
                img.save(output_path, 'PNG')
            elif img.mode != 'RGB':
                # Convert to RGB for other modes
                img = img.convert('RGB')
                img.save(output_path, 'PNG')
            else:
                img.save(output_path, 'PNG')
            print(f"Converted {input_path.name} -> {output_path.name}")
            return True
    except Exception as e:
        print(f"Failed to convert {input_path.name}: {e}")
        return False

def scan_and_convert(directory):
    """Scan directory for non-PNG images and convert them to PNG."""
    image_dir = Path(directory)

    if not image_dir.exists():
        print(f"Error: Directory {directory} does not exist")
        return

    # Setup output directory (PNG/ORIGINAL_SIZE)
    output_dir = image_dir.parent / "PNG" / "ORIGINAL_SIZE"
    output_dir.mkdir(parents=True, exist_ok=True)
    print(f"Output directory: {output_dir}\n")

    # Find all image files
    all_files = list(image_dir.iterdir())
    image_files = [f for f in all_files if f.is_file()]

    converted = 0
    moved = 0
    skipped = 0
    failed = 0

    print(f"Scanning {image_dir}...")
    print(f"Found {len(image_files)} files\n")

    # First pass: move any existing PNGs to output directory
    for file_path in image_files[:]:
        if file_path.suffix.lower() == '.png':
            dest_path = output_dir / file_path.name
            if dest_path.exists():
                print(f"Skipping move of {file_path.name} (already exists in output)")
                skipped += 1
            else:
                try:
                    shutil.move(str(file_path), str(dest_path))
                    print(f"Moved existing PNG: {file_path.name} -> PNG/ORIGINAL_SIZE/")
                    moved += 1
                except Exception as e:
                    print(f"Failed to move {file_path.name}: {e}")
                    failed += 1
            image_files.remove(file_path)

    # Second pass: convert non-PNG images
    for file_path in image_files:
        ext = file_path.suffix.lower()

        # Create output filename in the output directory
        output_path = output_dir / file_path.with_suffix('.png').name

        # Check if output already exists
        if output_path.exists():
            print(f"Skipping {file_path.name} (PNG already exists in output)")
            skipped += 1
            continue

        # Create temporary path for conversion (in same dir as source)
        temp_output = file_path.with_suffix('.png')

        # Convert based on format
        success = False
        if ext in RAW_FORMATS:
            success = convert_raw_to_png(file_path, temp_output)
        elif ext in COMMON_FORMATS:
            success = convert_common_to_png(file_path, temp_output)
        else:
            print(f"Unknown format {file_path.name} - attempting conversion anyway...")
            success = convert_common_to_png(file_path, temp_output)

        if success:
            # Move the converted PNG to output directory
            try:
                shutil.move(str(temp_output), str(output_path))
                print(f"Moved to PNG/ORIGINAL_SIZE/{output_path.name}")
                converted += 1
            except Exception as e:
                print(f"Failed to move converted file: {e}")
                failed += 1
        else:
            failed += 1

    print(f"\n{'='*50}")
    print(f"Summary:")
    print(f"  Converted: {converted}")
    print(f"  Moved:     {moved}")
    print(f"  Skipped:   {skipped}")
    print(f"  Failed:    {failed}")
    print(f"{'='*50}\n")

if __name__ == "__main__":
    # Directory to scan
    assets_dir = Path(__file__).parent.parent / "ASSETS" / "IMAGES" / "RAW"
    scan_and_convert(assets_dir)
