#!/usr/bin/env bash
# Placeholder asset conversion script for future pipeline.
# Milestone 1 uses C-drawn placeholder rectangles; no binary conversion required yet.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/assets_src"
OUT="$ROOT/src/assets"

echo "Project Solarfly GBA asset converter (placeholder)"
echo "Source: $SRC"
echo "Output: $OUT"
echo ""
echo "TODO: integrate grit, png2gba, or custom tools for:"
echo "  - sprites/*.png -> sprite tile data"
echo "  - backgrounds/*.png -> BG tilemaps"
echo "  - ui/font.png -> glyph tiles"
echo ""
echo "No conversion performed in milestone 1."
