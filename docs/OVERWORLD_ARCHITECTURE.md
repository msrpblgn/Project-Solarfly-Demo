# Project Solarfly GBA — Overworld Architecture Notes

## Approved video split

- Overworld field scenes use **GBA Mode 0**.
- Existing main-menu and battle scenes remain **Mode 3** for now.
- Scene entry is responsible for reconfiguring display registers and rebuilding any
  graphics data required by that mode.

## Intended field BG responsibilities

- `BG3`: ground plane and ordinary terrain
- `BG2`: raised terrain and bridge surfaces
- `BG1`: canopies, roofs, and other overhead art
- `BG0`: dialogue and field UI

Currently implemented:

- **OW-2 / OW-3 / OW-4 use BG3 only** for world terrain.
- BG3 holds one **32x32 hardware-tile circular screenblock**.
- The runtime streams **screen entries only** into that resident BG3 map.
- **OW-4** also enables **OBJ** with **1D** tile mapping for the player sprite.

Not implemented yet:

- BG2 raised terrain
- BG1 overhead art
- BG0 field UI

## Implemented OW-2 streamer

- The diagnostic world is a finite map of **40x30 authored 16x16 metatiles**.
- Each metatile expands to four 8x8 BG screen entries in this order:
  top-left, top-right, bottom-left, bottom-right.
- The diagnostic world therefore spans **80x60 world hardware tiles**
  or **640x480 pixels**.
- The resident BG3 surface remains one **32x32 hardware-tile** screenblock
  (`256x256` pixels).
- For a world hardware tile coordinate `(x, y)`, its circular destination is
  conceptually `(x mod 32, y mod 32)`.
- Runtime scrolling updates only the newly required **row** or **column**
  screen entries when the camera crosses an 8-pixel boundary.
- The project still uses its existing **VBlank polling** model.
  Streamer and OAM commits happen during the frame slice immediately after
  `System_WaitForVBlank()`.

## Implemented OW-3 Tiled ground compiler

- Authoring sources live under `assets_src/overworld/`:
  - `maps/overworld_test.tmx`
  - `tilesets/overworld_test.tsx`
  - `tilesets/overworld_test_preview.png` (editor preview only)
- Host tool: `tools/compile_tiled_map.py` (Python 3 stdlib only).
- Generated outputs:
  - `include/assets/maps/overworld_test_map.h`
  - `src/assets/maps/overworld_test_map.c`
- Runtime ownership remains `OverworldMapDef`; the Ground metatile-ID array is
  generated. Palette, 4bpp BG tiles, and metatile defs stay in
  `src/overworld/overworld_map.c`.
- The GBA never parses TMX/TSX/PNG/CSV.

## Implemented OW-4 player OBJ, Q24.8 movement, camera follow

### Coordinates

- Player world position uses signed **Q24.8** (`Fixed24_8`).
- Camera logical position uses signed **Q24.8**.
- Player position is **foot-anchored**:
  - `worldX` = horizontal center of the 16×32 sprite
  - `worldY` = bottom / foot edge
- Desired on-screen foot position is `(120, 96)` so the 16×32 sprite
  centers on screen `(120, 80)`.
- Camera follow:
  - `desiredCameraX = playerWorldX - fixed(120)`
  - `desiredCameraY = playerWorldY - fixed(96)`
  - Clamp camera to `X = 0..fixed(400)`, `Y = 0..fixed(320)`
- Integer camera pixels for the streamer are produced only after clamping, using
  a nonnegative floor conversion (no negative shifts).
- BG3 hardware scroll remains `cameraPixel mod 256`.
- Logical camera coordinates do **not** wrap.

### World-to-screen

```text
screenLeft = floor(playerWorldX) - cameraPixelX - 8
screenTop  = floor(playerWorldY) - cameraPixelY - 32
```

### Movement

