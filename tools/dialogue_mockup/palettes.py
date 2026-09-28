"""GBA-compatible portrait palette reduction and dim variants."""

from __future__ import annotations

from dataclasses import dataclass

from PIL import Image


@dataclass(frozen=True)
class PaletteEntry:
    index: int
    rgb888: tuple[int, int, int]
    bgr555: int
    role: str


@dataclass
class PortraitPaletteSet:
    active: Image.Image
    inactive: Image.Image
    entries: list[PaletteEntry]
    inactive_entries: list[PaletteEntry]


def rgb888_to_bgr555(r: int, g: int, b: int) -> int:
    r5 = r >> 3
    g5 = g >> 3
    b5 = b >> 3
    return (b5 << 10) | (g5 << 5) | r5


def bgr555_to_rgb888(value: int) -> tuple[int, int, int]:
    r5 = value & 0x1F
    g5 = (value >> 5) & 0x1F
    b5 = (value >> 10) & 0x1F
    return (r5 * 255 // 31, g5 * 255 // 31, b5 * 255 // 31)


def quantize_rgb888_to_bgr555(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    return bgr555_to_rgb888(rgb888_to_bgr555(*rgb))


def _color_distance(a: tuple[int, int, int], b: tuple[int, int, int]) -> int:
    return sum((a[i] - b[i]) ** 2 for i in range(3))


def _nearest_palette_index(
    rgb: tuple[int, int, int], palette: list[tuple[int, int, int]]
) -> int:
    best = 0
    best_dist = 1 << 30
    for index, entry in enumerate(palette):
        dist = _color_distance(rgb, entry)
        if dist < best_dist:
            best = index
            best_dist = dist
    return best


def _merge_closest_pair(colors: list[tuple[int, int, int]]) -> list[tuple[int, int, int]]:
    best_i = 0
    best_j = 1
    best_dist = 1 << 30
    for i in range(len(colors)):
        for j in range(i + 1, len(colors)):
            dist = _color_distance(colors[i], colors[j])
            if dist < best_dist:
                best_dist = dist
                best_i = i
                best_j = j
    merged = []
    for index, color in enumerate(colors):
        if index == best_i:
            a = colors[best_i]
            b = colors[best_j]
            merged.append(
                (
                    (a[0] + b[0]) // 2,
                    (a[1] + b[1]) // 2,
                    (a[2] + b[2]) // 2,
                )
            )
        elif index == best_j:
            continue
        else:
            merged.append(color)
    merged.sort()
    return merged


def build_opaque_palette(colors: set[tuple[int, int, int]], max_opaque: int = 15) -> list[tuple[int, int, int]]:
    ordered = sorted(colors)
    if (0, 0, 0) in ordered:
        ordered.remove((0, 0, 0))
        palette = [(0, 0, 0)]
    else:
        palette = []
    palette.extend(ordered)
    while len(palette) > max_opaque:
        non_black = palette[1:] if palette and palette[0] == (0, 0, 0) else palette[:]
        if palette and palette[0] == (0, 0, 0):
            merged = _merge_closest_pair(non_black)
            palette = [(0, 0, 0)] + merged
        else:
            palette = _merge_closest_pair(palette)
    palette = [quantize_rgb888_to_bgr555(color) for color in palette]
    # Re-merge after quantization if needed.
    unique = []
    for color in palette:
        if color not in unique:
            unique.append(color)
    palette = unique
    while len(palette) > max_opaque:
        if palette and palette[0] == (0, 0, 0):
            merged = _merge_closest_pair(palette[1:])
            palette = [(0, 0, 0)] + merged
        else:
            palette = _merge_closest_pair(palette)
    return palette


def dim_opaque_rgb(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    if rgb == (0, 0, 0):
        return (0, 0, 0)
    target = (36, 36, 36)
    factor = 0.58
    dimmed = tuple(
        int(rgb[i] * factor + target[i] * (1.0 - factor)) for i in range(3)
    )
    return quantize_rgb888_to_bgr555(dimmed)


def apply_palette(image: Image.Image, palette: list[tuple[int, int, int]]) -> Image.Image:
    src = image.convert("RGBA")
    out = Image.new("RGBA", src.size, (0, 0, 0, 0))
    src_px = src.load()
    out_px = out.load()
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = src_px[x, y]
            if a == 0:
                continue
            rgb = quantize_rgb888_to_bgr555((r, g, b))
            index = _nearest_palette_index(rgb, palette)
            pr, pg, pb = palette[index]
            out_px[x, y] = (pr, pg, pb, 255)
    return out


def classify_role(rgb: tuple[int, int, int], index: int) -> str:
    if rgb == (0, 0, 0):
        return "outline"
    if index == 0:
        return "reserved_transparent"
    lum = 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]
    if lum >= 170:
        return "highlight"
    if lum >= 95:
        return "midtone"
    return "shadow"


def create_portrait_palette_set(image: Image.Image, name: str) -> PortraitPaletteSet:
    src = image.convert("RGBA")
    opaque: set[tuple[int, int, int]] = set()
    px = src.load()
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = px[x, y]
            if a > 0:
                opaque.add(quantize_rgb888_to_bgr555((r, g, b)))
    opaque_palette = build_opaque_palette(opaque, max_opaque=15)
    active = apply_palette(src, opaque_palette)
    dim_palette = [dim_opaque_rgb(color) for color in opaque_palette]
    # Preserve black exactly.
    if opaque_palette and opaque_palette[0] == (0, 0, 0):
        dim_palette[0] = (0, 0, 0)
    dim_palette = list(dict.fromkeys(dim_palette))
    while len(dim_palette) > 15:
        if dim_palette[0] == (0, 0, 0):
            dim_palette = [(0, 0, 0)] + _merge_closest_pair(dim_palette[1:])
        else:
            dim_palette = _merge_closest_pair(dim_palette)
    inactive = apply_palette(src, dim_palette)

    entries = [
        PaletteEntry(
            index=0,
            rgb888=(0, 0, 0),
            bgr555=0,
            role="transparent",
        )
    ]
    for i, rgb in enumerate(opaque_palette, start=1):
        entries.append(
            PaletteEntry(
                index=i,
                rgb888=rgb,
                bgr555=rgb888_to_bgr555(*rgb),
                role=classify_role(rgb, i),
            )
        )
    inactive_entries = [
        PaletteEntry(
            index=0,
            rgb888=(0, 0, 0),
            bgr555=0,
            role="transparent",
        )
    ]
    for i, rgb in enumerate(dim_palette, start=1):
        inactive_entries.append(
            PaletteEntry(
                index=i,
                rgb888=rgb,
                bgr555=rgb888_to_bgr555(*rgb),
                role=classify_role(rgb, i),
            )
        )
    return PortraitPaletteSet(
        active=active,
        inactive=inactive,
        entries=entries,
        inactive_entries=inactive_entries,
    )


def normalize_protagonist(source: Image.Image) -> tuple[Image.Image, dict[str, object]]:
    src = source.convert("RGBA")
    if src.size != (66, 66):
        raise ValueError(f"expected 66x66 protagonist source, got {src.size}")
    candidate = src.crop((1, 1, 65, 65))
    if candidate.size != (64, 64):
        raise ValueError("protagonist normalization did not produce 64x64")

    src_opaque = {(x, y) for y in range(66) for x in range(66) if src.getpixel((x, y))[3] > 0}
    cand_opaque = {(x + 1, y + 1) for y in range(64) for x in range(64) if candidate.getpixel((x, y))[3] > 0}
    proof = {
        "source_size": src.size,
        "candidate_size": candidate.size,
        "source_opaque_count": len(src_opaque),
        "candidate_opaque_count": len(cand_opaque),
        "opaque_preserved": src_opaque == cand_opaque,
        "crop_box": (1, 1, 65, 65),
    }
    if not proof["opaque_preserved"]:
        missing = sorted(src_opaque - cand_opaque)
        raise ValueError(f"protagonist normalization lost opaque pixels: {missing[:8]}")
    return candidate, proof
