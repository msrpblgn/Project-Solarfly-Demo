#!/usr/bin/env python3
"""Convert approved 64x64 dialogue portrait candidates to 4bpp OBJ C arrays.

Uses the same palette reduction / dimming as tools/dialogue_mockup/palettes.py.
Does not modify source PNGs.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT))

from tools.dialogue_mockup.palettes import (  # noqa: E402
    create_portrait_palette_set,
    dim_opaque_rgb,
    rgb888_to_bgr555,
)

SOURCE_HASHES = {
    "aksil_front_sprite.png": "2df139faf57bc5de6183ef46dae789cd02aa3b7de95bf445266cd31ccc83970d",
    "protagonist_front_sprite.png": "5edfe6308c740594d0e8a33583b87cf7199eb73a08427c812a8f565d29de0863",
    "aksil_front_4bpp_candidate.png": "b5e3e9f330c796d0470a37f7eb7ef06d63275ec7b0fcce85d564d3b0c1c2a845",
    "protagonist_front_4bpp_candidate.png": "71d99b9ee4a950cc582e353cb2bc294a16f588ffb6fe3b576330bc3d63f7a1bd",
}


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pack_tile_row(indices: list[int]) -> int:
    value = 0
    for i, idx in enumerate(indices):
        value |= (idx & 15) << (i * 4)
    return value


def image_to_indices(image: Image.Image, palette_rgb: list[tuple[int, int, int]]) -> list[int]:
    """Map RGBA image to palette indices; transparent -> 0; opaque black outline -> matching black entry."""
    src = image.convert("RGBA")
    px = src.load()
    # palette_rgb is opaque colors only (15 max); index 0 is transparent in OBJ.
    full = [(0, 0, 0)] + list(palette_rgb)  # placeholder; real mapping uses entries from set
    return []  # unused


def indexed_from_palette_set(active_rgba: Image.Image, opaque_palette: list[tuple[int, int, int]]) -> list[int]:
    """Build 64*64 indices: 0 transparent, 1..N from opaque_palette order."""
    src = active_rgba.convert("RGBA")
    px = src.load()
    # Rebuild: create_portrait_palette_set's active image already quantized.
    # Map each opaque pixel to nearest in opaque_palette, +1 for index.
    indices: list[int] = []
    for y in range(64):
        for x in range(64):
            r, g, b, a = px[x, y]
            if a == 0:
                indices.append(0)
                continue
            best = 0
            best_d = 1 << 30
            for i, (pr, pg, pb) in enumerate(opaque_palette):
                d = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2
                if d < best_d:
                    best_d = d
                    best = i
            indices.append(best + 1)
    return indices


def tiles_from_indices(indices: list[int]) -> list[int]:
    """64x64 square OBJ, 1D mapping: 8x8 tile grid, row-major."""
    tiles: list[int] = []
    for tile_row in range(8):
        for tile_col in range(8):
            for y in range(8):
                py = tile_row * 8 + y
                row = []
                for x in range(8):
                    px = tile_col * 8 + x
                    row.append(indices[py * 64 + px])
                tiles.append(pack_tile_row(row))
    if len(tiles) != 512:  # 64 tiles * 8 words
        raise SystemExit(f"expected 512 tile words, got {len(tiles)}")
    return tiles


def palette_to_rgb15(entries_rgb: list[tuple[int, int, int]]) -> list[int]:
    """16 entries: index 0 transparent (0), then up to 15 colors."""
    out = [0]
    for rgb in entries_rgb[:15]:
        out.append(rgb888_to_bgr555(*rgb))
    while len(out) < 16:
        out.append(0)
    return out


def convert_actor(candidate: Path, name: str) -> dict:
    im = Image.open(candidate)
    if im.size != (64, 64):
        raise SystemExit(f"{candidate}: expected 64x64, got {im.size}")
    pset = create_portrait_palette_set(im, name)
    opaque = [e.rgb888 for e in pset.entries if e.index > 0]
    indices = indexed_from_palette_set(pset.active, opaque)
    tiles = tiles_from_indices(indices)
    active_pal = palette_to_rgb15(opaque)
    # Index-stable dimming: same tile indices, dimmed RGB15 values only.
    inactive_rgb = [dim_opaque_rgb(rgb) for rgb in opaque]
    inactive_pal = palette_to_rgb15(inactive_rgb)
    for i, rgb in enumerate(opaque, start=1):
        if rgb == (0, 0, 0):
            inactive_pal[i] = 0
    return {
        "name": name,
        "tiles": tiles,
        "active_pal": active_pal,
        "inactive_pal": inactive_pal,
        "opaque_count": len(opaque),
        "source": candidate.as_posix(),
        "source_hash": sha256_file(candidate),
    }


def emit_pair_c(aksil: dict, protag: dict) -> str:
    def pal_block(arr: list[int]) -> str:
        return ",\n".join(f"    0x{v:04X}u" for v in arr)

    def tile_block(tiles: list[int]) -> str:
        lines = []
        for i in range(0, len(tiles), 4):
            chunk = ", ".join(f"0x{v:08X}u" for v in tiles[i : i + 4])
            lines.append(f"    {chunk},")
        return "\n".join(lines)

    return f"""/* AUTOGENERATED, DO NOT EDIT
 * Sources:
 *   {aksil['source']} ({aksil['source_hash']})
 *   {protag['source']} ({protag['source_hash']})
 * Tool: tools/convert_dialogue_portraits.py
 * Format: 4bpp OBJ tiles, 64x64, 1D mapping (64 tiles = 2048 bytes each)
 */
