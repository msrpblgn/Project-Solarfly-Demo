# Dialogue System Resource Map (Milestone 0)

## Audit metadata

| Field | Value |
| ----- | ----- |
| Audit date | 2026-09-01 |
| Audited tree | Working tree on branch `main` |
| Base commit | `b994f029144c8a2f09db2dc589e11139bdc9d7c4` (2026-07-16) |
| Pre-existing dirty files | 16 modified tracked files + untracked overworld/dialogue assets (see final handoff) |
| Milestone 0 code hooks | `include/core/bg_vram_layout.h`, `include/dialogue/dialogue_resources.h`, `src/dialogue/dialogue_resources.c` |
| Milestone 0 result | **PASS** after correction pass — shared BG VRAM interval model; screenblock 30 (was unsafe screenblock 0) |

## Scope

This document maps current graphics, transfer, input, text, and application-state ownership
relevant to a future heap-free dialogue runner. It does **not** implement dialogue rendering.

### Source files inspected

- `src/main.c` — `AppMode` dispatch, mode transitions, main loop
- `src/core/video.c`, `include/core/video.h` — display modes, bitmap draw, Mode 0 BG3 setup
- `src/core/oam.c`, `include/core/oam.h` — shadow OAM, commit path
- `src/core/input.c`, `include/core/input.h` — key sampling
- `src/core/system.c` — VBlank polling
- `include/core/bg_vram_layout.h` — shared BG VRAM byte-interval helpers
- `src/overworld/*` — field Mode 0 runtime
- `src/battle/*`, `src/battle_ui/*` — Mode 3 battle/menu drawing
- `include/battle_ui/hud_layout.h` — battle message box layout
- `include/assets/overworld_player_sprite.h` — OBJ tile/palette ownership
- `Makefile`, `tools/compile_tiled_map.py`, `tools/convert_overworld_player_sprite.py`
- `docs/OVERWORLD_ARCHITECTURE.md`, `tools/notes_asset_pipeline.md`
- Portrait sources: `assets_src/sprites/aksil_front_sprite.png`,
  `assets_src/sprites/protagonist_front_sprite.png`
- Dialogue frame reference: `assets_src/ui/Dialogue Ui.png`

### Design references (terminology only)

- `docs/OVERWORLD_ARCHITECTURE.md` — planned BG0 dialogue/UI layer
- `docs/project_solarfly_31G_source_pack.md` — character names and game terminology
- No separate dialogue implementation plan file is present in the repository tree;
  Milestone 0 goals follow the user-supplied milestone brief.

---

## Display mode summary

| Mode | Active scenes | `REG_DISPCNT` | Evidence |
| ---- | ------------- | ------------- | -------- |
| **Mode 3** | Main menu, options, custom battle flows, battle | `DCNT_MODE3_BITMAP` (`MODE_3 \| BG2`) | `gba_types.h:43`, `video.c:176` |
| **Mode 0** | Overworld test field | `DCNT_MODE0_FIELD_BG3_OBJ` (`MODE_0 \| BG3 \| OBJ \| OBJ_1D`) | `gba_types.h:45`, `video.c:230` |
| Mode 4 / 5 | Unused | — | No references in source |

Modes 3 and 0 are **mutually exclusive** at runtime. Entry functions fully reconfigure
`REG_DISPCNT`, clear or reload VRAM/palettes, and rebuild subsystem state.

---

## Mode 0 BG VRAM: single shared address space

On the GBA, Mode 0 text-background **character blocks** and **screenblocks** alias the same
64 KiB BG VRAM (`0x06000000`–`0x0600FFFF`). Validation must use **byte intervals**, not
independent charblock-index versus screenblock-index comparisons.

| Unit | Byte size | Count | Address formula |
| ---- | --------- | ----- | ----------------- |
| Character block | `0x4000` (16 KiB) | 4 | `charblock * 0x4000` |
| Screenblock | `0x0800` (2 KiB) | 32 | `screenblock * 0x0800` |

Each character block spans **eight consecutive screenblocks**:

```text
Character block N  →  screenblocks (N*8) through (N*8 + 7)
```

Half-open overlap rule for intervals `[start, end)`:

```text
overlap when: a_start < b_end && b_start < a_end
```

Macros: `include/core/bg_vram_layout.h`.

### Physical BG VRAM layout (corrected Milestone 0)

