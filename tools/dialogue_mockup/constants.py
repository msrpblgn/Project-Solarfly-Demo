"""Constants for dialogue behavior mockup (Milestone 1)."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DIALOGUE_DIR = ROOT / "assets_src" / "ui" / "dialogue"
BUILD_DIR = DIALOGUE_DIR / "build"
LAYER_DIR = BUILD_DIR / "layers"

SOURCE_FRAME = ROOT / "assets_src" / "ui" / "Dialogue Ui.png"
SOURCE_AKSIL = ROOT / "assets_src" / "sprites" / "aksil_front_sprite.png"
SOURCE_PROTAGONIST = ROOT / "assets_src" / "sprites" / "protagonist_front_sprite.png"

ASEPRITE_EXE = Path(
    r"C:\Program Files (x86)\Steam\steamapps\common\Aseprite\Aseprite.exe"
)

CANVAS_W = 240
CANVAS_H = 160

# Supplied dialogue frame palette (RGBA).
FRAME_TRANSPARENT = (0, 0, 0, 0)
FRAME_BLACK = (0, 0, 0, 255)
FRAME_ORANGE = (252, 156, 0, 255)
FRAME_GOLD = (253, 191, 90, 255)
FRAME_LIGHT_GOLD = (255, 211, 140, 255)
FRAME_PALETTE = [
    FRAME_TRANSPARENT,
    FRAME_BLACK,
    FRAME_ORANGE,
    FRAME_GOLD,
    FRAME_LIGHT_GOLD,
]

TEXT_START_X = 12
TEXT_LINE1_Y = 132
TEXT_LINE2_Y = 144
TEXT_MAX_END_X = 221
TEXT_COLOR = FRAME_BLACK
CHAR_ADVANCE = 6
GLYPH_W = 5
GLYPH_H = 7
MAX_TEXT_WIDTH = TEXT_MAX_END_X - TEXT_START_X + 1
MAX_CHARS_PER_LINE = MAX_TEXT_WIDTH // CHAR_ADVANCE

PROMPT_X0 = 228
PROMPT_Y0 = 149
PROMPT_W = 6
PROMPT_H = 6

# Stored source / 4bpp candidate size. Presentation is a separate 2x canvas.
PORTRAIT_SOURCE_W = 64
PORTRAIT_SOURCE_H = 64
PRESENTATION_SCALE = 2
PORTRAIT_W = PORTRAIT_SOURCE_W * PRESENTATION_SCALE
PORTRAIT_H = PORTRAIT_SOURCE_H * PRESENTATION_SCALE

# Dialogue frame opaque rows use half-open interval [FRAME_OPAQUE_Y0, FRAME_OPAQUE_Y1).
FRAME_OPAQUE_Y0 = 108
FRAME_OPAQUE_Y1 = 159
# First fully opaque panel row. Portrait leak-clip uses y < this for visible busts.
FRAME_SOLID_Y = 116
PORTRAIT_CLIP_Y1 = FRAME_SOLID_Y

# 128x128 bust anchors. Inactive sits 8px down and 8px outward from active.
# Chosen from 2x head/chest landmarks so heads stay on-screen and the solid
# panel row (y=116) cuts across the upper chest.
ACTIVE_LEFT = (-8, 36)
INACTIVE_LEFT = (-16, 44)
ACTIVE_RIGHT = (116, 36)
INACTIVE_RIGHT = (124, 44)

LEFT_BASE_X = -12
LEFT_BASE_Y = 40
RIGHT_BASE_X = 120
RIGHT_BASE_Y = 40
# Local presentation rows hidden by the solid panel edge (y = FRAME_SOLID_Y).
CHEST_CUTOFF_LOCAL_ACTIVE = FRAME_SOLID_Y - ACTIVE_LEFT[1]
CHEST_CUTOFF_LOCAL_INACTIVE = FRAME_SOLID_Y - INACTIVE_LEFT[1]

FOCUS_TRAVEL_INSET = 8
FOCUS_TRAVEL_UP = 8

LEFT_ENTER_OFFSCREEN_X = -PORTRAIT_W
RIGHT_ENTER_OFFSCREEN_X = CANVAS_W
LEFT_EXIT_OFFSCREEN_X = -PORTRAIT_W

# Five unique focus-exchange positions (palette swap at frame index 2).
FOCUS_POSITIONS: list[tuple[tuple[int, int], tuple[int, int]]] = [
    (INACTIVE_LEFT, ACTIVE_RIGHT),
    ((-14, 42), (118, 38)),
    ((-12, 40), (120, 40)),
    ((-10, 38), (122, 42)),
    (ACTIVE_LEFT, INACTIVE_RIGHT),
]
FOCUS_PALETTE_SWITCH_FRAME = 2

# Overworld diagnostic green RGB15(6,20,8) -> RGB888.
WORLD_PREVIEW_RGB = (49, 164, 65)
BACKGROUND_FILL_RGB = (32, 96, 48)

NAMEPLATE_PAD_X = 4
NAMEPLATE_PAD_Y = 2
NAMEPLATE_HEIGHT = GLYPH_H + NAMEPLATE_PAD_Y * 2

MOTION_ENTER_FRAMES = 10
MOTION_FPS = 60
# Exact 60 fps logical step; GIF preview timing is handled in gif_timing.py.
LOGICAL_FRAME_MS = 1000.0 / MOTION_FPS
NATIVE_PREVIEW_SPEED = 1.0
REVIEW_4X_SPEED = 12.0  # 12 * (1000/60) = 200 ms per logical frame
FOCUS_FRAMES = 5
REPLACEMENT_FRAMES = 10

LAYER_NAMES = [
    "GUIDES_do_not_export",
    "ADVANCE_PROMPT",
    "TEXT",
    "NAMEPLATE",
    "FRAME_FRONT_MASK",
    "PORTRAIT_RIGHT",
    "PORTRAIT_LEFT",
    "WORLD_PREVIEW_do_not_export",
    "BACKGROUND_FILL_do_not_export",
]
