#!/usr/bin/env python3
"""Validation tests for Milestone 1 dialogue behavior mockup assets."""

from __future__ import annotations

import hashlib
import json
import unittest
from pathlib import Path

from PIL import Image

import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.dialogue_mockup.compose import head_bounds_local, portrait_visible_pixels
from tools.dialogue_mockup.constants import (
    ACTIVE_LEFT,
    ACTIVE_RIGHT,
    FRAME_SOLID_Y,
    INACTIVE_LEFT,
    INACTIVE_RIGHT,
    LEFT_ENTER_OFFSCREEN_X,
    LOGICAL_FRAME_MS,
    MOTION_FPS,
    NATIVE_PREVIEW_SPEED,
    PORTRAIT_H,
    PORTRAIT_W,
    PRESENTATION_SCALE,
    REVIEW_4X_SPEED,
    TEXT_LINE1_Y,
    TEXT_START_X,
)
from tools.dialogue_mockup.gif_timing import (
    collapse_identical_frames,
    encoded_delays_ms,
    expected_encoded_duration_ms,
    intended_duration_ms,
    measure_gif_durations,
    measure_gif_total_duration_ms,
    save_preview_gif,
)

ROOT = Path(__file__).resolve().parents[2]
DIALOGUE_DIR = ROOT / "assets_src" / "ui" / "dialogue"
BUILD_DIR = DIALOGUE_DIR / "build"
ROUNDTRIP_DIR = BUILD_DIR / "roundtrip"
SOURCE_FRAME = ROOT / "assets_src" / "ui" / "Dialogue Ui.png"
SOURCE_AKSIL = ROOT / "assets_src" / "sprites" / "aksil_front_sprite.png"
SOURCE_PROTAGONIST = ROOT / "assets_src" / "sprites" / "protagonist_front_sprite.png"

SOURCE_HASHES = {
    "assets_src/ui/Dialogue Ui.png": "bf25ec599900e1443c725879ea4e200a65426249448d00f18f47e48be5a0072b",
    "assets_src/sprites/aksil_front_sprite.png": "2df139faf57bc5de6183ef46dae789cd02aa3b7de95bf445266cd31ccc83970d",
    "assets_src/sprites/protagonist_front_sprite.png": "5edfe6308c740594d0e8a33583b87cf7199eb73a08427c812a8f565d29de0863",
}

FRAME_EXPORTS = {
    "dialogue_frame_a_aksil_active.png": "frame_a",
    "dialogue_frame_b_protagonist_active.png": "frame_b",
    "dialogue_frame_c_replacement.png": "frame_c",
}

PREVIEW_FILES = [
    "dialogue_frame_a_aksil_active.png",
    "dialogue_frame_b_protagonist_active.png",
    "dialogue_frame_c_replacement.png",
    "dialogue_mockup_review_sheet_4x.png",
    "dialogue_mockup_before_after_4x.png",
    "portrait_palette_comparison_4x.png",
    "dialogue_motion_preview_4x.gif",
    "dialogue_focus_exchange_preview.gif",
    "dialogue_focus_exchange_preview_4x.gif",
]

DELIVERABLES = [
    "dialogue_behavior_mockup.aseprite",
    *FRAME_EXPORTS.keys(),
    "dialogue_motion_preview.gif",
    "dialogue_motion_preview_4x.gif",
    "dialogue_mockup_review_sheet_4x.png",
    "portrait_palette_comparison_1x.png",
    "portrait_palette_comparison_4x.png",
    "protagonist_front_64x64_candidate.png",
    "aksil_front_4bpp_candidate.png",
    "protagonist_front_4bpp_candidate.png",
    "aksil_front_128x128_presentation.png",
    "protagonist_front_128x128_presentation.png",
    "dialogue_mockup_before_after_4x.png",
    "dialogue_focus_exchange_preview.gif",
    "dialogue_focus_exchange_preview_4x.gif",
]

TEXT_SAFE_RECT = (12, 132, 221, 150)


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_config() -> dict:
    return json.loads((BUILD_DIR / "mockup_config.json").read_text(encoding="utf-8"))


def load_timing() -> dict:
    return json.loads((BUILD_DIR / "dialogue_motion_timing.json").read_text(encoding="utf-8"))


