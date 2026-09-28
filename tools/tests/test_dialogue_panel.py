#!/usr/bin/env python3
"""Milestone 2 tests: dialogue frame conversion, font punctuation, UI geometry."""

from __future__ import annotations

import hashlib
import re
import subprocess
import sys
import unittest
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.convert_dialogue_frame import (  # noqa: E402
    SOURCE_HASH,
    convert,
    rgb15_from_rgb888,
)
from tools.dialogue_mockup.constants import (  # noqa: E402
    NAMEPLATE_HEIGHT,
    PROMPT_X0,
    PROMPT_Y0,
    TEXT_LINE1_Y,
    TEXT_LINE2_Y,
    TEXT_MAX_END_X,
    TEXT_START_X,
)
from tools.dialogue_mockup.font5x7 import GLYPH_CHARS, measure_text_width  # noqa: E402

DIALOGUE_UI = ROOT / "assets_src" / "ui" / "Dialogue Ui.png"
FRAME_C = ROOT / "src" / "assets" / "dialogue_frame.c"
FRAME_H = ROOT / "include" / "assets" / "dialogue_frame.h"
VIDEO_C = ROOT / "src" / "core" / "video.c"
RESOURCES_H = ROOT / "include" / "dialogue" / "dialogue_resources.h"
UI_ONLY_A = ROOT / "assets_src" / "ui" / "dialogue" / "build" / "ui_only_refs" / "dialogue_ui_only_sample_a.png"
UI_ONLY_B = ROOT / "assets_src" / "ui" / "dialogue" / "build" / "ui_only_refs" / "dialogue_ui_only_sample_b.png"

SAMPLE_A_LINE1 = "YOU THOUGHT THAT WOULD WORK?"
SAMPLE_A_LINE2 = "COME ON. ONE MORE TRY."
SAMPLE_B_LINE1 = "IT ALMOST DID."


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_define(path: Path, name: str) -> int:
    text = path.read_text(encoding="utf-8")
    match = re.search(rf"#define\s+{re.escape(name)}\s+\(?(-?\d+)\)?", text)
    if not match:
        raise AssertionError(f"missing {name} in {path}")
    return int(match.group(1))


class DialogueFrameConversionTests(unittest.TestCase):
    def test_source_hash_unchanged(self) -> None:
        self.assertEqual(sha256_file(DIALOGUE_UI), SOURCE_HASH)

    def test_conversion_is_deterministic_and_fits_reservation(self) -> None:
        palette, tiles, map_entries, meta = convert(DIALOGUE_UI)
        self.assertEqual(len(palette), 16)
        self.assertEqual(palette[1], 0)  # black
        self.assertEqual(palette[2], rgb15_from_rgb888(252, 156, 0))
        self.assertEqual(palette[3], rgb15_from_rgb888(255, 211, 140))
        self.assertEqual(palette[4], rgb15_from_rgb888(253, 191, 90))
        self.assertLessEqual(len(tiles), 512)
        self.assertEqual(meta["unique_tiles"], len(tiles))
        self.assertEqual(len(map_entries), 32 * 32)

        again = convert(DIALOGUE_UI)
        self.assertEqual(tiles, again[1])
        self.assertEqual(map_entries, again[2])

    def test_generated_assets_match_converter(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools" / "convert_dialogue_frame.py"),
                "--check",
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue(FRAME_C.is_file())
        self.assertTrue(FRAME_H.is_file())
        self.assertIn("DIALOGUE_FRAME_TILE_COUNT", FRAME_H.read_text(encoding="utf-8"))


class DialogueFontPunctuationTests(unittest.TestCase):
    def test_engine_font_contains_question_mark_glyph(self) -> None:
        text = VIDEO_C.read_text(encoding="utf-8")
        self.assertIn("/* ? */", text)
        self.assertIn("0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04", text)
        self.assertIn("c == '?'", text)

    def test_sample_strings_fit_safe_area(self) -> None:
        max_width = TEXT_MAX_END_X - TEXT_START_X + 1
        self.assertLessEqual(measure_text_width(SAMPLE_A_LINE1), max_width)
        self.assertLessEqual(measure_text_width(SAMPLE_A_LINE2), max_width)
        self.assertLessEqual(measure_text_width(SAMPLE_B_LINE1), max_width)
        self.assertIn("?", GLYPH_CHARS)


class DialogueUiGeometryTests(unittest.TestCase):
    def test_resource_header_matches_approved_mockup_metrics(self) -> None:
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_TEXT_X"), TEXT_START_X)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_TEXT_LINE1_Y"), TEXT_LINE1_Y)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_TEXT_LINE2_Y"), TEXT_LINE2_Y)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_NAMEPLATE_Y"), 106)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_NAMEPLATE_HEIGHT"), NAMEPLATE_HEIGHT)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PROMPT_X"), PROMPT_X0)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PROMPT_Y"), PROMPT_Y0)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_PRESENTATION_W"), 128)
        self.assertEqual(parse_define(RESOURCES_H, "DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X"), -8)

    def test_ui_only_references_exist_and_exclude_portraits(self) -> None:
        self.assertTrue(UI_ONLY_A.is_file(), "run tools/dialogue_mockup/build_ui_only_refs.py")
        self.assertTrue(UI_ONLY_B.is_file())
        sample_a = Image.open(UI_ONLY_A).convert("RGBA")
        sample_b = Image.open(UI_ONLY_B).convert("RGBA")
        self.assertEqual(sample_a.size, (240, 160))
        self.assertEqual(sample_b.size, (240, 160))

        # Nameplate side swap: right tab present in A, left tab present in B.
        # Sample A AKSIL plate around x=194; Sample B PROTAGONIST around x=8.
        self.assertNotEqual(
            sample_a.getpixel((200, 106))[:3],
            sample_b.getpixel((200, 106))[:3],
        )
        # Second line cleared in sample B vs present in sample A.
        # Line 2 glyph tops at y=144; sample A has ink, sample B should not in text start area.
        a_ink = sum(
            1
            for x in range(TEXT_START_X, TEXT_START_X + 60)
            for y in range(TEXT_LINE2_Y, TEXT_LINE2_Y + 7)
            if sample_a.getpixel((x, y))[:3] == (0, 0, 0)
        )
        b_ink = sum(
            1
            for x in range(TEXT_START_X, TEXT_START_X + 60)
            for y in range(TEXT_LINE2_Y, TEXT_LINE2_Y + 7)
            if sample_b.getpixel((x, y))[:3] == (0, 0, 0)
        )
        self.assertGreater(a_ink, 20)
        self.assertEqual(b_ink, 0)

        # Question mark present on sample A line 1.
        q_x = TEXT_START_X + len("YOU THOUGHT THAT WOULD WORK") * 6
        q_ink = sum(
            1
            for col in range(5)
            for row in range(7)
            if sample_a.getpixel((q_x + col, TEXT_LINE1_Y + row))[:3] == (0, 0, 0)
        )
        self.assertGreater(q_ink, 4)


class DialogueSampleClearingLogicTests(unittest.TestCase):
    def test_sample_b_is_shorter_than_sample_a(self) -> None:
        self.assertGreater(len(SAMPLE_A_LINE1), len(SAMPLE_B_LINE1))
        self.assertTrue(SAMPLE_A_LINE2)
        self.assertEqual(SAMPLE_B_LINE1, "IT ALMOST DID.")


if __name__ == "__main__":
    unittest.main()