#include "assets/dialogue_portraits.h"

const u16 gDialoguePortraitAksilActivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT] = {{
{pal_block(aksil['active_pal'])}
}};

const u16 gDialoguePortraitAksilInactivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT] = {{
{pal_block(aksil['inactive_pal'])}
}};

const u16 gDialoguePortraitProtagonistActivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT] = {{
{pal_block(protag['active_pal'])}
}};

const u16 gDialoguePortraitProtagonistInactivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT] = {{
{pal_block(protag['inactive_pal'])}
}};

const u32 gDialoguePortraitAksilTiles[DIALOGUE_PORTRAIT_TILE_WORDS] = {{
{tile_block(aksil['tiles'])}
}};

const u32 gDialoguePortraitProtagonistTiles[DIALOGUE_PORTRAIT_TILE_WORDS] = {{
{tile_block(protag['tiles'])}
}};
"""


def emit_h() -> str:
    return """/* AUTOGENERATED, DO NOT EDIT
 * Tool: tools/convert_dialogue_portraits.py
 */
#ifndef DIALOGUE_PORTRAITS_ASSET_H
#define DIALOGUE_PORTRAITS_ASSET_H

#include "core/gba_types.h"

#define DIALOGUE_PORTRAIT_SOURCE_W 64
#define DIALOGUE_PORTRAIT_SOURCE_H 64
#define DIALOGUE_PORTRAIT_PRESENTATION_SCALE 2
#define DIALOGUE_PORTRAIT_TILE_COUNT 64
#define DIALOGUE_PORTRAIT_TILE_BYTES (DIALOGUE_PORTRAIT_TILE_COUNT * 32)
#define DIALOGUE_PORTRAIT_TILE_WORDS (DIALOGUE_PORTRAIT_TILE_BYTES / 4)
#define DIALOGUE_PORTRAIT_PALETTE_COUNT 16

/* OBJ VRAM tile bases (1D). Overworld player uses tiles 0..7. */
#define DIALOGUE_OBJ_TILE_BASE_LEFT 64
#define DIALOGUE_OBJ_TILE_BASE_RIGHT 128

extern const u16 gDialoguePortraitAksilActivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT];
extern const u16 gDialoguePortraitAksilInactivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT];
extern const u16 gDialoguePortraitProtagonistActivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT];
extern const u16 gDialoguePortraitProtagonistInactivePalette[DIALOGUE_PORTRAIT_PALETTE_COUNT];
extern const u32 gDialoguePortraitAksilTiles[DIALOGUE_PORTRAIT_TILE_WORDS];
extern const u32 gDialoguePortraitProtagonistTiles[DIALOGUE_PORTRAIT_TILE_WORDS];

_Static_assert(DIALOGUE_PORTRAIT_TILE_BYTES == 2048, "64x64 4bpp portrait is 2048 bytes");
_Static_assert(
    (DIALOGUE_OBJ_TILE_BASE_RIGHT + DIALOGUE_PORTRAIT_TILE_COUNT) <= 1024,
    "Dialogue portraits must fit in OBJ VRAM");

#endif
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument(
        "--output-c",
        type=Path,
        default=REPO_ROOT / "src" / "assets" / "dialogue_portraits.c",
    )
    parser.add_argument(
        "--output-h",
        type=Path,
        default=REPO_ROOT / "include" / "assets" / "dialogue_portraits.h",
    )
    args = parser.parse_args()

    # Preserve master hashes.
    for rel, expect in SOURCE_HASHES.items():
        if "4bpp" in rel:
            path = REPO_ROOT / "assets_src" / "ui" / "dialogue" / rel
        else:
            path = REPO_ROOT / "assets_src" / "sprites" / rel
        got = sha256_file(path)
        if got != expect:
            raise SystemExit(f"hash mismatch {rel}: {got}")

    aksil = convert_actor(
        REPO_ROOT / "assets_src" / "ui" / "dialogue" / "aksil_front_4bpp_candidate.png",
        "aksil",
    )
    protag = convert_actor(
        REPO_ROOT / "assets_src" / "ui" / "dialogue" / "protagonist_front_4bpp_candidate.png",
        "protagonist",
    )
    c_text = emit_pair_c(aksil, protag)
    h_text = emit_h()

    if args.check:
        if not args.output_c.is_file() or not args.output_h.is_file():
            raise SystemExit("generated portrait assets missing")
        if args.output_c.read_text(encoding="utf-8") != c_text:
            raise SystemExit("dialogue_portraits.c out of date")
        if args.output_h.read_text(encoding="utf-8") != h_text:
            raise SystemExit("dialogue_portraits.h out of date")
        print("dialogue portraits OK")
        return 0

    args.output_c.parent.mkdir(parents=True, exist_ok=True)
    args.output_h.parent.mkdir(parents=True, exist_ok=True)
    args.output_c.write_text(c_text, encoding="utf-8", newline="\n")
    args.output_h.write_text(h_text, encoding="utf-8", newline="\n")
    print(f"wrote {args.output_c}")
    print(f"wrote {args.output_h}")
    print(f"aksil bytes={len(aksil['tiles'])*4} protag bytes={len(protag['tiles'])*4}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
