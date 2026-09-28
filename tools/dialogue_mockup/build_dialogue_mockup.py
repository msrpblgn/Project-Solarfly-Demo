#!/usr/bin/env python3
"""Build dialogue behavior mockup assets and Aseprite source (Milestone 1)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.dialogue_mockup.compose import (  # noqa: E402
    add_review_caption,
    apply_portrait_clip,
    blank_canvas,
    build_portrait_layer,
    compute_nameplate_y,
    enlarge_nearest,
    flatten_layers,
    head_bounds_local,
    left_nameplate,
    linear_positions,
    make_advance_prompt,
    make_background_fill,
    make_frame_mask,
    make_guides_layer,
    make_nameplate_layer,
    make_portrait_leak_clip,
    make_text_layer,
    make_world_preview,
    motion_positions,
    opaque_bounds,
    paste_sprite,
    right_nameplate,
    scale_nearest,
)
from tools.dialogue_mockup.constants import (  # noqa: E402
    ACTIVE_LEFT,
    ACTIVE_RIGHT,
    ASEPRITE_EXE,
    BUILD_DIR,
    CANVAS_W,
    DIALOGUE_DIR,
    CHEST_CUTOFF_LOCAL_ACTIVE,
    CHEST_CUTOFF_LOCAL_INACTIVE,
    FOCUS_FRAMES,
    FOCUS_PALETTE_SWITCH_FRAME,
    FOCUS_POSITIONS,
    FRAME_OPAQUE_Y0,
    FRAME_OPAQUE_Y1,
    FRAME_SOLID_Y,
    INACTIVE_LEFT,
    INACTIVE_RIGHT,
    LEFT_ENTER_OFFSCREEN_X,
    LEFT_EXIT_OFFSCREEN_X,
    LAYER_DIR,
    LAYER_NAMES,
    LOGICAL_FRAME_MS,
    MOTION_ENTER_FRAMES,
    MOTION_FPS,
    NATIVE_PREVIEW_SPEED,
    PORTRAIT_CLIP_Y1,
    PORTRAIT_H,
    PORTRAIT_SOURCE_H,
    PORTRAIT_SOURCE_W,
    PORTRAIT_W,
    PRESENTATION_SCALE,
    REPLACEMENT_FRAMES,
    REVIEW_4X_SPEED,
    RIGHT_ENTER_OFFSCREEN_X,
    SOURCE_AKSIL,
    SOURCE_FRAME,
    SOURCE_PROTAGONIST,
    TEXT_LINE1_Y,
    TEXT_START_X,
    TEXT_MAX_END_X,
    GLYPH_H,
)
from tools.dialogue_mockup.gif_timing import (  # noqa: E402
    save_preview_gif,
)
from tools.dialogue_mockup.palettes import (  # noqa: E402
    create_portrait_palette_set,
    normalize_protagonist,
    rgb888_to_bgr555,
)

ASEPRITE_DIR = Path(__file__).resolve().parent / "aseprite"


def save_layer(path: Path, image: Image.Image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def palette_entries_to_dict(entries) -> list[dict[str, object]]:
    return [
        {
            "index": entry.index,
            "rgb888": list(entry.rgb888),
            "bgr555": entry.bgr555,
            "bgr555_hex": f"0x{entry.bgr555:04X}",
            "role": entry.role,
        }
        for entry in entries
    ]


def write_palette_comparison(
    aksil_source: Image.Image,
    pro_source: Image.Image,
    pro_norm: Image.Image,
    aksil_set,
    pro_set,
    out_1x: Path,
    out_4x: Path,
) -> None:
    tile_h = 64
    labels = [
        ("Aksil source", aksil_source),
        ("Aksil 4bpp active", aksil_set.active),
        ("Aksil 4bpp inactive", aksil_set.inactive),
        ("Protagonist source", pro_source),
        ("Protagonist 64x64", pro_norm),
        ("Protagonist 4bpp active", pro_set.active),
        ("Protagonist 4bpp inactive", pro_set.inactive),
    ]
    sheet = Image.new("RGBA", (64 * 7 + 16, tile_h + 24), (32, 32, 32, 255))
    x = 8
    for label, image in labels:
        thumb = image.convert("RGBA")
        if thumb.height != tile_h:
            thumb = thumb.resize((tile_h, tile_h), Image.NEAREST)
        sheet.paste(thumb, (x, 20), thumb)
        x += 64 + 8
    sheet.save(out_1x)
    scale_nearest(sheet, 4).save(out_4x)


def build_review_sheet(frames: dict[str, Image.Image], out_path: Path) -> None:
    labels = list(frames.keys())
    tile_w, tile_h = 240 * 4, 160 * 4
    label_h = 24
    sheet = Image.new("RGBA", (tile_w * 3 + 32, tile_h + label_h + 24), (24, 24, 24, 255))
    draw = ImageDraw.Draw(sheet)
    x = 16
    for label in labels:
        scaled = scale_nearest(frames[label], 4)
        sheet.paste(scaled, (x, 24), scaled)
        draw.text((x, 6), label, fill=(255, 255, 255, 255))
        x += tile_w + 8
    sheet.save(out_path)


def build_before_after_sheet(old_frames: dict[str, Image.Image], new_frames: dict[str, Image.Image], out_path: Path) -> None:
    names = ["Frame A", "Frame B", "Frame C"]
    tile_w, tile_h = 240, 160
    gap = 8
    label_h = 16
    sheet_1x = Image.new(
        "RGBA",
        (tile_w * 3 + gap * 4, tile_h * 2 + gap * 3 + label_h * 2),
        (24, 24, 24, 255),
    )
    draw = ImageDraw.Draw(sheet_1x)
    draw.text((gap, 2), "BEFORE 64px full-body (identical 1x screen scale)", fill=(255, 220, 80, 255))
    draw.text(
        (gap, gap + label_h + tile_h + 2),
        "AFTER 128px bust, panel torso cutoff (identical 1x screen scale)",
        fill=(255, 220, 80, 255),
    )
    for i, name in enumerate(names):
        x = gap + i * (tile_w + gap)
        if name in old_frames:
            sheet_1x.paste(old_frames[name], (x, label_h), old_frames[name])
        if name in new_frames:
            sheet_1x.paste(
                new_frames[name],
                (x, label_h * 2 + gap + tile_h),
                new_frames[name],
            )
    scale_nearest(sheet_1x, 4).save(out_path)


def build_motion_frame_strip(
    frames: list[Image.Image],
    labels: list[str],
    out_path: Path,
) -> None:
    cols = 7
    rows = (len(frames) + cols - 1) // cols
    tile_w, tile_h = 240, 160
    margin = 8
    label_h = 14
    sheet = Image.new(
        "RGBA",
        (cols * tile_w + (cols + 1) * margin, rows * (tile_h + label_h) + (rows + 1) * margin),
        (24, 24, 24, 255),
    )
    draw = ImageDraw.Draw(sheet)
    for index, (frame, label) in enumerate(zip(frames, labels)):
        col = index % cols
        row = index // cols
        x = margin + col * (tile_w + margin)
        y = margin + row * (tile_h + label_h + margin)
        sheet.paste(frame, (x, y), frame)
        draw.text((x, y + tile_h + 2), label, fill=(255, 255, 255, 255))
    sheet.save(out_path)


def main() -> int:
    DIALOGUE_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    LAYER_DIR.mkdir(parents=True, exist_ok=True)

    frame_source = Image.open(SOURCE_FRAME)
    aksil_source = Image.open(SOURCE_AKSIL)
    pro_source = Image.open(SOURCE_PROTAGONIST)

    pro_norm, pro_proof = normalize_protagonist(pro_source)
    pro_norm_path = DIALOGUE_DIR / "protagonist_front_64x64_candidate.png"
    pro_norm.save(pro_norm_path)

    aksil_set = create_portrait_palette_set(aksil_source, "aksil")
    pro_set = create_portrait_palette_set(pro_norm, "protagonist")
    aksil_set.active.save(DIALOGUE_DIR / "aksil_front_4bpp_candidate.png")
    pro_set.active.save(DIALOGUE_DIR / "protagonist_front_4bpp_candidate.png")

    aksil_present_active = enlarge_nearest(aksil_set.active, PRESENTATION_SCALE)
    aksil_present_inactive = enlarge_nearest(aksil_set.inactive, PRESENTATION_SCALE)
    pro_present_active = enlarge_nearest(pro_set.active, PRESENTATION_SCALE)
    pro_present_inactive = enlarge_nearest(pro_set.inactive, PRESENTATION_SCALE)
    aksil_present_active.save(DIALOGUE_DIR / "aksil_front_128x128_presentation.png")
    pro_present_active.save(DIALOGUE_DIR / "protagonist_front_128x128_presentation.png")

    previous_dir = BUILD_DIR / "previous_64px"
    previous_dir.mkdir(parents=True, exist_ok=True)
    old_frames: dict[str, Image.Image] = {}
    for key, name in (
        ("Frame A", "dialogue_frame_a_aksil_active.png"),
        ("Frame B", "dialogue_frame_b_protagonist_active.png"),
        ("Frame C", "dialogue_frame_c_replacement.png"),
    ):
        src = DIALOGUE_DIR / name
        archive = previous_dir / name
        if src.is_file() and not archive.is_file():
            Image.open(src).convert("RGBA").save(archive)
        if archive.is_file():
            old_frames[key] = Image.open(archive).convert("RGBA")

    frame_mask = make_frame_mask(frame_source)
    portrait_clip = make_portrait_leak_clip(frame_source)
    background_fill = make_background_fill()
    world_preview = make_world_preview()

    nameplate_y = compute_nameplate_y(
        frame_source,
        [pro_present_active, pro_present_inactive, aksil_present_active, aksil_present_inactive],
        [ACTIVE_LEFT, INACTIVE_LEFT, ACTIVE_RIGHT, INACTIVE_RIGHT],
    )

    aksil_nameplate = right_nameplate("AKSIL", nameplate_y)
    protagonist_nameplate = left_nameplate("PROTAGONIST", nameplate_y, development=True)

    enter_positions = motion_positions(LEFT_ENTER_OFFSCREEN_X, ACTIVE_LEFT[0], MOTION_ENTER_FRAMES)
    exit_positions = motion_positions(ACTIVE_LEFT[0], LEFT_EXIT_OFFSCREEN_X, MOTION_ENTER_FRAMES)
    aksil_exit = linear_positions(ACTIVE_LEFT[0], LEFT_EXIT_OFFSCREEN_X, 5)
    pro_enter = linear_positions(LEFT_ENTER_OFFSCREEN_X, ACTIVE_LEFT[0], 5)
    # Frame C snapshots the first moving exit frame so Aksil is still readable.
    frame_c_aksil_x = aksil_exit[1]
    frame_c_pro_x = LEFT_ENTER_OFFSCREEN_X

    frame_specs = {
        "frame_a": {
            "tag": "frame_a",
            "label": "Frame A - Aksil active",
            "left_pos": INACTIVE_LEFT,
            "right_pos": ACTIVE_RIGHT,
            "left_sprite": pro_present_inactive,
            "right_sprite": aksil_present_active,
            "nameplate": aksil_nameplate,
            "line1": "YOU THOUGHT THAT WOULD WORK?",
            "line2": "COME ON. ONE MORE TRY.",
            "guide": "Pause after WORK? before line 2",
            "prompt": True,
        },
        "frame_b": {
            "tag": "frame_b",
            "label": "Frame B - Protagonist active",
            "left_pos": ACTIVE_LEFT,
            "right_pos": INACTIVE_RIGHT,
            "left_sprite": pro_present_active,
            "right_sprite": aksil_present_inactive,
            "nameplate": protagonist_nameplate,
            "line1": "IT ALMOST DID.",
            "line2": "",
            "guide": "Second line intentionally empty",
            "prompt": True,
        },
        "frame_c": {
            "tag": "frame_c",
            "label": "Frame C - technical replacement test",
            "left_pos": None,
            "right_pos": None,
            "left_sprite": None,
            "right_sprite": None,
            "nameplate": None,
            "line1": "",
            "line2": "",
            "guide": "Technical blocking test only - not story canon",
            "prompt": False,
            "replacement_mid": True,
        },
    }

    flattened_outputs: dict[str, Image.Image] = {}
    frame_layers_for_config: list[dict[str, object]] = []

    for key, spec in frame_specs.items():
        layer_dir = LAYER_DIR / key
        layers: dict[str, Image.Image] = {
            "BACKGROUND_FILL_do_not_export": background_fill,
            "WORLD_PREVIEW_do_not_export": world_preview,
            "FRAME_FRONT_MASK": frame_mask,
        }
        if spec.get("replacement_mid"):
            # Sequenced handover still: Aksil leaving left, Protagonist offscreen.
            # Faces do not overlap. Technical test, not story canon.
            mid = blank_canvas()
            paste_sprite(
                mid, aksil_present_active, frame_c_aksil_x, ACTIVE_LEFT[1], clip=(0, 0, 240, 160)
            )
            paste_sprite(
                mid, pro_present_active, frame_c_pro_x, ACTIVE_LEFT[1], clip=(0, 0, 240, 160)
            )
            layers["PORTRAIT_LEFT"] = apply_portrait_clip(mid, portrait_clip)
            layers["PORTRAIT_RIGHT"] = blank_canvas()
        else:
            left_x, left_y = spec["left_pos"]
            right_x, right_y = spec["right_pos"]
            layers["PORTRAIT_LEFT"] = build_portrait_layer(
                spec["left_sprite"], left_x, left_y, clip_mask=portrait_clip
            )
            layers["PORTRAIT_RIGHT"] = build_portrait_layer(
                spec["right_sprite"], right_x, right_y, clip_mask=portrait_clip
            )
        layers["NAMEPLATE"] = make_nameplate_layer(spec["nameplate"])
        layers["TEXT"] = make_text_layer(spec["line1"], spec["line2"])
        layers["ADVANCE_PROMPT"] = make_advance_prompt(spec["prompt"])
        layers["GUIDES_do_not_export"] = make_guides_layer(spec["guide"])

        for layer_name, image in layers.items():
            save_layer(layer_dir / f"{layer_name}.png", image)

        flat = flatten_layers(layers, exclude={"GUIDES_do_not_export"})
        out_name = {
            "frame_a": "dialogue_frame_a_aksil_active.png",
            "frame_b": "dialogue_frame_b_protagonist_active.png",
            "frame_c": "dialogue_frame_c_replacement.png",
        }[key]
        flat.save(DIALOGUE_DIR / out_name)
        flattened_outputs[key] = flat

        frame_layers_for_config.append(
            {
                "key": key,
                "tag": spec["tag"],
                "label": spec["label"],
                "layer_dir": str(layer_dir.relative_to(ROOT)).replace("\\", "/"),
            }
        )

    # Focus transition preview (5 unique motion frames).
    focus_frames: list[Image.Image] = []
    focus_manifest = []
    for i, (left_pos, right_pos) in enumerate(FOCUS_POSITIONS):
        use_active_left = i >= FOCUS_PALETTE_SWITCH_FRAME
        left_sprite = pro_present_active if use_active_left else pro_present_inactive
        right_sprite = aksil_present_inactive if use_active_left else aksil_present_active
        left_x, left_y = left_pos
        right_x, right_y = right_pos
        layers = {
            "BACKGROUND_FILL_do_not_export": background_fill,
            "WORLD_PREVIEW_do_not_export": world_preview,
            "PORTRAIT_LEFT": build_portrait_layer(left_sprite, left_x, left_y, clip_mask=portrait_clip),
            "PORTRAIT_RIGHT": build_portrait_layer(right_sprite, right_x, right_y, clip_mask=portrait_clip),
            "FRAME_FRONT_MASK": frame_mask,
            "NAMEPLATE": make_nameplate_layer(
                aksil_nameplate if not use_active_left else protagonist_nameplate
            ),
            "TEXT": make_text_layer("YOU THOUGHT THAT WOULD WORK?", "COME ON. ONE MORE TRY."),
            "ADVANCE_PROMPT": make_advance_prompt(True),
        }
        focus_frames.append(flatten_layers(layers, exclude={"GUIDES_do_not_export"}))
        focus_manifest.append(
            {
                "frame": i,
                "left": [left_x, left_y],
                "right": [right_x, right_y],
                "left_palette": "active" if use_active_left else "inactive",
                "right_palette": "inactive" if use_active_left else "active",
                "palette_switch_frame": FOCUS_PALETTE_SWITCH_FRAME,
            }
        )

    motion_strip = Image.new("RGBA", (240 * 5 + 40, 180), (16, 16, 16, 255))
    for i, frame in enumerate(focus_frames[:5]):
        motion_strip.paste(frame, (8 + i * 248, 8), frame)
    draw_strip = ImageDraw.Draw(motion_strip)
    for i in range(min(5, len(focus_manifest))):
        entry = focus_manifest[i]
        draw_strip.text(
            (8 + i * 248, 168),
            f"F{i} L{entry['left']} R{entry['right']}",
            fill=(255, 255, 255, 255),
        )
    motion_strip.save(BUILD_DIR / "dialogue_focus_strip.png")

    native_focus_gif = save_preview_gif(
        focus_frames,
        DIALOGUE_DIR / "dialogue_focus_exchange_preview.gif",
        speed_multiplier=NATIVE_PREVIEW_SPEED,
    )
    review_focus_gif = save_preview_gif(
        [
            add_review_caption(
                scale_nearest(frame, 4),
                "FOCUS EXCHANGE REVIEW — 12x SLOWER THAN 60 FPS TARGET",
            )
            for frame in focus_frames
        ],
        DIALOGUE_DIR / "dialogue_focus_exchange_preview_4x.gif",
        speed_multiplier=REVIEW_4X_SPEED,
    )

    # Enter / exit preview (10 frames each).
    enter_frames = [
        flatten_layers(
            {
                "BACKGROUND_FILL_do_not_export": background_fill,
                "WORLD_PREVIEW_do_not_export": world_preview,
                "PORTRAIT_LEFT": build_portrait_layer(pro_present_active, x, ACTIVE_LEFT[1], clip_mask=portrait_clip),
                "PORTRAIT_RIGHT": build_portrait_layer(
                    aksil_present_inactive, INACTIVE_RIGHT[0], INACTIVE_RIGHT[1], clip_mask=portrait_clip
                ),
                "FRAME_FRONT_MASK": frame_mask,
                "NAMEPLATE": blank_canvas(),
                "TEXT": blank_canvas(),
                "ADVANCE_PROMPT": blank_canvas(),
            },
            exclude={"GUIDES_do_not_export"},
        )
        for x in enter_positions
    ]
    exit_positions = motion_positions(ACTIVE_LEFT[0], LEFT_EXIT_OFFSCREEN_X, MOTION_ENTER_FRAMES)
    exit_frames = [
        flatten_layers(
            {
                "BACKGROUND_FILL_do_not_export": background_fill,
                "WORLD_PREVIEW_do_not_export": world_preview,
                "PORTRAIT_LEFT": build_portrait_layer(aksil_present_active, x, ACTIVE_LEFT[1], clip_mask=portrait_clip),
                "PORTRAIT_RIGHT": blank_canvas(),
                "FRAME_FRONT_MASK": frame_mask,
                "NAMEPLATE": blank_canvas(),
                "TEXT": blank_canvas(),
                "ADVANCE_PROMPT": blank_canvas(),
            },
            exclude={"GUIDES_do_not_export"},
        )
        for x in exit_positions
    ]

    replacement_frames = []
    replacement_manifest = []
    for i in range(REPLACEMENT_FRAMES):
        mid = blank_canvas()
        if i < 5:
            aksil_x = aksil_exit[i]
            pro_x = LEFT_ENTER_OFFSCREEN_X
            paste_sprite(mid, aksil_present_active, aksil_x, ACTIVE_LEFT[1], clip=(0, 0, 240, 160))
        else:
            aksil_x = LEFT_EXIT_OFFSCREEN_X
            pro_x = pro_enter[i - 5]
            paste_sprite(mid, pro_present_active, pro_x, ACTIVE_LEFT[1], clip=(0, 0, 240, 160))
        frame = flatten_layers(
            {
                "BACKGROUND_FILL_do_not_export": background_fill,
                "WORLD_PREVIEW_do_not_export": world_preview,
                "PORTRAIT_LEFT": apply_portrait_clip(mid, portrait_clip),
                "PORTRAIT_RIGHT": blank_canvas(),
                "FRAME_FRONT_MASK": frame_mask,
                "NAMEPLATE": blank_canvas(),
                "TEXT": blank_canvas(),
                "ADVANCE_PROMPT": blank_canvas(),
            },
            exclude={"GUIDES_do_not_export"},
        )
        replacement_frames.append(frame)
        replacement_manifest.append(
            {
                "frame": i,
                "aksil_x": aksil_x,
                "protagonist_x": pro_x,
                "y": ACTIVE_LEFT[1],
                "phase": "exit" if i < 5 else "enter",
                "track": "linear_sequenced",
            }
        )

    motion_preview = focus_frames + enter_frames + exit_frames + replacement_frames
    motion_labels: list[str] = []
    for i, entry in enumerate(focus_manifest):
        motion_labels.append(
            f"F{i} focus L{entry['left']} R{entry['right']} {entry['left_palette']}"
        )
    for i, x in enumerate(enter_positions):
        motion_labels.append(f"F{FOCUS_FRAMES + i} enter x={x}")
    for i, x in enumerate(exit_positions):
        motion_labels.append(f"F{FOCUS_FRAMES + MOTION_ENTER_FRAMES + i} exit x={x}")
    for i, entry in enumerate(replacement_manifest):
        motion_labels.append(
            f"F{FOCUS_FRAMES + MOTION_ENTER_FRAMES * 2 + i} "
            f"A{entry['aksil_x']} P{entry['protagonist_x']}"
        )

    native_gif = save_preview_gif(
        motion_preview,
        DIALOGUE_DIR / "dialogue_motion_preview.gif",
        speed_multiplier=NATIVE_PREVIEW_SPEED,
    )
    review_gif = save_preview_gif(
        [
            add_review_caption(
                scale_nearest(frame, 4),
                "MOTION REVIEW — 12x SLOWER THAN 60 FPS TARGET",
            )
            for frame in motion_preview
        ],
        DIALOGUE_DIR / "dialogue_motion_preview_4x.gif",
        speed_multiplier=REVIEW_4X_SPEED,
    )
    build_motion_frame_strip(
        motion_preview,
        motion_labels,
        BUILD_DIR / "dialogue_motion_frame_strip.png",
    )

    build_review_sheet(
        {
            "Frame A": flattened_outputs["frame_a"],
            "Frame B": flattened_outputs["frame_b"],
            "Frame C": flattened_outputs["frame_c"],
        },
        DIALOGUE_DIR / "dialogue_mockup_review_sheet_4x.png",
    )
    if old_frames:
        build_before_after_sheet(
            old_frames,
            {
                "Frame A": flattened_outputs["frame_a"],
                "Frame B": flattened_outputs["frame_b"],
                "Frame C": flattened_outputs["frame_c"],
            },
            DIALOGUE_DIR / "dialogue_mockup_before_after_4x.png",
        )
    else:
        raise FileNotFoundError(
            "previous 64px frames were not archived; cannot write before/after sheet"
        )

    write_palette_comparison(
        aksil_source,
        pro_source,
        pro_norm,
        aksil_set,
        pro_set,
        DIALOGUE_DIR / "portrait_palette_comparison_1x.png",
        DIALOGUE_DIR / "portrait_palette_comparison_4x.png",
    )

    timing_manifest = {
        "fps": MOTION_FPS,
        "logical_frame_ms": LOGICAL_FRAME_MS,
        "logical_frame_count": len(motion_preview),
        "logical_duration_ms": LOGICAL_FRAME_MS * len(motion_preview),
        "gif_delay_unit_ms": 10,
        "timing_policy": (
            "Runtime target is exact 60 fps. Preview GIFs use cumulative timestamp "
            "quantization to 10 ms GIF delay units. Identical consecutive frames collapse "
            "while preserving accumulated display time. The frame strip retains all "
            "logical frames. No sentinel pixels are written into preview images."
        ),
        "focus_exchange": focus_manifest,
        "portrait_enter_left": [{"frame": i, "x": enter_positions[i]} for i in range(MOTION_ENTER_FRAMES)],
        "portrait_exit_left": [{"frame": i, "x": exit_positions[i]} for i in range(MOTION_ENTER_FRAMES)],
        "replacement_left": replacement_manifest,
        "replacement_policy": (
            "Left-side replacement is sequenced: Aksil exits fully offscreen, then "
            "the Protagonist enters. Simultaneous overlap is not used because the "
            "128px presentation canvases would pass faces through one another."
        ),
        "focus_exchange_gif": {
            "path": "assets_src/ui/dialogue/dialogue_focus_exchange_preview.gif",
            "speed_multiplier": native_focus_gif.speed_multiplier,
            "intended_duration_ms": native_focus_gif.intended_duration_ms,
            "expected_encoded_duration_ms": native_focus_gif.expected_encoded_duration_ms,
            "measured_duration_ms": native_focus_gif.measured_duration_ms,
            "encoded_frame_count": native_focus_gif.encoded_frame_count,
        },
        "focus_exchange_review_4x_gif": {
            "path": "assets_src/ui/dialogue/dialogue_focus_exchange_preview_4x.gif",
            "speed_multiplier": review_focus_gif.speed_multiplier,
            "label": "FOCUS EXCHANGE REVIEW — 12x SLOWER THAN 60 FPS TARGET",
            "intended_duration_ms": review_focus_gif.intended_duration_ms,
            "expected_encoded_duration_ms": review_focus_gif.expected_encoded_duration_ms,
            "measured_duration_ms": review_focus_gif.measured_duration_ms,
            "encoded_frame_count": review_focus_gif.encoded_frame_count,
        },
        "native_gif": {
            "path": "assets_src/ui/dialogue/dialogue_motion_preview.gif",
            "speed_multiplier": native_gif.speed_multiplier,
            "intended_duration_ms": native_gif.intended_duration_ms,
            "expected_encoded_duration_ms": native_gif.expected_encoded_duration_ms,
            "measured_duration_ms": native_gif.measured_duration_ms,
            "encoded_frame_count": native_gif.encoded_frame_count,
            "frame_mapping": native_gif.frame_mapping,
        },
        "review_4x_gif": {
            "path": "assets_src/ui/dialogue/dialogue_motion_preview_4x.gif",
            "speed_multiplier": review_gif.speed_multiplier,
            "intended_duration_ms": review_gif.intended_duration_ms,
            "expected_encoded_duration_ms": review_gif.expected_encoded_duration_ms,
            "measured_duration_ms": review_gif.measured_duration_ms,
            "encoded_frame_count": review_gif.encoded_frame_count,
            "frame_mapping": review_gif.frame_mapping,
        },
    }
    (BUILD_DIR / "dialogue_motion_timing.json").write_text(
        json.dumps(timing_manifest, indent=2),
        encoding="utf-8",
    )

    config = {
        "root": str(ROOT).replace("\\", "/"),
        "width": 240,
        "height": 160,
        "frame_opaque_y_interval": [FRAME_OPAQUE_Y0, FRAME_OPAQUE_Y1],
        "frame_solid_y": FRAME_SOLID_Y,
        "portrait_clip_y1": PORTRAIT_CLIP_Y1,
        "presentation_scale": PRESENTATION_SCALE,
        "source_portrait_size": [PORTRAIT_SOURCE_W, PORTRAIT_SOURCE_H],
        "presentation_portrait_size": [PORTRAIT_W, PORTRAIT_H],
        "anchors": {
            "active_left": list(ACTIVE_LEFT),
            "inactive_left": list(INACTIVE_LEFT),
            "active_right": list(ACTIVE_RIGHT),
            "inactive_right": list(INACTIVE_RIGHT),
        },
        "head_bounds_local": {
            "aksil": list(head_bounds_local(aksil_present_active)),
            "protagonist": list(head_bounds_local(pro_present_active)),
        },
        "opaque_bounds_local": {
            "aksil": list(opaque_bounds(aksil_present_active)),
            "protagonist": list(opaque_bounds(pro_present_active)),
        },
        "offscreen": {
            "left_enter": LEFT_ENTER_OFFSCREEN_X,
            "left_exit": LEFT_EXIT_OFFSCREEN_X,
            "right_enter": RIGHT_ENTER_OFFSCREEN_X,
        },
        "chest_cutoff": {
            "panel_solid_y": FRAME_SOLID_Y,
            "active_local_y": CHEST_CUTOFF_LOCAL_ACTIVE,
            "inactive_local_y": CHEST_CUTOFF_LOCAL_INACTIVE,
            "policy": (
                "Portrait leak-clip keeps bust pixels for y < 116. Rows [116, 159) "
                "keep portrait pixels only where Dialogue Ui.png is opaque. Row 159 "
                "is fully transparent and is never filled. FRAME_FRONT_MASK still sits "
                "above both portrait layers."
            ),
        },
        "frame_c_snapshot": {
            "aksil_x": frame_c_aksil_x,
            "protagonist_x": frame_c_pro_x,
            "y": ACTIVE_LEFT[1],
            "phase": "exit",
        },
        "runtime_note": (
            "128x128 is mockup presentation size only. It does not prove that "
            "current dialogue OBJ reservations can display 128x128 at runtime."
        ),
        "nameplate_y": nameplate_y,
        "nameplates": {
            "aksil_right": {
                "text": aksil_nameplate.text,
                "x": aksil_nameplate.x,
                "y": aksil_nameplate.y,
                "width": aksil_nameplate.width,
                "height": aksil_nameplate.height,
                "development_label": False,
            },
            "protagonist_left": {
                "text": protagonist_nameplate.text,
                "x": protagonist_nameplate.x,
                "y": protagonist_nameplate.y,
                "width": protagonist_nameplate.width,
                "height": protagonist_nameplate.height,
                "development_label": True,
            },
        },
        "output_aseprite": str((DIALOGUE_DIR / "dialogue_behavior_mockup.aseprite").resolve()).replace("\\", "/"),
        "layer_order_bottom_to_top": list(reversed(LAYER_NAMES)),
        "frames": frame_layers_for_config,
        "motion_frame_count": len(motion_preview),
        "protagonist_proof": pro_proof,
        "palettes": {
            "aksil_active": palette_entries_to_dict(aksil_set.entries),
            "aksil_inactive": palette_entries_to_dict(aksil_set.inactive_entries),
            "protagonist_active": palette_entries_to_dict(pro_set.entries),
            "protagonist_inactive": palette_entries_to_dict(pro_set.inactive_entries),
        },
    }
    config_path = BUILD_DIR / "mockup_config.json"
    config_path.write_text(json.dumps(config, indent=2), encoding="utf-8")

    if not ASEPRITE_EXE.is_file():
        print(f"ERROR: Aseprite not found at {ASEPRITE_EXE}", file=sys.stderr)
        return 1

    roundtrip_dir = BUILD_DIR / "roundtrip"
    subprocess.run(
        [
            str(ASEPRITE_EXE),
            "-b",
            "--script",
            "tools/dialogue_mockup/aseprite/build_dialogue_mockup.lua",
        ],
        check=True,
        cwd=ROOT,
    )
    roundtrip_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            str(ASEPRITE_EXE),
            "-b",
            "--script",
            "tools/dialogue_mockup/aseprite/export_reference_frames.lua",
        ],
        check=True,
        cwd=ROOT,
    )
    inspect = subprocess.run(
        [
            str(ASEPRITE_EXE),
            "-b",
            "--script",
            "tools/dialogue_mockup/aseprite/inspect_mockup.lua",
        ],
        check=True,
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    inspect_report = BUILD_DIR / "aseprite_inspection.txt"
    inspect_report.write_text(inspect.stdout + inspect.stderr, encoding="utf-8")

    print("Dialogue mockup build complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