| Physical BG VRAM range | Byte range | Screenblock equivalents | Current owner |
| ---------------------- | ---------- | ----------------------: | ------------- |
| Character block 0 | `[0x0000, 0x4000)` | 0–7 | Overworld BG3 **tile graphics** (`Video_CopyWords` → charbase 0, `video.c:183-192`) |
| Character block 1 | `[0x4000, 0x8000)` | 8–15 | Dialogue BG0 **tile graphics** reservation (`DIALOGUE_MODE0_BG_CHARBLOCK`) |
| Character block 2 | `[0x8000, 0xC000)` | 16–23 | **Unreserved** — planned BG2 raised terrain (`OVERWORLD_ARCHITECTURE.md:13`) |
| Character block 3 (partial) | `[0xC000, 0xF000)` | 24–29 | **Unreserved** — planned BG1 overhead art may use charblock 3 |
| Dialogue BG0 tilemap | `[0xF000, 0xF800)` | **30** | Dialogue BG0 **screen map** reservation (`DIALOGUE_MODE0_BG_SCREENBLOCK`) |
| Overworld BG3 tilemap | `[0xF800, 0x10000)` | **31** | Overworld BG3 **screen map** (`VIDEO_MODE0_FIELD_SCREENBLOCK`, streamer writes via `overworld_streamer.c:8-12`) |

### Prior unsafe reservation (corrected)

| Item | Old value | Problem | Corrected value |
| ---- | --------- | ------- | --------------- |
| `DIALOGUE_MODE0_BG_SCREENBLOCK` | **0** | Screenblock 0 lies inside character block 0 `[0x0000, 0x4000)`, which already holds overworld BG3 tile graphics. A dialogue tilemap at SB0 would overwrite those tiles. | **30** `[0xF000, 0xF800)` |

The original Milestone 0 tests compared `dialogue_sb != field_sb` (0 ≠ 31) and
`dialogue_cb != field_cb` (1 ≠ 0) without converting both sides into the same byte space.
That missed the charblock-0 versus screenblock-0 alias.

### Pairwise non-overlap proof (byte intervals)

Let `F_CH = [0x0000, 0x4000)`, `D_CH = [0x4000, 0x8000)`, `D_SB = [0xF000, 0xF800)`,
`F_SB = [0xF800, 0x10000)`.

| Pair | Overlap? | Reason |
| ---- | -------- | ------ |
| F_CH vs D_CH | No | Adjacent halves of BG VRAM |
| F_CH vs D_SB | No | `0x4000 ≤ 0xF000` |
| F_CH vs F_SB | No | `0x4000 ≤ 0xF800` |
| D_CH vs D_SB | No | `0x8000 ≤ 0xF000` |
| D_CH vs F_SB | No | `0x8000 ≤ 0xF800` |
| D_SB vs F_SB | No | Adjacent 2 KiB maps |

Verified at compile time in `src/dialogue/dialogue_resources.c` and in
`tools/tests/test_dialogue_resources.py`.

### Future BG1 / BG2 constraints

- **BG2** (bridges) is documented for a future charblock — likely **charblock 2** (SB 16–23).
  Dialogue tilemap at SB 30 does not occupy that range.
- **BG1** (canopies) may use **charblock 3** tile graphics (SB 24–29). Dialogue SB 30 is
  adjacent to but does not overlap SB 24–29 or the overworld map at SB 31.
- Future layers must still be checked with the shared interval model before allocation.

---

## Mode-by-mode ownership table

| Mode | Entry function | Display configuration | BG/bitmap owners | OBJ/OAM owner | Palette owner | Exit/restoration path | Evidence |
| ---- | -------------- | --------------------- | ---------------- | ------------- | ------------- | --------------------- | -------- |
| **Main menu** | Boot → `Video_Init()` | Mode 3, BG2 bitmap, OBJ off | CPU writes `VRAM[240×160]` u16 pixels | None | Cleared, unused for draw | A → battle/overworld/options/custom | `main.c:1905-1907`, `video.c:169-176` |
| **Options** | Menu A on Options | Mode 3 (unchanged) | Same bitmap framebuffer | None | Same | B → main menu | `main.c:2031-2035`, `2992-2996` |
| **Custom battle** | Menu A on Custom Battle | Mode 3 | Same | None | Same | B → menu; A start → battle | `main.c:2025-2029`, `2099-2158` |
| **Battle** | `App_StartPetalburgShowdown` / menu paths | Mode 3 via `Video_EnterMode3Bitmap()` | `Battle_Draw*` → `Video_*` bitmap blits | None (sprites are bitmap, not OBJ) | Direct RGB15 in VRAM | A/B/Start when finished → menu or overworld | `main.c:376-383`, `2999-3023`, `battle.c:1140+` |
| **Overworld** | `Overworld_Init()` | Mode 0 BG3+OBJ | BG3 charblock 0, screenblock 31; streamer writes screen entries | OAM index 0; OBJ tiles 0–7 | BG palette 16 colors; OBJ palette bank 0 | B → `Overworld_Shutdown` + Mode 3; battle → Mode 3 then `Overworld_ResumeAfterBattle` | `overworld.c:76-103`, `video.c:179-230`, `oam.h:7` |

