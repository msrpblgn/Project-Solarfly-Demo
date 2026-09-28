#!/usr/bin/env bash
# Regenerate Aksil front sprite C bitmap from assets_src PNG via devkitPro grit.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/assets_src/sprites/aksil_front_sprite.png"
TMP="$ROOT/build/aksil_front_sprite_keyed.png"
OUT="$ROOT/src/assets/aksil_front_sprite"
GRIT="${DEVKITPRO:-/opt/devkitpro}/tools/bin/grit"
PREPROCESS="$ROOT/tools/preprocess_aksil_sprite.py"

if [[ ! -f "$SRC" ]]; then
  echo "Missing source PNG: $SRC" >&2
  exit 1
fi

mkdir -p "$ROOT/build"

PYTHON=""
if command -v py >/dev/null 2>&1; then
  PYTHON="py -3"
elif [[ -x /mnt/c/Windows/py.exe ]]; then
  PYTHON="/mnt/c/Windows/py.exe -3"
elif command -v python3 >/dev/null 2>&1; then
  PYTHON="python3"
fi

if [[ -z "$PYTHON" ]]; then
  echo "Python with Pillow is required to preprocess Aksil sprite transparency." >&2
  exit 1
fi

if [[ "$PYTHON" == "/mnt/c/Windows/py.exe -3" ]]; then
  WIN_SRC="$(wslpath -w "$SRC")"
  WIN_TMP="$(wslpath -w "$TMP")"
  WIN_PREPROCESS="$(wslpath -w "$PREPROCESS")"
  /mnt/c/Windows/py.exe -3 "$WIN_PREPROCESS" "$WIN_SRC" "$WIN_TMP"
else
  $PYTHON "$PREPROCESS" "$SRC" "$TMP"
fi

"$GRIT" "$TMP" -gb -gB16 -gu16 -gTFF00FF -m! -ftc -o "$OUT"

python_postprocess() {
  local cfile="$1"
  if [[ "$PYTHON" == "/mnt/c/Windows/py.exe -3" ]]; then
    WIN_CFILE="$(wslpath -w "$cfile")"
    /mnt/c/Windows/py.exe -3 - "$WIN_CFILE" <<'PY'
import sys
path = sys.argv[1]
with open(path, "r", encoding="utf-8") as handle:
    data = handle.read()
if '#include "assets/aksil_front_sprite.h"' not in data:
    data = '#include "assets/aksil_front_sprite.h"\n\n' + data
data = data.replace(
    "const unsigned short aksil_front_spriteBitmap[4096]",
    "const u16 aksil_front_spriteBitmap[AKSIL_FRONT_SPRITE_PIXEL_COUNT]",
)
data = data.replace(' __attribute__((visibility("hidden")))', "")
if data.startswith("#include \"assets/aksil_front_sprite.h\"\n\n\n"):
    data = data.replace(
        "#include \"assets/aksil_front_sprite.h\"\n\n\n",
        "#include \"assets/aksil_front_sprite.h\"\n\n",
        1,
    )
with open(path, "w", encoding="utf-8") as handle:
    handle.write(data)
PY
  elif [[ -n "$PYTHON" ]]; then
    $PYTHON - "$cfile" <<'PY'
import sys
path = sys.argv[1]
with open(path, "r", encoding="utf-8") as handle:
    data = handle.read()
if '#include "assets/aksil_front_sprite.h"' not in data:
    data = '#include "assets/aksil_front_sprite.h"\n\n' + data
data = data.replace(
    "const unsigned short aksil_front_spriteBitmap[4096]",
    "const u16 aksil_front_spriteBitmap[AKSIL_FRONT_SPRITE_PIXEL_COUNT]",
)
data = data.replace(' __attribute__((visibility("hidden")))', "")
if data.startswith("#include \"assets/aksil_front_sprite.h\"\n\n\n"):
    data = data.replace(
        "#include \"assets/aksil_front_sprite.h\"\n\n\n",
        "#include \"assets/aksil_front_sprite.h\"\n\n",
        1,
    )
with open(path, "w", encoding="utf-8") as handle:
    handle.write(data)
PY
  else
    sed -i '1i #include "assets/aksil_front_sprite.h"\n' "$cfile"
    sed -i 's/const unsigned short aksil_front_spriteBitmap\[4096\]/const u16 aksil_front_spriteBitmap[AKSIL_FRONT_SPRITE_PIXEL_COUNT]/' "$cfile"
  fi
}

python_postprocess "$OUT.c"
echo "Generated $OUT.c with magenta transparency key"
