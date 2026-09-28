# Project Solarfly GBA — Scope

## What this project is

This is a **battle-only native GBA prototype** for Project Solarfly.

- Rebuilds the battle-system *idea* from the older Godot prototype using GBA-compatible C.
- Focuses on a clean, expandable scaffold: structs, state machines, static data, update/draw modules.
- Uses Pokémon Emerald decomp only as a **conceptual architecture reference** (not a code copy source).

## What this project is not (yet)

- Not a Godot port.
- Not a full game.
- No overworld.
- No inventory, save data, or audio.
- No final art or full combat mechanics in milestone 1.

## Milestone 1 goal

```text
Boot ROM
→ initialize GBA video/input
→ immediately enter a placeholder battle screen
→ show placeholder player/enemy boxes
→ show a textbox/menu area
→ define basic battle structs and state enums
→ main loop calls Battle_Init, Battle_Update, Battle_Draw
→ build a working .gba ROM
```

## Long-term direction

Grow the scaffold into a full Project Solarfly battle system:

- Per-move SC uses (not a shared mana pool)
- LP upgrades and optional LP costs
- 5 conceptual player slots and 5 conceptual enemy slots
- Command flow: Attack, Skills, Brace, Item, Run
- UI modules: HUD, menu, message queue, target cursor, damage numbers, HP/LP/SC display
- Battle feel: pacing, feedback, configurable timing

Each feature should land in its own module with narrow responsibilities.
