#ifndef TARGET_SELECTION_H
#define TARGET_SELECTION_H

#include "battle/battle_state.h"
#include "battle/battle_move.h"

struct BattleAction;

int TargetSelection_GetDefaultItemTargetSlot(const BattleState *state);
int TargetSelection_GetDefaultAllyTargetSlot(
    const BattleState *state,
    int side,
    int actorSlot);
int TargetSelection_GetDefaultAllySwapSlot(
    const BattleState *state,
    int side,
    int actorSlot);
int TargetSelection_GetDefaultFieryStrikeSlot(
    const BattleState *state,
    int actorSide,
    int actorSlot);
int TargetSelection_IsValidItemTarget(const BattleState *state, int side, int slot);
int TargetSelection_StepItemSlot(const BattleState *state, int currentSlot, int direction);
int TargetSelection_GetDefaultEnemySlot(const BattleState *state);
int TargetSelection_GetDefaultSlot(const BattleState *state, int side);
int TargetSelection_GetDefaultMoveSlot(const BattleState *state, int actorSlot);
int TargetSelection_IsValidMoveDestination(const BattleState *state, int actorSlot, int slot);
int TargetSelection_HasValidMoveDestination(const BattleState *state, int actorSlot);
int TargetSelection_IsValidTarget(const BattleState *state, const MoveData *move, int side, int slot);
int TargetSelection_StepSlot(const BattleState *state, int side, int currentSlot, int direction);
int TargetSelection_StepMoveSlot(const BattleState *state, int actorSlot, int currentSlot, int direction);
int TargetSelection_GetTargetSideForMove(const MoveData *move, int actorSide);
void TargetSelection_BeginSkillConfirm(BattleState *state, struct BattleAction *action);
void TargetSelection_Update(BattleState *state, struct BattleAction *action);

#endif
