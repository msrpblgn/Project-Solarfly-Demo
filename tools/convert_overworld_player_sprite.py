#!/usr/bin/env python3
"""Convert overworld down-idle GIF (eyes open + blink) to 4bpp GBA OBJ C arrays.

Source is a 2-frame GIF. Frame 0 = eyes open, frame 1 = eyes closed.
Each frame is padded to 16x32 (bottom-aligned, horizontally centered).
Palette index 0 is treated as transparent (GIF background / unused fringe).
"""

from __future__ import annotations

import argparse
import hashlib
import os
import tempfile
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[1]
OUT_W = 16
OUT_H = 32
TILE_WORDS = 64  # 8 tiles * 8 rows


def rgb15(r8: int, g8: int, b8: int) -> int:
    return ((r8 * 31) // 255) | (((g8 * 31) // 255) << 5) | (((b8 * 31) // 255) << 10)


def pack_tile_row(indices: list[int]) -> int:
    value = 0
    for i, idx in enumerate(indices):
        value |= (idx & 15) << (i * 4)
    return value


def pad_frame_to_16x32(frame: Image.Image) -> Image.Image:
    """Bottom-align and center a P-mode frame into a 16x32 canvas (index 0 clear)."""
    if frame.mode != "P":
        raise SystemExit(f"expected indexed frame, got {frame.mode}")
    src_w, src_h = frame.size
    if src_w > OUT_W or src_h > OUT_H:
        raise SystemExit(f"frame {frame.size} larger than {OUT_W}x{OUT_H}")

    canvas = Image.new("P", (OUT_W, OUT_H), 0)
    canvas.putpalette(frame.getpalette() or [0] * (256 * 3))
    offset_x = (OUT_W - src_w) // 2
    offset_y = OUT_H - src_h
    canvas.paste(frame, (offset_x, offset_y))
    return canvas


def indices_to_tiles(indices: list[int]) -> list[int]:
    tiles: list[int] = []
    for tile_row in range(4):
        for tile_col in range(2):
            for y in range(8):
                py = tile_row * 8 + y
                row = []
                for x in range(8):
                    px = tile_col * 8 + x
                    row.append(indices[py * OUT_W + px])
                tiles.append(pack_tile_row(row))
    if len(tiles) != TILE_WORDS:
        raise SystemExit(f"expected {TILE_WORDS} tile words, got {len(tiles)}")
    return tiles


def remap_rgba_to_palette(rgba: Image.Image, palette_rgb: list[tuple[int, int, int]]) -> Image.Image:
    """Nearest-color remap onto a 16-entry palette (index 0 kept as-is for fringe/bg)."""
    out = Image.new("P", rgba.size, 0)
    flat = []
    for r, g, b in palette_rgb:
        flat.extend((r, g, b))
    flat.extend([0] * (768 - len(flat)))
    out.putpalette(flat)
    src = rgba.load()
    dest = out.load()
    for y in range(rgba.size[1]):
        for x in range(rgba.size[0]):
            r, g, b, _a = src[x, y]
            best = 0
            best_d = 1 << 30
            for i, (pr, pg, pb) in enumerate(palette_rgb):
                d = (r - pr) * (r - pr) + (g - pg) * (g - pg) + (b - pb) * (b - pb)
                if d < best_d:
                    best_d = d
                    best = i
            dest[x, y] = best
    return out


def convert_gif(gif_path: Path) -> tuple[list[int], list[int], list[int]]:
    im = Image.open(gif_path)
    if getattr(im, "n_frames", 1) < 2:
        raise SystemExit(f"{gif_path}: expected at least 2 frames, got {getattr(im, 'n_frames', 1)}")

    im.seek(0)
    # Prefer the GIF global/local palette from frame 0 when present.
    if im.mode == "P" and im.getpalette():
        pal = im.getpalette()
    else:
        raise SystemExit(f"{gif_path}: frame 0 must provide an indexed palette")

    palette_rgb = [tuple(pal[i * 3 : i * 3 + 3]) for i in range(16)]
    # Index 0 is reserved transparent on GBA regardless of GIF RGB.
    palette = [0]
    for i in range(1, 16):
        r, g, b = palette_rgb[i]
        palette.append(rgb15(r, g, b))

    frames_tiles: list[list[int]] = []
    for frame_index in range(2):
        im.seek(frame_index)
        # Later GIF frames may decode as RGB after disposal; remap onto frame-0 palette.
        indexed = remap_rgba_to_palette(im.convert("RGBA"), palette_rgb)
        padded = pad_frame_to_16x32(indexed)
        indices = list(padded.getdata())
        if any(i < 0 or i > 15 for i in indices):
            raise SystemExit(f"{gif_path} frame {frame_index}: indices must fit 4bpp")
        opaque = {i for i in indices if i != 0}
        if len(opaque) > 15:
            raise SystemExit(f"{gif_path} frame {frame_index}: too many opaque colors")
        if sum(1 for i in indices if i != 0) <= 0:
            raise SystemExit(f"{gif_path} frame {frame_index}: no opaque pixels")
        frames_tiles.append(indices_to_tiles(indices))

    return palette, frames_tiles[0], frames_tiles[1]


def convert_png(png_path: Path) -> tuple[list[int], list[int]]:
    """Legacy single-frame path kept for --check against old single-PNG workflows."""
    im = Image.open(png_path)
    if im.size != (OUT_W, OUT_H):
        raise SystemExit(f"{png_path}: expected {OUT_W}x{OUT_H}, got {im.size}")
    if im.mode != "P":
        raise SystemExit(f"{png_path}: expected indexed PNG, got {im.mode}")
    if im.info.get("transparency") != 0:
        raise SystemExit(f"{png_path}: palette index 0 must be transparent")
    indices = list(im.getdata())
    opaque = {i for i in indices if i != 0}
    if any(i < 0 or i > 15 for i in indices):
        raise SystemExit(f"{png_path}: indices must fit in 4bpp (0..15)")
    if len(opaque) > 15:
        raise SystemExit(f"{png_path}: {len(opaque)} nontransparent colors exceed 15")
    if sum(1 for i in indices if i != 0) <= 0:
        raise SystemExit(f"{png_path}: missing nontransparent pixels")
    pal = im.getpalette() or [0] * (16 * 3)
    palette = [0]
    for i in range(1, 16):
        r, g, b = pal[i * 3 : i * 3 + 3]
        palette.append(rgb15(r, g, b))
    return palette, indices_to_tiles(indices)


def emit_tile_array(name: str, tiles: list[int]) -> str:
    tile_lines = []
    for i in range(0, len(tiles), 4):
        chunk = ", ".join(f"0x{v:08X}u" for v in tiles[i : i + 4])
        tile_lines.append(f"    {chunk},")
    return (
        f"const u32 {name}[OVERWORLD_PLAYER_SPRITE_TILE_WORDS] = {{\n"
        + "\n".join(tile_lines)
        + "\n};\n"
    )


def emit_c(
    palette: list[int],
    open_tiles: list[int],
    blink_tiles: list[int],
    header_rel: str,
    source_name: str,
) -> str:
    pal_lines = ",\n".join(f"    0x{v:04X}u" for v in palette)
    return (
        "/* AUTOGENERATED, DO NOT EDIT\n"
        f" * Source: assets_src/overworld/player/{source_name}\n"
        " * Tool: tools/convert_overworld_player_sprite.py\n"
        " * Frame 0 = eyes open idle; frame 1 = eyes closed blink.\n"
        " */\n"
        f'#include "{header_rel}"\n'
        "\n"
        "const u16 gOverworldPlayerSpritePalette"
        "[OVERWORLD_PLAYER_SPRITE_PALETTE_COUNT] = {\n"
        f"{pal_lines}\n"
        "};\n"
        "\n"
        + emit_tile_array("gOverworldPlayerSpriteTiles", open_tiles)
        + "\n"
        + emit_tile_array("gOverworldPlayerSpriteBlinkTiles", blink_tiles)
    )


def emit_h(source_name: str) -> str:
    return f"""/* AUTOGENERATED, DO NOT EDIT
 * Source: assets_src/overworld/player/{source_name}
 * Tool: tools/convert_overworld_player_sprite.py
 * Frame 0 = eyes open idle; frame 1 = eyes closed blink.
 */
#ifndef OVERWORLD_PLAYER_SPRITE_H
#define OVERWORLD_PLAYER_SPRITE_H

#include "core/gba_types.h"

#define OVERWORLD_PLAYER_SPRITE_WIDTH 16
#define OVERWORLD_PLAYER_SPRITE_HEIGHT 32
#define OVERWORLD_PLAYER_SPRITE_TILE_COUNT 8
#define OVERWORLD_PLAYER_SPRITE_TILE_BYTES (OVERWORLD_PLAYER_SPRITE_TILE_COUNT * 32)
#define OVERWORLD_PLAYER_SPRITE_TILE_WORDS (OVERWORLD_PLAYER_SPRITE_TILE_BYTES / 4)
#define OVERWORLD_PLAYER_SPRITE_OBJ_TILE_INDEX 0
#define OVERWORLD_PLAYER_SPRITE_BLINK_OBJ_TILE_INDEX \\
    (OVERWORLD_PLAYER_SPRITE_OBJ_TILE_INDEX + OVERWORLD_PLAYER_SPRITE_TILE_COUNT)
#define OVERWORLD_PLAYER_SPRITE_PALETTE_BANK 0
#define OVERWORLD_PLAYER_SPRITE_PALETTE_COUNT 16

extern const u16 gOverworldPlayerSpritePalette[OVERWORLD_PLAYER_SPRITE_PALETTE_COUNT];
extern const u32 gOverworldPlayerSpriteTiles[OVERWORLD_PLAYER_SPRITE_TILE_WORDS];
extern const u32 gOverworldPlayerSpriteBlinkTiles[OVERWORLD_PLAYER_SPRITE_TILE_WORDS];

_Static_assert(
    OVERWORLD_PLAYER_SPRITE_TILE_BYTES == 256,
    "Player OBJ tiles must occupy 256 bytes");
_Static_assert(
    OVERWORLD_PLAYER_SPRITE_WIDTH == 16 && OVERWORLD_PLAYER_SPRITE_HEIGHT == 32,
    "Player OBJ must be 16x32");
_Static_assert(
    (OVERWORLD_PLAYER_SPRITE_BLINK_OBJ_TILE_INDEX + OVERWORLD_PLAYER_SPRITE_TILE_COUNT)
        <= 1024,
    "Player open+blink OBJ tiles must fit in OBJ VRAM");
_Static_assert(
    (OVERWORLD_PLAYER_SPRITE_PALETTE_COUNT * 2) == 32,
    "Player OBJ palette must occupy 32 bytes");

#endif
"""


def atomic_write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=str(path.parent))
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        os.replace(tmp, path)
    except Exception:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise


def write_reference_pngs(
    gif_path: Path,
    open_png: Path,
    blink_png: Path,
) -> None:
    im = Image.open(gif_path)
    im.seek(0)
    pal = im.getpalette() or [0] * (256 * 3)
    palette_rgb = [tuple(pal[i * 3 : i * 3 + 3]) for i in range(16)]
    for frame_index, out_path in ((0, open_png), (1, blink_png)):
        im.seek(frame_index)
        indexed = remap_rgba_to_palette(im.convert("RGBA"), palette_rgb)
        padded = pad_frame_to_16x32(indexed)
        padded.info["transparency"] = 0
        padded.save(out_path, format="PNG")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output-c", required=True)
    parser.add_argument("--output-h", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    src = Path(args.input)
    out_c = Path(args.output_c)
    out_h = Path(args.output_h)
    source_name = src.name

    if src.suffix.lower() == ".gif":
        palette, open_tiles, blink_tiles = convert_gif(src)
        # Keep companion PNGs in sync for previews / older tooling.
        write_reference_pngs(
            src,
            src.with_name("protagonist_overworld_down_idle.png"),
            src.with_name("protagonist_overworld_down_idle_blink.png"),
        )
    else:
        palette, open_tiles = convert_png(src)
        blink_tiles = list(open_tiles)

    try:
        header_rel = out_h.resolve().relative_to(REPO_ROOT / "include").as_posix()
    except ValueError:
        header_rel = out_h.name
    c_text = emit_c(palette, open_tiles, blink_tiles, header_rel, source_name)
    h_text = emit_h(source_name)

    if args.check:
        if not out_c.is_file() or not out_h.is_file():
            raise SystemExit("generated player sprite outputs missing")
        if out_c.read_text(encoding="utf-8") != c_text or out_h.read_text(encoding="utf-8") != h_text:
            raise SystemExit("generated player sprite outputs are stale")
        return 0

    atomic_write(out_h, h_text)
    atomic_write(out_c, c_text)
    print("palette_bytes", len(palette) * 2)
    print("open_tile_bytes", len(open_tiles) * 4)
    print("blink_tile_bytes", len(blink_tiles) * 4)
    print("sha_src", hashlib.sha256(src.read_bytes()).hexdigest())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
