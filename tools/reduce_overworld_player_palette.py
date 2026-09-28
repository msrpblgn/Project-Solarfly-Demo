#!/usr/bin/env python3
"""Reduce protagonist overworld PNG to <=15 opaque colors (no dither)."""

from __future__ import annotations

import hashlib
from collections import Counter
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets_src" / "overworld" / "player" / "protagonist_overworld_down_idle.png"
OUT = SRC
COMPARE = ROOT / "assets_src" / "overworld" / "player" / "protagonist_overworld_down_idle_palette_compare.png"
EXPECTED_SHA = "e9c46e8775d4ce2a50e02b310a225db1159d9e26e14aab6825b2c52f228e87bb"


def dist2(a, b):
    return sum((x - y) * (x - y) for x, y in zip(a, b))


def main() -> None:
    raw = SRC.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_SHA:
        raise SystemExit(f"SHA mismatch: got {digest}, expected {EXPECTED_SHA}")

    im = Image.open(SRC)
    if im.size != (16, 32):
        raise SystemExit(f"wrong size {im.size}")
    if im.mode != "P":
        raise SystemExit(f"expected indexed PNG, got {im.mode}")
    if im.info.get("transparency") != 0:
        raise SystemExit(f"expected transparency=0, got {im.info.get('transparency')}")

    rgba = im.convert("RGBA")
    pixels = list(rgba.getdata())
    opaque_counts: Counter = Counter()
    for px in pixels:
        if px[3] != 0:
            opaque_counts[px[:3]] += 1

    if len(opaque_counts) != 23:
        raise SystemExit(f"expected 23 opaque colors, got {len(opaque_counts)}")

    # Keep the 15 most frequent opaque colors; merge the rest.
    kept = [c for c, _ in opaque_counts.most_common(15)]
    merge_away = [c for c, _ in opaque_counts.most_common()[15:]]
    mapping = {c: c for c in kept}
    for c in merge_away:
        nearest = min(kept, key=lambda k: dist2(c, k))
        mapping[c] = nearest

    # Rebuild indexed image: index 0 transparent, then unique mapped colors.
    palette_colors = []
    color_to_index = {}
    for c in kept:
        if c not in color_to_index:
            color_to_index[c] = len(palette_colors) + 1  # reserve 0
            palette_colors.append(c)

    out_indices = []
    visible = 0
    became_transparent = 0
    for px in pixels:
        if px[3] == 0:
            out_indices.append(0)
            continue
        mapped = mapping[px[:3]]
        idx = color_to_index[mapped]
        if idx == 0:
            became_transparent += 1
        out_indices.append(idx)
        visible += 1

    if became_transparent:
        raise SystemExit("visible pixel became transparent")
    if visible != 260:
        raise SystemExit(f"visible count {visible} != 260")
    if len(palette_colors) > 15:
        raise SystemExit(f"too many opaque palette entries: {len(palette_colors)}")

    pal = [0, 0, 0]  # index 0
    for r, g, b in palette_colors:
        pal.extend((r, g, b))
    while len(pal) < 16 * 3:
        pal.extend((0, 0, 0))

    out = Image.new("P", (16, 32))
    out.putpalette(pal)
    out.putdata(out_indices)
    out.info["transparency"] = 0
    # Save without dithering; Pillow P-mode save preserves indices.
    out.save(OUT, format="PNG", optimize=False)

    # Nearest-neighbor before/after compare at 8x.
    before = rgba.resize((16 * 8, 32 * 8), Image.NEAREST)
    after_rgba = out.convert("RGBA").resize((16 * 8, 32 * 8), Image.NEAREST)
    compare = Image.new("RGBA", (16 * 8 * 2 + 8, 32 * 8), (32, 32, 32, 255))
    compare.paste(before, (0, 0))
    compare.paste(after_rgba, (16 * 8 + 8, 0))
    compare.save(COMPARE)

    print("KEEP_COUNT", len(palette_colors))
    print("VISIBLE", visible)
    print("TRANSPARENT", sum(1 for i in out_indices if i == 0))
    print("PALETTE")
    print("0 TRANSPARENT")
    for i, (r, g, b) in enumerate(palette_colors, start=1):
        print(f"{i} RGB({r},{g},{b})")
    print("MAPPINGS")
    for old, new in sorted(mapping.items(), key=lambda kv: opaque_counts[kv[0]], reverse=True):
        mark = "" if old == new else f" -> RGB{new}"
        print(f"RGB{old} x{opaque_counts[old]}{mark}")
    print("COMPARE", COMPARE.relative_to(ROOT).as_posix())
    print("OUT_SHA", hashlib.sha256(OUT.read_bytes()).hexdigest())


if __name__ == "__main__":
    main()
