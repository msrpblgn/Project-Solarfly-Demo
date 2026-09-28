#!/usr/bin/env python3
"""Build UI-only dialogue reference PNGs from approved mockup layers (no portraits)."""

from __future__ import annotations

import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
DIALOGUE_DIR = ROOT / "assets_src" / "ui" / "dialogue"
LAYER_DIR = DIALOGUE_DIR / "build" / "layers"
OUT_DIR = DIALOGUE_DIR / "build" / "ui_only_refs"
BG = (32, 96, 48, 255)  # matches BACKGROUND_FILL used under the panel in mockup


def flatten_ui(frame_key: str) -> Image.Image:
    layers = [
        "BACKGROUND_FILL_do_not_export",
        "FRAME_FRONT_MASK",
        "NAMEPLATE",
        "TEXT",
        "ADVANCE_PROMPT",
    ]
    out = Image.new("RGBA", (240, 160), BG)
    for name in layers:
        path = LAYER_DIR / frame_key / f"{name}.png"
        if not path.is_file():
            raise SystemExit(f"missing layer: {path}")
        layer = Image.open(path).convert("RGBA")
        out.paste(layer, (0, 0), layer)
    return out


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames = {
        "sample_a": "frame_a",
        "sample_b": "frame_b",
    }
    meta = {"background_rgb": list(BG[:3]), "exclude": ["portraits", "guides", "world_preview"]}
    for out_name, frame_key in frames.items():
        image = flatten_ui(frame_key)
        path = OUT_DIR / f"dialogue_ui_only_{out_name}.png"
        image.save(path)
        meta[out_name] = str(path.relative_to(ROOT)).replace("\\", "/")
    (OUT_DIR / "ui_only_refs.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    print(f"Wrote UI-only refs under {OUT_DIR.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
