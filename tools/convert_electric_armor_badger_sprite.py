#!/usr/bin/env python3
"""Regenerate Electric-Armor Badger front sprite C/H from assets_src PNG."""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

MAGENTA = (255, 0, 255)
CANVAS_SIZE = 64


def rgb15(red: int, green: int, blue: int) -> int:
    return ((red >> 3) & 31) | (((green >> 3) & 31) << 5) | (((blue >> 3) & 31) << 10)


def is_visible_pixel(red: int, green: int, blue: int, alpha: int) -> bool:
    if alpha == 0:
        return False
    return (red, green, blue) != MAGENTA


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


def preprocess_sprite(source_path: Path, output_path: Path) -> tuple[int, int, int]:
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

    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path)
    return dest_y, dest_y + crop_h - 1, crop_h


def write_header(path: Path, visual_top_y: int, visual_bottom_y: int, visual_height: int) -> None:
    path.write_text(
        f"""#ifndef ELECTRIC_ARMOR_BADGER_SPRITE_H
#define ELECTRIC_ARMOR_BADGER_SPRITE_H

#include "core/gba_types.h"

#define ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH {CANVAS_SIZE}
#define ELECTRIC_ARMOR_BADGER_SPRITE_HEIGHT {CANVAS_SIZE}
#define ELECTRIC_ARMOR_BADGER_SPRITE_PIXEL_COUNT \\
    (ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH * ELECTRIC_ARMOR_BADGER_SPRITE_HEIGHT)
#define ELECTRIC_ARMOR_BADGER_SPRITE_TRANSPARENT_COLOR RGB15(31, 0, 31)
#define ELECTRIC_ARMOR_BADGER_SPRITE_KEY_COLOR ELECTRIC_ARMOR_BADGER_SPRITE_TRANSPARENT_COLOR

#define ELECTRIC_ARMOR_BADGER_VISUAL_TOP_Y {visual_top_y}
#define ELECTRIC_ARMOR_BADGER_VISUAL_BOTTOM_Y {visual_bottom_y}
#define ELECTRIC_ARMOR_BADGER_VISUAL_HEIGHT {visual_height}

extern const u16 electric_armor_badger_spriteBitmap[ELECTRIC_ARMOR_BADGER_SPRITE_PIXEL_COUNT];

#endif
""",
        encoding="utf-8",
    )


def write_source(path: Path, keyed_path: Path) -> None:
    image = Image.open(keyed_path).convert("RGB")
    pixels = image.load()
    values: list[int] = []
    for y in range(CANVAS_SIZE):
        for x in range(CANVAS_SIZE):
            red, green, blue = pixels[x, y]
            values.append(rgb15(red, green, blue))

    lines: list[str] = []
    for index in range(0, len(values), 8):
        chunk = ", ".join(f"0x{value:04X}" for value in values[index : index + 8])
        lines.append(f"\t{chunk},")

    path.write_text(
        """#include "assets/electric_armor_badger_sprite.h"

//{{BLOCK(electric_armor_badger_sprite)

//======================================================================
//
//\telectric_armor_badger_sprite, 64x64@16,
//\tTransparent color : FF,00,FF
//\t+ bitmap not compressed
//\tTotal size: 8192 = 8192
//
//\tGenerated from assets_src/sprites/Electric-Armor Badger.png
//
//======================================================================

const u16 electric_armor_badger_spriteBitmap[ELECTRIC_ARMOR_BADGER_SPRITE_PIXEL_COUNT] __attribute__((aligned(4)))=
{
"""
        + "\n".join(lines)
        + "\n};\n\n//}}BLOCK(electric_armor_badger_sprite)\n",
        encoding="utf-8",
    )


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    source_path = root / "assets_src/sprites/Electric-Armor Badger.png"
    keyed_path = root / "build/electric_armor_badger_sprite_keyed.png"
    header_path = root / "include/assets/electric_armor_badger_sprite.h"
    source_out_path = root / "src/assets/electric_armor_badger_sprite.c"

    if not source_path.is_file():
        print(f"Missing source PNG: {source_path}", file=sys.stderr)
        return 1

    visual_top_y, visual_bottom_y, visual_height = preprocess_sprite(source_path, keyed_path)
    write_header(header_path, visual_top_y, visual_bottom_y, visual_height)
    write_source(source_out_path, keyed_path)
    print(
        "electric_armor_badger visual_top_y=%d visual_bottom_y=%d visual_height=%d"
        % (visual_top_y, visual_bottom_y, visual_height)
    )
    print(f"Generated {header_path} and {source_out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