### Per-mode hardware detail

#### Mode 3 (menu / battle)

| Resource | Owner | Value / range | Lifetime |
| -------- | ----- | ------------- | -------- |
| `REG_DISPCNT` | `Video_EnterMode3Bitmap` | `0x0403` forced blank during setup | While in any Mode 3 scene |
| `REG_BGxCNT` | Reset to 0 | All BG layers disabled | Mode 3 |
| Framebuffer | `Video_Clear`, `Video_DrawRect`, `Video_DrawText`, `Video_DrawBitmap*` | `VRAM` @ `0x06000000`, 240×160×2 bytes | Mode 3 |
| BG palettes | Cleared on entry | 256 entries zeroed | Unused for rendering |
| OBJ / OAM | Disabled | Not written in Mode 3 paths | — |
| Blend / windows | Unused | No `REG_BLD*` / `REG_WIN*` references | — |
| Scroll regs | Zeroed in `Video_ResetBgState` | — | `video.c:148-161` |

#### Mode 0 (overworld field)

| Resource | Owner | Value / range | Lifetime |
| -------- | ----- | ------------- | -------- |
| `REG_DISPCNT` | `Video_EndMode0FieldBg3` | Mode 0 + BG3 + OBJ + 1D mapping | `APP_MODE_OVERWORLD` |
| `REG_BG3CNT` | `Video_BeginMode0FieldBg3` | Priority 3, charbase **0**, screenbase **31**, 4bpp, 256×256 | Overworld active |
| BG3 charblock | Overworld ground tiles | Block **0** (`0x06000000`), 2 tiles today | Overworld active |
| BG3 screenblock | `OverworldBg3Streamer` | Block **31** (`0x0600F800`), 32×32 entries | Overworld active |
| `REG_BG3HOFS/VOFS` | `Video_SetMode0Bg3Scroll` | Camera pixel mod 256 | Per frame in overworld |
| BG0–BG2 | **Unallocated** (BG0 reserved for future dialogue) | CNT = 0 | — |
| OBJ mapping | 1D (`DCNT_OBJ_1D`) | — | `gba_types.h:45` |
| OBJ VRAM | Player sprite | Tiles **0–7** (256 B) @ `0x06010000` | Overworld active |
| OBJ palette | Player | Bank **0**, 16 colors | `overworld_player_sprite.h:16` |
| OAM | Player | Index **0**, 16×32 tall | `oam.h:7`, `overworld_player.c:307-317` |
| Shadow OAM | `Oam_Commit` CPU copy | `s_shadowOam[128]` in IWRAM `.bss` | Always allocated; committed in overworld |

---

## Transfer resources

| Resource | Owner | Behavior | Evidence |
| -------- | ----- | -------- | -------- |
| DMA channels | **None** | No `REG_DMA` usage | Grep: no matches in `.c`/`.h` |
| VBlank sync | `System_WaitForVBlank` | Poll `REG_VCOUNT` | `system.c:8-15` |
| VBlank workload | Main loop | Once per frame before `Input_Update` and mode logic | `main.c:1946-1947` |
| OAM upload | `Oam_Commit` | Immediate CPU word copy after VBlank wait | `oam.c:74-85`, `overworld.c:137` |
| BG3 streamer | `OverworldBg3Streamer_SetCamera` | Row/column screen-entry patches post-VBlank | `overworld.c:133-136` |
| IRQ / VBlank ISR | **None** | — | — |

All graphics uploads occur **after** `System_WaitForVBlank()` in the main loop. No documented
unsafe mid-frame VRAM writes outside that slice.

---

## RAM resources (IWRAM / EWRAM)

| Buffer | Size | Section | Owner | Evidence |
| ------ | ---- | ------- | ----- | -------- |
| `s_shadowOam[128]` | 1024 B | IWRAM `.bss` | OAM subsystem | `oam.c:5`, map file |
| `s_keysHeld` / `s_keysPressed` | 4 B | IWRAM `.bss` | Input | `input.c:3-4` |
| `s_bg3Streamer` | ~28 B | IWRAM `.bss` | Overworld | `overworld.c:16` |
| Battle state / message queue | ~2.5 KiB | IWRAM `.bss` | Battle | `build/project_solarfly_gba.map` |
| EWRAM | 0 B used | — | Unused | No `.ewram` sections |
| Heap | Stub only | — | No `malloc` in project code | Linker `fake_heap` |