- Held D-pad; axes resolve independently with opposing-direction cancellation.
- Cardinal speed: `384` in Q24.8 (1.5 px/frame).
- Diagonal per-axis speed: `272` in Q24.8.
- Fractional Q24.8 position is preserved across frames.
- Facing tracks the last nonzero 8-way direction; no directional graphics yet.
- Player bounds keep the full sprite inside the 640×480 map:
  - `X = 8..632`, `Y = 32..480`
- Initial state on every overworld entry:
  - player foot `(120, 96)`, camera `(0, 0)`, facing down
  - screen top-left `(112, 64)`

### OAM

- 128-entry shadow OAM (`1024` bytes `.bss`), 8-byte hardware-compatible entries.
- OBJ index `0` is reserved for the overworld player.
- Attributes are prepared in shadow OAM; hardware OAM is updated only via
  `Oam_Commit()` during the post-VBlank update slice (CPU word copy, no DMA).
- Player graphic: one temporary 16×32 4bpp OBJ, eight consecutive tiles at OBJ
  tile index `0`, palette bank `0`.
- OBJ VRAM `0x06010000`, OBJ palette `0x05000200`, OAM `0x07000000` are distinct
  from BG charblock `0` and screenblock `31`.

### Mode transitions

- Leaving with `B` skips further overworld OAM/streamer writes that frame,
  hides shadow OAM, then enters Mode 3 (OBJ disabled in `DISPCNT`).
- Re-entering reloads OBJ tiles/palette, reinitializes OAM, resets player and
  camera, and refills the BG3 streamer. OBJ VRAM/OAM are not assumed to survive
  Mode 3.

### Not implemented in OW-4

- Collision (added in OW-5)
- Directional frames / walking animation
- NPCs, entities, dynamic OBJ allocation
- BG2 / BG1 / BG0
- Warps, dialogue, scripts (battle trigger added in OW-6)

## Implemented OW-5 Tiled collision masks and player-versus-map collision

### Authoring

- TMX layers: exactly `Ground` and `Collision` (CSV, finite, no flips).
- Ground TSX remains the visual metatile set.
- Collision TSX: `assets_src/overworld/tilesets/overworld_collision_profiles.tsx`
  (+ editor-only preview PNG; not in the ROM).
- Each collision tile has string property `mask=0xHHHH`.
- Host compiler emits:
  - `gOverworldTestMapMetatileIds[1200]` (`u16`)
  - `gOverworldTestMapCollisionProfileIds[1200]` (`u8`)
  - `gOverworldTestMapCollisionProfileMasks[6]` (`u16`)
- Regenerate with `make maps` / the established map rule.
- Generated files must not be edited manually.

### Profile IDs and masks

| Profile | Meaning     | Mask     |
| ------: | ----------- | -------- |
|       0 | Empty       | `0x0000` |
|       1 | Solid       | `0xFFFF` |
|       2 | Top half    | `0x00FF` |
|       3 | Bottom half | `0xFF00` |
|       4 | Left half   | `0x3333` |
|       5 | Right half  | `0xCCCC` |

- Nonzero collision GID → `localId + 1`.
- Each `u16` mask is a 4×4 grid over the 16×16 metatile; bit 0 = top-left,
  bit 15 = bottom-right; set bit = solid.
- Each bit covers a 4×4-pixel world region.
- Runtime collision never reads BG VRAM / screenblock 31 / wrapped scroll.

### Diagnostic layout after OW-6

- Entire Ground layer is solid green metatile `0`, except one red battle
  metatile `1` at `(10,8)` → world `[160,176) × [128,144)`, and one blue
  dialogue marker metatile `2` at `(9,6)` → world `[144,160) × [96,112)`.
- Collision layer is empty (all profile IDs `0`); partial-mask profiles remain
  in the collision tileset / compiler for later maps.
- Walkable bounds remain the approved player world limits only.

### Expected 8×8 foot-box blocking (OW-5 diagnostic walls)

OW-5 authored wall placements were cleared in OW-6 for the green test map.
Compiler/runtime collision support is unchanged for future authored maps.

### Runtime movement

- Player visual sprite remains 16×32; collision AABB is a separate 8×8 foot box:
  `[x-4, x+4) × [y-8, y)` in Q24.8 half-open intervals.
