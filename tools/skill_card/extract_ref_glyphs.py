#!/usr/bin/env python3
"""Extract skill-card reference glyphs/art from Aseprite layer exports."""
from __future__ import annotations

import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/reports/skill_card_visual_repair_logs"
EXP = ROOT / "assets_src/ui/skills/export"
GEN = ROOT / "assets_src/ui/skills/generated"
GEN.mkdir(parents=True, exist_ok=True)


def load_layer(name: str) -> Image.Image:
    for base in (OUT, EXP):
        p = base / f"layer_{name}.png"
        if p.exists():
            return Image.open(p).convert("RGBA")
    raise FileNotFoundError(name)


def is_ink(px: tuple[int, int, int, int], kind: str) -> bool:
    r, g, b, a = px
    if a == 0:
        return False
    if kind == "navy":
        return r < 40 and g < 50 and b < 80
    if kind == "pale":
        return r > 180 and g > 180 and b > 180
    if kind == "orange":
        return r > 200 and g > 80 and b < 80
    if kind == "pale_orange":
        return r > 200 and g > 160 and b > 80 and b < 180
    return a > 0


def extract_mask(im: Image.Image, x0: int, y0: int, w: int, h: int, kind: str) -> list[list[int]]:
    rows: list[list[int]] = []
    for y in range(y0, y0 + h):
        row = []
        for x in range(x0, x0 + w):
            row.append(1 if is_ink(im.getpixel((x, y)), kind) else 0)
        rows.append(row)
    return rows


def downsample_2x(mask: list[list[int]]) -> list[list[int]]:
    """Average 2x2 blocks → logical pixels (any ink counts)."""
    h = len(mask)
    w = len(mask[0]) if h else 0
    out = []
    for y in range(0, h, 2):
        row = []
        for x in range(0, w, 2):
            ink = 0
            for dy in range(2):
                for dx in range(2):
                    yy, xx = y + dy, x + dx
                    if yy < h and xx < w and mask[yy][xx]:
                        ink = 1
            row.append(ink)
        out.append(row)
    return out


def mask_to_rows_u32(mask: list[list[int]]) -> list[int]:
    """Pack left-to-right bits into ints (bit 0 = leftmost)."""
    out = []
    for row in mask:
        v = 0
        for i, bit in enumerate(row):
            if bit:
                v |= 1 << i
        out.append(v)
    return out


def segment_runs(mask: list[list[int]]) -> list[tuple[int, int]]:
    h = len(mask)
    w = len(mask[0]) if h else 0
    occupied = [any(mask[y][x] for y in range(h)) for x in range(w)]
    runs = []
    start = None
    for x, hit in enumerate(occupied):
        if hit and start is None:
            start = x
        elif not hit and start is not None:
            runs.append((start, x - 1))
            start = None
    if start is not None:
        runs.append((start, w - 1))
    return runs


def crop_cols(mask: list[list[int]], x0: int, x1: int) -> list[list[int]]:
    return [row[x0 : x1 + 1] for row in mask]


def c_array_u8_rows(name: str, glyphs: dict[str, list[int]], width_bits: int) -> str:
    lines = [f"/* Auto-generated {name}; bit0 = leftmost pixel. */", f"typedef struct {{"]
    lines.append(f"    int advance;")
    lines.append(f"    int width;")
    lines.append(f"    u8 rows[{width_bits}]; /* unused placeholder */")
    # Better: emit separate tables
    return ""


