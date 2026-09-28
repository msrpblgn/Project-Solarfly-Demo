#!/usr/bin/env python3
"""
Build complete skill-card fonts from Aseprite extractions + consistent extensions.
Emits assets_src/ui/skills/generated/skill_card_fonts.inc
"""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GEN = ROOT / "assets_src/ui/skills/generated"
GEN.mkdir(parents=True, exist_ok=True)

# Logical 6-row name font, bit0 = leftmost. Scale ×2 on paint.
# Extracted from skills_menu Text layer ("Hammer Fist"); others extended to match.
NAME: dict[str, dict] = {
    # Extracted
    "H": {"w": 4, "adv": 5, "rows": [0b1001, 0b1001, 0b1111, 0b1001, 0b1001, 0b1001]},
    "a": {"w": 4, "adv": 5, "rows": [0b0000, 0b0110, 0b1000, 0b1110, 0b1001, 0b1110]},
    "m": {"w": 7, "adv": 8, "rows": [0, 0b0111111, 0b1001001, 0b1001001, 0b1001001, 0b1001001]},
    "e": {"w": 4, "adv": 5, "rows": [0, 0b0110, 0b1001, 0b1111, 0b0001, 0b0110]},
    "r": {"w": 3, "adv": 4, "rows": [0, 0b0101, 0b0011, 0b0001, 0b0001, 0b0001]},
    "F": {"w": 4, "adv": 5, "rows": [0b1111, 0b0001, 0b0111, 0b0001, 0b0001, 0b0001]},
    "i": {"w": 1, "adv": 2, "rows": [1, 0, 1, 1, 1, 1]},
    "s": {"w": 3, "adv": 4, "rows": [0, 0b0110, 0b0001, 0b0010, 0b0100, 0b0011]},
    "t": {"w": 2, "adv": 3, "rows": [0b01, 0b11, 0b01, 0b01, 0b01, 0b10]},
    " ": {"w": 0, "adv": 4, "rows": [0, 0, 0, 0, 0, 0]},
    # Extended caps (match H/F weight)
    "A": {"w": 4, "adv": 5, "rows": [0b0110, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001]},
    "B": {"w": 4, "adv": 5, "rows": [0b0111, 0b1001, 0b0111, 0b1001, 0b1001, 0b0111]},
    "C": {"w": 4, "adv": 5, "rows": [0b0110, 0b1001, 0b0001, 0b0001, 0b1001, 0b0110]},
    "D": {"w": 4, "adv": 5, "rows": [0b0111, 0b1001, 0b1001, 0b1001, 0b1001, 0b0111]},
    "E": {"w": 4, "adv": 5, "rows": [0b1111, 0b0001, 0b0111, 0b0001, 0b0001, 0b1111]},
    "G": {"w": 4, "adv": 5, "rows": [0b0110, 0b0001, 0b1101, 0b1001, 0b1001, 0b0110]},
    "I": {"w": 3, "adv": 4, "rows": [0b111, 0b010, 0b010, 0b010, 0b010, 0b111]},
    "J": {"w": 4, "adv": 5, "rows": [0b1110, 0b0100, 0b0100, 0b0100, 0b0101, 0b0010]},
    "K": {"w": 4, "adv": 5, "rows": [0b1001, 0b0101, 0b0011, 0b0101, 0b1001, 0b1001]},
    "L": {"w": 4, "adv": 5, "rows": [0b0001, 0b0001, 0b0001, 0b0001, 0b0001, 0b1111]},
    "M": {"w": 5, "adv": 6, "rows": [0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001]},
    "N": {"w": 4, "adv": 5, "rows": [0b1001, 0b1001, 0b1101, 0b1011, 0b1001, 0b1001]},
    "O": {"w": 4, "adv": 5, "rows": [0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110]},
    "P": {"w": 4, "adv": 5, "rows": [0b0111, 0b1001, 0b1001, 0b0111, 0b0001, 0b0001]},
    "Q": {"w": 4, "adv": 5, "rows": [0b0110, 0b1001, 0b1001, 0b1001, 0b0101, 0b1010]},
    "R": {"w": 4, "adv": 5, "rows": [0b0111, 0b1001, 0b1001, 0b0111, 0b1001, 0b1001]},
    "S": {"w": 4, "adv": 5, "rows": [0b1110, 0b0001, 0b0110, 0b1000, 0b1000, 0b0111]},
    "T": {"w": 5, "adv": 6, "rows": [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100]},
    "U": {"w": 4, "adv": 5, "rows": [0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110]},
    "V": {"w": 4, "adv": 5, "rows": [0b1001, 0b1001, 0b1001, 0b1001, 0b0110, 0b0110]},
    "W": {"w": 5, "adv": 6, "rows": [0b10001, 0b10001, 0b10001, 0b10101, 0b11011, 0b10001]},
    "X": {"w": 4, "adv": 5, "rows": [0b1001, 0b1001, 0b0110, 0b0110, 0b1001, 0b1001]},
    "Y": {"w": 4, "adv": 5, "rows": [0b1001, 0b1001, 0b1001, 0b0110, 0b0100, 0b0100]},
    "Z": {"w": 4, "adv": 5, "rows": [0b1111, 0b1000, 0b0100, 0b0010, 0b0001, 0b1111]},
    # Extended lowercase
    "b": {"w": 4, "adv": 5, "rows": [0b0001, 0b0001, 0b0111, 0b1001, 0b1001, 0b0111]},
    "c": {"w": 4, "adv": 5, "rows": [0, 0b0110, 0b0001, 0b0001, 0b0001, 0b0110]},
    "d": {"w": 4, "adv": 5, "rows": [0b1000, 0b1000, 0b1110, 0b1001, 0b1001, 0b1110]},
    "f": {"w": 3, "adv": 4, "rows": [0b110, 0b001, 0b011, 0b001, 0b001, 0b001]},
    "g": {"w": 4, "adv": 5, "rows": [0, 0b0110, 0b1001, 0b1001, 0b1110, 0b1000]},  # last row into descender zone clipped
    "h": {"w": 4, "adv": 5, "rows": [0b0001, 0b0001, 0b0111, 0b1001, 0b1001, 0b1001]},
    "j": {"w": 2, "adv": 3, "rows": [0b10, 0, 0b10, 0b10, 0b10, 0b01]},
    "k": {"w": 4, "adv": 5, "rows": [0b0001, 0b0001, 0b1001, 0b0101, 0b0011, 0b1001]},
    "l": {"w": 1, "adv": 2, "rows": [1, 1, 1, 1, 1, 1]},
    "n": {"w": 4, "adv": 5, "rows": [0, 0b0111, 0b1001, 0b1001, 0b1001, 0b1001]},
    "o": {"w": 4, "adv": 5, "rows": [0, 0b0110, 0b1001, 0b1001, 0b1001, 0b0110]},
    "p": {"w": 4, "adv": 5, "rows": [0, 0b0111, 0b1001, 0b1001, 0b0111, 0b0001]},
    "q": {"w": 4, "adv": 5, "rows": [0, 0b1110, 0b1001, 0b1001, 0b1110, 0b1000]},
    "u": {"w": 4, "adv": 5, "rows": [0, 0b1001, 0b1001, 0b1001, 0b1001, 0b1110]},
    "v": {"w": 4, "adv": 5, "rows": [0, 0b1001, 0b1001, 0b1001, 0b0110, 0b0110]},
    "w": {"w": 5, "adv": 6, "rows": [0, 0b10001, 0b10001, 0b10101, 0b11011, 0b10001]},
    "x": {"w": 4, "adv": 5, "rows": [0, 0b1001, 0b0110, 0b0110, 0b1001, 0]},
    "y": {"w": 4, "adv": 5, "rows": [0, 0b1001, 0b1001, 0b1001, 0b1110, 0b1000]},
    "z": {"w": 4, "adv": 5, "rows": [0, 0b1111, 0b0100, 0b0010, 0b0001, 0b1111]},
    "-": {"w": 3, "adv": 4, "rows": [0, 0, 0b111, 0, 0, 0]},
    ".": {"w": 1, "adv": 2, "rows": [0, 0, 0, 0, 0, 1]},
    "'": {"w": 1, "adv": 2, "rows": [1, 1, 0, 0, 0, 0]},
    ":": {"w": 1, "adv": 2, "rows": [0, 1, 0, 1, 0, 0]},
}