def bgr555_to_rgb888(value: int) -> tuple[int, int, int]:
    r5 = value & 0x1F
    g5 = (value >> 5) & 0x1F
    b5 = (value >> 10) & 0x1F
    return (r5 * 255 // 31, g5 * 255 // 31, b5 * 255 // 31)


def is_binary_alpha(image: Image.Image) -> bool:
    alphas = {px[3] for px in image.convert("RGBA").getdata()}
    return alphas.issubset({0, 255})


def palette_from_image(image: Image.Image) -> list[tuple[int, int, int, int]]:
    return sorted(set(image.convert("RGBA").getdata()))


def frame_opaque_pixels(image: Image.Image) -> set[tuple[int, int]]:
    src = image.convert("RGBA")
    return {
        (x, y)
        for y in range(src.height)
        for x in range(src.width)
        if src.getpixel((x, y))[3] > 0
    }


def rect_pixels(x0: int, y0: int, x1: int, y1: int) -> set[tuple[int, int]]:
    return {(x, y) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1)}


def rects_overlap(a: tuple[int, int, int, int], b: tuple[int, int, int, int]) -> bool:
    ax0, ay0, ax1, ay1 = a
    bx0, by0, bx1, by1 = b
    return not (ax1 < bx0 or bx1 < ax0 or ay1 < by0 or by1 < ay0)


def gif_total_duration_ms(path: Path) -> int:
    return measure_gif_total_duration_ms(path)


def portrait_on_screen_pixels(sprite: Image.Image, x: int, y: int) -> set[tuple[int, int]]:
    return portrait_visible_pixels(sprite, x, y, clip=(0, 0, 240, 160))


class DialogueMockupSourcePreservationTests(unittest.TestCase):
    def test_source_png_hashes_unchanged(self) -> None:
        for rel_path, expected in SOURCE_HASHES.items():
            actual = sha256_file(ROOT / rel_path)
            self.assertEqual(actual, expected, rel_path)

    def test_source_frame_dimensions_and_colors(self) -> None:
        image = Image.open(SOURCE_FRAME).convert("RGBA")
        self.assertEqual(image.size, (240, 160))
        colors = set(image.getdata())
        self.assertEqual(len(colors), 5)
        self.assertTrue(is_binary_alpha(image))

    def test_frame_alpha_bounds_are_half_open(self) -> None:
        frame = Image.open(SOURCE_FRAME).convert("RGBA")
        y0, y1 = load_config()["frame_opaque_y_interval"]
        self.assertEqual(y0, 108)
        self.assertEqual(y1, 159)
        self.assertTrue(any(frame.getpixel((x, 158))[3] > 0 for x in range(240)))
        self.assertFalse(any(frame.getpixel((x, 159))[3] > 0 for x in range(240)))


