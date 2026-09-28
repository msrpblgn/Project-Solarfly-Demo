#!/usr/bin/env python3
"""Fill transparent pixels with magenta for grit chroma-key conversion."""

import sys

from PIL import Image

MAGENTA = (255, 0, 255)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: preprocess_aksil_sprite.py <input.png> <output.png>", file=sys.stderr)
        return 1

    source_path = sys.argv[1]
    output_path = sys.argv[2]

    image = Image.open(source_path).convert("RGBA")
    pixels = image.load()
    width, height = image.size

    for y in range(height):
        for x in range(width):
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0:
                pixels[x, y] = MAGENTA + (255,)
            else:
                pixels[x, y] = (red, green, blue, 255)

    image.save(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
