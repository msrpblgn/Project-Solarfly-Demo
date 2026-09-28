#!/usr/bin/env python3
"""Analyze skills_menu.aseprite layer exports for skill-card visual repair."""
from __future__ import annotations

import os
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/reports/skill_card_visual_repair_logs"
EXP = ROOT / "assets_src/ui/skills/export"
OUT.mkdir(parents=True, exist_ok=True)


def load_layer(name: str) -> Image.Image:
    for base in (OUT, EXP):
        p = base / f"layer_{name}.png"
        if p.exists():
            return Image.open(p).convert("RGBA")
    raise FileNotFoundError(name)


def flatten(layers: dict[str, Image.Image], order: list[str]) -> Image.Image:
    base = Image.new("RGBA", (224, 34), (0, 0, 0, 0))
    for n in order:
        base = Image.alpha_composite(base, layers[n])
    return base


def crop_info(im: Image.Image, x: int, y: int, w: int, h: int, label: str) -> Image.Image:
    c = im.crop((x, y, x + w, y + h))
    c.save(OUT / f"crop_{label}.png")
    cols: dict[tuple, int] = {}
    for px in c.getdata():
        if px[3] == 0:
            continue
        cols[px] = cols.get(px, 0) + 1
    print(f"\n{label} ({w}x{h} @ {x},{y}): {len(cols)} opaque colors, {sum(cols.values())} opaque px")
    for col, n in sorted(cols.items(), key=lambda kv: -kv[1])[:8]:
        print(f"  {col}: {n}")
    return c


def main() -> None:
    names = [
        "Background color",
        "bORDER",
        "Text",
        "Element symbol",
        "Categ",
        "SC",
        "SC MAX",
        "Targets",
    ]
    layers = {n: load_layer(n) for n in names}
    for n, im in layers.items():
        nt = sum(1 for px in im.getdata() if px[3] > 0)
        print(f"{n}: {im.size} nontransparent={nt}")

    full = flatten(layers, names)
    full.save(OUT / "reference_full_rgba.png")
    full.convert("RGB").save(OUT / "reference_full_rgb.png")

    flat_path = EXP / "skills_menu_flat.png"
    if flat_path.exists():
        flat = Image.open(flat_path).convert("RGBA")
        flat.save(OUT / "skills_menu_flat_copy.png")
        print(
            "flat vs composed equal?",
            list(flat.getdata()) == list(full.getdata()),
            flat.size,
        )

    # Support+MAX reference (Categ visible, SC MAX visible; SC alone may be empty)
    support_max = flatten(
        layers,
        [
            "Background color",
            "bORDER",
            "Text",
            "Element symbol",
            "Categ",
            "SC MAX",
            "Targets",
        ],
    )
    support_max.save(OUT / "reference_support_max_rgba.png")
    support_max.convert("RGB").save(OUT / "reference_support_max_rgb.png")

    # Without MAX: hide SC MAX; use SC layer if it has content
    no_max = flatten(
        layers,
        [
            "Background color",
            "bORDER",
            "Text",
            "Element symbol",
            "Categ",
            "SC",
            "Targets",
        ],
    )
    no_max.save(OUT / "reference_support_nomax_rgba.png")

    crop_info(layers["Text"], 20, 12, 104, 12, "name_hammer_fist")
    crop_info(layers["Element symbol"], 4, 13, 12, 12, "element_neutral")
    crop_info(layers["Categ"], 130, 12, 31, 7, "support")
    crop_info(layers["SC"], 130, 20, 15, 6, "sc8_from_sc")
    crop_info(layers["SC MAX"], 130, 20, 15, 6, "sc8_from_scmax")
    crop_info(layers["SC MAX"], 149, 19, 16, 7, "max_only")
    crop_info(layers["Targets"], 172, 8, 48, 18, "targets_region")

    print("\nSC layer opaque", sum(1 for px in layers["SC"].getdata() if px[3] > 0))
    print("Categ opaque", sum(1 for px in layers["Categ"].getdata() if px[3] > 0))

    nm = layers["Text"]
    pale = []
    for y in range(12, 24):
        for x in range(20, 124):
            r, g, b, a = nm.getpixel((x, y))
            if a > 0 and (r + g + b) > 200:
                pale.append((x, y))
    if pale:
        xs = [p[0] for p in pale]
        ys = [p[1] for p in pale]
        print(f"\nName pale pixels: {len(pale)} x[{min(xs)}..{max(xs)}] y[{min(ys)}..{max(ys)}]")

    print("\nName ASCII:")
    for y in range(12, 24):
        line = []
        for x in range(20, 124):
            r, g, b, a = nm.getpixel((x, y))
            if a == 0:
                line.append(" ")
            elif r > 180 and g > 180 and b > 180:
                line.append("#")
            else:
                line.append("+")
        print("".join(line))

    # Segment name glyphs by vertical gaps
    occupied = []
    for x in range(20, 124):
        hit = any(nm.getpixel((x, y))[3] > 0 for y in range(12, 24))
        occupied.append(hit)
    runs = []
    start = None
    for i, hit in enumerate(occupied):
        x = 20 + i
        if hit and start is None:
            start = x
        elif not hit and start is not None:
            runs.append((start, x - 1))
            start = None
    if start is not None:
        runs.append((start, 123))
    print("\nName glyph runs (inclusive x):", runs)

    # Dump Support / SC8 / 30 regions as bitmasks relative to pale
    for label, layer_name, x0, y0, w, h in [
        ("support", "Categ", 130, 12, 31, 7),
        ("sc8", "SC MAX", 130, 20, 15, 6),
        ("max", "SC MAX", 149, 19, 16, 7),
        ("neutral", "Element symbol", 4, 13, 12, 12),
    ]:
        layer = layers[layer_name]
        print(f"\n=== {label} mask ===")
        for y in range(y0, y0 + h):
            row = []
            for x in range(x0, x0 + w):
                r, g, b, a = layer.getpixel((x, y))
                if a == 0:
                    row.append(".")
                elif r > 200 and g > 100 and b < 80:
                    row.append("O")  # orange
                elif r > 200 and g > 180 and b > 100:
                    row.append("o")  # pale orange
                elif r > 180 and g > 180 and b > 180:
                    row.append("#")
                elif abs(r - 16) < 40 and abs(g - 164) < 40:
                    row.append("g")
                else:
                    row.append("+")
            print("".join(row))

    # Target square at primary center x193,y8
    tg = layers["Targets"]
    print("\n=== target primary center 8x8 @193,8 ===")
    for y in range(8, 16):
        row = []
        for x in range(193, 201):
            r, g, b, a = tg.getpixel((x, y))
            if a == 0:
                row.append(".")
            elif r > 200 and g > 100 and b < 50:
                row.append("P")
            elif r > 200 and g > 150:
                row.append("S")
            elif r > 150 and g < 100:
                row.append("R")
            else:
                row.append("+")
        print("".join(row))

    print("\n=== ally base 8x8 @190,18 ===")
    for y in range(18, 26):
        row = []
        for x in range(190, 198):
            r, g, b, a = tg.getpixel((x, y))
            if a == 0:
                row.append(".")
            elif b > r and b > g:
                row.append("N")
            else:
                row.append("+")
        print("".join(row))


if __name__ == "__main__":
    main()