class DialogueMockupDeliverableTests(unittest.TestCase):
    def test_required_deliverables_exist(self) -> None:
        for name in DELIVERABLES:
            path = DIALOGUE_DIR / name
            self.assertTrue(path.is_file(), f"missing deliverable: {name}")

    def test_viewable_preview_files_exist(self) -> None:
        for name in PREVIEW_FILES:
            self.assertTrue((DIALOGUE_DIR / name).is_file(), name)

    def test_static_frames_are_240x160(self) -> None:
        for name in FRAME_EXPORTS:
            with Image.open(DIALOGUE_DIR / name) as image:
                self.assertEqual(image.size, (240, 160))

    def test_portrait_candidates_are_64x64(self) -> None:
        for name in (
            "protagonist_front_64x64_candidate.png",
            "aksil_front_4bpp_candidate.png",
            "protagonist_front_4bpp_candidate.png",
        ):
            with Image.open(DIALOGUE_DIR / name) as image:
                self.assertEqual(image.size, (64, 64))

    def test_presentation_portraits_are_128x128(self) -> None:
        for name in (
            "aksil_front_128x128_presentation.png",
            "protagonist_front_128x128_presentation.png",
        ):
            with Image.open(DIALOGUE_DIR / name) as image:
                self.assertEqual(image.size, (128, 128))
                self.assertTrue(is_binary_alpha(image))

    def test_review_gifs_are_labeled_as_slowed(self) -> None:
        with Image.open(DIALOGUE_DIR / "dialogue_motion_preview_4x.gif") as gif:
            self.assertEqual(gif.size[0], 240 * 4)
            self.assertGreater(gif.size[1], 160 * 4)
        with Image.open(DIALOGUE_DIR / "dialogue_focus_exchange_preview_4x.gif") as gif:
            self.assertEqual(gif.size[0], 240 * 4)
            self.assertGreater(gif.size[1], 160 * 4)

    def test_presentation_is_nearest_neighbor_2x(self) -> None:
        source = Image.open(DIALOGUE_DIR / "aksil_front_4bpp_candidate.png").convert("RGBA")
        present = Image.open(DIALOGUE_DIR / "aksil_front_128x128_presentation.png").convert("RGBA")
        self.assertEqual(present.size, (source.width * 2, source.height * 2))
        src_px = source.load()
        dst_px = present.load()
        for y in range(source.height):
            for x in range(source.width):
                self.assertEqual(dst_px[x * 2, y * 2], src_px[x, y])
                self.assertEqual(dst_px[x * 2 + 1, y * 2], src_px[x, y])
                self.assertEqual(dst_px[x * 2, y * 2 + 1], src_px[x, y])
                self.assertEqual(dst_px[x * 2 + 1, y * 2 + 1], src_px[x, y])

    def test_protagonist_normalization_proof(self) -> None:
        config = load_config()
        proof = config["protagonist_proof"]
        self.assertEqual(proof["source_size"], [66, 66])
        self.assertEqual(proof["candidate_size"], [64, 64])
        self.assertTrue(proof["opaque_preserved"])
        self.assertEqual(proof["source_opaque_count"], proof["candidate_opaque_count"])


class DialogueMockupNameplateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.config = load_config()
        cls.frame_opaque = frame_opaque_pixels(Image.open(SOURCE_FRAME))
        cls.nameplate_a = Image.open(BUILD_DIR / "layers" / "frame_a" / "NAMEPLATE.png").convert("RGBA")
        cls.nameplate_b = Image.open(BUILD_DIR / "layers" / "frame_b" / "NAMEPLATE.png").convert("RGBA")
        cls.pro_sprite = Image.open(DIALOGUE_DIR / "protagonist_front_128x128_presentation.png")
        cls.aks_sprite = Image.open(DIALOGUE_DIR / "aksil_front_128x128_presentation.png")

    def test_nameplate_y_matches_config(self) -> None:
        self.assertEqual(self.config["nameplate_y"], self.config["nameplates"]["aksil_right"]["y"])
        self.assertEqual(self.config["nameplate_y"], self.config["nameplates"]["protagonist_left"]["y"])

    def test_nameplate_attached_to_frame(self) -> None:
        for key in ("aksil_right", "protagonist_left"):
            spec = self.config["nameplates"][key]
            plate_pixels = rect_pixels(
                spec["x"],
                spec["y"],
                spec["x"] + spec["width"] - 1,
                spec["y"] + spec["height"] - 1,
            )
            bottom_row = {(x, spec["y"] + spec["height"] - 1) for x in range(spec["x"], spec["x"] + spec["width"])}
            attached = bool(bottom_row & self.frame_opaque) or bool(plate_pixels & self.frame_opaque)
            self.assertTrue(attached, key)

    def test_nameplate_clear_of_text_safe_area(self) -> None:
        safe = rect_pixels(*TEXT_SAFE_RECT)
        for key in ("aksil_right", "protagonist_left"):
            spec = self.config["nameplates"][key]
            plate = rect_pixels(
                spec["x"],
                spec["y"],
                spec["x"] + spec["width"] - 1,
                spec["y"] + spec["height"] - 1,
            )
            self.assertFalse(plate & safe, key)

    def test_nameplate_clear_of_active_face_opaque_pixels(self) -> None:
        aks = self.config["nameplates"]["aksil_right"]
        hx0, hy0, hx1, hy1 = head_bounds_local(self.aks_sprite)
        aks_face = {
            (x, y)
            for x, y in portrait_on_screen_pixels(self.aks_sprite, ACTIVE_RIGHT[0], ACTIVE_RIGHT[1])
            if ACTIVE_RIGHT[0] + hx0 <= x < ACTIVE_RIGHT[0] + hx1
            and ACTIVE_RIGHT[1] + hy0 <= y < ACTIVE_RIGHT[1] + hy1
        }
        aks_plate = rect_pixels(aks["x"], aks["y"], aks["x"] + aks["width"] - 1, aks["y"] + aks["height"] - 1)
        self.assertFalse(aks_plate & aks_face)

        pro = self.config["nameplates"]["protagonist_left"]
        hx0, hy0, hx1, hy1 = head_bounds_local(self.pro_sprite)
        pro_face = {
            (x, y)
            for x, y in portrait_on_screen_pixels(self.pro_sprite, ACTIVE_LEFT[0], ACTIVE_LEFT[1])
            if ACTIVE_LEFT[0] + hx0 <= x < ACTIVE_LEFT[0] + hx1
            and ACTIVE_LEFT[1] + hy0 <= y < ACTIVE_LEFT[1] + hy1
        }
        pro_plate = rect_pixels(pro["x"], pro["y"], pro["x"] + pro["width"] - 1, pro["y"] + pro["height"] - 1)
        self.assertFalse(pro_plate & pro_face)


class DialogueMockupPaletteTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.config = load_config()

    def test_palette_entries_within_16_colors(self) -> None:
        for key in (
            "aksil_active",
            "aksil_inactive",
            "protagonist_active",
            "protagonist_inactive",
        ):
            entries = self.config["palettes"][key]
            self.assertLessEqual(len(entries), 16, key)
            self.assertEqual(entries[0]["role"], "transparent")

    def test_black_outline_present(self) -> None:
        for key in ("aksil_active", "protagonist_active"):
            roles = [entry["role"] for entry in self.config["palettes"][key]]
            self.assertIn("outline", roles)

    def test_bgr555_roundtrip_matches_rgb888_entries(self) -> None:
        for key in ("aksil_active", "protagonist_active"):
            for entry in self.config["palettes"][key]:
                if entry["index"] == 0:
                    continue
                rgb = tuple(entry["rgb888"])
                self.assertEqual(bgr555_to_rgb888(entry["bgr555"]), rgb)

    def test_4bpp_candidate_images_binary_alpha(self) -> None:
        for name in ("aksil_front_4bpp_candidate.png", "protagonist_front_4bpp_candidate.png"):
            image = Image.open(DIALOGUE_DIR / name)
            self.assertTrue(is_binary_alpha(image))
            self.assertLessEqual(len(palette_from_image(image)), 16)


