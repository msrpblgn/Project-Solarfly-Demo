#!/usr/bin/env python3
"""Milestone 3/4: host C printer + panel dirty tests and demo contract checks."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PRINTER_C = ROOT / "src" / "dialogue" / "dialogue_text_printer.c"
PRINTER_H = ROOT / "include" / "dialogue" / "dialogue_text_printer.h"
DEMO_C = ROOT / "src" / "dialogue" / "dialogue_demo.c"
PANEL_C = ROOT / "src" / "dialogue" / "dialogue_panel.c"
PLAN_H = ROOT / "include" / "dialogue" / "dialogue_demo_plan.h"
HOST_TEST = ROOT / "tools" / "host_tests" / "test_dialogue_text_printer_host.c"
HOST_DIRTY = ROOT / "tools" / "host_tests" / "test_dialogue_panel_dirty_host.c"
HOST_STUB = ROOT / "tools" / "host_tests" / "host_video_stub.c"
HOST_VRAM = ROOT / "tools" / "host_tests" / "host_vram_shim.c"
FRAME_C = ROOT / "src" / "assets" / "dialogue_frame.c"
UI_ONLY_A = ROOT / "assets_src" / "ui" / "dialogue" / "build" / "ui_only_refs" / "dialogue_ui_only_sample_a.png"
UI_ONLY_B = ROOT / "assets_src" / "ui" / "dialogue" / "build" / "ui_only_refs" / "dialogue_ui_only_sample_b.png"


def _gcc() -> str:
    gcc = shutil.which("gcc")
    if not gcc:
        raise AssertionError("gcc required for host dialogue tests")
    return gcc


class DialogueTextPrinterHostTests(unittest.TestCase):
    def test_host_c_printer_and_plan(self) -> None:
        gcc = _gcc()
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / ("printer_test.exe" if os.name == "nt" else "printer_test")
            build = subprocess.run(
                [
                    gcc,
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-I",
                    str(ROOT / "include"),
                    "-I",
                    str(ROOT / "tools" / "host_tests"),
                    "-o",
                    str(exe),
                    str(HOST_TEST),
                    str(HOST_STUB),
                    str(PRINTER_C),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(build.returncode, 0, build.stdout + "\n" + build.stderr)
            run = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + "\n" + run.stderr)
            self.assertIn("all host dialogue text printer tests passed", run.stdout)


class DialoguePanelDirtyHostTests(unittest.TestCase):
    def test_four_tile_glyph_dirty_tracking(self) -> None:
        gcc = _gcc()
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / ("dirty_test.exe" if os.name == "nt" else "dirty_test")
            build = subprocess.run(
                [
                    gcc,
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-DDIALOGUE_HOST_TEST",
                    "-I",
                    str(ROOT / "include"),
                    "-I",
                    str(ROOT / "tools" / "host_tests"),
                    "-o",
                    str(exe),
                    str(HOST_DIRTY),
                    str(HOST_VRAM),
                    str(HOST_STUB),
                    str(PANEL_C),
                    str(FRAME_C),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(build.returncode, 0, build.stdout + "\n" + build.stderr)
            run = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + "\n" + run.stderr)
            self.assertIn("all host dialogue panel dirty tests passed", run.stdout)


class DialogueDemoContractTests(unittest.TestCase):
    def test_demo_hosts_shared_scene_runner(self) -> None:
        demo = DEMO_C.read_text(encoding="utf-8")
        self.assertIn("DialogueScene_GetSharedConversation", demo)
        self.assertIn("DialogueScene_GetReplacementTest", demo)
        self.assertIn("Dialogue_Start", demo)
        self.assertIn("Dialogue_Update", demo)
        self.assertIn("KEY_START", demo)
        self.assertIn("KEY_SELECT", demo)
        self.assertIn("CONVERSATION COMPLETE", demo)
        self.assertNotRegex(demo, r"JustPressed\(KEY_B\).*return 1")

    def test_panel_dirty_flush_path(self) -> None:
        panel = PANEL_C.read_text(encoding="utf-8")
        self.assertIn("dirtyFlags", panel)
        self.assertIn("DialoguePanel_FlushDirty", panel)
        self.assertIn("DialoguePanel_DrawTextCharColor", panel)
        self.assertIn("DIALOGUE_PANEL_DYNAMIC_TILE_MAX 192", panel)

    def test_printer_commands_and_timing(self) -> None:
        header = PRINTER_H.read_text(encoding="utf-8")
        self.assertIn("DIALOGUE_TEXT_OP_PAUSE", header)
        self.assertIn("DIALOGUE_TEXT_OP_WAIT", header)
        self.assertIn("DIALOGUE_TEXT_OP_PAGE", header)
        self.assertIn("Slow=4 / Normal=2 / Fast=1", header)
        self.assertIn("Automatic punctuation delays remain disabled", header)

    def test_ui_only_refs_still_present_for_pixel_baselines(self) -> None:
        self.assertTrue(UI_ONLY_A.is_file())
        self.assertTrue(UI_ONLY_B.is_file())

    def test_plan_header_updated(self) -> None:
        plan = PLAN_H.read_text(encoding="utf-8")
        self.assertIn("needsUpdate", plan)
        self.assertIn("passAPressed", plan)


if __name__ == "__main__":
    unittest.main()