- Exact contact is legal; one quantum of penetration is not.
- Resolve **X then Y** after bounds clamps; wall sliding is enabled.
- Blocked axis snaps to the nearest 4px subcell boundary; the other axis keeps
  fractional Q24.8 when free.
- Camera follow runs after collision resolution.
- Out-of-map pixel queries are solid.
- Invalid profile IDs are treated as solid and never index past the mask table.

### Compiler errors

Useful stderr includes file, layer/tileset, cell `(x,y)` when relevant, GID/property,
and expected format. Common causes: missing Collision layer, ground GID in Collision,
missing `mask`, flipped GIDs, wrong dimensions, extra layers.

## Streaming direction

- Later field planes are intended to use **32x32 hardware-tile circular buffers**
  as well, extending the OW-2 BG3 approach to additional layers.
- Authored maps will use **16x16 metatiles** even though GBA backgrounds use
  8x8 hardware tiles.
- The GBA runtime will consume compact compiled data; it will not parse TMX,
  XML, JSON, CSV, or PNG on hardware.

## Planned spatial model

- Elevation is planned to start with **ground** and **bridge** levels while
  remaining extensible for later traversal rules.
- Behavior flags beyond solid masks are not implemented yet.

## Implemented OW-6 Green field, protagonist sprite, Petalburg trigger

### Ground graphics

- Authored TSX has three 16×16 metatiles: green `RGB15(6,20,8)`, red
  `RGB15(29,3,3)`, and blue dialogue marker `RGB15(4,12,28)`.
- Runtime BG palette/tiles mirror that: hardware tile 0 green, 1 red, 2 blue.
- Red battle trigger and blue dialogue marker are Ground-layer metatile data
  streamed like every other cell.

### Protagonist OBJ

- Source: `assets_src/overworld/player/protagonist_overworld_down_idle.png`
  (16×32 indexed; palette index 0 transparent; ≤15 opaque colors).
- Converter: `tools/convert_overworld_player_sprite.py` →
  `src/assets/overworld_player_sprite.c` / matching header.
- OBJ tiles: 256 bytes; OBJ palette: 32 bytes; tile index 0; palette bank 0.
- Down-facing idle is used for every logical facing this milestone.
- Aseprite master remains authoring-only and is not converted at build time.

### Battle trigger

- Foot sample: `(worldX, worldY - 1)` against `[160,176) × [128,144)`.
- Latch: fire once on enter while armed; stay consumed across battle return;
  rearm only after the foot leaves the square.
- `B` exit-to-menu still beats the trigger in the same frame.
- Launch uses the same Petalburg Showdown settings path as the main menu via
  `App_StartPetalburgShowdown(returnTarget, …)`.

### Dialogue marker (Milestone 6)

- Blue metatile `(9,6)` AABB `[144,160) × [96,112)`.
- Explicit **A** press while foot is inside; does not auto-fire on entry.
- Distinct from the red walk-on battle trigger.
- Starts `DialogueScene_GetSharedConversation()` with FIELD overlay (BG3 map kept).
- `Overworld_LockForDialogue` / `UnlockAfterDialogue` suspend movement and
  restore player OAM; START cancels dialogue without opening the main menu.
- Held A after close is latched so dialogue does not instantly retrigger.

### Battle return

- `BattleReturnTarget`: menu vs overworld.
- Overworld return restores Mode 0, BG tiles/palettes, OBJ tiles/palette,
  streamer, camera follow, player position/facing, and consumed latch.
- Fresh menu → Overworld Test still calls `Overworld_Init()` (clean start).

## Deliberately absent after OW-6

The current overworld milestones still do **not** add:

- NPC / entity / dynamic collision
- terrain behaviors, slopes, ledges, water
- elevation / bridges
- BG2 or BG1 planes
- warps, doors, interactions, dialogue, scripts
- a general trigger/scripting system
- player animation / additional directional frames
- DMA or IRQ-driven BG/OAM updates
