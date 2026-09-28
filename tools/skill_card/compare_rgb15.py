#!/usr/bin/env python3
"""Exact RGB15 comparison: Aseprite reference vs production PaintView PPM."""
from __future__ import annotations

import struct
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/reports/skill_card_visual_repair_logs"
OUT.mkdir(parents=True, exist_ok=True)

def fix_rgb15(r: int, g: int, b: int) -> int:
    return ((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)


def image_to_rgb15(im: Image.Image) -> list[int]:
    rgb = im.convert("RGB")
    w, h = rgb.size
    out = []
    for y in range(h):
        for x in range(w):
            r, g, b = rgb.getpixel((x, y))
            out.append(fix_rgb15(r, g, b))
    return out


def load_ppm_rgb15(path: Path) -> tuple[int, int, list[int]]:
    data = path.read_bytes()
    if data.startswith(b"P6"):
        parts = data.split(b"\n", 3)
        # Handle possible comment
        idx = 1
        while parts[idx].startswith(b"#"):
            idx += 1
        w, h = map(int, parts[idx].split())
        idx += 1
        while parts[idx].startswith(b"#"):
            idx += 1
        maxv = int(parts[idx])
        raw = parts[idx + 1] if idx + 1 < len(parts) else b""
        # Re-parse more carefully
    # Robust PPM parse
    i = 0
    assert data[0:2] == b"P6"
    i = 2
    def next_token():
        nonlocal i
        while i < len(data) and data[i] in b" \t\r\n":
            i += 1
        if i < len(data) and data[i] == ord("#"):
            while i < len(data) and data[i] not in b"\n":
                i += 1
            return next_token()
        start = i
        while i < len(data) and data[i] not in b" \t\r\n":
            i += 1
        return data[start:i]

    next_token()  # already consumed P6 oddly — restart
    i = 0
    assert data.startswith(b"P6")
    i = 2
    w = int(next_token())
    h = int(next_token())
    maxv = int(next_token())
    if data[i] in b"\r\n":
        i += 1
        if data[i - 1] == 13 and i < len(data) and data[i] == 10:
            i += 1
    raw = data[i:]
    assert maxv == 255
    pixels = []
    for n in range(w * h):
        r, g, b = raw[n * 3], raw[n * 3 + 1], raw[n * 3 + 2]
        pixels.append(fix_rgb15(r, g, b))
    return w, h, pixels


def rgb15_to_rgb8(c: int) -> tuple[int, int, int]:
    return ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)


def save_rgb15_png(path: Path, w: int, h: int, px: list[int], scale: int = 1) -> None:
    im = Image.new("RGB", (w, h))
    for y in range(h):
        for x in range(w):
            im.putpixel((x, y), rgb15_to_rgb8(px[y * w + x]))
    if scale > 1:
        im = im.resize((w * scale, h * scale), Image.NEAREST)
    im.save(path)