No EWRAM staging buffers. Mode 3 and Mode 0 do not share VRAM simultaneously.

---

## Input path

| Concern | Facility | Evidence |
| ------- | -------- | -------- |
| Sampling | `Input_Update()` once per frame after VBlank | `main.c:1947`, `input.c:12-16` |
| Held state | `Input_IsPressed(key)` — `s_keysHeld` mask | `input.c:19-22` |
| Edge / one-shot | `Input_JustPressed(key)` — `s_keysPressed` mask | `input.c:24-27` |
| Released state | **Not implemented** | — |
| Consumption / latch | Mode handlers read masks same frame; no global consume API | Per-mode `if (Input_JustPressed(...))` |
| Avoid carry-over | Edge detected vs previous `held`; new mode entered next frame after release unless key still held | `input.c:15` |
| Direct `REG_KEYINPUT` | **Avoid** — use `Input_*` helpers | `input.c:14` |

Future dialogue should call `Input_JustPressed` for advance/skip and must not read
`REG_KEYINPUT` directly.

---

## Text and fonts

| Concern | Current implementation | Evidence |
| ------- | ---------------------- | -------- |
| Font source | Embedded 5×7 bitmap table in `video.c` | `video.c:9-55` |
| Color depth | Direct RGB15 framebuffer pixels (Mode 3) | `Video_DrawGlyph` → `Video_DrawRect` |
| Width | 6 px per character | `video.c:334` |
| Newlines | Not handled (single-line strings) | `Video_DrawText` |
| Clipping | `Video_DrawTextClipped` with scroll offset | `video.c:350-374` |
| Measure | `Video_MeasureTextWidth` | `video.c:339-348` |
| Battle message box | `BattleMessageQueue_Draw` → `Video_DrawText` | `battle_message_queue.c`, `hud_layout.h:24-31` |
| Text speed | Hold-frame timer only (no typewriter) | `battle_message_queue.h:9-11` |
| Overworld text | **None** | No `Video_DrawText` in `src/overworld/*` |
| Tile/OBJ font | **None** | — |
| Generated font assets | Listed in `tools/notes_asset_pipeline.md` as planned, not built | `notes_asset_pipeline.md:23-24` |

Overworld dialogue will require a **new** text path (likely BG0 tile font or Mode-0-specific
renderer). Battle/menu text remains Mode 3 software font.

---

## Application state and transitions

| Concern | Owner | Evidence |
| ------- | ----- | -------- |
| Top-level modes | `AppMode` enum in `main.c` | `main.c:35-59` |
| Main loop | `while(1)` with VBlank → input → mode branch | `main.c:1942+` |
| Battle return | `BattleReturnTarget` + `battleFinishedReadyForReturn` latch | `main.c:2999-3034` |
| Overworld resume | `Overworld_ResumeAfterBattle()` preserves player/camera/trigger latch | `overworld.c:92-103` |
| Fade / transition | **None** | Instant mode switches |
| Message queue | `BattleMessageQueue_*` (battle only) | `battle_message_queue.c` |
| Safest future dialogue host | **Overworld sub-state or new `AppMode` staying in Mode 0**, entering only from `APP_MODE_OVERWORLD`, restoring via existing overworld statics + `Overworld_Shutdown` pattern | Architecture + current gaps |

No dialogue `AppMode` exists yet. Battle message queue is battle-scoped and Mode 3 only.

---

## Dialogue source assets

### `assets_src/ui/Dialogue Ui.png`

| Property | Repository value | Supplied brief | Match |
| -------- | ---------------- | -------------- | ----- |
| Path | `assets_src/ui/Dialogue Ui.png` | — | — |
| Dimensions | 240 × 160 | 240 × 160 | Yes |
| Format | Indexed PNG (mode P), binary alpha 0/255 | Indexed, 5 RGBA entries | Yes |
| Opaque colors | 4 | 4 opaque + transparency | Yes |
| Opaque bounds | (0, 108) – (240, 159) → **240 × 51** | Same | Yes |
| Pipeline ingest | **No** — not in `Makefile`, no converter | — | — |
| 4bpp runtime cost (full canvas) | 600 tiles × 32 B = **19,200 B** | — | — |
| 8bpp runtime cost (full canvas) | 600 tiles × 64 B = **38,400 B** | — | — |
| Practical panel-only 4bpp | 30×7 = **210 tiles** = **6,720 B** (fits one charblock) | — | — |
| Palette / OBJ / OAM | BG0 4bpp tile asset; no OBJ | — | — |
| Later normalization | Extract lower 240×51 panel; convert to 4bpp BG tiles + ≤16 color palette; assign palette indices 16–19 | — | — |
| Byte-for-byte | Unchanged in Milestone 0 | — | — |

