#!/usr/bin/env python3
"""
Milestone 1 layout previews for Skills Carousel (240x160 PNGs).

LAYOUT MOCKUPS — not emulator screenshots. Glyphs approximate the engine 5x7
font with VIDEO_FONT_ADVANCE=6. Colors approximate Mode 3 HUD RGB15 mappings.
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "reports" / "skills_carousel_tutsil_m1_previews"

W, H = 240, 160
ADV = 6  # VIDEO_FONT_ADVANCE
GLYPH_H = 7

# Approximate RGB15 → 8-bit
def rgb15(r: int, g: int, b: int) -> tuple[int, int, int]:
    return ((r * 255) // 31, (g * 255) // 31, (b * 255) // 31)


COL_BG = rgb15(4, 4, 12)
COL_FIELD = rgb15(6, 20, 8)
COL_PANEL = rgb15(8, 8, 16)
COL_CARD = rgb15(6, 6, 18)
COL_BORDER = rgb15(8, 16, 31)
COL_TITLE = rgb15(24, 24, 31)
COL_BODY = rgb15(31, 31, 31)
COL_MUTED = rgb15(20, 20, 20)
COL_ACCENT = rgb15(31, 28, 0)
COL_PARTY_BG = rgb15(4, 4, 16)
COL_PARTY_PANEL = rgb15(6, 6, 18)
COL_PARTY_BORDER = rgb15(12, 12, 28)


def measure(text: str) -> int:
    return len(text) * ADV


def draw_text(draw: ImageDraw.ImageDraw, x: int, y: int, text: str, color, clip=None) -> None:
    """Readable mockup text. Advance width matches VIDEO_FONT_ADVANCE=6 for layout math."""
    if clip is not None:
        clip_x, clip_w = clip
        # Approximate clip by trimming characters that would start outside the clip window.
        max_chars = max(0, (clip_x + clip_w - x) // ADV)
        text = text[:max_chars]
    draw.text((x, y - 1), text, fill=color)


def trunc(text: str, max_px: int) -> str:
    max_chars = max_px // ADV
    if len(text) <= max_chars:
        return text
    if max_chars <= 1:
        return text[:1]
    return text[: max_chars - 1] + "."


def frame_offsets(travel: int = 220) -> list[int]:
    """12 frames inclusive endpoints: frame 0 offset 0, frame 11 offset travel. Ease-out."""
    out = []
    max_f = 11
    for f in range(12):
        t = f / max_f
        eased = 1.0 - (1.0 - t) * (1.0 - t)
        out.append(int(round(travel * eased)))
    return out


def battle_base(title: str = "Skills") -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), COL_BG)
    draw = ImageDraw.Draw(im)
    # Field remnant
    draw.rectangle([0, 0, W - 1, 99], fill=COL_FIELD)
    draw.rectangle([8, 8, 80, 40], outline=rgb15(31, 31, 31))
    draw_text(draw, 12, 12, "PROTAGONIST", COL_BODY)
    draw_text(draw, 12, 24, "HP 42/42", COL_MUTED)
    draw.rectangle([160, 8, 232, 40], outline=rgb15(31, 8, 8))
    draw_text(draw, 164, 12, "TUTSIL", COL_BODY)
    # Expanded Skills panel (proposed): y=100..159 (60px) — was 112/48
    draw.rectangle([0, 100, W - 1, H - 1], fill=COL_PANEL)
    draw.rectangle([0, 100, W - 1, H - 1], outline=COL_BORDER)
    draw_text(draw, 8, 102, title, COL_TITLE)
    return im, draw


def draw_skill_card(
    draw: ImageDraw.ImageDraw,
    *,
    name: str,
    element: str,
    category: str,
    power_label: str,
    sc_label: str,
    target: str,
    effect: str,
    slot_hint: str,
    content_dx: int = 0,
) -> None:
    # Card frame stationary
    card = (8, 112, 232, 154)
    draw.rectangle(card, fill=COL_CARD, outline=COL_BORDER)
    clip = (10, 220)  # clipX, clipW inside card
    # Moving content origin
    x0 = 12 + content_dx
    draw_text(draw, x0, 114, trunc(name, 216), COL_ACCENT, clip=clip)
    meta = f"{element}  {category}  {power_label}"
    draw_text(draw, x0, 124, trunc(meta, 216), COL_BODY, clip=clip)
    draw_text(draw, x0, 134, trunc(f"{sc_label}  {target}", 216), COL_MUTED, clip=clip)
    draw_text(draw, x0, 144, trunc(effect, 216), COL_BODY, clip=clip)
    # Stationary nav
    draw_text(draw, 180, 102, slot_hint, COL_MUTED)
    draw_text(draw, 8, 156, "A:USE  B:BACK  < >", COL_MUTED)


def preview_battle_damage() -> None:
    im, draw = battle_base("Skills")
    draw_skill_card(
        draw,
        name="Direct Attack",
        element="NEUTRAL",
        category="PHYSICAL",
        power_label="PWR 20",
        sc_label="SC 14/14",
        target="1 ENEMY",
        effect="Contact strike.",
        slot_hint="1/3",
    )
    im.save(OUT / "01_battle_skills_damage.png")


def preview_battle_support() -> None:
    im, draw = battle_base("Skills")
    draw_skill_card(
        draw,
        name="Long Shot",
        element="NEUTRAL",
        category="PHYSICAL",
        power_label="BASE 10~",
        sc_label="SC 8/12",
        target="1 ENEMY",
        effect="Power scales with distance.",
        slot_hint="2/3",
    )
    im.save(OUT / "02_battle_skills_variable.png")


def preview_party_detail() -> None:
    im = Image.new("RGB", (W, H), COL_PARTY_BG)
    draw = ImageDraw.Draw(im)
    # Shell 8,8 224x144
    draw.rectangle([8, 8, 231, 151], fill=COL_PARTY_PANEL, outline=COL_PARTY_BORDER)
    draw.line([120, 9, 120, 97], fill=rgb15(10, 10, 24))
    draw.line([9, 98, 230, 98], fill=rgb15(10, 10, 24))
    # Left moves overview (compact) with highlight on row 1
    draw_text(draw, 18, 12, "MOVES", COL_TITLE)
    rows = [
        ("DIRECT ATT.", "SC14", False),
        ("HOLD BACK", "SC16", True),
        ("INFLUENCE", "SC8", False),
        ("EMPTY", "--", False),
    ]
    y = 28
    for name, sc, sel in rows:
        if sel:
            draw.rectangle([16, y - 1, 116, y + 9], outline=COL_ACCENT)
        draw_text(draw, 22, y, name, COL_BODY if sel else COL_MUTED)
        draw_text(draw, 94, y, sc, COL_MUTED)
        y += 14
    # Right dial stub
    draw.rectangle([130, 40, 210, 58], fill=rgb15(12, 12, 24), outline=COL_TITLE)
    draw_text(draw, 138, 44, "PROTAGONIST", COL_BODY)
    # Description / full detail region
    draw_text(draw, 12, 102, "Hold Back - Supporting", COL_TITLE)
    draw_text(draw, 12, 114, "NEUTRAL  SUPPORT  --", COL_BODY)
    draw_text(draw, 12, 124, "SC 16/16  SELF", COL_MUTED)
    draw_text(draw, 12, 134, "Take 25% less damage 1 turn.", COL_BODY)
    draw_text(draw, 12, 144, "Next attack hits all enemies.", COL_BODY)
    draw_text(draw, 150, 144, "< > SLOT  B", COL_MUTED)
    im.save(OUT / "03_party_move_detail.png")


def preview_custom_candidate() -> None:
    im = Image.new("RGB", (W, H), COL_BG)
    draw = ImageDraw.Draw(im)
    draw_text(draw, 8, 8, "CUSTOM BATTLE - MOVES", COL_TITLE)
    draw_text(draw, 8, 20, "EDIT SLOT 2/4", COL_ACCENT)
    draw_text(draw, 140, 20, "CAND 3/10", COL_MUTED)
    # Candidate card
    draw.rectangle([8, 40, 232, 120], fill=COL_CARD, outline=COL_BORDER)
    draw_text(draw, 12, 44, "Temperature Difference", COL_ACCENT)
    # 22 chars * 6 = 132px — fits in 220
    assert measure("Temperature Difference") == 22 * ADV
    draw_text(draw, 12, 56, "NEUTRAL  SUPPORT  --", COL_BODY)
    draw_text(draw, 12, 68, "PREVIEW SC 16/16  SELF", COL_MUTED)
    draw_text(draw, 12, 80, "Next move becomes Wind.", COL_BODY)
    draw_text(draw, 12, 92, "A:ASSIGN  B:CANCEL  < >", COL_MUTED)
    # Equipped strip reminder
    draw_text(draw, 8, 130, "SLOT1:DIRECT  SLOT2:?  SLOT3:--  SLOT4:--", COL_MUTED)
    draw_text(draw, 8, 144, "LOOKS GOOD! on prior screen", COL_MUTED)
    im.save(OUT / "04_custom_candidate.png")


def preview_anim_strip() -> None:
    """Optional strip showing settle offsets for documentation."""
    offsets = frame_offsets(220)
    im = Image.new("RGB", (W, 48), COL_PANEL)
    draw = ImageDraw.Draw(im)
    draw_text(draw, 4, 4, "ANIM DX ease-out travel=220", COL_TITLE)
    draw_text(draw, 4, 16, " ".join(f"{o:3d}" for o in offsets[:6]), COL_BODY)
    draw_text(draw, 4, 28, " ".join(f"{o:3d}" for o in offsets[6:]), COL_BODY)
    im.save(OUT / "05_anim_offset_table.png")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    preview_battle_damage()
    preview_battle_support()
    preview_party_detail()
    preview_custom_candidate()
    preview_anim_strip()
    print(f"wrote previews to {OUT}")
    for p in sorted(OUT.glob("*.png")):
        print(f"  {p.name} {p.stat().st_size}")


if __name__ == "__main__":
    main()