# Compact font: 6-row default (SC/power), 7-row for Support lowercase with descenders.
# bit0 = leftmost. Extracted S,C,8,u,p,o,r,t from Aseprite; digits styled like '8'.
COMPACT: dict[str, dict] = {
    # Extracted
    "S": {"w": 4, "adv": 5, "h": 6, "rows": [14, 1, 6, 8, 8, 7]},
    # Advance 6 matches measured SC8 (C to digit gap is 2px, not 1).
    "C": {"w": 4, "adv": 6, "h": 6, "rows": [14, 1, 1, 1, 1, 14]},
    "8": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 6, 9, 9, 6]},
    "u": {"w": 4, "adv": 5, "h": 7, "rows": [0, 9, 9, 9, 9, 14, 0]},
    "p": {"w": 4, "adv": 5, "h": 7, "rows": [0, 7, 9, 9, 7, 1, 1]},
    "o": {"w": 4, "adv": 5, "h": 7, "rows": [0, 6, 9, 9, 9, 6, 0]},
    "r": {"w": 3, "adv": 4, "h": 7, "rows": [0, 5, 3, 1, 1, 1, 0]},
    "t": {"w": 2, "adv": 3, "h": 7, "rows": [1, 3, 1, 1, 1, 2, 0]},
    # Digits matching '8' metrics (4×6) — "30" occupies 9×6 with adv 5+4
    "0": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 9, 9, 9, 6]},
    "1": {"w": 3, "adv": 4, "h": 6, "rows": [2, 3, 2, 2, 2, 7]},
    "2": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 8, 4, 2, 15]},
    "3": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 4, 8, 9, 6]},
    "4": {"w": 4, "adv": 5, "h": 6, "rows": [9, 9, 15, 8, 8, 8]},
    "5": {"w": 4, "adv": 5, "h": 6, "rows": [15, 1, 7, 8, 9, 6]},
    "6": {"w": 4, "adv": 5, "h": 6, "rows": [6, 1, 7, 9, 9, 6]},
    "7": {"w": 4, "adv": 5, "h": 6, "rows": [15, 8, 4, 4, 2, 2]},
    "9": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 9, 14, 8, 6]},
    # Caps for PHYS/SPEC/SUPP/STAT/DISR and full words
    "A": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 9, 15, 9, 9]},
    "B": {"w": 4, "adv": 5, "h": 6, "rows": [7, 9, 7, 9, 9, 7]},
    "D": {"w": 4, "adv": 5, "h": 6, "rows": [7, 9, 9, 9, 9, 7]},
    "E": {"w": 4, "adv": 5, "h": 6, "rows": [15, 1, 7, 1, 1, 15]},
    "H": {"w": 4, "adv": 5, "h": 6, "rows": [9, 9, 15, 9, 9, 9]},
    "I": {"w": 3, "adv": 4, "h": 6, "rows": [7, 2, 2, 2, 2, 7]},
    "L": {"w": 4, "adv": 5, "h": 6, "rows": [1, 1, 1, 1, 1, 15]},
    "P": {"w": 4, "adv": 5, "h": 6, "rows": [7, 9, 9, 7, 1, 1]},
    "R": {"w": 4, "adv": 5, "h": 6, "rows": [7, 9, 9, 7, 5, 9]},
    "T": {"w": 5, "adv": 6, "h": 6, "rows": [31, 4, 4, 4, 4, 4]},
    "U": {"w": 4, "adv": 5, "h": 6, "rows": [9, 9, 9, 9, 9, 6]},
    "Y": {"w": 4, "adv": 5, "h": 6, "rows": [9, 9, 9, 6, 4, 4]},
    "F": {"w": 4, "adv": 5, "h": 6, "rows": [15, 1, 7, 1, 1, 1]},
    "G": {"w": 4, "adv": 5, "h": 6, "rows": [6, 1, 13, 9, 9, 6]},
    "J": {"w": 4, "adv": 5, "h": 6, "rows": [14, 4, 4, 4, 5, 2]},
    "K": {"w": 4, "adv": 5, "h": 6, "rows": [9, 5, 3, 5, 9, 9]},
    "M": {"w": 5, "adv": 6, "h": 6, "rows": [17, 27, 21, 17, 17, 17]},
    "N": {"w": 4, "adv": 5, "h": 6, "rows": [9, 11, 13, 9, 9, 9]},
    "O": {"w": 4, "adv": 5, "h": 6, "rows": [6, 9, 9, 9, 9, 6]},
    "V": {"w": 4, "adv": 5, "h": 6, "rows": [9, 9, 9, 9, 6, 6]},
    "W": {"w": 5, "adv": 6, "h": 6, "rows": [17, 17, 17, 21, 27, 17]},
    "X": {"w": 4, "adv": 5, "h": 6, "rows": [9, 9, 6, 6, 9, 9]},
    "Z": {"w": 4, "adv": 5, "h": 6, "rows": [15, 8, 4, 2, 1, 15]},
    # lowercase for Support + others
    "a": {"w": 4, "adv": 5, "h": 7, "rows": [0, 6, 8, 14, 9, 14, 0]},
    "c": {"w": 4, "adv": 5, "h": 7, "rows": [0, 6, 1, 1, 1, 6, 0]},
    "d": {"w": 4, "adv": 5, "h": 7, "rows": [8, 8, 14, 9, 9, 14, 0]},
    "e": {"w": 4, "adv": 5, "h": 7, "rows": [0, 6, 9, 15, 1, 6, 0]},
    "i": {"w": 1, "adv": 2, "h": 7, "rows": [1, 0, 1, 1, 1, 1, 0]},
    "l": {"w": 1, "adv": 2, "h": 7, "rows": [1, 1, 1, 1, 1, 1, 0]},
    "n": {"w": 4, "adv": 5, "h": 7, "rows": [0, 7, 9, 9, 9, 9, 0]},
    "s": {"w": 3, "adv": 4, "h": 7, "rows": [0, 6, 1, 2, 4, 3, 0]},
    "y": {"w": 4, "adv": 5, "h": 7, "rows": [0, 9, 9, 9, 14, 8, 6]},
    " ": {"w": 0, "adv": 3, "h": 6, "rows": [0] * 7},
    "-": {"w": 3, "adv": 4, "h": 6, "rows": [0, 0, 7, 0, 0, 0]},
    ".": {"w": 1, "adv": 2, "h": 6, "rows": [0, 0, 0, 0, 0, 1]},
    # Ellipsis uses three '.' — also provide a dedicated glyph
    "\u2026": {"w": 5, "adv": 6, "h": 6, "rows": [0, 0, 0, 0, 0, 0b10101]},
}

