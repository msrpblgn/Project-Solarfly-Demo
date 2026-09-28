# Asset Pipeline Notes

## Milestone 1

- Battle visuals use **Mode 3** placeholder rectangles drawn in `placeholder_sprites.c` and `placeholder_backgrounds.c`.
- `assets_src/` holds future source PNGs; they are **not** linked into the ROM yet.

## Planned pipeline

1. Author PNGs in `assets_src/sprites`, `assets_src/backgrounds`, `assets_src/ui`.
2. Convert with grit (devkitPro) or a project-specific tool.
3. Emit C arrays into `src/assets/` with matching `asset_ids.h` entries.
4. Replace placeholder draw calls in `battle_hud.c` with real OAM/BG blits.

## Reserved source files

```text
assets_src/sprites/protagonist_placeholder.png
assets_src/sprites/enemy_placeholder.png
assets_src/sprites/target_cursor.png
assets_src/sprites/ui_cursor.png
assets_src/backgrounds/battle_bg_placeholder.png
assets_src/ui/font.png
assets_src/ui/textbox_tiles.png
```

Add PNGs when art is ready; run `tools/convert_assets.sh` after the converter is implemented.