### `assets_src/sprites/aksil_front_sprite.png`

| Property | Value |
| -------- | ----- |
| Dimensions | 64 × 64 |
| RGBA entries | 24 (23 opaque) |
| Alpha | Binary 0 / 255 |
| Opaque bounds | (19, 3) – (42, 63) → **23 × 60** |
| Pipeline | `tools/convert_aksil_sprite.sh` + grit → Mode 3 **RGB15 bitmap** in ROM (`include/assets/aksil_front_sprite.h`) |
| Ingest unchanged as OBJ | **No** — 23 colors exceed one 4bpp OBJ palette (16) |
| Valid 64×64 OBJ after normalization | 4bpp: 64 tiles × 32 B = **2,048 B**; 8bpp: 64 tiles × 64 B = **4,096 B** |
| Source canvas tile-padded staging (not valid OBJ size) | Same as valid 64×64 for this asset |
| Later normalization | Palette reduction to ≤15 opaque colors + index 0 transparent; optional crop to opaque bounds |
| Byte-for-byte | Unchanged |

### `assets_src/sprites/protagonist_front_sprite.png`

| Property | Value |
| -------- | ----- |
| Dimensions | **66 × 66** (exceeds GBA max single OBJ 64×64) |
| RGBA entries | 31 (30 opaque) |
| Alpha | Binary 0 / 255 |
| Opaque bounds | (20, 4) – (54, 64) → **34 × 60** |
| Pipeline | `tools/convert_protagonist_sprite.sh` + grit → Mode 3 RGB15 bitmap |
| Ingest unchanged as OBJ | **No** — 66×66 canvas exceeds max single OBJ 64×64; >16 colors |
| Valid 64×64 OBJ after normalization | 4bpp: **2,048 B**; 8bpp: **4,096 B** |
| Source 66×66 tile-padded staging estimate (not valid single OBJ) | 81 tiles × 32 B = **2,592 B** (4bpp); 81 tiles × 64 B = **5,184 B** (8bpp) |
| Later normalization | Crop to ≤64×64 (likely 34×60 content box padded); palette reduction; possible multi-OBJ join |
| Byte-for-byte | Unchanged |

---

## Proposed dialogue reservations

Named constants live in `include/dialogue/dialogue_resources.h`.
Compile-time checks: `src/dialogue/dialogue_resources.c`.

### Reservation table

| Resource | Current owner | Mode/lifetime | Existing range / capacity | Proposed dialogue reservation | Conflict status | Evidence/notes |
| -------- | ------------- | ------------- | ------------------------- | ----------------------------- | --------------- | -------------- |
| Display mode | Menu/battle vs overworld | Mutually exclusive | Mode 3 vs Mode 0 | Mode 0 dialogue overlays overworld only | **Safe** (exclusive modes) | `video.c:169-230` |
| BG0 charblock (tile graphics) | Unallocated | Mode 0 dialogue | CB 0–3 | **Charblock 1** → bytes `[0x4000, 0x8000)` | **No overlap** | `dialogue_resources.h`, interval asserts |
| BG0 screenblock (tilemap) | Unallocated | Mode 0 dialogue | SB 0–31 | **Screenblock 30** → bytes `[0xF000, 0xF800)` | **No overlap** | Outside CB0/CB1; ≠ field SB31 |
| BG0 priority | — | Mode 0 | Priorities 0–3 | **Priority 0** (above BG3 pri 3) | **Safe** | `video.c:193-197` |
| BG3 charblock / screenblock | Overworld ground | Mode 0 overworld | CB **0**, SB **31** | No dialogue claim | — | `video.h`, `OVERWORLD_ARCHITECTURE.md` |
| BG palette colors | Overworld diagnostic | Mode 0 | Indices 0–15 loaded | **Indices 16–31** reserved (`DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE` + 16) | **No overlap** | `overworld_map.c:11-28` |
| Mode 3 VRAM | Battle/menu bitmap | Mode 3 | Full 96 KiB | **Not reserved** — separate backend TBD | Deferred | Milestone 0 scope |
| OBJ tile VRAM | Player tiles 0–7 | Mode 0 | 256 B at base | **Not reserved** (format TBD) | **Blocked** | Milestone 0 brief |
| OBJ palette bank 0 | Player | Mode 0 | 16 colors | No dialogue claim | — | `overworld_player_sprite.h:16` |
| OBJ palette banks 1–2 | Unused | Mode 0 dialogue | Banks 0–15 | **Bank 1 left, bank 2 right** | **No overlap** | `dialogue_resources.h` |
| OAM index 0 | Overworld player | Mode 0 | 0 | No dialogue claim | — | `oam.h:7` |
| OAM indices 1–2 | Unused | Mode 0 dialogue | 1–127 | **Index 1 left, index 2 right** | **No overlap** | `dialogue_resources.h` |
| OAM 3–127 | Unused | Available | — | Unreserved | — | — |
| Shadow OAM buffer | Global | All modes | 1024 B IWRAM | Shared facility; dialogue uses indices 1–2 only | **Safe** | `oam.c:5` |
| DMA | None | — | 4 channels | None | — | — |
| Font / text | Mode 3 software font | Mode 3 only | 5×7 in `video.c` | Field dialogue needs new renderer | Deferred | §Text |

