#include "battle/target_selection.h"
#include "battle/battle.h"
#include "battle/battle_action.h"
#include "battle/battle_slot.h"
#include "battle_ui/battle_message_queue.h"
#include "core/input.h"

static const BattleMember *TargetSelection_GetMember(const BattleState *state, int side, int slot)
{
    if (side == BATTLE_SIDE_ENEMY)
    {
        if (slot < 0 || slot >= BATTLE_ENEMY_SLOT_COUNT)
        {
            return 0;
        }
        return &state->enemySlots[slot];
    }
    if (slot < 0 || slot >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return 0;
    }
    return &state->playerSlots[slot];
}

int TargetSelection_GetDefaultSlot(const BattleState *state, int side)
{
    int i;
    if (side == BATTLE_SIDE_ENEMY)
    {
        for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
        {
            if (state->enemySlots[i].isAlive)
            {
                return i;
            }
        }
        return 0;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (state->playerSlots[i].isAlive)
        {
            return i;
        }
    }
    return 0;
}

int TargetSelection_IsValidMoveDestination(const BattleState *state, int actorSlot, int slot)
{
    return BattleState_CanMoveMemberToSlot(state, BATTLE_SIDE_PLAYER, actorSlot, slot);
}

int TargetSelection_HasValidMoveDestination(const BattleState *state, int actorSlot)
{
    int i;

    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (TargetSelection_IsValidMoveDestination(state, actorSlot, i))
        {
            return 1;
        }
    }
    return 0;
}

int TargetSelection_GetDefaultMoveSlot(const BattleState *state, int actorSlot)
{
    static const int s_offsets[] = {1, -1, 2, -2, 3, -3, 4, -4};
    int i;
    int slot;

    for (i = 0; i < 8; i++)
    {
        slot = actorSlot + s_offsets[i];
        if (slot < 0 || slot >= BATTLE_PLAYER_SLOT_COUNT)
        {
            continue;
        }
        if (TargetSelection_IsValidMoveDestination(state, actorSlot, slot))
        {
            return slot;
        }
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (TargetSelection_IsValidMoveDestination(state, actorSlot, i))
        {
            return i;
        }
    }
    return -1;
}

int TargetSelection_GetDefaultEnemySlot(const BattleState *state)
{
    return TargetSelection_GetDefaultSlot(state, BATTLE_SIDE_ENEMY);
}

static int TargetSelection_GetSideSlotCount(int side)
{
    return side == BATTLE_SIDE_ENEMY ? BATTLE_ENEMY_SLOT_COUNT : BATTLE_PLAYER_SLOT_COUNT;
}

static int TargetSelection_IsValidAllyTarget(const BattleState *state, int side, int slot)
{
    if (side != BATTLE_SIDE_PLAYER && side != BATTLE_SIDE_ENEMY)
    {
        return 0;
    }
    return BattleState_IsAliveOccupiedSlot(state, side, slot);
}

static int TargetSelection_GetClosestEnemyDistance(
    const BattleState *state,
    int actorSide,
    int actorSlot)
{
    int opponentSide;
    int slotCount;
    int slot;
    int distance;
    int closest = -1;

    if (!state)
    {
        return -1;
    }
    opponentSide = (actorSide == BATTLE_SIDE_PLAYER) ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
    slotCount = TargetSelection_GetSideSlotCount(opponentSide);
    for (slot = 0; slot < slotCount; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, opponentSide, slot))
        {
            continue;
        }
        distance = BattleSlot_GetDistance(actorSide, actorSlot, opponentSide, slot);
        if (distance == BATTLE_SLOT_DISTANCE_INVALID)
        {
            continue;
        }
        if (closest < 0 || distance < closest)
        {
            closest = distance;
        }
    }
    return closest;
}

