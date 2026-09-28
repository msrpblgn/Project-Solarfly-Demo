#!/usr/bin/env python3
"""Regenerate Maren front sprite C/H from assets_src PNG."""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

MAGENTA = (255, 0, 255)
CANVAS_SIZE = 64


def rgb15(red: int, green: int, blue: int) -> int:
    return ((red >> 3) & 31) | (((green >> 3) & 31) << 5) | (((blue >> 3) & 31) << 10)


def preprocess_sprite(source_path: Path, output_path: Path) -> None:
    image = Image.open(source_path).convert("RGBA")
    pixels = image.load()
    width, height = image.size

    if width != CANVAS_SIZE or height != CANVAS_SIZE:
        print(
            f"Expected {CANVAS_SIZE}x{CANVAS_SIZE} source, got {width}x{height}",
            file=sys.stderr,
        )
        raise SystemExit(1)

    for y in range(height):
        for x in range(width):
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0:
                pixels[x, y] = MAGENTA + (255,)
            else:
                pixels[x, y] = (red, green, blue, 255)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    image.save(output_path)


def write_header(path: Path) -> None:
    path.write_text(
        f"""#ifndef MAREN_FRONT_SPRITE_H
#define MAREN_FRONT_SPRITE_H

#include "core/gba_types.h"

#define MAREN_FRONT_SPRITE_WIDTH {CANVAS_SIZE}
#define MAREN_FRONT_SPRITE_HEIGHT {CANVAS_SIZE}
#define MAREN_FRONT_SPRITE_PIXEL_COUNT \\
    (MAREN_FRONT_SPRITE_WIDTH * MAREN_FRONT_SPRITE_HEIGHT)
#define MAREN_FRONT_SPRITE_TRANSPARENT_COLOR RGB15(31, 0, 31)
#define MAREN_FRONT_SPRITE_KEY_COLOR MAREN_FRONT_SPRITE_TRANSPARENT_COLOR

extern const u16 maren_front_spriteBitmap[MAREN_FRONT_SPRITE_PIXEL_COUNT];

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
        """#include "assets/maren_front_sprite.h"

//{{BLOCK(maren_front_sprite)

//======================================================================
//
//\tmaren_front_sprite, 64x64@16,
//\tTransparent color : FF,00,FF
//\t+ bitmap not compressed
//\tTotal size: 8192 = 8192
//
//\tGenerated from assets_src/sprites/maren_front_sprite.png
//
//======================================================================

const u16 maren_front_spriteBitmap[MAREN_FRONT_SPRITE_PIXEL_COUNT] __attribute__((aligned(4)))=
{
"""
        + "\n".join(lines)
        + "\n};\n\n//}}BLOCK(maren_front_sprite)\n",
        encoding="utf-8",
    )


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    source_path = root / "assets_src/sprites/maren_front_sprite.png"
    keyed_path = root / "build/maren_front_sprite_keyed.png"
    header_path = root / "include/assets/maren_front_sprite.h"
    source_out_path = root / "src/assets/maren_front_sprite.c"

    if not source_path.is_file():
        print(f"Missing source PNG: {source_path}", file=sys.stderr)
        return 1

    preprocess_sprite(source_path, keyed_path)
    write_header(header_path)
    write_source(source_out_path, keyed_path)
    print(f"Generated {header_path} and {source_out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
