#!/usr/bin/env python3
"""Milestone 5: dialogue portrait assets, reservations, focus geometry."""

from __future__ import annotations

import hashlib
import re
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

RESOURCES_H = ROOT / "include" / "dialogue" / "dialogue_resources.h"
PORTRAITS_H = ROOT / "include" / "assets" / "dialogue_portraits.h"
PORTRAITS_C = ROOT / "src" / "assets" / "dialogue_portraits.c"
OAM_H = ROOT / "include" / "core" / "oam.h"
CONTROLLER_C = ROOT / "src" / "dialogue" / "dialogue_portrait_controller.c"
DEMO_C = ROOT / "src" / "dialogue" / "dialogue_demo.c"
CONVERTER = ROOT / "tools" / "convert_dialogue_portraits.py"

SOURCE_HASHES = {
    ROOT / "assets_src" / "sprites" / "aksil_front_sprite.png":
        "2df139faf57bc5de6183ef46dae789cd02aa3b7de95bf445266cd31ccc83970d",
    ROOT / "assets_src" / "sprites" / "protagonist_front_sprite.png":
        "5edfe6308c740594d0e8a33583b87cf7199eb73a08427c812a8f565d29de0863",
    ROOT / "assets_src" / "ui" / "dialogue" / "aksil_front_4bpp_candidate.png":
        "b5e3e9f330c796d0470a37f7eb7ef06d63275ec7b0fcce85d564d3b0c1c2a845",
    ROOT / "assets_src" / "ui" / "dialogue" / "protagonist_front_4bpp_candidate.png":
        "71d99b9ee4a950cc582e353cb2bc294a16f588ffb6fe3b576330bc3d63f7a1bd",
}


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_define(path: Path, name: str) -> int:
    text = path.read_text(encoding="utf-8")
    match = re.search(rf"#define\s+{re.escape(name)}\s+\(?(-?\d+)\)?", text)
    if not match:
        raise AssertionError(f"missing {name} in {path}")
    return int(match.group(1))


class DialoguePortraitAssetTests(unittest.TestCase):
    def test_source_hashes_unchanged(self) -> None:
        for path, expect in SOURCE_HASHES.items():
            self.assertEqual(sha256_file(path), expect, path.name)

    def test_converter_check(self) -> None:
        result = subprocess.run(
            [sys.executable, str(CONVERTER), "--check"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_tile_byte_counts(self) -> None:
        self.assertEqual(parse_define(PORTRAITS_H, "DIALOGUE_PORTRAIT_TILE_COUNT"), 64)
        header = PORTRAITS_H.read_text(encoding="utf-8")
        self.assertIn("DIALOGUE_PORTRAIT_TILE_BYTES", header)
        self.assertIn("DIALOGUE_PORTRAIT_TILE_COUNT * 32", header)
        self.assertTrue(PORTRAITS_C.is_file())
        text = PORTRAITS_C.read_text(encoding="utf-8")
        self.assertIn("gDialoguePortraitAksilTiles", text)
        self.assertIn("gDialoguePortraitProtagonistTiles", text)
        self.assertIn("InactivePalette", text)


class DialoguePortraitReservationTests(unittest.TestCase):
    def test_oam_and_palette_banks(self) -> None:
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_OAM_INDEX_LEFT_PORTRAIT"), 1)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT"), 2)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_OBJ_PALETTE_BANK_LEFT"), 1)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_OBJ_PALETTE_BANK_RIGHT"), 2)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT"), 3)
        left = parse_define(PORTRAITS_H, "DIALOGUE_OBJ_TILE_BASE_LEFT")
        right = parse_define(PORTRAITS_H, "DIALOGUE_OBJ_TILE_BASE_RIGHT")
        self.assertEqual(left, 64)
        self.assertEqual(right, 128)
        self.assertGreaterEqual(left, 8)  # after overworld player tiles 0..7
        self.assertEqual(right - left, 64)

    def test_clip_and_anchors(self) -> None:
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_CLIP_Y"), 116)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X"), -8)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y"), 36)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X"), -16)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X"), -128)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X"), 240)

    def test_affine_api_present(self) -> None:
        text = OAM_H.read_text(encoding="utf-8")
        self.assertIn("Oam_SetAffine", text)
        self.assertIn("Oam_SetAffineMatrix", text)
        self.assertIn("OAM_AFFINE_SCALE_2X", text)
        self.assertIn("OAM_SIZE_SQUARE_64X64", text)


class DialoguePortraitControllerContractTests(unittest.TestCase):
    def test_focus_table_and_window(self) -> None:
        text = CONTROLLER_C.read_text(encoding="utf-8")
        self.assertIn("-16, -14, -12, -10, -8", text)
        self.assertIn("116, 118, 120, 122, 124", text)
        self.assertIn("DIALOGUE_UI_PORTRAIT_CLIP_Y", text)
        self.assertIn("WIN_OBJ", text)
        self.assertIn("OAM_AFFINE_SCALE_2X", text)

    def test_demo_enables_obj_and_select(self) -> None:
        demo = DEMO_C.read_text(encoding="utf-8")
        self.assertIn("DCNT_OBJ", demo)
        self.assertIn("DCNT_WIN0", demo)
        self.assertIn("KEY_SELECT", demo)
        self.assertIn("DialoguePortrait_BeginTechTest", demo)
        self.assertIn("DialoguePortrait_BeginFocus", demo)


if __name__ == "__main__":
    unittest.main()