static int TargetSelection_IsValidFieryStrikeTarget(
    const BattleState *state,
    int actorSide,
    int actorSlot,
    int targetSide,
    int targetSlot)
{
    int closest;
    int distance;
    int opponentSide;

    opponentSide = (actorSide == BATTLE_SIDE_PLAYER) ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
    if (targetSide != opponentSide ||
        !BattleState_IsAliveOccupiedSlot(state, targetSide, targetSlot))
    {
        return 0;
    }
    closest = TargetSelection_GetClosestEnemyDistance(state, actorSide, actorSlot);
    if (closest < 0)
    {
        return 0;
    }
    distance = BattleSlot_GetDistance(actorSide, actorSlot, targetSide, targetSlot);
    return distance == closest;
}

static int TargetSelection_GetDefaultFieryStrikeSlot_Internal(
    const BattleState *state,
    int actorSide,
    int actorSlot)
{
    int opponentSide;
    int slotCount;
    int slot;
    int closest;

    opponentSide = (actorSide == BATTLE_SIDE_PLAYER) ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
    closest = TargetSelection_GetClosestEnemyDistance(state, actorSide, actorSlot);
    if (closest < 0)
    {
        return TargetSelection_GetDefaultEnemySlot(state);
    }
    slotCount = TargetSelection_GetSideSlotCount(opponentSide);
    for (slot = 0; slot < slotCount; slot++)
    {
        if (TargetSelection_IsValidFieryStrikeTarget(
                state, actorSide, actorSlot, opponentSide, slot))
        {
            return slot;
        }
    }
    return TargetSelection_GetDefaultEnemySlot(state);
}

int TargetSelection_GetDefaultFieryStrikeSlot(
    const BattleState *state,
    int actorSide,
    int actorSlot)
{
    if (!state)
    {
        return 0;
    }
    return TargetSelection_GetDefaultFieryStrikeSlot_Internal(state, actorSide, actorSlot);
}

static int TargetSelection_StepFieryStrikeSlot(
    const BattleState *state,
    int actorSide,
    int actorSlot,
    int currentSlot,
    int direction)
{
    int opponentSide;
    int slotCount;
    int slot;
    int i;

    opponentSide = (actorSide == BATTLE_SIDE_PLAYER) ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
    slotCount = TargetSelection_GetSideSlotCount(opponentSide);
    slot = currentSlot;
    for (i = 0; i < slotCount; i++)
    {
        slot = (slot + direction + slotCount) % slotCount;
        if (TargetSelection_IsValidFieryStrikeTarget(
                state, actorSide, actorSlot, opponentSide, slot))
        {
            return slot;
        }
    }
    return currentSlot;
}

int TargetSelection_GetDefaultAllySwapSlot(
    const BattleState *state,
    int side,
    int actorSlot)
{
    int slotCount;
    int slot;

    if (!state)
    {
        return -1;
    }
    slotCount = TargetSelection_GetSideSlotCount(side);
    for (slot = 0; slot < slotCount; slot++)
    {
        if (slot == actorSlot)
        {
            continue;
        }
        if (TargetSelection_IsValidAllyTarget(state, side, slot))
        {
            return slot;
        }
    }
    return -1;
}

static int TargetSelection_StepAllySwapSlot(
    const BattleState *state,
    int side,
    int actorSlot,
    int currentSlot,
    int direction)
{
    int slot = currentSlot;
    int slotCount = TargetSelection_GetSideSlotCount(side);
    int i;

    for (i = 0; i < slotCount; i++)
    {
        slot = (slot + direction + slotCount) % slotCount;
        if (slot == actorSlot)
        {
            continue;
        }
        if (TargetSelection_IsValidAllyTarget(state, side, slot))
        {
            return slot;
        }
    }
    return currentSlot;
}

static int TargetSelection_StepAllySlot(
    const BattleState *state,
    int side,
    int currentSlot,
    int direction)
{
    int slot = currentSlot;
    int slotCount = TargetSelection_GetSideSlotCount(side);
    int i;

    for (i = 0; i < slotCount; i++)
    {
        slot = (slot + direction + slotCount) % slotCount;
        if (TargetSelection_IsValidAllyTarget(state, side, slot))
        {
            return slot;
        }
    }
    return currentSlot;
}