# Neutral element 12×12 mask, bit0=left (extracted).
NEUTRAL = [0x0F0, 0x30C, 0x4F2, 0x5FA, 0xBFD, 0xBFD, 0xBFD, 0xBFD, 0x5FA, 0x4F2, 0x30C, 0x0F0]


def emit_rows(rows: list[int], n: int) -> str:
    padded = list(rows) + [0] * n
    return ", ".join(str(int(v)) for v in padded[:n])


def main() -> None:
    lines: list[str] = []
    lines.append("/* Generated by tools/skill_card/build_skill_card_fonts.py — do not hand-edit. */")
    lines.append("#ifndef SKILL_CARD_FONTS_INC")
    lines.append("#define SKILL_CARD_FONTS_INC")
    lines.append("")
    lines.append("#define SKILL_CARD_NAME_LOGICAL_H 6")
    lines.append("#define SKILL_CARD_NAME_SCALE 2")
    lines.append("#define SKILL_CARD_COMPACT_MAX_H 7")
    lines.append("")
    lines.append("typedef struct SkillCardVarGlyph {")
    lines.append("    u8 width;")
    lines.append("    u8 advance;")
    lines.append("    u8 height;")
    lines.append("    u8 rows[7]; /* bit0 = leftmost pixel */")
    lines.append("} SkillCardVarGlyph;")
    lines.append("")

    lines.append("static const SkillCardVarGlyph s_nameFont[128] = {")
    for i in range(128):
        ch = chr(i)
        g = NAME.get(ch)
        if not g:
            lines.append("    {0, 0, 0, {0}},")
            continue
        rows = emit_rows(g["rows"], 7)
        comment = repr(ch)
        lines.append(
            "    {"
            + str(g["w"])
            + ", "
            + str(g["adv"])
            + ", 6, {"
            + rows
            + "}}, /* "
            + comment
            + " */"
        )
    lines.append("};")
    lines.append("")

    lines.append("static const SkillCardVarGlyph s_compactFont[128] = {")
    for i in range(128):
        ch = chr(i)
        g = COMPACT.get(ch)
        if not g:
            lines.append("    {0, 0, 0, {0}},")
            continue
        rows = emit_rows(g["rows"], 7)
        comment = repr(ch)
        lines.append(
            "    {"
            + str(g["w"])
            + ", "
            + str(g["adv"])
            + ", "
            + str(g["h"])
            + ", {"
            + rows
            + "}}, /* "
            + comment
            + " */"
        )
    lines.append("};")
    lines.append("")

    lines.append("/* Ellipsis glyph for overflow policy (three-dot). */")
    eg = COMPACT["\u2026"]
    lines.append(
        "static const SkillCardVarGlyph s_compactEllipsis = {"
        + str(eg["w"])
        + ", "
        + str(eg["adv"])
        + ", "
        + str(eg["h"])
        + ", {"
        + emit_rows(eg["rows"], 7)
        + "}};"
    )
    lines.append("")

    lines.append("static const u16 s_neutralElementMask[12] = {")
    lines.append("    " + ", ".join(f"0x{v:03X}" for v in NEUTRAL))
    lines.append("};")
    lines.append("")
    lines.append("#endif /* SKILL_CARD_FONTS_INC */")

    out = GEN / "skill_card_fonts.inc"
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("Wrote", out)

    def text_right(font, s, scale=1):
        cursor = 0
        right = 0
        for ch in s:
            g = font[ch]
            right = cursor + g["w"] * scale
            cursor += g["adv"] * scale
        return right

    print(f"Hammer Fist right width={text_right(NAME, 'Hammer Fist', 2)}")
    print(f"Support width={text_right(COMPACT, 'Support')}")
    print(f"SC8 width={text_right(COMPACT, 'SC8')}")
    print(f"30 width={text_right(COMPACT, '30')}")


if __name__ == "__main__":
    main()
