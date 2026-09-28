# Battle System Spec — Project Solarfly GBA

## Command menu (planned)

| Command   | Description                                      | Status now        |
|-----------|--------------------------------------------------|-------------------|
| Attack    | Basic attack action                              | Menu label only   |
| Skills    | Choose a equipped move                           | Menu label only   |
| Brace     | Restore SC uses, guard next hit                  | Implemented       |
| Item      | Use inventory item                               | Not implemented   |
| Run       | Attempt escape                                   | Menu label only   |

## Resources

### SC (per-move uses)

- Each equipped move has its own remaining uses (`moveSc`), capped by `MoveData.maxSc`.
- **Not** a single shared mana pool.
- Example: Pressure Cannon `maxSc = 4`.

### LP (Luck Points)

- Runtime `currentLp` / `maxLp` on `BattleMember`.
- Protagonist test data: `0 / 9`.
- Moves may have `optionalLpCost` and `lpBonusDamageMultiplierPercent`.
- Example: Lucky Swing Test — optional LP cost `1`, LP bonus `200%` (2x damage).

## Moves (data defined)

| Move               | Power | maxSc | Notes                                      | Behavior |
|--------------------|-------|-------|--------------------------------------------|----------|
| Pressure Cannon    | 80    | 4     | Single enemy, raised crit chance           | Data only |
| Lucky Swing Test   | 30    | 20    | Optional 1 LP for 2x damage, contact       | Data only |

## Slots

- **5 player slots** and **5 enemy slots** (conceptual layout).
- Milestone 1 uses slot 0 only: Protagonist vs Test Dummy.

## Battle phases

```text
INIT → INTRO_MESSAGE → PLAYER_COMMAND → PLAYER_SKILL → TARGET_SELECT
→ RESOLVE_ACTION → MESSAGE_QUEUE → ENEMY_TURN → VICTORY / DEFEAT / ESCAPE
```

| Phase              | Status now                                      |
|--------------------|-------------------------------------------------|
| INIT               | Transitions immediately                         |
| INTRO_MESSAGE      | Placeholder queue messages                      |
| PLAYER_COMMAND     | D-pad moves menu highlight                      |
| PLAYER_SKILL       | Enum only                                       |
| TARGET_SELECT      | Cursor module stub                              |
| RESOLVE_ACTION     | Not implemented                                 |
| MESSAGE_QUEUE      | Basic enqueue/display timer                     |
| ENEMY_TURN         | Not implemented                                 |
| VICTORY/DEFEAT/ESCAPE | Not implemented                              |

## Test battle (milestone 1)

| Combatant    | HP   | LP    | Moves                                      |
|--------------|------|-------|--------------------------------------------|
| Protagonist  | 100  | 0/9   | Pressure Cannon (4 SC), Lucky Swing Test     |
| Test Dummy   | 400  | —     | (enemy moves not wired yet)                |

## Intentionally not implemented yet

- Real damage calculation and hit/crit rolls
- Target selection input and validation flow
- Turn order resolution
- Enemy AI
- SC consumption and Brace
- LP spending and bonuses
- Items and inventory
- Victory/defeat/escape outcomes
- Sprite/tile asset pipeline (placeholder rectangles only)
- Typewriter text effect
- Sound and screen shake