int TargetSelection_GetDefaultAllyTargetSlot(
    const BattleState *state,
    int side,
    int actorSlot)
{
    if (!state)
    {
        return 0;
    }
    if (TargetSelection_IsValidAllyTarget(state, side, actorSlot))
    {
        return actorSlot;
    }
    return TargetSelection_GetDefaultSlot(state, side);
}

int TargetSelection_GetDefaultItemTargetSlot(const BattleState *state)
{
    if (!state)
    {
        return 0;
    }
    return TargetSelection_GetDefaultAllyTargetSlot(
        state, BATTLE_SIDE_PLAYER, state->activePlayerSlot);
}

int TargetSelection_IsValidItemTarget(const BattleState *state, int side, int slot)
{
    if (side != BATTLE_SIDE_PLAYER)
    {
        return 0;
    }
    return TargetSelection_IsValidAllyTarget(state, side, slot);
}

int TargetSelection_StepItemSlot(const BattleState *state, int currentSlot, int direction)
{
    return TargetSelection_StepAllySlot(
        state, BATTLE_SIDE_PLAYER, currentSlot, direction);
}

int TargetSelection_IsValidTarget(const BattleState *state, const MoveData *move, int side, int slot)
{
    const BattleMember *member;

    if (move && move->targetPattern == TARGET_PATTERN_SINGLE_ALLY)
    {
        return TargetSelection_IsValidAllyTarget(state, side, slot);
    }
    member = TargetSelection_GetMember(state, side, slot);
    if (!member)
    {
        return 0;
    }
    return member->isAlive;
}

int TargetSelection_GetTargetSideForMove(const MoveData *move, int actorSide)
{
    if (!move)
    {
        return BATTLE_SIDE_ENEMY;
    }
    if (move->targetPattern == TARGET_PATTERN_SINGLE_ALLY ||
        move->targetPattern == TARGET_PATTERN_ADJACENT_ALLIES)
    {
        return actorSide;
    }
    if (move->targetPattern == TARGET_PATTERN_SELF ||
        move->targetPattern == TARGET_PATTERN_ALLY_SLOT)
    {
        return BATTLE_SIDE_PLAYER;
    }
    return BATTLE_SIDE_ENEMY;
}

int TargetSelection_StepSlot(const BattleState *state, int side, int currentSlot, int direction)
{
    int slot = currentSlot;
    int i;
    int slotCount = (side == BATTLE_SIDE_ENEMY) ? BATTLE_ENEMY_SLOT_COUNT : BATTLE_PLAYER_SLOT_COUNT;

    for (i = 0; i < slotCount; i++)
    {
        slot = (slot + direction + slotCount) % slotCount;
        if (TargetSelection_IsValidTarget(state, 0, side, slot))
        {
            return slot;
        }
    }
    return currentSlot;
}

int TargetSelection_StepMoveSlot(const BattleState *state, int actorSlot, int currentSlot, int direction)
{
    int slot = currentSlot;
    int i;

    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        slot = (slot + direction + BATTLE_PLAYER_SLOT_COUNT) % BATTLE_PLAYER_SLOT_COUNT;
        if (TargetSelection_IsValidMoveDestination(state, actorSlot, slot))
        {
            return slot;
        }
    }
    return currentSlot;
}

static const BattleMember *TargetSelection_GetActor(const BattleState *state, const BattleAction *action)
{
    if (action->actorSide == BATTLE_SIDE_PLAYER)
    {
        if (action->actorSlot < 0 || action->actorSlot >= BATTLE_PLAYER_SLOT_COUNT)
        {
            return 0;
        }
        return &state->playerSlots[action->actorSlot];
    }
    if (action->actorSlot < 0 || action->actorSlot >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return 0;
    }
    return &state->enemySlots[action->actorSlot];
}

