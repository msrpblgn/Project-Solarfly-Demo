#!/usr/bin/env python3
"""Compile a finite Tiled TMX Ground+Collision map into Solarfly C arrays.

Supported subset (intentionally narrow):
  - Orthogonal finite TMX, 16x16 tiles
  - Exactly two external TSX tilesets (ground + collision profiles)
  - Exactly two tile layers: Ground and Collision
  - CSV encoding, uncompressed, row-major
  - Map property solarfly_fallback_metatile_id
  - Collision tileset tiles each expose a string property mask=0xHHHH

The GBA never runs this tool. Generated outputs are const ROM data.
"""

from __future__ import annotations

import argparse
import csv
import io
import os
import re
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple
import xml.etree.ElementTree as ET

TILED_FLIP_FLAGS = 0xE0000000
TILED_GID_MASK = 0x1FFFFFFF
MAX_NONEMPTY_PROFILES = 255

REPO_ROOT = Path(__file__).resolve().parents[1]


class CompileError(Exception):
    """Expected validation failure with a concise message."""


@dataclass(frozen=True)
class TilesetInfo:
    firstgid: int
    tilecount: int
    path: Path
    masks_by_local: Optional[Dict[int, int]] = None


@dataclass(frozen=True)
class CompileResult:
    width: int
    height: int
    ground_tilecount: int
    fallback_id: int
    metatile_ids: List[int]
    collision_profile_ids: List[int]
    collision_masks: List[int]
    tmx_path: Path


