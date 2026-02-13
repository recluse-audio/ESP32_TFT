#!/usr/bin/env python3
"""
Resize PNG images from ORIGINAL_SIZE to specified dimensions.
By default, maintains aspect ratio and centers image on black background.
Use --stretch for artistic scaling without aspect ratio preservation.
"""

import argparse
from pathlib import Path
from PIL import Image

def resize_with_aspect_ratio(img, target_size=(320, 240), bg_color=(0, 0, 0)):
    """
    Resize image to fit within target size while maintaining aspect ratio.
    Centers the image on a background of the target size.
    """
    # Calculate scaling factor to fit within target size
    img_ratio = img.width / img.height
    target_ratio = target_size[0] / target_size[1]

    if img_ratio > target_ratio:
        # Image is wider than target - scale based on width
        new_width = target_size[0]
        new_height = int(new_width / img_ratio)
    else:
        # Image is taller than target - scale based on height
        new_height = target_size[1]
        new_width = int(new_height * img_ratio)

    # Resize image
    resized = img.resize((new_width, new_height), Image.Resampling.LANCZOS)

    # Create new image with target size and background color
    result = Image.new('RGB', target_size, bg_color)

    # Calculate position to center the resized image
    x = (target_size[0] - new_width) // 2
    y = (target_size[1] - new_height) // 2

    # Paste resized image onto background
    result.paste(resized, (x, y))

    return result

def resize_stretch(img, target_size=(320, 240)):
    """
    Resize image to exact target size, stretching/distorting as needed.
    For artistic scaling effects.
    """
    return img.resize(target_size, Image.Resampling.LANCZOS)

def resize_images(source_dir, target_dir, target_size=(320, 240), stretch=False):
    """Resize all PNG images from source to target directory."""
    source_path = Path(source_dir)
    target_path = Path(target_dir)

    if not source_path.exists():
        print(f"Error: Source directory {source_dir} does not exist")
        return

    # Create target directory
    target_path.mkdir(parents=True, exist_ok=True)
    print(f"Source: {source_path}")
    print(f"Target: {target_path}")
    print(f"Target size: {target_size[0]}x{target_size[1]}")
    print(f"Mode: {'Stretch (no aspect ratio)' if stretch else 'Maintain aspect ratio'}\n")

    # Find all PNG files
    png_files = list(source_path.glob("*.png"))

    if not png_files:
        print("No PNG files found in source directory")
        return

    print(f"Found {len(png_files)} PNG files\n")

    resized = 0
    skipped = 0
    failed = 0

    for png_file in png_files:
        # Create output filename with size suffix (e.g., Stop_Victory_320x240.png)
        output_filename = f"{png_file.stem}_{target_size[0]}x{target_size[1]}{png_file.suffix}"
        output_file = target_path / output_filename

        # Skip if already exists
        if output_file.exists():
            print(f"Skipping {png_file.name} (already exists in target)")
            skipped += 1
            continue

        try:
            # Open image
            with Image.open(png_file) as img:
                # Convert to RGB if necessary
                if img.mode != 'RGB':
                    img = img.convert('RGB')

                # Resize based on mode
                if stretch:
                    resized_img = resize_stretch(img, target_size)
                else:
                    resized_img = resize_with_aspect_ratio(img, target_size)

                # Save to target directory
                resized_img.save(output_file, 'PNG')
                print(f"Resized {png_file.name} -> {output_file.name} ({img.width}x{img.height} -> {target_size[0]}x{target_size[1]})")
                resized += 1

        except Exception as e:
            print(f"Failed to resize {png_file.name}: {e}")
            failed += 1

    print(f"\n{'='*50}")
    print(f"Summary:")
    print(f"  Resized: {resized}")
    print(f"  Skipped: {skipped}")
    print(f"  Failed:  {failed}")
    print(f"{'='*50}\n")

if __name__ == "__main__":
    # Paths relative to script location
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # Default paths
    default_source = project_root / "ASSETS" / "IMAGES" / "PNG" / "ORIGINAL_SIZE"

    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description="Resize PNG images to specified dimensions.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Use defaults (320x240, maintain aspect ratio)
  python resize_png.py

  # Custom dimensions
  python resize_png.py --width 480 --height 320

  # Artistic stretch (no aspect ratio)
  python resize_png.py --width 100 --height 500 --stretch

  # Custom source and target
  python resize_png.py --source /path/to/images --target /path/to/output
        """
    )

    parser.add_argument(
        '-w', '--width',
        type=int,
        default=320,
        help='Target width in pixels (default: 320, range: 1-10000)'
    )

    parser.add_argument(
        '-H', '--height',
        type=int,
        default=240,
        help='Target height in pixels (default: 240, range: 1-10000)'
    )

    parser.add_argument(
        '-s', '--source',
        type=str,
        default=str(default_source),
        help=f'Source directory (default: {default_source})'
    )

    parser.add_argument(
        '-t', '--target',
        type=str,
        default=None,
        help='Target directory (default: auto-generated based on dimensions)'
    )

    parser.add_argument(
        '--stretch',
        action='store_true',
        help='Stretch image to exact dimensions (artistic scaling, no aspect ratio)'
    )

    args = parser.parse_args()

    # Validate dimensions
    if not (1 <= args.width <= 10000):
        parser.error(f"Width must be between 1 and 10000 (got {args.width})")

    if not (1 <= args.height <= 10000):
        parser.error(f"Height must be between 1 and 10000 (got {args.height})")

    # Setup directories
    source_dir = Path(args.source)

    if args.target:
        target_dir = Path(args.target)
    else:
        # Auto-generate target directory based on dimensions
        target_dir = project_root / "ASSETS" / "IMAGES" / "PNG" / f"{args.width}x{args.height}"

    # Run resize
    target_size = (args.width, args.height)
    resize_images(source_dir, target_dir, target_size=target_size, stretch=args.stretch)
