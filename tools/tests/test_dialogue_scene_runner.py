#!/usr/bin/env python3
"""Milestone 6: scene runner host C tests and shared-scene contracts."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOST_TEST = ROOT / "tools" / "host_tests" / "test_dialogue_scene_runner_host.c"
HOST_STUBS = ROOT / "tools" / "host_tests" / "host_dialogue_scene_stubs.c"
HOST_VIDEO = ROOT / "tools" / "host_tests" / "host_video_stub.c"
SCENE_C = ROOT / "src" / "dialogue" / "dialogue_scene.c"
RUNNER_C = ROOT / "src" / "dialogue" / "dialogue_scene_runner.c"
PRINTER_C = ROOT / "src" / "dialogue" / "dialogue_text_printer.c"
DEMO_C = ROOT / "src" / "dialogue" / "dialogue_demo.c"
MAIN_C = ROOT / "src" / "main.c"
OVERWORLD_C = ROOT / "src" / "overworld" / "overworld.c"
OVERWORLD_H = ROOT / "include" / "overworld" / "overworld.h"


def _gcc() -> str:
    gcc = shutil.which("gcc")
    if not gcc:
        raise AssertionError("gcc required for host dialogue scene tests")
    return gcc


class DialogueSceneRunnerHostTests(unittest.TestCase):
    def test_host_c_scene_runner(self) -> None:
        gcc = _gcc()
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / ("scene_runner_test.exe" if os.name == "nt" else "scene_runner_test")
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
                    str(HOST_STUBS),
                    str(HOST_VIDEO),
                    str(SCENE_C),
                    str(RUNNER_C),
                    str(PRINTER_C),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(build.returncode, 0, build.stdout + "\n" + build.stderr)
            run = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + "\n" + run.stderr)
            self.assertIn("all host dialogue scene runner tests passed", run.stdout)


class DialogueSceneContractTests(unittest.TestCase):
    def test_shared_scene_used_by_both_hosts(self) -> None:
        scene = SCENE_C.read_text(encoding="utf-8")
        demo = DEMO_C.read_text(encoding="utf-8")
        main = MAIN_C.read_text(encoding="utf-8")
        self.assertIn("DialogueScene_GetSharedConversation", scene)
        self.assertIn("YOU THOUGHT THAT WOULD WORK?", scene)
        self.assertIn("IT ALMOST DID.", scene)
        self.assertIn("DIALOGUE_EVENT_TEST_COMPLETE", scene)
        self.assertIn("DialogueScene_GetSharedConversation", demo)
        self.assertIn("DialogueScene_GetSharedConversation", main)
        self.assertIn("DIALOGUE_OVERLAY_MODE_FIELD", main)
        self.assertIn("OVERWORLD_UPDATE_START_DIALOGUE", main)

    def test_overworld_dialogue_trigger_documented(self) -> None:
        header = OVERWORLD_H.read_text(encoding="utf-8")
        source = OVERWORLD_C.read_text(encoding="utf-8")
        self.assertIn("OVERWORLD_DIALOGUE_TRIGGER_LEFT 144", header)
        self.assertIn("Overworld_LockForDialogue", header)
        self.assertIn("Overworld_UnlockAfterDialogue", source)
        self.assertIn("s_dialogueInteractLatch", source)


if __name__ == "__main__":
    unittest.main()