def compare(ref: list[int], impl: list[int], w: int, h: int, label: str) -> dict:
    assert len(ref) == len(impl) == w * h
    diffs = []
    for i, (a, b) in enumerate(zip(ref, impl)):
        if a != b:
            diffs.append((i % w, i // w, a, b))
    # Diff image
    dim = Image.new("RGB", (w, h), (0, 0, 0))
    for x, y, a, b in diffs:
        dim.putpixel((x, y), (255, 0, 0))
    dim.save(OUT / f"diff_{label}.png")
    dim.resize((w * 4, h * 4), Image.NEAREST).save(OUT / f"diff_{label}_x4.png")

    # Bounds per coarse component regions
    regions = {
        "border": (0, 0, w, h),  # full for count; specialized below
        "name": (20, 12, 104, 12),
        "icon": (4, 13, 12, 12),
        "detail": (130, 12, 40, 8),
        "sc": (130, 20, 19, 7),
        "max": (149, 19, 16, 7),
        "targets": (172, 8, 52, 18),
    }
    region_counts = {}
    for name, (x0, y0, rw, rh) in regions.items():
        n = 0
        for x, y, a, b in diffs:
            if x0 <= x < x0 + rw and y0 <= y < y0 + rh:
                n += 1
        region_counts[name] = n

    print(f"\n== {label} ==")
    print(f"mismatched pixels: {len(diffs)} / {w*h}")
    if diffs:
        xs = [d[0] for d in diffs]
        ys = [d[1] for d in diffs]
        print(f"bounds: x[{min(xs)}..{max(xs)}] y[{min(ys)}..{max(ys)}]")
    for name, n in region_counts.items():
        print(f"  region {name}: {n} mismatches")
    return {"label": label, "mismatches": len(diffs), "regions": region_counts, "diffs": diffs[:20]}


def flatten_reference(mode: str) -> Image.Image:
    """mode: support_max | support_nomax | power30_nomax"""
    layers = {}
    for name in [
        "Background color",
        "bORDER",
        "Text",
        "Element symbol",
        "Categ",
        "SC",
        "SC MAX",
        "Targets",
    ]:
        p = OUT / f"layer_{name}.png"
        if not p.exists():
            p = ROOT / "assets_src/ui/skills/export" / f"layer_{name}.png"
        layers[name] = Image.open(p).convert("RGBA")

    order = ["Background color", "bORDER", "Text", "Element symbol", "Targets"]
    if mode == "support_max":
        order = [
            "Background color",
            "bORDER",
            "Text",
            "Element symbol",
            "Categ",
            "SC MAX",
            "Targets",
        ]
    elif mode == "support_nomax":
        order = [
            "Background color",
            "bORDER",
            "Text",
            "Element symbol",
            "Categ",
            "SC",
            "Targets",
        ]
    elif mode == "power30_nomax":
        # Build composite then overwrite detail with synthesized "30" from painter? 
        # For reference: hide Categ/SC MAX, keep SC, and we need a "30" layer.
        # Aseprite has no Power layer — synthesize reference detail from glyph extract.
        order = [
            "Background color",
            "bORDER",
            "Text",
            "Element symbol",
            "SC",
            "Targets",
        ]
    base = Image.new("RGBA", (224, 34), (0, 0, 0, 0))
    for n in order:
        base = Image.alpha_composite(base, layers[n])
    if mode == "power30_nomax":
        # Paint "30" using same compact glyphs as production.
        import importlib.util

        spec = importlib.util.spec_from_file_location(
            "build_skill_card_fonts",
            ROOT / "tools/skill_card/build_skill_card_fonts.py",
        )
        mod = importlib.util.module_from_spec(spec)
        assert spec and spec.loader
        spec.loader.exec_module(mod)
        COMPACT = mod.COMPACT

        pale = (213, 213, 213, 255)
        x = 130
        y = 12
        for ch in "30":
            g = COMPACT[ch]
            for row in range(g["h"]):
                for col in range(g["w"]):
                    if g["rows"][row] & (1 << col):
                        base.putpixel((x + col, y + row), pale)
            x += g["adv"]
    return base


def main() -> int:
    # Expect PPM from host test
    impl_path = OUT / "impl_support_max.ppm"
    if not impl_path.exists():
        print("Missing", impl_path, "- run host fixture painter first")
        return 2

    results = []
    for mode, ppm_name in [
        ("support_max", "impl_support_max.ppm"),
        ("support_nomax", "impl_support_nomax.ppm"),
        ("power30_nomax", "impl_power30_nomax.ppm"),
    ]:
        ppm = OUT / ppm_name
        if not ppm.exists():
            print("skip missing", ppm)
            continue
        ref_im = flatten_reference(mode)
        ref_im.convert("RGB").save(OUT / f"reference_{mode}_rgb.png")
        ref15 = image_to_rgb15(ref_im)
        w, h, impl15 = load_ppm_rgb15(ppm)
        assert (w, h) == (224, 34)
        save_rgb15_png(OUT / f"reference_{mode}_rgb15.png", w, h, ref15)
        save_rgb15_png(OUT / f"impl_{mode}_rgb15.png", w, h, impl15)
        save_rgb15_png(OUT / f"reference_{mode}_x4.png", w, h, ref15, 4)
        save_rgb15_png(OUT / f"impl_{mode}_x4.png", w, h, impl15, 4)
        # Side-by-side
        side = Image.new("RGB", (w * 4 * 2 + 8, h * 4), (32, 32, 32))
        side.paste(Image.open(OUT / f"reference_{mode}_x4.png"), (0, 0))
        side.paste(Image.open(OUT / f"impl_{mode}_x4.png"), (w * 4 + 8, 0))
        side.save(OUT / f"compare_{mode}_x4.png")
        results.append(compare(ref15, impl15, w, h, mode))

    worst = max((r["mismatches"] for r in results), default=999999)
    print("\nworst mismatches:", worst)
    # Write summary
    (OUT / "compare_summary.txt").write_text(
        "\n".join(f"{r['label']}: {r['mismatches']}" for r in results) + "\n",
        encoding="utf-8",
    )
    return 0 if worst == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