### Lifetime rules

1. **Mode 0 dialogue reservations** apply only while overworld field is active (`APP_MODE_OVERWORLD`
   with a future dialogue sub-state). They must be released before `Overworld_Shutdown` or
   `Video_EnterMode3Bitmap`.
2. **BG0** must be disabled (`REG_BG0CNT = 0`) and its charblock/screenblock cleared or overwritten
   on dialogue exit; overworld BG3 state is rebuilt by `Overworld_LoadFieldGraphicsAndCommit` today.
3. **Portrait OAM entries 1–2** must be hidden (`Oam_Hide`) when dialogue ends; index 0 remains
   player-owned.
4. **OBJ palette banks 1–2** may be reloaded after dialogue; bank 0 remains player palette.
5. **Mode 3 dialogue** (if ever needed in battle) cannot reuse BG0 reservations; requires a
   separate audit when implemented.

---

## Conflicts, unknowns, and assumptions

| Item | Status |
| ---- | ------ |
| OBJ tile VRAM range for portraits | **Blocked** until 4bpp vs 8bpp and normalization |
| Mode 3 dialogue panel backend | **Unknown** — battle uses software rectangles + `hud_layout.h` |
| BG1/BG2 future overworld layers | Charblock 2 (SB 16–23) and charblock 3 tile band (SB 24–29) remain free; dialogue map at SB 30 |
| `Dialogue Ui.png` full-canvas vs panel-only | Assumed panel-only conversion (240×51) for VRAM efficiency |
| Text font for field dialogue | **Unknown** — no tile font exists |
| Prior screenblock 0 reservation | **Corrected** — aliased overworld CB0 tile graphics |
| README still says "battle-only, no overworld" | **Conflict** with current source; **source wins** (`main.c` overworld test) |
| Later 64×64 single-OBJ portrait plan | **Must be reevaluated** — Milestone 1 mockup now presents 128×128 busts. That is a mockup presentation size only. It does **not** prove current OBJ-tile or OAM reservations can display 128×128 at runtime. Do not change Milestone 0 allocations in this revision. |

---

## Secondary re-audit (palette, OAM, overworld refresh)

| Check | Result | Evidence |
| ----- | ------ | -------- |
| BG palette indices 16–31 vs Mode 0 writers | **Safe today** — overworld uploads only 16 colors at indices 0–15 (`overworld_map.c:11-28`, `video.c:189-191`). **Lifetime note:** `Video_BeginMode0FieldBg3` clears all 256 BG palette entries (`video.c:187-188`); dialogue must reload indices 16–31 after any full field graphics refresh. |
| OBJ palette banks 1–2 vs writers | **Safe** — only bank 0 written in overworld (`overworld.c:58-61`, `video.c:202-212`). No other `Video_LoadObjPaletteBank` call sites. |
| OAM indices 1–2 vs writers | **Safe** — only index 0 receives `Oam_SetRegular` (`overworld_player.c:307-317`). **Lifetime note:** `Oam_InitHidden` disables all 128 entries (`oam.c:9-16`) on overworld load/shutdown (`overworld.c:55-56`, `143-144`); dialogue must repopulate indices 1–2 after refresh. |
| Full OAM clear during dialogue | **Not implemented yet** — `Oam_InitHidden` would hide dialogue portraits if called mid-dialogue. |
| Full BG palette reload | **Yes on field (re)entry** — `Video_ClearHalfwords(BG_PALETTE, 256)` (`video.c:187-188`). |
| Overworld refresh vs dialogue BG VRAM | **Would overwrite** — `Video_BeginMode0FieldBg3` clears first 64 KiB BG VRAM (`video.c:188-190`) and reloads charblock 0 tiles. Dialogue CB1 / SB30 not preserved across init/resume. Streamer only writes screenblock 31 (`overworld_streamer.c:8-12`). |
| Screenblock 31 inside charblock 3 | **Yes physically** (SB 24–31 ∈ CB3), but only SB31's 2 KiB is the overworld map. |
| Streamer vs dialogue tilemap | **No overlap** — streamer targets screenblock 31 only. |

