#!/usr/bin/env python3
"""Milestone 0 validation for dialogue resource reservations and source assets."""

from __future__ import annotations

import re
import unittest
from dataclasses import dataclass
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover - environment without Pillow
    Image = None  # type: ignore[assignment,misc]

ROOT = Path(__file__).resolve().parents[2]
RESOURCES_H = ROOT / "include" / "dialogue" / "dialogue_resources.h"
VIDEO_H = ROOT / "include" / "core" / "video.h"
GBA_TYPES_H = ROOT / "include" / "core" / "gba_types.h"
OAM_H = ROOT / "include" / "core" / "oam.h"
PLAYER_H = ROOT / "include" / "assets" / "overworld_player_sprite.h"

DIALOGUE_UI_PNG = ROOT / "assets_src" / "ui" / "Dialogue Ui.png"
AKSIL_PNG = ROOT / "assets_src" / "sprites" / "aksil_front_sprite.png"
PROTAGONIST_PNG = ROOT / "assets_src" / "sprites" / "protagonist_front_sprite.png"

BG_VRAM_CHARBLOCK_BYTE_SIZE = 0x4000
BG_VRAM_SCREENBLOCK_BYTE_SIZE = 0x800
BG_VRAM_CHARBLOCK_SCREENBLOCK_SPAN = 8


@dataclass(frozen=True)
class BgVramInterval:
  name: str
  start: int
  end: int

  def overlaps(self, other: BgVramInterval) -> bool:
    return self.start < other.end and other.start < self.end


def charblock_interval(index: int, name: str) -> BgVramInterval:
  start = index * BG_VRAM_CHARBLOCK_BYTE_SIZE
  return BgVramInterval(name, start, start + BG_VRAM_CHARBLOCK_BYTE_SIZE)


def screenblock_interval(index: int, name: str) -> BgVramInterval:
  start = index * BG_VRAM_SCREENBLOCK_BYTE_SIZE
  return BgVramInterval(name, start, start + BG_VRAM_SCREENBLOCK_BYTE_SIZE)


def screenblock_outside_charblock(screenblock: int, charblock: int) -> bool:
  screen = screenblock_interval(screenblock, "screen")
  char = charblock_interval(charblock, "char")
  return not screen.overlaps(char)


def parse_define(path: Path, name: str, constants: dict[str, int] | None = None) -> int:
  text = path.read_text(encoding="utf-8")
  pattern = re.compile(
    rf"#define\s+{re.escape(name)}\s+(.+?)(?=\n#define|\n\n|\Z)",
    re.DOTALL,
  )
  match = pattern.search(text)
  if not match:
    raise AssertionError(f"missing #define {name} in {path}")
  expression = " ".join(match.group(1).split())
  expression = re.sub(r"/\*.*?\*/", "", expression).strip()
  expression = re.sub(r"//.*$", "", expression).strip()
  if re.fullmatch(r"[A-Z0-9_]+", expression) and constants and expression in constants:
    return constants[expression]
  if re.fullmatch(r"[0-9]+", expression):
    return int(expression)
  if re.fullmatch(r"[0-9+\-*/() ]+", expression):
    return int(eval(expression, {"__builtins__": {}}, constants or {}))
  raise AssertionError(f"unsupported #define expression for {name}: {expression}")