static int TargetSelection_ShouldPromptLp(const BattleState *state, const BattleAction *action, const MoveData *move)
{
    const BattleMember *actor;

    if (!move || move->optionalLpCost <= 0)
    {
        return 0;
    }
    actor = TargetSelection_GetActor(state, action);
    if (!actor)
    {
        return 0;
    }
    return actor->currentLp >= move->optionalLpCost;
}

static void TargetSelection_BeginResolve(BattleState *state)
{
    Battle_BeginResolveAction(state);
}

void TargetSelection_BeginSkillConfirm(BattleState *state, BattleAction *action)
{
    const MoveData *move;

    if (!state || !action)
    {
        return;
    }
    move = MoveData_Get(action->moveId);
    if (TargetSelection_ShouldPromptLp(state, action, move))
    {
        state->lpPromptSelection = 0;
        BattleState_SetPhase(state, BATTLE_PHASE_LP_PROMPT);
        return;
    }
    TargetSelection_BeginResolve(state);
}

static int TargetSelection_IsAllySlotSkill(const BattleAction *action)
{
    const MoveData *move;

    if (!action || action->type != BATTLE_ACTION_SKILL)
    {
        return 0;
    }
    move = MoveData_Get(action->moveId);
    return move && move->targetPattern == TARGET_PATTERN_ALLY_SLOT;
}

static int TargetSelection_IsSingleAllySkill(const BattleAction *action)
{
    const MoveData *move;

    if (!action || action->type != BATTLE_ACTION_SKILL)
    {
        return 0;
    }
    move = MoveData_Get(action->moveId);
    return move && move->targetPattern == TARGET_PATTERN_SINGLE_ALLY;
}

static void TargetSelection_HandleMoveConfirm(BattleState *state, BattleAction *action)
{
    int actorSlot = action->actorSlot;
    int destSlot = state->targetSlot;
    const BattleMember *actor;

    action->targetSlot = destSlot;
    action->targetSide = BATTLE_SIDE_PLAYER;

    if (BattleState_CanMoveMemberToSlot(state, BATTLE_SIDE_PLAYER, actorSlot, destSlot))
    {
        action->useLpBonus = 0;
        TargetSelection_BeginSkillConfirm(state, action);
        return;
    }

    actor = BattleState_GetMemberAtSlotConst(state, BATTLE_SIDE_PLAYER, actorSlot);
    if (actor && BattleMember_IsMovementLocked(actor))
    {
        BattleMessageQueue_Enqueue("Cannot move!");
        return;
    }
    if (destSlot == actorSlot)
    {
        BattleMessageQueue_Enqueue("Already there.");
        return;
    }
    if (BattleState_IsSlotOccupied(state, BATTLE_SIDE_PLAYER, destSlot))
    {
        BattleMessageQueue_Enqueue("Tile occupied.");
        return;
    }
    BattleMessageQueue_Enqueue("Cannot move!");
}

