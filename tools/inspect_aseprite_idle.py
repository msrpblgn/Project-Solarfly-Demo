#!/usr/bin/env python3
"""Inspect / export Protagonist idle .aseprite files (ASE binary format)."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

from PIL import Image

ASE_MAGIC = 0xA5E0
FRAME_MAGIC = 0xF1FA
CHUNK_LAYER = 0x2004
CHUNK_CEL = 0x2005
CHUNK_PALETTE_OLD = 0x0004
CHUNK_PALETTE = 0x2019
CHUNK_COLOR_PROFILE = 0x2007


def read_string(data: bytes, offset: int) -> tuple[str, int]:
    (length,) = struct.unpack_from("<H", data, offset)
    offset += 2
    raw = data[offset : offset + length]
    offset += length
    return raw.decode("utf-8", errors="replace"), offset


def parse_ase(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) < 128:
        raise SystemExit(f"{path}: too small")
    file_size, magic, frames, width, height, color_depth, flags = struct.unpack_from(
        "<IHHHHHI", data, 0
    )
    if magic != ASE_MAGIC:
        raise SystemExit(f"{path}: bad magic {magic:#x}")
    if file_size != len(data):
        # Some exporters leave size loose; still try.
        pass
    if color_depth not in (8, 16, 32):
        raise SystemExit(f"{path}: unsupported color depth {color_depth}")

    palette = [(0, 0, 0, 0)] * 256
    layers: list[dict] = []
    frame_cels: list[list[dict]] = []

    offset = 128
    for frame_index in range(frames):
        if offset + 16 > len(data):
            raise SystemExit(f"{path}: truncated frame header {frame_index}")
        frame_bytes, frame_magic, old_chunks, _duration, _pad, new_chunks = (
            struct.unpack_from("<IHHH2sI", data, offset)
        )
        if frame_magic != FRAME_MAGIC:
            raise SystemExit(f"{path}: bad frame magic at {offset:#x}")
        chunk_count = new_chunks if new_chunks else old_chunks
        chunk_offset = offset + 16
        frame_end = offset + frame_bytes
        cels: list[dict] = []
        for _ in range(chunk_count):
            if chunk_offset + 6 > frame_end:
                break
            (chunk_size, chunk_type) = struct.unpack_from("<IH", data, chunk_offset)
            body = data[chunk_offset + 6 : chunk_offset + chunk_size]
            body_off = 0
            if chunk_type == CHUNK_LAYER:
                flags, type_, child, _dw, _dh, _blend, opacity, _res = (
                    struct.unpack_from("<HHHHHHB3s", body, 0)
                )
                body_off = 16
                name, _ = read_string(body, body_off)
                layers.append(
                    {
                        "name": name,
                        "flags": flags,
                        "type": type_,
                        "child": child,
                        "opacity": opacity,
                        "visible": bool(flags & 1),
                    }
                )
            elif chunk_type == CHUNK_CEL:
                layer_index, x, y, opacity, cel_type = struct.unpack_from(
                    "<HhhBH", body, 0
                )
                body_off = 16
                cel = {
                    "layer": layer_index,
                    "x": x,
                    "y": y,
                    "opacity": opacity,
                    "type": cel_type,
                }
                if cel_type == 0:  # raw
                    w, h = struct.unpack_from("<HH", body, body_off)
                    body_off += 4
                    pixels = body[body_off : body_off + w * h * (color_depth // 8)]
                    cel.update({"w": w, "h": h, "pixels": pixels})
                    cels.append(cel)
                elif cel_type == 2:  # compressed
                    w, h = struct.unpack_from("<HH", body, body_off)
                    body_off += 4
                    pixels = zlib.decompress(body[body_off:])
                    cel.update({"w": w, "h": h, "pixels": pixels})
                    cels.append(cel)
                elif cel_type == 1:
                    # linked cel
                    (frame_pos,) = struct.unpack_from("<H", body, body_off)
                    cel["link"] = frame_pos
                    cels.append(cel)
            elif chunk_type == CHUNK_PALETTE:
                (color_count,) = struct.unpack_from("<I", body, 0)
                body_off = 20
                for i in range(color_count):
                    flags, r, g, b, a = struct.unpack_from("<HBBBB", body, body_off)
                    body_off += 6
                    if flags & 1:
                        _name, body_off = read_string(body, body_off)
                    first = struct.unpack_from("<I", body, 4)[0]
                    palette[first + i] = (r, g, b, a)
            elif chunk_type == CHUNK_PALETTE_OLD:
                packets = struct.unpack_from("<H", body, 0)[0]
                body_off = 2
                index = 0
                for _ in range(packets):
                    skip, count = struct.unpack_from("<BB", body, body_off)
                    body_off += 2
                    index += skip
                    n = 256 if count == 0 else count
                    for _c in range(n):
                        r, g, b = struct.unpack_from("<BBB", body, body_off)
                        body_off += 3
                        palette[index] = (r, g, b, 255)
                        index += 1
            chunk_offset += chunk_size
        frame_cels.append(cels)
        offset = frame_end

    return {
        "path": path,
        "frames": frames,
        "width": width,
        "height": height,
        "color_depth": color_depth,
        "palette": palette,
        "layers": layers,
        "frame_cels": frame_cels,
    }


def layer_is_guide(name: str) -> bool:
    lower = name.lower()
    return ("sketch" in lower) or ("reference" in lower) or ("sketc" in lower)


def render_frame(doc: dict, frame_index: int, mode: str = "art") -> Image.Image:
    """mode: 'art' = non-guide layers (incl. hidden finished), 'visible', 'all'."""
    w, h = doc["width"], doc["height"]
    depth = doc["color_depth"]
    canvas = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    cels = doc["frame_cels"][frame_index]
    for cel in cels:
        layer = doc["layers"][cel["layer"]] if cel["layer"] < len(doc["layers"]) else None
        if layer is not None and layer.get("type", 0) != 0:
            continue  # skip groups
        if mode == "visible":
            if layer is not None and not layer.get("visible", True):
                continue
        elif mode == "art":
            if layer is not None and layer_is_guide(layer.get("name", "")):
                continue
        if cel.get("type") == 1:
            link = cel["link"]
            src_cels = [
                c
                for c in doc["frame_cels"][link]
                if c.get("layer") == cel["layer"] and "pixels" in c
            ]
            if not src_cels:
                continue
            src = src_cels[0]
            px = src["pixels"]
            cw, ch = src["w"], src["h"]
            x, y = cel["x"], cel["y"]
        else:
            if "pixels" not in cel:
                continue
            px = cel["pixels"]
            cw, ch = cel["w"], cel["h"]
            x, y = cel["x"], cel["y"]
        if depth == 8:
            for row in range(ch):
                for col in range(cw):
                    idx = px[row * cw + col]
                    r, g, b, a = doc["palette"][idx]
                    if a == 0:
                        continue
                    px_x, px_y = x + col, y + row
                    if 0 <= px_x < w and 0 <= px_y < h:
                        canvas.putpixel((px_x, px_y), (r, g, b, a))
        elif depth == 32:
            for row in range(ch):
                for col in range(cw):
                    i = (row * cw + col) * 4
                    r, g, b, a = px[i : i + 4]
                    if a == 0:
                        continue
                    px_x, px_y = x + col, y + row
                    if 0 <= px_x < w and 0 <= px_y < h:
                        canvas.putpixel((px_x, px_y), (r, g, b, a))
        else:
            raise SystemExit(f"unsupported depth {depth} for render")
    return canvas


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--export-dir", type=Path, default=None)
    args = ap.parse_args()

    if args.export_dir:
        args.export_dir.mkdir(parents=True, exist_ok=True)

    for path in args.files:
        doc = parse_ase(path)
        print(
            f"{path.name}: {doc['frames']} frame(s), "
            f"{doc['width']}x{doc['height']}, depth={doc['color_depth']}, "
            f"layers={[l['name'] for l in doc['layers']]}"
        )
        for fi in range(doc["frames"]):
            img = render_frame(doc, fi, mode="visible")
            if sum(1 for p in img.getdata() if p[3] > 0) <= 0:
                img = render_frame(doc, fi, mode="all")
            opaque = sum(1 for p in img.getdata() if p[3] > 0)
            print(f"  frame {fi}: opaque_pixels={opaque}")
            if args.export_dir:
                out = args.export_dir / f"{path.stem}_f{fi}.png"
                img.save(out)
                print(f"    wrote {out}")


if __name__ == "__main__":
    main()