def audit_png(path: Path) -> dict[str, object]:
  if Image is None:
    raise unittest.SkipTest("Pillow is required for PNG asset audit tests")
  image = Image.open(path).convert("RGBA")
  width, height = image.size
  pixels = image.load()
  colors: set[tuple[int, int, int, int]] = set()
  opaque_coords: list[tuple[int, int]] = []
  for y in range(height):
    for x in range(width):
      rgba = pixels[x, y]
      colors.add(rgba)
      if rgba[3] > 0:
        opaque_coords.append((x, y))
  if opaque_coords:
    xs = [coord[0] for coord in opaque_coords]
    ys = [coord[1] for coord in opaque_coords]
    bounds = (min(xs), min(ys), max(xs) + 1, max(ys) + 1)
  else:
    bounds = (0, 0, 0, 0)
  opaque_colors = sum(1 for color in colors if color[3] > 0)
  alpha_values = sorted({color[3] for color in colors})
  tiles = ((width + 7) // 8) * ((height + 7) // 8)
  return {
    "width": width,
    "height": height,
    "rgba_count": len(colors),
    "opaque_count": opaque_colors,
    "alpha_values": alpha_values,
    "opaque_bounds": bounds,
    "tile_padded_4bpp_bytes": tiles * 32,
    "tile_padded_8bpp_bytes": tiles * 64,
  }


class BgVramAliasRegressionTests(unittest.TestCase):
  def test_charblock_screenblock_alias_boundaries(self) -> None:
    cb0 = charblock_interval(0, "cb0")
    self.assertTrue(screenblock_interval(0, "sb0").overlaps(cb0))
    self.assertTrue(screenblock_interval(7, "sb7").overlaps(cb0))
    self.assertFalse(screenblock_interval(8, "sb8").overlaps(cb0))

    cb1 = charblock_interval(1, "cb1")
    self.assertTrue(screenblock_interval(8, "sb8").overlaps(cb1))
    self.assertTrue(screenblock_interval(15, "sb15").overlaps(cb1))
    self.assertFalse(screenblock_interval(16, "sb16").overlaps(cb1))

  def test_old_unsafe_screenblock_zero_would_overlap_field_charblock(self) -> None:
    field_char = charblock_interval(0, "field_char")
    unsafe_dialogue_screen = screenblock_interval(0, "unsafe_dialogue_screen")
    self.assertTrue(unsafe_dialogue_screen.overlaps(field_char))


class DialogueResourceReservationTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    constants = {
      "SCREEN_WIDTH": parse_define(GBA_TYPES_H, "SCREEN_WIDTH"),
      "SCREEN_HEIGHT": parse_define(GBA_TYPES_H, "SCREEN_HEIGHT"),
    }
    cls.dialogue_cb = parse_define(RESOURCES_H, "DIALOGUE_MODE0_BG_CHARBLOCK", constants)
    cls.dialogue_sb = parse_define(RESOURCES_H, "DIALOGUE_MODE0_BG_SCREENBLOCK", constants)
    cls.dialogue_palette_base = parse_define(
      RESOURCES_H, "DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE", constants
    )
    cls.dialogue_palette_count = parse_define(
      RESOURCES_H, "DIALOGUE_MODE0_BG_PALETTE_COLOR_COUNT", constants
    )
    cls.oam_left = parse_define(RESOURCES_H, "DIALOGUE_OAM_INDEX_LEFT_PORTRAIT", constants)
    cls.oam_right = parse_define(RESOURCES_H, "DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT", constants)
    cls.palette_left = parse_define(RESOURCES_H, "DIALOGUE_OBJ_PALETTE_BANK_LEFT", constants)
    cls.palette_right = parse_define(RESOURCES_H, "DIALOGUE_OBJ_PALETTE_BANK_RIGHT", constants)
    panel_w = parse_define(RESOURCES_H, "DIALOGUE_UI_PANEL_W", constants)
    panel_h = parse_define(RESOURCES_H, "DIALOGUE_UI_PANEL_H", constants)
    cls.panel_tile_count = ((panel_w + 7) // 8) * ((panel_h + 7) // 8)
    cls.charblock_capacity = parse_define(
      RESOURCES_H, "DIALOGUE_MODE0_CHARBLOCK_TILE_CAPACITY", constants
    )

    cls.field_cb = parse_define(VIDEO_H, "VIDEO_MODE0_FIELD_CHARBLOCK", constants)
    cls.field_sb = parse_define(VIDEO_H, "VIDEO_MODE0_FIELD_SCREENBLOCK", constants)
    cls.player_oam = parse_define(OAM_H, "OAM_INDEX_OVERWORLD_PLAYER", constants)
    cls.player_palette = parse_define(
      PLAYER_H, "OVERWORLD_PLAYER_SPRITE_PALETTE_BANK", constants
    )

    cls.field_char = charblock_interval(cls.field_cb, "field_char")
    cls.field_screen = screenblock_interval(cls.field_sb, "field_screen")
    cls.dialogue_char = charblock_interval(cls.dialogue_cb, "dialogue_char")
    cls.dialogue_screen = screenblock_interval(cls.dialogue_sb, "dialogue_screen")

  def test_project_reservations_do_not_share_physical_bg_vram(self) -> None:
    allocations = [
      self.field_char,
      self.field_screen,
      self.dialogue_char,
      self.dialogue_screen,
    ]
    for left in allocations:
      for right in allocations:
        if left is right:
          continue
        self.assertFalse(
          left.overlaps(right),
          f"{left.name} [{left.start:#x}, {left.end:#x}) overlaps "
          f"{right.name} [{right.start:#x}, {right.end:#x})",
        )

  def test_dialogue_screenblock_outside_occupied_charblocks(self) -> None:
    self.assertTrue(
      screenblock_outside_charblock(self.dialogue_sb, self.field_cb),
      "dialogue screenblock must lie outside field charblock span",
    )
    self.assertTrue(
      screenblock_outside_charblock(self.dialogue_sb, self.dialogue_cb),
      "dialogue screenblock must lie outside dialogue charblock span",
    )
    self.assertTrue(
      screenblock_outside_charblock(self.field_sb, self.field_cb),
      "field screenblock must lie outside field charblock span",
    )
    self.assertTrue(
      screenblock_outside_charblock(self.field_sb, self.dialogue_cb),
      "field screenblock must lie outside dialogue charblock span",
    )

  def test_dialogue_screenblock_is_not_zero(self) -> None:
    self.assertNotEqual(self.dialogue_sb, 0)

  def test_panel_tiles_fit_charblock(self) -> None:
    self.assertLessEqual(self.panel_tile_count, self.charblock_capacity)

  def test_bg_palette_range_within_hardware(self) -> None:
    self.assertGreaterEqual(self.dialogue_palette_base, 0)
    self.assertLessEqual(
      self.dialogue_palette_base + self.dialogue_palette_count, 256
    )
    self.assertGreaterEqual(self.dialogue_palette_base, 16)

  def test_oam_indices_do_not_overlap_player(self) -> None:
    self.assertNotEqual(self.oam_left, self.player_oam)
    self.assertNotEqual(self.oam_right, self.player_oam)
    self.assertNotEqual(self.oam_left, self.oam_right)
    self.assertGreaterEqual(self.oam_left, 0)
    self.assertLess(self.oam_left, 128)
    self.assertGreaterEqual(self.oam_right, 0)
    self.assertLess(self.oam_right, 128)

  def test_obj_palette_banks_do_not_overlap_player(self) -> None:
    self.assertNotEqual(self.palette_left, self.player_palette)
    self.assertNotEqual(self.palette_right, self.player_palette)
    self.assertNotEqual(self.palette_left, self.palette_right)
    self.assertGreaterEqual(self.palette_left, 0)
    self.assertLessEqual(self.palette_left, 15)
    self.assertGreaterEqual(self.palette_right, 0)
    self.assertLessEqual(self.palette_right, 15)


class DialogueAssetAuditTests(unittest.TestCase):
  def test_dialogue_ui_png_metadata(self) -> None:
    meta = audit_png(DIALOGUE_UI_PNG)
    self.assertEqual(meta["width"], 240)
    self.assertEqual(meta["height"], 160)
    self.assertEqual(meta["rgba_count"], 5)
    self.assertEqual(meta["opaque_count"], 4)
    self.assertEqual(meta["alpha_values"], [0, 255])
    self.assertEqual(meta["opaque_bounds"], (0, 108, 240, 159))
    self.assertEqual(meta["tile_padded_4bpp_bytes"], 19200)

  def test_aksil_png_metadata(self) -> None:
    meta = audit_png(AKSIL_PNG)
    self.assertEqual(meta["width"], 64)
    self.assertEqual(meta["height"], 64)
    self.assertEqual(meta["rgba_count"], 24)
    self.assertEqual(meta["opaque_count"], 23)
    self.assertEqual(meta["opaque_bounds"], (19, 3, 42, 63))
    # Valid single 64x64 OBJ after normalization.
    self.assertEqual(meta["tile_padded_4bpp_bytes"], 2048)
    self.assertEqual(meta["tile_padded_8bpp_bytes"], 4096)

  def test_protagonist_png_metadata(self) -> None:
    meta = audit_png(PROTAGONIST_PNG)
    self.assertEqual(meta["width"], 66)
    self.assertEqual(meta["height"], 66)
    self.assertEqual(meta["rgba_count"], 31)
    self.assertEqual(meta["opaque_count"], 30)
    self.assertEqual(meta["opaque_bounds"], (20, 4, 54, 64))
    # Source canvas exceeds hardware OBJ size; values are tile-padded staging estimates.
    self.assertEqual(meta["tile_padded_4bpp_bytes"], 2592)
    self.assertEqual(meta["tile_padded_8bpp_bytes"], 5184)
    self.assertNotEqual(meta["width"], 64)
    self.assertNotEqual(meta["height"], 64)


if __name__ == "__main__":
  unittest.main()
