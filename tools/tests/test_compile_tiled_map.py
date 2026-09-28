#!/usr/bin/env python3
"""Unit tests for tools/compile_tiled_map.py (OW-3 + OW-5)."""

from __future__ import annotations

import shutil
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import compile_tiled_map as compiler  # noqa: E402

DIAG_TMX = ROOT / "assets_src" / "overworld" / "maps" / "overworld_test.tmx"
OW2_IDS = ROOT / "tools" / "tests" / "fixtures" / "overworld_test_expected_ids.csv"


def write_mini_png(path: Path, width: int, height: int) -> None:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw = bytearray()
    for _y in range(height):
        raw.append(0)
        raw.extend(bytes([255, 0, 0, 255]) * width)
    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png.extend(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
    png.extend(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
    png.extend(chunk(b"IEND", b""))
    path.write_bytes(png)


class CompileTiledMapTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = Path(tempfile.mkdtemp(prefix="solarfly_map_test_"))
        self.addCleanup(shutil.rmtree, self.temp_dir, True)

    def _copy_diag(self) -> Path:
        maps_dir = self.temp_dir / "maps"
        tilesets_dir = self.temp_dir / "tilesets"
        maps_dir.mkdir()
        tilesets_dir.mkdir()
        tmx = maps_dir / "overworld_test.tmx"
        shutil.copy2(DIAG_TMX, tmx)
        for name in (
            "overworld_test.tsx",
            "overworld_test_preview.png",
            "overworld_collision_profiles.tsx",
            "overworld_collision_profiles.png",
        ):
            shutil.copy2(
                ROOT / "assets_src" / "overworld" / "tilesets" / name,
                tilesets_dir / name,
            )
        return tmx

    def test_valid_diagnostic_map(self) -> None:
        tmx = self._copy_diag()
        result = compiler.compile_map(tmx)
        self.assertEqual(result.width, 40)
        self.assertEqual(result.height, 30)
        self.assertEqual(result.ground_tilecount, 3)
        self.assertEqual(result.fallback_id, 0)
        self.assertEqual(len(result.metatile_ids), 1200)
        self.assertEqual(len(result.collision_profile_ids), 1200)
        self.assertEqual(result.collision_masks, [0x0000, 0xFFFF, 0x00FF, 0xFF00, 0x3333, 0xCCCC])
        expected = [int(x) for x in OW2_IDS.read_text(encoding="utf-8").split(",") if x]
        self.assertEqual(result.metatile_ids, expected)
        self.assertEqual(result.metatile_ids.count(0), 1198)
        self.assertEqual(result.metatile_ids.count(1), 1)
        self.assertEqual(result.metatile_ids.count(2), 1)
        self.assertEqual(result.metatile_ids[8 * 40 + 10], 1)
        self.assertEqual(result.metatile_ids[6 * 40 + 9], 2)
        self.assertTrue(all(v == 0 for v in result.collision_profile_ids))

    def test_empty_collision_and_local_zero_mapping(self) -> None:
        tmx = self._copy_diag()
        result = compiler.compile_map(tmx)
        self.assertEqual(result.collision_profile_ids[0], 0)
        self.assertEqual(result.collision_profile_ids[3 * 40 + 12], 0)

    def test_deterministic_generation(self) -> None:
        tmx = self._copy_diag()
        out_c = self.temp_dir / "out.c"
        out_h = self.temp_dir / "out.h"
        args = [
            "--input",
            str(tmx),
            "--output-c",
            str(out_c),
            "--output-h",
            str(out_h),
            "--symbol-prefix",
            "gOverworldTestMap",
        ]
        self.assertEqual(compiler.main(args), 0)
        first_c = out_c.read_bytes()
        first_h = out_h.read_bytes()
        self.assertEqual(compiler.main(args), 0)
        self.assertEqual(out_c.read_bytes(), first_c)
        self.assertEqual(out_h.read_bytes(), first_h)
        self.assertEqual(compiler.main(args + ["--check"]), 0)
        self.assertIn(b"CollisionProfileIds", first_c)
        self.assertIn(b"0xFFFF", first_c)

    def _write_pair_map(
        self,
        *,
        ground_csv: str = "1,1,\n1,1",
        collision_csv: str = "0,0,\n0,0",
        width: int = 2,
        height: int = 2,
        include_collision_layer: bool = True,
        collision_mask: str = "0xFFFF",
        collision_tilecount: int = 1,
        ground_in_collision: bool = False,
        collision_in_ground: bool = False,
        flip_collision: bool = False,
        missing_mask: bool = False,
        bad_mask: str = "",
        extra_layer: bool = False,
        encoding: str = "csv",
        compression: str = "",
        infinite: str = "0",
        tilewidth: int = 16,
        tileheight: int = 16,
        wrong_collision_size: bool = False,
    ) -> Path:
        tilesets = self.temp_dir / "tilesets"
        maps = self.temp_dir / "maps"
        tilesets.mkdir(exist_ok=True)
        maps.mkdir(exist_ok=True)
        write_mini_png(tilesets / "ground.png", 16, 16)
        write_mini_png(tilesets / "collision.png", 16 * max(1, collision_tilecount), 16)

        ground_tsx = tilesets / "ground.tsx"
        ground_tsx.write_text(
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            '<tileset name="ground" tilewidth="16" tileheight="16" tilecount="1" columns="1">\n'
            ' <image source="ground.png" width="16" height="16"/>\n'
            "</tileset>\n",
            encoding="utf-8",
            newline="\n",
        )

        tile_xml = ""
        for i in range(collision_tilecount):
            if missing_mask and i == 0:
                tile_xml += f' <tile id="{i}"/>\n'
            else:
                mask = bad_mask if bad_mask and i == 0 else collision_mask
                tile_xml += (
                    f' <tile id="{i}">\n'
                    "  <properties>\n"
                    f'   <property name="mask" type="string" value="{mask}"/>\n'
                    "  </properties>\n"
                    " </tile>\n"
                )
        collision_tsx = tilesets / "collision.tsx"
        collision_tsx.write_text(
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<tileset name="collision" tilewidth="16" tileheight="16" '
            f'tilecount="{collision_tilecount}" columns="{collision_tilecount}">\n'
            f' <image source="collision.png" width="{16 * collision_tilecount}" height="16"/>\n'
            f"{tile_xml}"
            "</tileset>\n",
            encoding="utf-8",
            newline="\n",
        )

        if ground_in_collision:
            collision_csv = "1,0,\n0,0"
        if collision_in_ground:
            ground_csv = "2,1,\n1,1"
        if flip_collision:
            collision_csv = "2147483650,0,\n0,0"

        compression_attr = f' compression="{compression}"' if compression else ""
        coll_w = 1 if wrong_collision_size else width
        coll_h = 1 if wrong_collision_size else height
        collision_xml = ""
        if include_collision_layer:
            collision_xml = (
                f' <layer id="2" name="Collision" width="{coll_w}" height="{coll_h}">\n'
                f'  <data encoding="{encoding}"{compression_attr}>\n'
                f"{collision_csv}\n"
                "  </data>\n"
                " </layer>\n"
            )
        extra = ""
        if extra_layer:
            extra = (
                ' <layer id="3" name="Extra" width="2" height="2">\n'
                '  <data encoding="csv">\n1,1,1,1\n  </data>\n'
                " </layer>\n"
            )

        tmx = maps / "pair.tmx"
        tmx.write_text(
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<map orientation="orthogonal" width="{width}" height="{height}" '
            f'tilewidth="{tilewidth}" tileheight="{tileheight}" infinite="{infinite}">\n'
            " <properties>\n"
            '  <property name="solarfly_fallback_metatile_id" type="int" value="0"/>\n'
            " </properties>\n"
            ' <tileset firstgid="1" source="../tilesets/ground.tsx"/>\n'
            ' <tileset firstgid="2" source="../tilesets/collision.tsx"/>\n'
            f' <layer id="1" name="Ground" width="{width}" height="{height}">\n'
            f'  <data encoding="{encoding}"{compression_attr}>\n'
            f"{ground_csv}\n"
            "  </data>\n"
            " </layer>\n"
            f"{collision_xml}"
            f"{extra}"
            "</map>\n",
            encoding="utf-8",
            newline="\n",
        )
        return tmx

    def test_wrong_tile_dimensions(self) -> None:
        tmx = self._write_pair_map(tilewidth=8, tileheight=8)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_missing_collision_layer(self) -> None:
        tmx = self._write_pair_map(include_collision_layer=False)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_wrong_collision_dimensions(self) -> None:
        tmx = self._write_pair_map(wrong_collision_size=True)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_ground_gid_in_collision(self) -> None:
        tmx = self._write_pair_map(ground_in_collision=True)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_collision_gid_in_ground(self) -> None:
        tmx = self._write_pair_map(collision_in_ground=True)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_flipped_gid(self) -> None:
        tmx = self._write_pair_map(flip_collision=True)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_missing_mask(self) -> None:
        tmx = self._write_pair_map(missing_mask=True)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_invalid_mask_text(self) -> None:
        tmx = self._write_pair_map(bad_mask="FFFF")
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_oversized_mask(self) -> None:
        tmx = self._write_pair_map(bad_mask="0x1FFFF")
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_too_many_profiles(self) -> None:
        tmx = self._write_pair_map(collision_tilecount=256)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_extra_layer(self) -> None:
        tmx = self._write_pair_map(extra_layer=True)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_non_csv_encoding(self) -> None:
        tmx = self._write_pair_map(encoding="base64")
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_compressed_data(self) -> None:
        tmx = self._write_pair_map(compression="zlib")
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_infinite_map(self) -> None:
        tmx = self._write_pair_map(infinite="1")
        with self.assertRaises(compiler.CompileError):
            compiler.compile_map(tmx)

    def test_check_mode_detects_stale_output(self) -> None:
        tmx = self._copy_diag()
        out_c = self.temp_dir / "stale.c"
        out_h = self.temp_dir / "stale.h"
        args = [
            "--input",
            str(tmx),
            "--output-c",
            str(out_c),
            "--output-h",
            str(out_h),
            "--symbol-prefix",
            "gOverworldTestMap",
        ]
        self.assertEqual(compiler.main(args), 0)
        out_c.write_text(out_c.read_text(encoding="utf-8") + "/* stale */\n", encoding="utf-8")
        self.assertEqual(compiler.main(args + ["--check"]), 1)


if __name__ == "__main__":
    unittest.main()
