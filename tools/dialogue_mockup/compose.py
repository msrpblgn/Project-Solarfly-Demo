"""Raster composition helpers for dialogue mockup frames."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable

from PIL import Image, ImageDraw

from tools.dialogue_mockup.constants import (
    BACKGROUND_FILL_RGB,
    CANVAS_H,
    CANVAS_W,
    CHAR_ADVANCE,
    FRAME_BLACK,
    FRAME_GOLD,
    FRAME_LIGHT_GOLD,
    FRAME_OPAQUE_Y0,
    FRAME_OPAQUE_Y1,
    FRAME_ORANGE,
    FRAME_SOLID_Y,
    FRAME_TRANSPARENT,
    GLYPH_H,
    NAMEPLATE_HEIGHT,
    NAMEPLATE_PAD_X,
    PORTRAIT_CLIP_Y1,
    PORTRAIT_H,
    PORTRAIT_W,
    PRESENTATION_SCALE,
    PROMPT_H,
    PROMPT_W,
    PROMPT_X0,
    PROMPT_Y0,
    TEXT_COLOR,
    TEXT_LINE1_Y,
    TEXT_LINE2_Y,
    TEXT_START_X,
    WORLD_PREVIEW_RGB,
)
from tools.dialogue_mockup.font5x7 import draw_text


def blank_canvas() -> Image.Image:
    return Image.new("RGBA", (CANVAS_W, CANVAS_H), FRAME_TRANSPARENT)


def rgba_pixels(image: Image.Image) -> list[list[tuple[int, int, int, int]]]:
    src = image.convert("RGBA")
    px = src.load()
    return [[px[x, y] for x in range(src.width)] for y in range(src.height)]


def from_pixel_buffer(pixels: list[list[tuple[int, int, int, int]]]) -> Image.Image:
    image = Image.new("RGBA", (len(pixels[0]), len(pixels)), FRAME_TRANSPARENT)
    out = image.load()
    for y, row in enumerate(pixels):
        for x, color in enumerate(row):
            out[x, y] = color
    return image


def paste_sprite(
    canvas: Image.Image,
    sprite: Image.Image,
    x: int,
    y: int,
    clip: tuple[int, int, int, int] | None = None,
) -> None:
    if clip:
        cx0, cy0, cx1, cy1 = clip
        sprite = sprite.crop(
            (
                max(0, cx0 - x),
                max(0, cy0 - y),
                max(0, cx1 - x),
                max(0, cy1 - y),
            )
        )
        x = max(x, cx0)
        y = max(y, cy0)
    canvas.paste(sprite, (x, y), sprite)


def make_background_fill() -> Image.Image:
    image = blank_canvas()
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, CANVAS_W, CANVAS_H), fill=BACKGROUND_FILL_RGB + (255,))
    return image


def make_world_preview() -> Image.Image:
    image = blank_canvas()
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, CANVAS_W, 108), fill=WORLD_PREVIEW_RGB + (255,))
    return image


def enlarge_nearest(image: Image.Image, scale: int = PRESENTATION_SCALE) -> Image.Image:
    src = image.convert("RGBA")
    return src.resize((src.width * scale, src.height * scale), Image.NEAREST)


def opaque_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    src = image.convert("RGBA")
    xs: list[int] = []
    ys: list[int] = []
    px = src.load()
    for y in range(src.height):
        for x in range(src.width):
            if px[x, y][3] > 0:
                xs.append(x)
                ys.append(y)
    if not xs:
        return (0, 0, 0, 0)
    return (min(xs), min(ys), max(xs) + 1, max(ys) + 1)


def head_bounds_local(image: Image.Image) -> tuple[int, int, int, int]:
    """Approximate head box: opaque pixels in the top 40% of the presentation canvas."""
    src = image.convert("RGBA")
    y_limit = max(1, src.height * 40 // 100)
    xs: list[int] = []
    ys: list[int] = []
    px = src.load()
    for y in range(y_limit):
        for x in range(src.width):
            if px[x, y][3] > 0:
                xs.append(x)
                ys.append(y)
    if not xs:
        return opaque_bounds(src)
    return (min(xs), min(ys), max(xs) + 1, max(ys) + 1)


def make_portrait_leak_clip(frame_source: Image.Image) -> Image.Image:
    """
    Allow portrait pixels above the solid panel, and behind opaque frame pixels.

    Visible busts occupy y in [0, FRAME_SOLID_Y). Pixels in [FRAME_SOLID_Y, 159)
    remain only where the supplied frame is opaque so they sit behind the panel
    without leaking through holes or the transparent bottom row.
    """
    frame = frame_source.convert("RGBA")
    clip = Image.new("L", (CANVAS_W, CANVAS_H), 0)
    clip_px = clip.load()
    frame_px = frame.load()
    for y in range(CANVAS_H):
        for x in range(CANVAS_W):
            if y < PORTRAIT_CLIP_Y1:
                clip_px[x, y] = 255
            elif y < FRAME_OPAQUE_Y1 and frame_px[x, y][3] > 0:
                clip_px[x, y] = 255
    return clip


def apply_portrait_clip(layer: Image.Image, clip_mask: Image.Image) -> Image.Image:
    src = layer.convert("RGBA")
    mask = clip_mask.convert("L")
    out = Image.new("RGBA", src.size, FRAME_TRANSPARENT)
    src_px = src.load()
    mask_px = mask.load()
    out_px = out.load()
    for y in range(src.height):
        for x in range(src.width):
            if mask_px[x, y] == 0:
                continue
            out_px[x, y] = src_px[x, y]
    return out


def make_frame_mask(frame_source: Image.Image) -> Image.Image:
    frame = frame_source.convert("RGBA")
    if frame.size != (CANVAS_W, CANVAS_H):
        raise ValueError("dialogue frame source must be 240x160")
    return frame


def make_text_layer(line1: str, line2: str = "") -> Image.Image:
    pixels = rgba_pixels(blank_canvas())
    draw_text(pixels, TEXT_START_X, TEXT_LINE1_Y, line1, TEXT_COLOR)
    if line2:
        draw_text(pixels, TEXT_START_X, TEXT_LINE2_Y, line2, TEXT_COLOR)
    return from_pixel_buffer(pixels)


def make_advance_prompt(visible: bool) -> Image.Image:
    image = blank_canvas()
    if not visible:
        return image
    draw = ImageDraw.Draw(image)
    # Small black continuation marker (triangle + dot).
    points = [
        (PROMPT_X0 + 1, PROMPT_Y0 + 1),
        (PROMPT_X0 + 4, PROMPT_Y0 + 1),
        (PROMPT_X0 + 2, PROMPT_Y0 + 4),
    ]
    draw.polygon(points, fill=FRAME_BLACK[:3])
    draw.point((PROMPT_X0 + 4, PROMPT_Y0 + 4), fill=FRAME_BLACK[:3])
    draw.point((PROMPT_X0 + 5, PROMPT_Y0 + 5), fill=FRAME_BLACK[:3])
    return image


@dataclass(frozen=True)
class NameplateSpec:
    text: str
    x: int
    y: int
    width: int
    height: int
    fill: tuple[int, int, int, int]
    is_development_label: bool


def measure_nameplate(text: str) -> tuple[int, int]:
    width = len(text) * CHAR_ADVANCE + NAMEPLATE_PAD_X * 2
    height = NAMEPLATE_HEIGHT
    return width, height


def make_nameplate_layer(spec: NameplateSpec | None) -> Image.Image:
    image = blank_canvas()
    if spec is None:
        return image
    draw = ImageDraw.Draw(image)
    outer = (spec.x, spec.y, spec.x + spec.width - 1, spec.y + spec.height - 1)
    inner = (spec.x + 1, spec.y + 1, spec.x + spec.width - 2, spec.y + spec.height - 2)
    draw.rectangle(outer, outline=FRAME_BLACK[:3], fill=spec.fill[:3])
    draw.rectangle(inner, outline=FRAME_ORANGE[:3], fill=spec.fill[:3])
    pixels = rgba_pixels(image)
    text_x = spec.x + NAMEPLATE_PAD_X
    text_y = spec.y + 2
    draw_text(pixels, text_x, text_y, spec.text, FRAME_BLACK)
    return from_pixel_buffer(pixels)


def frame_opaque_top_y(frame_source: Image.Image) -> int:
    """Return the first opaque row of the supplied dialogue frame."""
    frame = frame_source.convert("RGBA")
    for y in range(frame.height):
        for x in range(frame.width):
            if frame.getpixel((x, y))[3] > 0:
                return y
    raise ValueError("dialogue frame has no opaque pixels")


def portrait_opaque_bottom_y(sprite: Image.Image, anchor_x: int, anchor_y: int) -> int:
    """Return the lowest on-screen row occupied by opaque portrait pixels."""
    src = sprite.convert("RGBA")
    bottom = anchor_y - 1
    for y in range(src.height):
        for x in range(src.width):
            if src.getpixel((x, y))[3] > 0:
                bottom = max(bottom, anchor_y + y)
    return bottom


def compute_nameplate_y(
    frame_source: Image.Image,
    portrait_sprites: list[Image.Image],
    portrait_anchors: list[tuple[int, int]],
) -> int:
    """Attach nameplates to the solid panel row, remaining clear of visible heads."""
    attach_y = FRAME_SOLID_Y - NAMEPLATE_HEIGHT + 1
    head_bottom = 0
    for sprite, (ax, ay) in zip(portrait_sprites, portrait_anchors):
        hx0, hy0, hx1, hy1 = head_bounds_local(sprite)
        head_bottom = max(head_bottom, ay + hy1)
    y = attach_y
    if y < head_bottom + 1:
        y = head_bottom + 1
    if y + NAMEPLATE_HEIGHT > TEXT_LINE1_Y:
        raise ValueError("nameplate would intersect text-safe area")
    return y


def nameplate_rect(spec: NameplateSpec) -> tuple[int, int, int, int]:
    return (spec.x, spec.y, spec.x + spec.width - 1, spec.y + spec.height - 1)


def left_nameplate(text: str, y: int, development: bool = False) -> NameplateSpec:
    width, height = measure_nameplate(text)
    return NameplateSpec(
        text=text,
        x=8,
        y=y,
        width=width,
        height=height,
        fill=FRAME_LIGHT_GOLD,
        is_development_label=development,
    )


def right_nameplate(text: str, y: int, development: bool = False) -> NameplateSpec:
    width, height = measure_nameplate(text)
    return NameplateSpec(
        text=text,
        x=CANVAS_W - 8 - width,
        y=y,
        width=width,
        height=height,
        fill=FRAME_GOLD,
        is_development_label=development,
    )


def make_guides_layer(note: str) -> Image.Image:
    image = blank_canvas()
    draw = ImageDraw.Draw(image)
    draw.rectangle(
        (TEXT_START_X, TEXT_LINE1_Y, TEXT_START_X + 209, TEXT_LINE2_Y + GLYPH_H - 1),
        outline=(255, 0, 255, 255),
    )
    draw.rectangle(
        (PROMPT_X0, PROMPT_Y0, PROMPT_X0 + PROMPT_W - 1, PROMPT_Y0 + PROMPT_H - 1),
        outline=(255, 0, 255, 255),
    )
    draw.text((4, 4), note, fill=(255, 0, 255, 255))
    return image


def ease_out_int(start: int, end: int, frame_index: int, frame_count: int) -> int:
    if frame_count <= 1:
        return end
    t = (frame_index + 1) / frame_count
    eased = 1.0 - (1.0 - t) ** 2
    value = start + int(round((end - start) * eased))
    if frame_index == frame_count - 1:
        return end
    return value


def anchor_track(start: int, end: int, frames: int) -> list[int]:
    if frames <= 1:
        return [end]
    track = [start]
    for i in range(1, frames):
        if i == frames - 1:
            track.append(end)
        else:
            track.append(ease_out_int(start, end, i, frames - 1))
    return track


def motion_positions(start: int, end: int, frame_count: int) -> list[int]:
    """Ease-out integer track that begins at start and ends at end."""
    if frame_count <= 1:
        return [end]
    positions = [start]
    for i in range(1, frame_count):
        if i == frame_count - 1:
            positions.append(end)
        else:
            pos = ease_out_int(start, end, i, frame_count - 1)
            if end > start:
                pos = min(pos, end - 1)
            elif end < start:
                pos = max(pos, end + 1)
            positions.append(pos)
    return positions


def linear_positions(start: int, end: int, frame_count: int) -> list[int]:
    """Inclusive integer track with unique endpoints; used for sequenced replacement."""
    if frame_count <= 1:
        return [end]
    positions = []
    for i in range(frame_count):
        if i == 0:
            positions.append(start)
        elif i == frame_count - 1:
            positions.append(end)
        else:
            pos = start + int(round((end - start) * i / (frame_count - 1)))
            if end > start:
                pos = min(max(pos, start + 1), end - 1)
            elif end < start:
                pos = max(min(pos, start - 1), end + 1)
            else:
                pos = start
            positions.append(pos)
    return positions


def portrait_visible_pixels(
    sprite: Image.Image,
    x: int,
    y: int,
    clip: tuple[int, int, int, int] = (0, 0, CANVAS_W, CANVAS_H),
) -> set[tuple[int, int]]:
    """Return clipped on-screen opaque portrait pixel coordinates."""
    src = sprite.convert("RGBA")
    cx0, cy0, cx1, cy1 = clip
    visible: set[tuple[int, int]] = set()
    for py in range(src.height):
        for px in range(src.width):
            if src.getpixel((px, py))[3] == 0:
                continue
            sx = x + px
            sy = y + py
            if cx0 <= sx < cx1 and cy0 <= sy < cy1:
                visible.add((sx, sy))
    return visible


def flatten_layers(layer_map: dict[str, Image.Image], exclude: Iterable[str]) -> Image.Image:
    out = blank_canvas()
    order = [
        "BACKGROUND_FILL_do_not_export",
        "WORLD_PREVIEW_do_not_export",
        "PORTRAIT_LEFT",
        "PORTRAIT_RIGHT",
        "FRAME_FRONT_MASK",
        "NAMEPLATE",
        "TEXT",
        "ADVANCE_PROMPT",
    ]
    excluded = set(exclude)
    for name in order:
        if name in excluded:
            continue
        layer = layer_map.get(name)
        if layer is not None:
            paste_sprite(out, layer, 0, 0)
    return out


def build_portrait_layer(
    sprite: Image.Image,
    x: int,
    y: int,
    clip_mask: Image.Image | None = None,
) -> Image.Image:
    layer = blank_canvas()
    paste_sprite(layer, sprite, x, y, clip=(0, 0, CANVAS_W, CANVAS_H))
    if clip_mask is not None:
        return apply_portrait_clip(layer, clip_mask)
    return layer


def scale_nearest(image: Image.Image, factor: int) -> Image.Image:
    return image.resize((image.width * factor, image.height * factor), Image.NEAREST)


def add_review_caption(image: Image.Image, caption: str) -> Image.Image:
    """Append a labeled bar below a review image. Does not alter the source pixels."""
    src = image.convert("RGBA")
    bar_h = 28
    out = Image.new("RGBA", (src.width, src.height + bar_h), (16, 16, 16, 255))
    out.paste(src, (0, 0), src)
    draw = ImageDraw.Draw(out)
    draw.text((8, src.height + 8), caption, fill=(255, 220, 80, 255))
    return out
