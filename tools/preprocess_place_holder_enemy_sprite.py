#!/usr/bin/env python3
"""Bottom-align visible placeholder enemy art in a 64x64 magenta-keyed canvas."""

import sys

from PIL import Image

MAGENTA = (255, 0, 255)
CANVAS_SIZE = 64


def is_visible_pixel(red: int, green: int, blue: int, alpha: int) -> bool:
    if alpha == 0:
        return False
    return not (red == MAGENTA[0] and green == MAGENTA[1] and blue == MAGENTA[2])


def find_visible_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    width, height = image.size
    pixels = image.load()
    min_x = width
    min_y = height
    max_x = -1
    max_y = -1

    for y in range(height):
        for x in range(width):
            red, green, blue, alpha = pixels[x, y]
            if is_visible_pixel(red, green, blue, alpha):
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)

    if max_x < 0:
        return 0, 0, width - 1, height - 1
    return min_x, min_y, max_x, max_y


def main() -> int:
    if len(sys.argv) != 3:
        print(
            "usage: preprocess_place_holder_enemy_sprite.py <input.png> <output.png>",
            file=sys.stderr,
        )
        return 1

    source_path = sys.argv[1]
    output_path = sys.argv[2]

    source = Image.open(source_path).convert("RGBA")
    min_x, min_y, max_x, max_y = find_visible_bounds(source)
    crop = source.crop((min_x, min_y, max_x + 1, max_y + 1))
    crop_w, crop_h = crop.size

    canvas = Image.new("RGBA", (CANVAS_SIZE, CANVAS_SIZE), MAGENTA + (255,))
    dest_x = (CANVAS_SIZE - crop_w) // 2
    dest_y = CANVAS_SIZE - crop_h
    canvas.paste(crop, (dest_x, dest_y), crop)

    pixels = canvas.load()
    for y in range(CANVAS_SIZE):
        for x in range(CANVAS_SIZE):
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0:
                pixels[x, y] = MAGENTA + (255,)
            else:
                pixels[x, y] = (red, green, blue, 255)

    canvas.save(output_path)
    print(
        "place_holder_enemy visual_top_y=%d visual_bottom_y=%d visual_height=%d"
        % (dest_y, dest_y + crop_h - 1, crop_h)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
