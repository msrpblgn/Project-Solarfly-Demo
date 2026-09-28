# Solarfly Portfolio

GBA tech demo for **Project Solarfly**: Mode 3 battles, a Mode 0 overworld field test, and a Mode 0 dialogue stack. Built in C for real Game Boy Advance hardware / emulators.

## Download and play

1. Grab the prebuilt ROM at **`build/project_solarfly_gba.gba`**
2. Install **[mGBA](https://mgba.io/)** (0.10.x or newer)
3. Open the ROM: **File → Load ROM…** and select `build/project_solarfly_gba.gba`  
   Or from a terminal:

```bash
mgba build/project_solarfly_gba.gba
```

4. Boot to the main menu and pick a demo (A to confirm; D-pad to move the cursor)

Default mGBA keyboard map: **X = A**, **Z = B**, arrows = D-pad, Enter = Start.

## What’s in this build

| Menu option | What it does |
| ----------- | ------------ |
| **Petalburg Showdown** | Full Mode 3 battle (Protagonist + Aksil vs Tutsil / Electric-Armor Badger). Skills carousel (3 equipped skills), items (Potion / Cure), party screen, Run. Tutsil HP scales with Options → Tutsil difficulty. |
| **Dummy Test Battle** | Minimal battle regression setup. |
| **Overworld Test** | Mode 0 diagnostic map: walk (8-way), collide, camera. Idle poses for eight facings; blink on facings that have a blink frame. Red tile → Petalburg battle (resume keeps position). Blue tile → A starts field dialogue. **B** returns to the menu. |
| **Dialogue Demo** | Mode 0 dialogue runner (portraits + text printer). **B** advances at a next-line prompt (same as A); while text is printing, **B** reveals the rest of the line. |
| **Custom Battle** | Formation editor with session RAM save slots. Maren is selectable; Conall is a labeled stub (not playable). |
| **Options** | Text speed and Tutsil difficulty for this session (not saved to cartridge). |

Also in this build:

- Battle core: targeting, resolve, status, enemy AI for the demos above; DOWNED multiplies incoming attack damage by 1.5×
- Skill card UI with carousel slide when switching skills
- Overworld player OBJ with directional idle / blink banks from Aseprite sources
- Dialogue panel, printer, and portraits wired into Overworld Test and Dialogue Demo

## Not in this build

- Connected story maps, warps, NPCs, followers
- Cartridge save / persistent party or story flags
- Audio / music
- Full playable Conall data or additional boss packs beyond Petalburg

## Build from source

Requires **devkitARM / devkitPro** (`arm-none-eabi-gcc`, GNU Make, grit for optional asset regen).

Install: https://devkitpro.org/wiki/Getting_Started

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH=$DEVKITARM/bin:$PATH
```

From this repository root:

```bash
make -j8
```

Outputs:

- `build/project_solarfly_gba.elf`
- `build/project_solarfly_gba.gba`

On Windows, build inside WSL or MSYS2 with the same `make` command from the repo root (after mounting or cloning the tree where your toolchain can see it).

Committed `src/assets/` and `include/assets/` C arrays are enough for a normal ROM build. Regenerating maps/sprites from `assets_src/` is optional and uses the converters under `tools/` (see `Makefile`).

## Layout

```text
include/      Public headers
src/          Game code + committed generated asset arrays
assets_src/   Source art / Tiled maps for optional regeneration
docs/         Architecture and system notes
tools/        Asset converters and host-test sources
build/        Prebuilt ROM (and local build output)
```

## License

Portfolio demo source and ROM for evaluation / hiring review. Not an open-source license grant for shipping the game commercially, and not affiliated with Nintendo.