---

## Evidence index (quick links)

| Topic | Location |
| ----- | -------- |
| `AppMode` enum | `src/main.c:35-59` |
| Main loop VBlank + input | `src/main.c:1946-1947` |
| Mode 3 entry | `src/core/video.c:169-176` |
| Mode 0 BG3 setup | `src/core/video.c:179-230` |
| Mode 0 field constants | `include/core/video.h` |
| Overworld init/resume/shutdown | `src/overworld/overworld.c:76-150` |
| Player OAM | `src/overworld/overworld_player.c:307-317` |
| Shadow OAM commit | `src/core/oam.c:74-85` |
| Input API | `src/core/input.c:12-27` |
| Software font | `src/core/video.c:9-55`, `279-374` |
| Battle message box layout | `include/battle_ui/hud_layout.h:24-31` |
| Planned BG0 dialogue | `docs/OVERWORLD_ARCHITECTURE.md:12-28` |
| BG VRAM interval macros | `include/core/bg_vram_layout.h` |
| Dialogue reservations | `include/dialogue/dialogue_resources.h` |
| Reservation static asserts | `src/dialogue/dialogue_resources.c` |

---

## Milestone 0 exit checklist

| Criterion | Status |
| --------- | ------ |
| Written evidence-backed resource map in source tree | **Yes** — this file |
| Every proposed reservation named and traceable | **Yes** — `DIALOGUE_*` in `dialogue_resources.h` |
| No reservation overlaps existing owner in applicable mode | **Yes** — byte-interval static asserts + Python regression tests |
| ROM build and automated tests pass | Verified in correction pass |
| No unrelated behavior/assets changed | **Yes** — PNGs untouched; no renderer added |

**Result: MILESTONE 0 PASS** — shared BG VRAM model corrects the prior screenblock-0 alias bug.
OBJ tile VRAM and Mode 3 dialogue backend remain explicitly deferred.

## Milestone 1 presentation vs later runtime (do not treat as allocation proof)

The Milestone 1 mockup now composites **128×128 nearest-neighbor presentations** of the
normalized 64×64 candidates. Stored source sprites remain 64×64 (Aksil) and 66×66
(Protagonist, cropped to a 64×64 candidate). Later portrait rendering must reevaluate:

- Whether a 128×128 on-screen bust requires multiple OBJ affine/regular sprites, a BG blit, or a cropped bust atlas
- OBJ tile VRAM sizing (a 128×128 4bpp sheet is 256 tiles / 8,192 B, four times a 64×64 sheet)
- OAM index count (indices 1–2 as reserved today cannot cover a 128×128 as one 64×64 OBJ)
- Whether inactive/active 8 px rise can still be a single rigid OAM move

Milestone 0 reservations are unchanged. Runtime dialogue implementation remains out of scope
until visual approval of Milestone 1.

## Milestone 2 static panel ownership (implemented)

| Field | Value |
| ----- | ----- |
| Status | **Implemented — automated checks passed; runtime verification pending** |
| User runtime / emulator approval | **Not recorded** (do not invent) |

| Resource | Owner | Notes |
| -------- | ----- | ----- |
| BG0 | Dialogue panel | Charblock **1**, screenblock **30**, priority **0**, 4bpp |
| BG0 palette bank 1 (colors 16–31) | Dialogue frame + UI ink | Exact approved RGB15 mapping; index 0 transparent, 1 black, 2–4 frame golds |
| BG3 | Dialogue Demo inspect background only | Charblock **0**, screenblock **31**, priority **3**; solid green |
| Frame tiles | Static resident | **70** unique tiles from `Dialogue Ui.png` |
| Dynamic tiles in CB1 | Nameplate / text / prompt / demo hint | Up to **192** slots; dirty-tile upload in Milestone 3 |
| OBJ / OAM | Unused in Milestone 2 | Hidden on demo entry/exit; portrait slots 1–2 remain reserved for later |