void TargetSelection_Update(BattleState *state, BattleAction *action)
{
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    if (Input_JustPressed(KEY_B))
    {
        if (action->type == BATTLE_ACTION_ATTACK)
        {
            BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_COMMAND);
        }
        else if (action->type == BATTLE_ACTION_ITEM)
        {
            BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_ITEM);
        }
        else
        {
            BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_SKILL);
        }
        return;
    }
    if (action->type == BATTLE_ACTION_ITEM)
    {
        if (Input_JustPressed(KEY_LEFT))
        {
            state->targetSlot =
                TargetSelection_StepItemSlot(state, state->targetSlot, -1);
        }
        if (Input_JustPressed(KEY_RIGHT))
        {
            state->targetSlot =
                TargetSelection_StepItemSlot(state, state->targetSlot, 1);
        }
        if (Input_JustPressed(KEY_A))
        {
            action->targetSlot = state->targetSlot;
            action->targetSide = BATTLE_SIDE_PLAYER;
            TargetSelection_BeginResolve(state);
        }
        return;
    }
    if (TargetSelection_IsSingleAllySkill(action))
    {
        int allySide = action->actorSide;
        const MoveData *allyMove = MoveData_Get(action->moveId);
        int isAllySwap = allyMove && allyMove->id == MOVE_ALLY_SWAP;

        if (Input_JustPressed(KEY_LEFT))
        {
            if (isAllySwap)
            {
                state->targetSlot = TargetSelection_StepAllySwapSlot(
                    state, allySide, action->actorSlot, state->targetSlot, -1);
            }
            else
            {
                state->targetSlot =
                    TargetSelection_StepAllySlot(state, allySide, state->targetSlot, -1);
            }
        }
        if (Input_JustPressed(KEY_RIGHT))
        {
            if (isAllySwap)
            {
                state->targetSlot = TargetSelection_StepAllySwapSlot(
                    state, allySide, action->actorSlot, state->targetSlot, 1);
            }
            else
            {
                state->targetSlot =
                    TargetSelection_StepAllySlot(state, allySide, state->targetSlot, 1);
            }
        }
        if (Input_JustPressed(KEY_A))
        {
            if (!TargetSelection_IsValidAllyTarget(state, allySide, state->targetSlot))
            {
                return;
            }
            if (isAllySwap && state->targetSlot == action->actorSlot)
            {
                BattleMessageQueue_Enqueue("Cannot swap with self!");
                return;
            }
            action->targetSlot = state->targetSlot;
            action->targetSide = allySide;
            action->useLpBonus = 0;
            TargetSelection_BeginSkillConfirm(state, action);
        }
        return;
    }
    if (TargetSelection_IsAllySlotSkill(action))
    {
        if (Input_JustPressed(KEY_LEFT))
        {
            state->targetSlot =
                TargetSelection_StepMoveSlot(state, action->actorSlot, state->targetSlot, -1);
        }
        if (Input_JustPressed(KEY_RIGHT))
        {
            state->targetSlot =
                TargetSelection_StepMoveSlot(state, action->actorSlot, state->targetSlot, 1);
        }
        if (Input_JustPressed(KEY_A))
        {
            TargetSelection_HandleMoveConfirm(state, action);
        }
        return;
    }
    {
        const MoveData *enemySkillMove = MoveData_Get(action->moveId);
        int isFieryStrike =
            action->type == BATTLE_ACTION_SKILL &&
            enemySkillMove &&
            enemySkillMove->id == MOVE_FIERY_STRIKE;

        if (Input_JustPressed(KEY_LEFT))
        {
            if (isFieryStrike)
            {
                state->targetSlot = TargetSelection_StepFieryStrikeSlot(
                    state,
                    action->actorSide,
                    action->actorSlot,
                    state->targetSlot,
                    -1);
            }
            else
            {
                state->targetSlot =
                    TargetSelection_StepSlot(state, state->targetSide, state->targetSlot, -1);
            }
        }
        if (Input_JustPressed(KEY_RIGHT))
        {
            if (isFieryStrike)
            {
                state->targetSlot = TargetSelection_StepFieryStrikeSlot(
                    state,
                    action->actorSide,
                    action->actorSlot,
                    state->targetSlot,
                    1);
            }
            else
            {
                state->targetSlot =
                    TargetSelection_StepSlot(state, state->targetSide, state->targetSlot, 1);
            }
        }
        if (Input_JustPressed(KEY_A))
        {
            if (isFieryStrike &&
                !TargetSelection_IsValidFieryStrikeTarget(
                    state,
                    action->actorSide,
                    action->actorSlot,
                    state->targetSide,
                    state->targetSlot))
            {
                BattleMessageQueue_Enqueue("Only the closest enemy can be hit!");
                return;
            }
            action->targetSlot = state->targetSlot;
            action->targetSide = state->targetSide;
            action->useLpBonus = 0;
            if (action->type == BATTLE_ACTION_ATTACK)
            {
                TargetSelection_BeginResolve(state);
                return;
            }
            TargetSelection_BeginSkillConfirm(state, action);
        }
    }
}