def repo_relative(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def local_name(tag: str) -> str:
    if "}" in tag:
        return tag.rsplit("}", 1)[1]
    return tag


def require_attr(elem: ET.Element, name: str, path: Path) -> str:
    value = elem.get(name)
    if value is None or value == "":
        raise CompileError(f"{repo_relative(path)}: missing required attribute '{name}'")
    return value


def parse_int_attr(elem: ET.Element, name: str, path: Path) -> int:
    raw = require_attr(elem, name, path)
    try:
        return int(raw)
    except ValueError as exc:
        raise CompileError(
            f"{repo_relative(path)}: attribute '{name}' must be an integer, got '{raw}'"
        ) from exc


def get_property_value(parent: ET.Element, name: str) -> Optional[str]:
    for child in parent:
        if local_name(child.tag) != "properties":
            continue
        for prop in child:
            if local_name(prop.tag) != "property":
                continue
            if prop.get("name") == name:
                return prop.get("value")
    return None


def get_property_typed(
    parent: ET.Element, name: str
) -> Tuple[Optional[str], Optional[str]]:
    for child in parent:
        if local_name(child.tag) != "properties":
            continue
        for prop in child:
            if local_name(prop.tag) != "property":
                continue
            if prop.get("name") == name:
                return prop.get("value"), prop.get("type")
    return None, None


def load_xml(path: Path) -> ET.Element:
    try:
        tree = ET.parse(path)
    except ET.ParseError as exc:
        raise CompileError(f"{repo_relative(path)}: invalid XML ({exc})") from exc
    except OSError as exc:
        raise CompileError(f"{repo_relative(path)}: unable to read file ({exc})") from exc
    return tree.getroot()


def parse_mask_hex(raw: str, tsx_path: Path, local_id: int) -> int:
    text = (raw or "").strip()
    if not re.fullmatch(r"0[xX][0-9A-Fa-f]{1,4}", text):
        raise CompileError(
            f"{repo_relative(tsx_path)}: tile {local_id} mask must be hex like "
            f"0xFFFF, got '{raw}'"
        )
    value = int(text, 16)
    if value < 0 or value > 0xFFFF:
        raise CompileError(
            f"{repo_relative(tsx_path)}: tile {local_id} mask {value} does not fit in u16"
        )
    return value


def parse_ground_tsx(tsx_path: Path) -> Tuple[int, Path]:
    root = load_xml(tsx_path)
    if local_name(root.tag) != "tileset":
        raise CompileError(f"{repo_relative(tsx_path)}: root element must be <tileset>")

    tilewidth = parse_int_attr(root, "tilewidth", tsx_path)
    tileheight = parse_int_attr(root, "tileheight", tsx_path)
    if tilewidth != 16 or tileheight != 16:
        raise CompileError(
            f"{repo_relative(tsx_path)}: tileset tile size must be 16x16, "
            f"got {tilewidth}x{tileheight}"
        )

    tilecount = parse_int_attr(root, "tilecount", tsx_path)
    if tilecount <= 0:
        raise CompileError(f"{repo_relative(tsx_path)}: tilecount must be positive")

    image_elem = None
    for child in root:
        if local_name(child.tag) == "image":
            if image_elem is not None:
                raise CompileError(
                    f"{repo_relative(tsx_path)}: multiple <image> elements are not supported"
                )
            image_elem = child
    if image_elem is None:
        raise CompileError(f"{repo_relative(tsx_path)}: missing preview <image>")

    image_source = require_attr(image_elem, "source", tsx_path)
    image_path = (tsx_path.parent / image_source).resolve()
    if not image_path.is_file():
        raise CompileError(
            f"{repo_relative(tsx_path)}: preview image not found: {image_source}"
        )
    return tilecount, image_path


def parse_collision_tsx(tsx_path: Path) -> Tuple[int, Path, Dict[int, int]]:
    root = load_xml(tsx_path)
    if local_name(root.tag) != "tileset":
        raise CompileError(f"{repo_relative(tsx_path)}: root element must be <tileset>")

    tilewidth = parse_int_attr(root, "tilewidth", tsx_path)
    tileheight = parse_int_attr(root, "tileheight", tsx_path)
    if tilewidth != 16 or tileheight != 16:
        raise CompileError(
            f"{repo_relative(tsx_path)}: collision tileset tile size must be 16x16, "
            f"got {tilewidth}x{tileheight}"
        )

    tilecount = parse_int_attr(root, "tilecount", tsx_path)
    if tilecount <= 0:
        raise CompileError(f"{repo_relative(tsx_path)}: tilecount must be positive")
    if tilecount > MAX_NONEMPTY_PROFILES:
        raise CompileError(
            f"{repo_relative(tsx_path)}: too many nonempty collision profiles "
            f"({tilecount} > {MAX_NONEMPTY_PROFILES})"
        )

    image_elem = None
    masks: Dict[int, int] = {}
    for child in root:
        name = local_name(child.tag)
        if name == "image":
            if image_elem is not None:
                raise CompileError(
                    f"{repo_relative(tsx_path)}: multiple <image> elements are not supported"
                )
            image_elem = child
        elif name == "tile":
            local_id = parse_int_attr(child, "id", tsx_path)
            if local_id < 0 or local_id >= tilecount:
                raise CompileError(
                    f"{repo_relative(tsx_path)}: tile id {local_id} outside "
                    f"[0, {tilecount})"
                )
            if local_id in masks:
                raise CompileError(
                    f"{repo_relative(tsx_path)}: duplicate tile id {local_id}"
                )
            raw_mask, prop_type = get_property_typed(child, "mask")
            if raw_mask is None:
                raise CompileError(
                    f"{repo_relative(tsx_path)}: tile {local_id} missing string "
                    "property 'mask'"
                )
            if prop_type not in (None, "string"):
                raise CompileError(
                    f"{repo_relative(tsx_path)}: tile {local_id} mask property type "
                    f"must be string, got '{prop_type}'"
                )
            masks[local_id] = parse_mask_hex(raw_mask, tsx_path, local_id)

    if image_elem is None:
        raise CompileError(f"{repo_relative(tsx_path)}: missing preview <image>")
    image_source = require_attr(image_elem, "source", tsx_path)
    image_path = (tsx_path.parent / image_source).resolve()
    if not image_path.is_file():
        raise CompileError(
            f"{repo_relative(tsx_path)}: preview image not found: {image_source}"
        )

    for local_id in range(tilecount):
        if local_id not in masks:
            raise CompileError(
                f"{repo_relative(tsx_path)}: missing tile id {local_id} with mask property"
            )
    return tilecount, image_path, masks


def parse_csv_gids(
    data_text: str,
    expected_count: int,
    tmx_path: Path,
    layer_name: str,
) -> List[int]:
    reader = csv.reader(io.StringIO(data_text.strip()))
    gids: List[int] = []
    for row in reader:
        for cell in row:
            cell = cell.strip()
            if not cell:
                continue
            try:
                gid = int(cell)
            except ValueError as exc:
                raise CompileError(
                    f"{repo_relative(tmx_path)}: {layer_name} CSV contains "
                    f"non-integer '{cell}'"
                ) from exc
            gids.append(gid)
    if len(gids) != expected_count:
        raise CompileError(
            f"{repo_relative(tmx_path)}: {layer_name} layer cell count must be "
            f"{expected_count}, got {len(gids)}"
        )
    return gids


def extract_layer_data(
    layer: ET.Element,
    width: int,
    height: int,
    tmx_path: Path,
    layer_name: str,
) -> List[int]:
    layer_width = parse_int_attr(layer, "width", tmx_path)
    layer_height = parse_int_attr(layer, "height", tmx_path)
    if layer_width != width or layer_height != height:
        raise CompileError(
            f"{repo_relative(tmx_path)}: {layer_name} layer size "
            f"{layer_width}x{layer_height} must match map size {width}x{height}"
        )
    if layer.get("offsetx") not in (None, "0") or layer.get("offsety") not in (None, "0"):
        raise CompileError(
            f"{repo_relative(tmx_path)}: {layer_name} layer offsets are not supported"
        )
    if layer.get("parallaxx") not in (None, "1") or layer.get("parallaxy") not in (
        None,
        "1",
    ):
        raise CompileError(
            f"{repo_relative(tmx_path)}: {layer_name} parallax is not supported"
        )

    data_elem = None
    for child in layer:
        name = local_name(child.tag)
        if name == "data":
            if data_elem is not None:
                raise CompileError(
                    f"{repo_relative(tmx_path)}: {layer_name} layer has multiple <data> nodes"
                )
            data_elem = child
        elif name == "properties":
            continue
        else:
            raise CompileError(
                f"{repo_relative(tmx_path)}: unsupported {layer_name} child <{name}>"
            )
    if data_elem is None:
        raise CompileError(
            f"{repo_relative(tmx_path)}: {layer_name} layer missing <data>"
        )

    encoding = data_elem.get("encoding")
    if encoding != "csv":
        raise CompileError(
            f"{repo_relative(tmx_path)}: {layer_name} data encoding must be csv, "
            f"got '{encoding}'"
        )
    if data_elem.get("compression"):
        raise CompileError(
            f"{repo_relative(tmx_path)}: compressed {layer_name} data is not supported"
        )
    for child in data_elem:
        if local_name(child.tag) == "chunk":
            raise CompileError(
                f"{repo_relative(tmx_path)}: chunked {layer_name} layers are not supported"
            )
    return parse_csv_gids(data_elem.text or "", width * height, tmx_path, layer_name)


def gid_owner(
    gid: int,
    ground: TilesetInfo,
    collision: TilesetInfo,
) -> Optional[str]:
    if gid == 0:
        return None
    if ground.firstgid <= gid < ground.firstgid + ground.tilecount:
        return "ground"
    if collision.firstgid <= gid < collision.firstgid + collision.tilecount:
        return "collision"
    return "unknown"


def cell_xy(index: int, width: int) -> str:
    return f"({index % width}, {index // width})"


def gids_to_metatile_ids(
    gids: Sequence[int],
    ground: TilesetInfo,
    collision: TilesetInfo,
    tmx_path: Path,
    width: int,
) -> List[int]:
    ids: List[int] = []
    for index, gid in enumerate(gids):
        if gid & TILED_FLIP_FLAGS:
            raise CompileError(
                f"{repo_relative(tmx_path)}: Ground cell {cell_xy(index, width)} uses "
                f"Tiled flip/transform flags (gid={gid})"
            )
        raw = gid & TILED_GID_MASK
        if raw == 0:
            raise CompileError(
                f"{repo_relative(tmx_path)}: Ground cell {cell_xy(index, width)} "
                "is empty (gid=0)"
            )
        owner = gid_owner(raw, ground, collision)
        if owner == "collision":
            raise CompileError(
                f"{repo_relative(tmx_path)}: Ground cell {cell_xy(index, width)} "
                f"contains collision GID {raw}"
            )
        if owner != "ground":
            raise CompileError(
                f"{repo_relative(tmx_path)}: Ground cell {cell_xy(index, width)} "
                f"unknown gid {raw}"
            )
        local_id = raw - ground.firstgid
        ids.append(local_id)
    return ids


def gids_to_collision_profiles(
    gids: Sequence[int],
    ground: TilesetInfo,
    collision: TilesetInfo,
    tmx_path: Path,
    width: int,
) -> List[int]:
    ids: List[int] = []
    for index, gid in enumerate(gids):
        if gid & TILED_FLIP_FLAGS:
            raise CompileError(
                f"{repo_relative(tmx_path)}: Collision cell {cell_xy(index, width)} uses "
                f"Tiled flip/transform flags (gid={gid})"
            )
        raw = gid & TILED_GID_MASK
        if raw == 0:
            ids.append(0)
            continue
        owner = gid_owner(raw, ground, collision)
        if owner == "ground":
            raise CompileError(
                f"{repo_relative(tmx_path)}: Collision cell {cell_xy(index, width)} "
                f"contains ground GID {raw}"
            )
        if owner != "collision":
            raise CompileError(
                f"{repo_relative(tmx_path)}: Collision cell {cell_xy(index, width)} "
                f"unknown gid {raw}"
            )
        local_id = raw - collision.firstgid
        profile_id = local_id + 1
        if profile_id > 255:
            raise CompileError(
                f"{repo_relative(tmx_path)}: Collision cell {cell_xy(index, width)} "
                f"profile id {profile_id} does not fit in u8"
            )
        ids.append(profile_id)
    return ids


def compile_map(tmx_path: Path) -> CompileResult:
    if not tmx_path.is_file():
        raise CompileError(f"{repo_relative(tmx_path)}: TMX file not found")

    root = load_xml(tmx_path)
    if local_name(root.tag) != "map":
        raise CompileError(f"{repo_relative(tmx_path)}: root element must be <map>")

    orientation = require_attr(root, "orientation", tmx_path)
    if orientation != "orthogonal":
        raise CompileError(
            f"{repo_relative(tmx_path)}: only orthogonal maps are supported, "
            f"got '{orientation}'"
        )
    if root.get("infinite") in ("1", "true", "True"):
        raise CompileError(f"{repo_relative(tmx_path)}: infinite maps are not supported")

    tilewidth = parse_int_attr(root, "tilewidth", tmx_path)
    tileheight = parse_int_attr(root, "tileheight", tmx_path)
    if tilewidth != 16 or tileheight != 16:
        raise CompileError(
            f"{repo_relative(tmx_path)}: map tile size must be 16x16, "
            f"got {tilewidth}x{tileheight}"
        )

    width = parse_int_attr(root, "width", tmx_path)
    height = parse_int_attr(root, "height", tmx_path)
    if width <= 0 or height <= 0:
        raise CompileError(
            f"{repo_relative(tmx_path)}: map dimensions must be positive, "
            f"got {width}x{height}"
        )

    fallback_raw = get_property_value(root, "solarfly_fallback_metatile_id")
    if fallback_raw is None:
        raise CompileError(
            f"{repo_relative(tmx_path)}: missing map property "
            "'solarfly_fallback_metatile_id'"
        )
    try:
        fallback_id = int(fallback_raw)
    except ValueError as exc:
        raise CompileError(
            f"{repo_relative(tmx_path)}: solarfly_fallback_metatile_id must be "
            f"an integer, got '{fallback_raw}'"
        ) from exc

    tileset_elems: List[ET.Element] = []
    layers: List[ET.Element] = []
    for child in root:
        name = local_name(child.tag)
        if name == "tileset":
            tileset_elems.append(child)
        elif name == "layer":
            layers.append(child)
        elif name in ("objectgroup", "imagelayer", "group"):
            raise CompileError(
                f"{repo_relative(tmx_path)}: unsupported layer type <{name}>"
            )
        elif name in ("properties", "editorsettings"):
            continue
        else:
            raise CompileError(
                f"{repo_relative(tmx_path)}: unsupported map child <{name}>"
            )

    if len(tileset_elems) != 2:
        raise CompileError(
            f"{repo_relative(tmx_path)}: exactly two external tilesets are required "
            f"(ground + collision), got {len(tileset_elems)}"
        )

    parsed_tilesets: List[TilesetInfo] = []
    for elem in tileset_elems:
        if elem.get("source") is None:
            raise CompileError(
                f"{repo_relative(tmx_path)}: embedded tilesets are not supported; "
                "use an external TSX"
            )
        firstgid = parse_int_attr(elem, "firstgid", tmx_path)
        tsx_path = (tmx_path.parent / elem.get("source")).resolve()
        if not tsx_path.is_file():
            raise CompileError(
                f"{repo_relative(tmx_path)}: external TSX not found: {elem.get('source')}"
            )
        # Probe for collision masks vs ground preview-only tileset.
        root_tsx = load_xml(tsx_path)
        has_mask = False
        for child in root_tsx:
            if local_name(child.tag) != "tile":
                continue
            raw_mask, _ = get_property_typed(child, "mask")
            if raw_mask is not None:
                has_mask = True
                break
        if has_mask:
            tilecount, _image, masks = parse_collision_tsx(tsx_path)
            parsed_tilesets.append(
                TilesetInfo(firstgid, tilecount, tsx_path, masks_by_local=masks)
            )
        else:
            tilecount, _image = parse_ground_tsx(tsx_path)
            parsed_tilesets.append(TilesetInfo(firstgid, tilecount, tsx_path))

    ground_sets = [t for t in parsed_tilesets if t.masks_by_local is None]
    collision_sets = [t for t in parsed_tilesets if t.masks_by_local is not None]
    if len(ground_sets) != 1 or len(collision_sets) != 1:
        raise CompileError(
            f"{repo_relative(tmx_path)}: need one ground TSX and one collision-profile TSX"
        )
    ground = ground_sets[0]
    collision = collision_sets[0]
    if ground.firstgid != 1:
        raise CompileError(
            f"{repo_relative(tmx_path)}: ground tileset firstgid must be 1, "
            f"got {ground.firstgid}"
        )
    if collision.firstgid != ground.firstgid + ground.tilecount:
        raise CompileError(
            f"{repo_relative(tmx_path)}: collision firstgid must be "
            f"{ground.firstgid + ground.tilecount}, got {collision.firstgid}"
        )

    if fallback_id < 0 or fallback_id >= ground.tilecount:
        raise CompileError(
            f"{repo_relative(tmx_path)}: solarfly_fallback_metatile_id {fallback_id} "
            f"is outside ground tileset range [0, {ground.tilecount})"
        )

    names = [layer.get("name") for layer in layers]
    if names.count("Ground") != 1:
        raise CompileError(
            f"{repo_relative(tmx_path)}: exactly one Ground layer is required"
        )
    if names.count("Collision") != 1:
        raise CompileError(
            f"{repo_relative(tmx_path)}: exactly one Collision layer is required"
        )
    if len(layers) != 2:
        raise CompileError(
            f"{repo_relative(tmx_path)}: exactly two tile layers (Ground, Collision) "
            f"are required, got {len(layers)}"
        )
    if any(n not in ("Ground", "Collision") for n in names):
        raise CompileError(
            f"{repo_relative(tmx_path)}: unsupported tile layer name among {names}"
        )

    ground_layer = next(layer for layer in layers if layer.get("name") == "Ground")
    collision_layer = next(
        layer for layer in layers if layer.get("name") == "Collision"
    )
    ground_gids = extract_layer_data(ground_layer, width, height, tmx_path, "Ground")
    collision_gids = extract_layer_data(
        collision_layer, width, height, tmx_path, "Collision"
    )

    metatile_ids = gids_to_metatile_ids(
        ground_gids, ground, collision, tmx_path, width
    )
    profile_ids = gids_to_collision_profiles(
        collision_gids, ground, collision, tmx_path, width
    )

    assert collision.masks_by_local is not None
    masks = [0]
    for local_id in range(collision.tilecount):
        masks.append(collision.masks_by_local[local_id])

    return CompileResult(
        width=width,
        height=height,
        ground_tilecount=ground.tilecount,
        fallback_id=fallback_id,
        metatile_ids=metatile_ids,
        collision_profile_ids=profile_ids,
        collision_masks=masks,
        tmx_path=tmx_path,
    )


def format_u16_array(values: Sequence[int], values_per_line: int = 16) -> str:
    lines = []
    for i in range(0, len(values), values_per_line):
        chunk = values[i : i + values_per_line]
        lines.append("    " + ", ".join(f"{v}u" for v in chunk) + ",")
    return "\n".join(lines)


def format_u8_array(values: Sequence[int], values_per_line: int = 16) -> str:
    lines = []
    for i in range(0, len(values), values_per_line):
        chunk = values[i : i + values_per_line]
        lines.append("    " + ", ".join(f"{v}u" for v in chunk) + ",")
    return "\n".join(lines)


def format_mask_array(values: Sequence[int]) -> str:
    lines = []
    for value in values:
        lines.append(f"    0x{value:04X}u,")
    return "\n".join(lines)


def generate_header(result: CompileResult, symbol_prefix: str, source_rel: str) -> str:
    guard = f"{symbol_prefix.upper()}_H"
    profile_count = len(result.collision_masks)
    return (
        f"/* AUTOGENERATED, DO NOT EDIT\n"
        f" * Source: {source_rel}\n"
        f" * Tool: tools/compile_tiled_map.py\n"
        f" */\n"
        f"#ifndef {guard}\n"
        f"#define {guard}\n"
        f"\n"
        f"#include \"core/gba_types.h\"\n"
        f"\n"
        f"#define {symbol_prefix}_WIDTH_METATILES {result.width}\n"
        f"#define {symbol_prefix}_HEIGHT_METATILES {result.height}\n"
        f"#define {symbol_prefix}_CELL_COUNT "
        f"({symbol_prefix}_WIDTH_METATILES * {symbol_prefix}_HEIGHT_METATILES)\n"
        f"#define {symbol_prefix}_TILESET_METATILE_COUNT {result.ground_tilecount}\n"
        f"#define {symbol_prefix}_FALLBACK_METATILE_ID {result.fallback_id}\n"
        f"#define {symbol_prefix}_COLLISION_PROFILE_COUNT {profile_count}\n"
        f"\n"
        f"extern const u16 {symbol_prefix}MetatileIds[{symbol_prefix}_CELL_COUNT];\n"
        f"extern const u8 {symbol_prefix}CollisionProfileIds"
        f"[{symbol_prefix}_CELL_COUNT];\n"
        f"extern const u16 {symbol_prefix}CollisionProfileMasks"
        f"[{symbol_prefix}_COLLISION_PROFILE_COUNT];\n"
        f"\n"
        f"#endif\n"
    )


def generate_c(
    result: CompileResult,
    symbol_prefix: str,
    header_rel: str,
    source_rel: str,
) -> str:
    return (
        f"/* AUTOGENERATED, DO NOT EDIT\n"
        f" * Source: {source_rel}\n"
        f" * Tool: tools/compile_tiled_map.py\n"
        f" */\n"
        f"#include \"{header_rel}\"\n"
        f"\n"
        f"const u16 {symbol_prefix}MetatileIds[{symbol_prefix}_CELL_COUNT] = {{\n"
        f"{format_u16_array(result.metatile_ids)}\n"
        f"}};\n"
        f"\n"
        f"const u8 {symbol_prefix}CollisionProfileIds[{symbol_prefix}_CELL_COUNT] = {{\n"
        f"{format_u8_array(result.collision_profile_ids)}\n"
        f"}};\n"
        f"\n"
        f"const u16 {symbol_prefix}CollisionProfileMasks"
        f"[{symbol_prefix}_COLLISION_PROFILE_COUNT] = {{\n"
        f"{format_mask_array(result.collision_masks)}\n"
        f"}};\n"
    )


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(
        prefix=path.name + ".",
        suffix=".tmp",
        dir=str(path.parent),
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        os.replace(tmp_name, path)
    except Exception:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise


def files_match(path: Path, text: str) -> bool:
    if not path.is_file():
        return False
    return path.read_text(encoding="utf-8") == text


def run_compile(args: argparse.Namespace) -> int:
    tmx_path = Path(args.input)
    out_c = Path(args.output_c)
    out_h = Path(args.output_h)
    symbol_prefix = args.symbol_prefix

    result = compile_map(tmx_path)
    source_rel = repo_relative(result.tmx_path)
    try:
        header_rel = out_h.resolve().relative_to(REPO_ROOT / "include").as_posix()
    except ValueError:
        header_rel = out_h.name

    header_text = generate_header(result, symbol_prefix, source_rel)
    c_text = generate_c(result, symbol_prefix, header_rel, source_rel)

    if args.check:
        ok = files_match(out_c, c_text) and files_match(out_h, header_text)
        if not ok:
            raise CompileError(
                f"{repo_relative(tmx_path)}: generated outputs are missing or stale"
            )
        return 0

    atomic_write_text(out_h, header_text)
    atomic_write_text(out_c, c_text)
    return 0


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compile Tiled Ground+Collision layers into Solarfly C map data."
    )
    parser.add_argument("--input", required=True, help="Path to input TMX")
    parser.add_argument("--output-c", required=True, help="Path to generated .c")
    parser.add_argument("--output-h", required=True, help="Path to generated .h")
    parser.add_argument(
        "--symbol-prefix",
        required=True,
        help="C symbol prefix, e.g. gOverworldTestMap",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate that outputs already match compiler results",
    )
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    try:
        return run_compile(args)
    except CompileError as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