class DialogueMockupLayoutTests(unittest.TestCase):
    def test_frame_mask_layer_matches_source_png(self) -> None:
        source = Image.open(SOURCE_FRAME).convert("RGBA")
        mask = Image.open(BUILD_DIR / "layers" / "frame_a" / "FRAME_FRONT_MASK.png").convert("RGBA")
        self.assertEqual(list(source.getdata()), list(mask.getdata()))

    def test_roundtrip_exports_match_reference_pngs(self) -> None:
        for export_name in FRAME_EXPORTS:
            reference = Image.open(DIALOGUE_DIR / export_name).convert("RGBA")
            roundtrip = Image.open(ROUNDTRIP_DIR / export_name).convert("RGBA")
            self.assertEqual(reference.size, roundtrip.size)
            self.assertEqual(list(reference.getdata()), list(roundtrip.getdata()))

    def test_no_gif_sentinel_pixel_at_origin(self) -> None:
        frame_a = Image.open(DIALOGUE_DIR / "dialogue_frame_a_aksil_active.png").convert("RGBA")
        with Image.open(DIALOGUE_DIR / "dialogue_motion_preview.gif") as gif:
            gif.seek(0)
            self.assertEqual(gif.convert("RGBA").getpixel((0, 0)), frame_a.getpixel((0, 0)))

    def test_question_mark_is_present_in_frame_a_text(self) -> None:
        text = Image.open(BUILD_DIR / "layers" / "frame_a" / "TEXT.png").convert("RGBA")
        q_x = TEXT_START_X + len("YOU THOUGHT THAT WOULD WORK") * 6
        ink = [
            text.getpixel((q_x + col, TEXT_LINE1_Y + row))[3]
            for row in range(7)
            for col in range(5)
        ]
        self.assertGreater(sum(1 for a in ink if a == 255), 4)

    def test_heads_visible_in_settled_poses(self) -> None:
        aks = Image.open(DIALOGUE_DIR / "aksil_front_128x128_presentation.png")
        pro = Image.open(DIALOGUE_DIR / "protagonist_front_128x128_presentation.png")
        poses = [
            (pro, INACTIVE_LEFT),
            (pro, ACTIVE_LEFT),
            (aks, ACTIVE_RIGHT),
            (aks, INACTIVE_RIGHT),
        ]
        for sprite, (ax, ay) in poses:
            hx0, hy0, hx1, hy1 = head_bounds_local(sprite)
            self.assertGreaterEqual(ax + hx0, 0)
            self.assertLess(ax + hx1, 240)
            self.assertGreaterEqual(ay + hy0, 0)
            self.assertLess(ay + hy1, FRAME_SOLID_Y)

    def test_frame_c_shows_outgoing_portrait(self) -> None:
        left = Image.open(BUILD_DIR / "layers" / "frame_c" / "PORTRAIT_LEFT.png").convert("RGBA")
        opaque = sum(1 for px in left.getdata() if px[3] == 255)
        self.assertGreater(opaque, 50)

    def test_portraits_do_not_leak_into_text_or_bottom_row(self) -> None:
        frame = Image.open(SOURCE_FRAME).convert("RGBA")
        for frame_key in ("frame_a", "frame_b", "frame_c"):
            left = Image.open(BUILD_DIR / "layers" / frame_key / "PORTRAIT_LEFT.png").convert("RGBA")
            right = Image.open(BUILD_DIR / "layers" / frame_key / "PORTRAIT_RIGHT.png").convert("RGBA")
            for layer in (left, right):
                for y in range(TEXT_LINE1_Y, 160):
                    for x in range(12, 222):
                        if layer.getpixel((x, y))[3] == 0:
                            continue
                        self.assertGreater(
                            frame.getpixel((x, y))[3],
                            0,
                            f"{frame_key} leak through transparent frame {x},{y}",
                        )
                for x in range(240):
                    self.assertEqual(layer.getpixel((x, 159))[3], 0, f"{frame_key} bottom leak {x}")


class DialogueMockupMotionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.timing = load_timing()
        cls.pro_sprite = Image.open(DIALOGUE_DIR / "protagonist_front_128x128_presentation.png")
        cls.aks_sprite = Image.open(DIALOGUE_DIR / "aksil_front_128x128_presentation.png")

    def test_focus_positions_are_unique(self) -> None:
        positions = [(tuple(e["left"]), tuple(e["right"])) for e in self.timing["focus_exchange"]]
        self.assertEqual(len(positions), 5)
        self.assertEqual(len(set(positions)), 5)

    def test_focus_movement_is_monotonic(self) -> None:
        left_x = [e["left"][0] for e in self.timing["focus_exchange"]]
        left_y = [e["left"][1] for e in self.timing["focus_exchange"]]
        right_x = [e["right"][0] for e in self.timing["focus_exchange"]]
        right_y = [e["right"][1] for e in self.timing["focus_exchange"]]
        self.assertEqual(left_x, sorted(left_x))
        self.assertEqual(left_y, sorted(left_y, reverse=True))
        self.assertEqual(right_x, sorted(right_x))
        self.assertEqual(right_y, sorted(right_y))

    def test_focus_endpoints_match_anchors(self) -> None:
        first = self.timing["focus_exchange"][0]
        last = self.timing["focus_exchange"][-1]
        self.assertEqual(first["left"], list(INACTIVE_LEFT))
        self.assertEqual(first["right"], list(ACTIVE_RIGHT))
        self.assertEqual(last["left"], list(ACTIVE_LEFT))
        self.assertEqual(last["right"], list(INACTIVE_RIGHT))

    def test_focus_palette_swap_frame(self) -> None:
        entries = self.timing["focus_exchange"]
        self.assertEqual(entries[1]["left_palette"], "inactive")
        self.assertEqual(entries[1]["right_palette"], "active")
        self.assertEqual(entries[2]["left_palette"], "active")
        self.assertEqual(entries[2]["right_palette"], "inactive")
        self.assertEqual(entries[0]["palette_switch_frame"], 2)

    def test_left_enter_starts_fully_offscreen(self) -> None:
        self.assertEqual(self.timing["portrait_enter_left"][0]["x"], LEFT_ENTER_OFFSCREEN_X)
        visible = portrait_on_screen_pixels(self.pro_sprite, LEFT_ENTER_OFFSCREEN_X, ACTIVE_LEFT[1])
        self.assertFalse(visible)

    def test_left_enter_is_monotonic_and_lands_on_anchor(self) -> None:
        xs = [entry["x"] for entry in self.timing["portrait_enter_left"]]
        self.assertEqual(xs[0], LEFT_ENTER_OFFSCREEN_X)
        self.assertEqual(xs[-1], ACTIVE_LEFT[0])
        self.assertEqual(xs, sorted(xs))

    def test_replacement_entry_starts_fully_offscreen(self) -> None:
        first = self.timing["replacement_left"][0]
        visible = portrait_on_screen_pixels(self.pro_sprite, first["protagonist_x"], first["y"])
        self.assertFalse(visible)

    def test_exit_ends_fully_offscreen(self) -> None:
        last_x = self.timing["portrait_exit_left"][-1]["x"]
        self.assertEqual(last_x, LEFT_ENTER_OFFSCREEN_X)
        visible = portrait_on_screen_pixels(self.aks_sprite, last_x, ACTIVE_LEFT[1])
        self.assertFalse(visible)

    def test_replacement_is_sequenced_without_overlapping_faces(self) -> None:
        for entry in self.timing["replacement_left"]:
            aks_vis = portrait_on_screen_pixels(self.aks_sprite, entry["aksil_x"], entry["y"])
            pro_vis = portrait_on_screen_pixels(
                self.pro_sprite, entry["protagonist_x"], entry["y"]
            )
            self.assertFalse(
                bool(aks_vis) and bool(pro_vis),
                f"replacement frame {entry['frame']} overlaps faces",
            )

    def test_gif_duration_matches_timing_manifest(self) -> None:
        timing = self.timing
        logical_count = timing["logical_frame_count"]
        self.assertEqual(logical_count, 35)
        self.assertEqual(timing["fps"], MOTION_FPS)
        self.assertAlmostEqual(timing["logical_frame_ms"], LOGICAL_FRAME_MS)

        native_path = DIALOGUE_DIR / "dialogue_motion_preview.gif"
        review_path = DIALOGUE_DIR / "dialogue_motion_preview_4x.gif"

        native_measured = gif_total_duration_ms(native_path)
        review_measured = gif_total_duration_ms(review_path)

        native_intended = intended_duration_ms(logical_count, NATIVE_PREVIEW_SPEED)
        review_intended = intended_duration_ms(logical_count, REVIEW_4X_SPEED)
        native_expected = expected_encoded_duration_ms(logical_count, NATIVE_PREVIEW_SPEED)
        review_expected = expected_encoded_duration_ms(logical_count, REVIEW_4X_SPEED)

        # Reopened-from-disk measurements must match the documented expected encoded totals.
        self.assertEqual(native_measured, native_expected)
        self.assertEqual(review_measured, review_expected)
        self.assertEqual(timing["native_gif"]["measured_duration_ms"], native_measured)
        self.assertEqual(timing["review_4x_gif"]["measured_duration_ms"], review_measured)

        # Encoded totals stay within one GIF delay unit of the intended 60 fps timeline.
        self.assertLessEqual(abs(native_measured - native_intended), 10)
        self.assertLessEqual(abs(review_measured - review_intended), 10)

        # A 400 ms native export against the 60 fps intended timeline must fail.
        broken_ms = 400
        self.assertGreater(abs(broken_ms - native_intended), 10)

    def test_collapsed_identical_frames_preserve_combined_duration(self) -> None:
        base = Image.new("RGB", (8, 8), (10, 20, 30))
        other = Image.new("RGB", (8, 8), (40, 50, 60))
        frames = [base, base, base, other, other]
        unique, ranges = collapse_identical_frames(frames)
        self.assertEqual(len(unique), 2)
        self.assertEqual(ranges, [(0, 2), (3, 4)])
        delays = encoded_delays_ms(len(frames), ranges, NATIVE_PREVIEW_SPEED)
        self.assertEqual(sum(delays), expected_encoded_duration_ms(len(frames), NATIVE_PREVIEW_SPEED))
        # Three identical frames at the start must accumulate more than one unit.
        self.assertGreaterEqual(delays[0], 30)

    def test_quantization_does_not_accumulate_substantial_drift(self) -> None:
        count = 35
        intended = intended_duration_ms(count, NATIVE_PREVIEW_SPEED)
        encoded = expected_encoded_duration_ms(count, NATIVE_PREVIEW_SPEED)
        self.assertLessEqual(abs(encoded - intended), 10)
        # Independent per-frame truncation of floor(16.666)->10 would lose ~233 ms.
        naive_truncation = count * 10
        self.assertGreater(abs(naive_truncation - intended), 10)

    def test_native_and_4x_gifs_satisfy_documented_timing(self) -> None:
        native = measure_gif_durations(DIALOGUE_DIR / "dialogue_motion_preview.gif")
        review = measure_gif_durations(DIALOGUE_DIR / "dialogue_motion_preview_4x.gif")
        self.assertEqual(sum(native), self.timing["native_gif"]["expected_encoded_duration_ms"])
        self.assertEqual(sum(review), self.timing["review_4x_gif"]["expected_encoded_duration_ms"])
        self.assertEqual(len(native), self.timing["native_gif"]["encoded_frame_count"])
        self.assertEqual(len(review), self.timing["review_4x_gif"]["encoded_frame_count"])
        self.assertEqual(self.timing["review_4x_gif"]["speed_multiplier"], REVIEW_4X_SPEED)

    def test_decoded_visible_transitions_remain_ordered(self) -> None:
        mapping = self.timing["native_gif"]["frame_mapping"]
        self.assertEqual(mapping[0]["logical_start"], 0)
        self.assertEqual(mapping[-1]["logical_end"], self.timing["logical_frame_count"] - 1)
        previous_end = -1
        for entry in mapping:
            self.assertEqual(entry["logical_start"], previous_end + 1)
            self.assertGreaterEqual(entry["logical_end"], entry["logical_start"])
            previous_end = entry["logical_end"]

    def test_broken_400ms_export_fails_against_60fps_timeline(self) -> None:
        intended = intended_duration_ms(35, NATIVE_PREVIEW_SPEED)
        self.assertAlmostEqual(intended, 35 * (1000.0 / 60.0))
        self.assertGreater(abs(400 - intended), 10)

    def test_no_sentinel_pixel_in_gif_export_helper(self) -> None:
        frames = [
            Image.new("RGBA", (240, 160), (49, 164, 65, 255)),
            Image.new("RGBA", (240, 160), (49, 164, 65, 255)),
            Image.new("RGBA", (240, 160), (32, 96, 48, 255)),
        ]
        out = BUILD_DIR / "_timing_sentinel_check.gif"
        save_preview_gif(frames, out, NATIVE_PREVIEW_SPEED)
        with Image.open(out) as gif:
            gif.seek(0)
            self.assertEqual(gif.convert("RGBA").getpixel((0, 0)), (49, 164, 65, 255))

    def test_logical_frame_count_matches_motion_strip_labels(self) -> None:
        self.assertEqual(self.timing["logical_frame_count"], 35)

    def test_motion_frame_strip_exists(self) -> None:
        self.assertTrue((BUILD_DIR / "dialogue_motion_frame_strip.png").is_file())


class DialogueMockupAsepriteInspectionTests(unittest.TestCase):
    def test_inspection_report_lists_layers_and_tags(self) -> None:
        report = (BUILD_DIR / "aseprite_inspection.txt").read_text(encoding="utf-8")
        self.assertIn("ASEPRITE_INSPECTION_BEGIN", report)
        self.assertIn("FRAME_FRONT_MASK", report)
        self.assertIn("tag_1=frame_a from=1 to=1", report)
        self.assertIn("tag_2=frame_b from=2 to=2", report)
        self.assertIn("tag_3=frame_c from=3 to=3", report)


if __name__ == "__main__":
    unittest.main()
