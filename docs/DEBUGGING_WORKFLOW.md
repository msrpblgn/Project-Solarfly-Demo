# Debugging Workflow — Project Solarfly GBA

Do **not** randomly try fixes when something looks wrong on hardware or in an emulator.

## Step 1 — Describe the symptom

Example: "Enemy box draws but player HP text is missing."

## Step 2 — Report relevant code paths

Before changing code, identify where the behavior originates:

| Symptom area        | Check these modules first                          |
|---------------------|----------------------------------------------------|
| Sprites/boxes       | `battle_hud.c`, `placeholder_sprites.c`, `video.c` |
| Palettes/colors     | `palettes.h`, `video.c`, `gba_types.h` RGB macros  |
| Input               | `input.c`, `battle_menu.c`, `battle.c`             |
| Battle state/phase  | `battle.c`, `battle_state.c`, `test_battle.c`      |
| Text/menu           | `battle_message_queue.c`, `battle_menu.c`, `video.c` |
| Static data         | `move_database.c`, `test_battle.c`, `member_database.c` |

## Step 3 — Narrow the failure

- If nothing renders: verify `Video_Init`, Mode 3 `REG_DISPCNT`, and that `Battle_Draw` runs every frame after VBlank wait.
- If input dead: verify `Input_Update` is called once per frame and keys are active-low (`~REG_KEYINPUT`).
- If wrong phase: log or breakpoint `BattleState.phase` transitions in `battle.c`.
- If text garbled: check `Video_DrawText` glyph index and string buffers in message queue.

## Step 4 — Apply a small fix

- One hypothesis, one change, rebuild, re-test.
- Prefer fixes localized to the module that owns the symptom.
- Avoid cross-cutting refactors during bugfix passes.

## Step 5 — Rebuild and verify

```bash
make clean && make
```

Run `build/project_solarfly_gba.gba` in mGBA or similar and confirm only the intended behavior changed.

## Emulator tips

- mGBA: use tile/map viewers sparingly in Mode 3 (bitmap); rely on visual inspection and module isolation.
- Enable frame advance to step through intro messages and menu transitions.