Future portrait-budget questions remain open (see above). Milestone 2 does **not** claim 128×128 OBJ feasibility.

## Milestone 3 incremental text printer (implemented)

| Field | Value |
| ----- | ----- |
| Status | **Implemented — automated checks passed; runtime verification pending** |
| Printer module | `dialogue_text_printer.*` (bounded static state; one Update per game tick) |
| Timing | Reuses `TextSpeed`; Slow=4 / Normal=2 / Fast=1 **updates between printable characters** (not battle message hold frames) |
| Dirty tiles | `SetPixel` marks **every** written tile. A 5×7 glyph whose bbox crosses both a tile-column and tile-row boundary dirties up to **four** slots (Milestone 3 handoff text saying “1–2” was inaccurate). Host test: `test_dialogue_panel_dirty_host.c`. |
| Flush | `DialoguePanel_FlushDirty` uploads only dirty dynamic slots (max 192 × 32 B if every slot dirty; typical glyph 1–4 tiles) |
| Demo controls | A advances after complete; B held reveals remainder; START exits; L/R cycles next-line speed locally |
| Host tests | `tools/host_tests/test_dialogue_text_printer_host.c` via `tools/tests/test_dialogue_text_printer.py` |

## Milestone 4 authored pacing controls (implemented)

| Field | Value |
| ----- | ----- |
| Status | **Implemented — automated checks passed; runtime verification pending** |
| Representation | Typed ROM `DialogueTextCommand` array (`TEXT`/`NEWLINE`/`PAUSE`/`WAIT`/`PAGE`/`SPEED`/`COLOR`/`SFX`/`END`) |
| Plain text | `DialogueTextPrinter_Start` builds an internal TEXT/NEWLINE/END stream (Milestone 3 samples unchanged) |
| Punctuation | Automatic punctuation delays **disabled**; only explicit `PAUSE` |
| Color | Semantic roles Default (palette index 1 / black) and Emphasis (index 2 / orange); per-glyph, no shared-entry recolor |
| Audio | SFX opcode emits a **callback** once; no audio engine — audible playback deferred. Demo shows `[…] SFX` diagnostic in the hint band |
| Capacities | Max 48 commands, pause ≤240 updates, process ≤256 steps/update, text ≤96 chars |

## Milestone 5 runtime bust portraits (implemented)

| Field | Value |
| ----- | ----- |
| Status | **USER RUNTIME APPROVED** (mGBA — portraits, speaker switches, replacement, menu return) |
| Strategy | One **64×64 affine OBJ** per side, **ATTR0 double-size**, shared affine matrix **3** with `pa=pd=128` (2×). Presentation top-left maps 1:1 to OAM x/y of the 128×128 box. |
| Raw graphics | 2048 B × 2 portraits = **4096 B** tiles (not pre-expanded 128×128) |
| OAM | Index **1** left, **2** right; matrix **3** (fill of entries 12–15) |
| OBJ tiles (1D) | Left base **64**, right base **128** (64 tiles each); player remains 0–7 |
| OBJ palettes | Bank **1** left, bank **2** right; active/dim via palette reload (no tile re-upload on focus) |
| Window | **WIN0** exclusive bottom **y=116**; inside: BG0+BG3+OBJ; outside: BG0+BG3 (OBJ off) — chest cutoff |
| Motion | Focus 5 frames (palette swap at frame 2); enter/exit **linear 10 frames** incl. endpoints (easing simplified vs mockup ease-out to avoid padding stall) |
| SELECT test | Replacement scene via scene runner (`TECH_REPLACE`); demo complete idle offers SELECT |

## Milestone 6 scene runner and hosts (implemented)

| Field | Value |
| ----- | ----- |
| Status | **Implemented — awaiting user runtime check** |
| Scene data | ROM `DialogueScene` + `DialogueSceneCommand` macros (`ENTER_PAIR`/`SAY`/`EXIT_BOTH`/`EVENT`/`END`/…) |
| Runner API | `Dialogue_Start` / `Update` / `IsActive` / `Cancel` / `TakeResult` |
| Host | `DialogueHost` lock/unlock/event/finished; overlay mode DEMO (green BG3) vs FIELD (keep map) |
| Shared scene | `DialogueScene_GetSharedConversation()` — menu demo + overworld blue marker |
| Overworld | Blue metatile `(9,6)` world `[144,160)×[96,112)`; **A** interact; lock freezes movement/triggers; START cancels to field only |
| Policy | Host player OAM (index 0) hidden while field dialogue active; BG3 map retained; cleanup/finish exactly once |