def main() -> None:
    text = load_layer("Text")
    categ = load_layer("Categ")
    sc = load_layer("SC")
    scmax = load_layer("SC MAX")
    elem = load_layer("Element symbol")

    # --- Name font (navy, 2x scaled) ---
    name_mask = extract_mask(text, 20, 12, 104, 12, "navy")
    logical = downsample_2x(name_mask)
    runs = segment_runs(name_mask)
    # Expected chars for "Hammer Fist" with space gap
    # runs at full-res: H a m m e r [gap] F i s t
    labels = ["H", "a", "m", "m", "e", "r", "F", "i", "s", "t"]
    # Filter out tiny noise; space is the gap between r and F
    print("Full-res runs:", runs)
    print("Logical size:", len(logical), "x", len(logical[0]) if logical else 0)

    name_glyphs: dict[str, dict] = {}
    for label, (x0, x1) in zip(labels, runs):
        # map to logical coords
        lx0, lx1 = x0 // 2, x1 // 2
        # relative to name band start (already at 0)
        g = crop_cols(logical, lx0, lx1)
        # trim empty logical rows top/bottom? keep full 6
        w = lx1 - lx0 + 1
        rows = mask_to_rows_u32(g)
        # advance = width + 1 (1px logical gap observed ~2px full = 1 logical)
        name_glyphs[label] = {
            "width": w,
            "height": len(g),
            "rows": rows,
            "advance": w + 1,
            "full_x0": x0,
            "full_x1": x1,
        }
        print(f"glyph '{label}' logical {w}x{len(g)} rows={rows}")

    # Space advance from gap between r and F
    r_end = runs[5][1]
    f_start = runs[6][0]
    space_adv = (f_start - r_end - 1) // 2
    print("space logical advance:", space_adv)
    name_glyphs[" "] = {"width": 0, "height": 6, "rows": [0] * 6, "advance": max(1, space_adv)}

    # --- Compact pale fonts: Support, SC8 ---
    support = extract_mask(categ, 130, 12, 31, 7, "pale")
    sc8 = extract_mask(sc, 130, 20, 15, 6, "pale")
    print("\nSupport runs:", segment_runs(support))
    print("SC8 runs:", segment_runs(sc8))

    # Segment Support → S u p p o r t (7 letters) — height 7 with possible descender row
    support_runs = segment_runs(support)
    support_labels = list("Support")
    compact_glyphs: dict[str, dict] = {}
    for label, (x0, x1) in zip(support_labels, support_runs):
        g = crop_cols(support, x0, x1)
        # Use top 5 or 6 rows that have content for consistency with SC
        rows = mask_to_rows_u32(g)
        w = x1 - x0 + 1
        compact_glyphs[label] = {"width": w, "height": len(g), "rows": rows, "advance": w + 1}
        print(f"Support '{label}' {w}x{len(g)} {rows}")

    sc_labels = ["S", "C", "8"]
    for label, (x0, x1) in zip(sc_labels, segment_runs(sc8)):
        g = crop_cols(sc8, x0, x1)
        rows = mask_to_rows_u32(g)
        w = x1 - x0 + 1
        # Prefer SC digit/letter over Support if duplicate
        compact_glyphs[label] = {"width": w, "height": len(g), "rows": rows, "advance": w + 1}
        print(f"SC '{label}' {w}x{len(g)} {rows}")

    # Digits 0-9: reconstruct from SC '8' style using existing s_compact3x5 AFTER bit-fix,
    # but first dump what we have from reference only.

    # --- Neutral icon 12x12 pale ---
    icon = extract_mask(elem, 4, 13, 12, 12, "pale")
    icon_rows = mask_to_rows_u32(icon)
    print("\nNeutral icon rows:", icon_rows)

    # --- MAX two-tone ---
    max_img = scmax.crop((149, 19, 165, 26))
    max_rgba = []
    for y in range(7):
        row = []
        for x in range(16):
            r, g, b, a = max_img.getpixel((x, y))
            if a == 0:
                row.append(None)
            else:
                row.append((r, g, b))
        max_rgba.append(row)

    # Save JSON dump for C codegen
    payload = {
        "name_glyphs": name_glyphs,
        "name_scale": 2,
        "name_height_logical": 6,
        "compact_glyphs": compact_glyphs,
        "neutral_icon_12x12_rows_bit0_left": icon_rows,
        "neutral_icon_mask": icon,
        "space_advance_logical": name_glyphs[" "]["advance"],
    }
    (GEN / "extracted_glyphs.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print("\nWrote", GEN / "extracted_glyphs.json")

    # Also save neutral PNG and MAX PNG
    Image.fromarray(
        __import__("numpy").array(
            [[[213, 213, 213, 255 if icon[y][x] else 0] for x in range(12)] for y in range(12)],
            dtype="uint8",
        )
    ).save(GEN / "neutral_element_12x12.png")

    # Write C header fragment for neutral icon + name glyphs
    write_c_assets(name_glyphs, compact_glyphs, icon_rows, max_rgba)


def rgb15(r: int, g: int, b: int) -> int:
    return ((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)


def write_c_assets(name_glyphs, compact_glyphs, icon_rows, max_rgba) -> None:
    path = GEN / "skill_card_ref_assets.inc"
    lines = []
    lines.append("/* Generated from skills_menu.aseprite — do not hand-edit. */")
    lines.append("#ifndef SKILL_CARD_REF_ASSETS_INC")
    lines.append("#define SKILL_CARD_REF_ASSETS_INC")
    lines.append("")
    lines.append("#define SKILL_CARD_NAME_FONT_H 6")
    lines.append("#define SKILL_CARD_NAME_FONT_SCALE 2")
    lines.append("#define SKILL_CARD_COMPACT_FONT_H 6")
    lines.append("")

    # Neutral icon: 12 rows as u16 bitmasks bit0=left
    lines.append("static const u16 s_neutralElementMask[12] = {")
    lines.append("    " + ", ".join(f"0x{row:03X}" for row in icon_rows))
    lines.append("};")
    lines.append("")

    # Name glyphs as sparse table by char code
    lines.append("typedef struct SkillCardNameGlyph {")
    lines.append("    u8 width;")
    lines.append("    u8 advance;")
    lines.append("    u8 rows[6]; /* bit0 = leftmost */")
    lines.append("} SkillCardNameGlyph;")
    lines.append("")

    # Emit only known glyphs; missing handled at runtime
    ordered = []
    for ch, g in sorted(name_glyphs.items(), key=lambda kv: kv[0]):
        if ch == " ":
            continue
        ordered.append((ch, g))

    lines.append(f"/* Extracted name glyphs from 'Hammer Fist' sample ({len(ordered)}). */")
    for ch, g in ordered:
        tag = ch if ch.isalnum() else "X"
        rows = g["rows"]
        # pad/truncate to 6
        while len(rows) < 6:
            rows.append(0)
        rows = rows[:6]
        lines.append(
            f"static const SkillCardNameGlyph s_nameGlyph_{ord(ch)}_{tag} = {{"
            f"{g['width']}, {g['advance']}, {{{', '.join(str(r) for r in rows)}}}}};"
        )

    lines.append("")
    lines.append("static const SkillCardNameGlyph *SkillCard_LookupExtractedNameGlyph(char c)")
    lines.append("{")
    lines.append("    switch (c) {")
    for ch, g in ordered:
        tag = ch if ch.isalnum() else "X"
        lines.append(f"    case '{ch}': return &s_nameGlyph_{ord(ch)}_{tag};")
    lines.append("    default: return 0;")
    lines.append("    }")
    lines.append("}")
    lines.append("")

    # Compact extracted
    lines.append("typedef struct SkillCardCompactGlyph {")
    lines.append("    u8 width;")
    lines.append("    u8 advance;")
    lines.append("    u8 height;")
    lines.append("    u8 rows[7]; /* bit0 = leftmost */")
    lines.append("} SkillCardCompactGlyph;")
    lines.append("")
    for ch, g in sorted(compact_glyphs.items(), key=lambda kv: kv[0]):
        rows = list(g["rows"])
        while len(rows) < 7:
            rows.append(0)
        rows = rows[:7]
        safe = ch if ch.isalnum() else "x"
        lines.append(
            f"static const SkillCardCompactGlyph s_compactExt_{ord(ch)}_{safe} = {{"
            f"{g['width']}, {g['advance']}, {g['height']}, {{{', '.join(str(r) for r in rows)}}}}};"
        )
    lines.append("")
    lines.append("static const SkillCardCompactGlyph *SkillCard_LookupExtractedCompactGlyph(char c)")
    lines.append("{")
    lines.append("    switch (c) {")
    for ch in sorted(compact_glyphs.keys()):
        safe = ch if ch.isalnum() else "x"
        # Prefer uppercase mapping for letters we extracted
        lines.append(f"    case '{ch}': return &s_compactExt_{ord(ch)}_{safe};")
    lines.append("    default: return 0;")
    lines.append("    }")
    lines.append("}")
    lines.append("")
    lines.append("#endif")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("Wrote", path)

    # Also emit MAX verification
    max_path = GEN / "max_mark_verify.txt"
    with max_path.open("w", encoding="utf-8") as f:
        for y, row in enumerate(max_rgba):
            for x, px in enumerate(row):
                if px is None:
                    f.write(f"{y},{x}:T\n")
                else:
                    f.write(f"{y},{x}:{rgb15(*px)}\n")


if __name__ == "__main__":
    main()
