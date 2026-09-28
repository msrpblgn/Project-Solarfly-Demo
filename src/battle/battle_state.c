#include "battle/battle_state.h"
#include "core/random.h"
#include "data/item_database.h"

/* Slots whose KO burrow advanced this frame — for dirty-rect field redraws. */
static unsigned int s_koBurrowDirtyPlayerMask;
static unsigned int s_koBurrowDirtyEnemyMask;

void BattleState_Init(BattleState *state)
{
    int i;
    state->phase = BATTLE_PHASE_INIT;
    state->activePlayerSlot = 0;
    state->activeEnemySlot = 0;
    state->frameCounter = 0;
    state->menuSelection = 0;
    state->skillMenuSelection = 0;
    state->itemMenuSelection = 0;
    state->targetSlot = 0;
    state->targetSide = BATTLE_SIDE_ENEMY;
    state->lpPromptSelection = 0;
    state->bonusActionPlayerSlot = BATTLE_BONUS_ACTION_SLOT_NONE;
    state->allyNextMoveCannotMiss = 0;
    state->isPlayerBonusCommand = 0;
    BattleState_ClearTileEffects(state);
    BattleState_ClearVeilTiles(state);
    BattleState_ClearEnemyEffectReactions(state);
    BattleState_ClearPlayerTurnFlags(state);
    BattleState_ClearInventory(state);
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        BattleMember_InitEmpty(&state->playerSlots[i], i, BATTLE_SIDE_PLAYER);
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        BattleMember_InitEmpty(&state->enemySlots[i], i, BATTLE_SIDE_ENEMY);
    }
}

void BattleState_SetPhase(BattleState *state, BattlePhase phase)
{
    state->phase = phase;
}

int BattleState_GetMeanOpponentSpeed(const BattleState *state, int runnerSide)
{
    int totalSpeed = 0;
    int aliveCount = 0;
    int i;
    int meanSpeed;

    if (!state)
    {
        return 1;
    }
    if (runnerSide == BATTLE_SIDE_PLAYER)
    {
        for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
        {
            if (state->enemySlots[i].maxHp > 0 && state->enemySlots[i].isAlive)
            {
                totalSpeed += BattleMember_GetEffectiveSpeed(&state->enemySlots[i]);
                aliveCount++;
            }
        }
    }
    else
    {
        for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
        {
            if (state->playerSlots[i].maxHp > 0 && state->playerSlots[i].isAlive)
            {
                totalSpeed += BattleMember_GetEffectiveSpeed(&state->playerSlots[i]);
                aliveCount++;
            }
        }
    }
    if (aliveCount <= 0)
    {
        return 1;
    }
    meanSpeed = totalSpeed / aliveCount;
    if (meanSpeed <= 0)
    {
        return 1;
    }
    return meanSpeed;
}

int BattleState_ComputeRunChancePercent(const BattleState *state, int runnerSide, int runnerSlot)
{
    const BattleMember *runner;
    int runnerSpeed;
    int meanOpponentSpeed;
    int chancePercent;

    if (!state)
    {
        return 0;
    }
    if (runnerSide == BATTLE_SIDE_PLAYER)
    {
        if (runnerSlot < 0 || runnerSlot >= BATTLE_PLAYER_SLOT_COUNT)
        {
            return 0;
        }
        runner = &state->playerSlots[runnerSlot];
    }
    else
    {
        if (runnerSlot < 0 || runnerSlot >= BATTLE_ENEMY_SLOT_COUNT)
        {
            return 0;
        }
        runner = &state->enemySlots[runnerSlot];
    }
    runnerSpeed = BattleMember_GetEffectiveSpeed(runner);
    meanOpponentSpeed = BattleState_GetMeanOpponentSpeed(state, runnerSide);
    chancePercent = (50 * runnerSpeed) / meanOpponentSpeed;
    if (chancePercent < 0)
    {
        chancePercent = 0;
    }
    if (chancePercent > 100)
    {
        chancePercent = 100;
    }
    return chancePercent;
}

int BattleState_AllEnemiesDefeated(const BattleState *state)
{
    int i;

    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        if (state->enemySlots[i].maxHp > 0 && state->enemySlots[i].isAlive)
        {
            return 0;
        }
    }
    return 1;
}

int BattleState_AllPlayersDefeated(const BattleState *state)
{
    int i;

    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (state->playerSlots[i].maxHp > 0 && state->playerSlots[i].isAlive)
        {
            return 0;
        }
    }
    return 1;
}

int BattleState_IsSlotValid(int side, int slotIndex)
{
    if (side != BATTLE_SIDE_PLAYER && side != BATTLE_SIDE_ENEMY)
    {
        return 0;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        return slotIndex >= 0 && slotIndex < BATTLE_PLAYER_SLOT_COUNT;
    }
    return slotIndex >= 0 && slotIndex < BATTLE_ENEMY_SLOT_COUNT;
}

BattleMember *BattleState_GetMemberAtSlot(BattleState *state, int side, int slotIndex)
{
    if (!state || !BattleState_IsSlotValid(side, slotIndex))
    {
        return 0;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        return &state->playerSlots[slotIndex];
    }
    return &state->enemySlots[slotIndex];
}

const BattleMember *BattleState_GetMemberAtSlotConst(const BattleState *state, int side, int slotIndex)
{
    if (!state || !BattleState_IsSlotValid(side, slotIndex))
    {
        return 0;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        return &state->playerSlots[slotIndex];
    }
    return &state->enemySlots[slotIndex];
}

int BattleState_IsSlotOccupied(const BattleState *state, int side, int slotIndex)
{
    const BattleMember *member;

    member = BattleState_GetMemberAtSlotConst(state, side, slotIndex);
    if (!member || member->maxHp <= 0)
    {
        return 0;
    }
    /* Dead / burrowed units keep their BattleMember but vacate the tile. */
    if (!member->isAlive || member->currentHp <= 0 || member->removedFromField)
    {
        return 0;
    }
    return 1;
}

int BattleState_IsAliveOccupiedSlot(const BattleState *state, int side, int slotIndex)
{
    const BattleMember *member;

    member = BattleState_GetMemberAtSlotConst(state, side, slotIndex);
    if (!member || member->maxHp <= 0)
    {
        return 0;
    }
    if (!member->isAlive || member->currentHp <= 0 || member->removedFromField)
    {
        return 0;
    }
    return 1;
}

int BattleState_FindFirstEmptySlot(const BattleState *state, int side)
{
    int slotCount;
    int i;

    if (!state)
    {
        return -1;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        slotCount = BATTLE_PLAYER_SLOT_COUNT;
    }
    else if (side == BATTLE_SIDE_ENEMY)
    {
        slotCount = BATTLE_ENEMY_SLOT_COUNT;
    }
    else
    {
        return -1;
    }
    for (i = 0; i < slotCount; i++)
    {
        if (!BattleState_IsSlotOccupied(state, side, i))
        {
            return i;
        }
    }
    return -1;
}

int BattleState_CanMoveMemberToSlot(const BattleState *state, int side, int fromSlot, int toSlot)
{
    if (!state || !BattleState_IsSlotValid(side, fromSlot) || !BattleState_IsSlotValid(side, toSlot))
    {
        return 0;
    }
    if (fromSlot == toSlot)
    {
        return 0;
    }
    if (!BattleState_IsSlotOccupied(state, side, fromSlot))
    {
        return 0;
    }
    if (BattleState_IsSlotOccupied(state, side, toSlot))
    {
        return 0;
    }
    {
        const BattleMember *fromMember;

        if (side == BATTLE_SIDE_ENEMY)
        {
            fromMember = &state->enemySlots[fromSlot];
        }
        else
        {
            fromMember = &state->playerSlots[fromSlot];
        }
        if (BattleMember_IsMovementLocked(fromMember))
        {
            return 0;
        }
    }
    return 1;
}

int BattleState_MoveMemberToSlot(BattleState *state, int side, int fromSlot, int toSlot)
{
    BattleMember *fromMember;
    BattleMember *toMember;
    BattleMember moved;

    if (!BattleState_CanMoveMemberToSlot(state, side, fromSlot, toSlot))
    {
        return 0;
    }
    fromMember = BattleState_GetMemberAtSlot(state, side, fromSlot);
    toMember = BattleState_GetMemberAtSlot(state, side, toSlot);
    moved = *fromMember;
    moved.slotIndex = toSlot;
    moved.side = side;
    *toMember = moved;
    BattleMember_InitEmpty(fromMember, fromSlot, side);
    return 1;
}

int BattleState_CanSwapMembersOnSide(const BattleState *state, int side, int slotA, int slotB)
{
    const BattleMember *memberA;
    const BattleMember *memberB;

    if (!state || !BattleState_IsSlotValid(side, slotA) || !BattleState_IsSlotValid(side, slotB))
    {
        return 0;
    }
    if (slotA == slotB)
    {
        return 0;
    }
    if (!BattleState_IsAliveOccupiedSlot(state, side, slotA) ||
        !BattleState_IsAliveOccupiedSlot(state, side, slotB))
    {
        return 0;
    }
    memberA = BattleState_GetMemberAtSlotConst(state, side, slotA);
    memberB = BattleState_GetMemberAtSlotConst(state, side, slotB);
    if (!memberA || !memberB)
    {
        return 0;
    }
    if (BattleMember_IsMovementLocked(memberA) || BattleMember_IsMovementLocked(memberB))
    {
        return 0;
    }
    return 1;
}

int BattleState_SwapMembersOnSide(BattleState *state, int side, int slotA, int slotB)
{
    BattleMember *memberA;
    BattleMember *memberB;
    BattleMember temp;

    if (!BattleState_CanSwapMembersOnSide(state, side, slotA, slotB))
    {
        return 0;
    }
    memberA = BattleState_GetMemberAtSlot(state, side, slotA);
    memberB = BattleState_GetMemberAtSlot(state, side, slotB);
    if (!memberA || !memberB)
    {
        return 0;
    }

    temp = *memberA;
    *memberA = *memberB;
    *memberB = temp;
    memberA->slotIndex = slotA;
    memberA->side = side;
    memberB->slotIndex = slotB;
    memberB->side = side;

    if (side == BATTLE_SIDE_PLAYER)
    {
        int actedA = state->playerSlotActed[slotA];
        int actedB = state->playerSlotActed[slotB];

        state->playerSlotActed[slotA] = actedB;
        state->playerSlotActed[slotB] = actedA;
        if (state->bonusActionPlayerSlot == slotA)
        {
            state->bonusActionPlayerSlot = slotB;
        }
        else if (state->bonusActionPlayerSlot == slotB)
        {
            state->bonusActionPlayerSlot = slotA;
        }
    }
    return 1;
}

int BattleState_DebugValidateSlotInvariants(const BattleState *state)
{
    int i;

    if (!state)
    {
        return 0;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (state->playerSlots[i].maxHp > 0)
        {
            if (state->playerSlots[i].slotIndex != i)
            {
                return 0;
            }
            if (state->playerSlots[i].side != BATTLE_SIDE_PLAYER)
            {
                return 0;
            }
        }
        else if (state->playerSlots[i].maxHp != 0 || state->playerSlots[i].isAlive != 0)
        {
            return 0;
        }
        else if (state->playerSlots[i].slotIndex != i || state->playerSlots[i].side != BATTLE_SIDE_PLAYER)
        {
            return 0;
        }
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        if (state->enemySlots[i].maxHp > 0)
        {
            if (state->enemySlots[i].slotIndex != i)
            {
                return 0;
            }
            if (state->enemySlots[i].side != BATTLE_SIDE_ENEMY)
            {
                return 0;
            }
        }
        else if (state->enemySlots[i].maxHp != 0 || state->enemySlots[i].isAlive != 0)
        {
            return 0;
        }
        else if (state->enemySlots[i].slotIndex != i || state->enemySlots[i].side != BATTLE_SIDE_ENEMY)
        {
            return 0;
        }
    }
    return 1;
}

static int BattleState_NameEquals(const char *a, const char *b)
{
    if (!a || !b)
    {
        return 0;
    }
    while (*a && *b)
    {
        if (*a != *b)
        {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

int BattleState_IsProtagonistMember(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return BattleState_NameEquals(member->name, "Protagonist");
}

static int BattleState_IsAksilMember(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return BattleState_NameEquals(member->name, "Aksil");
}

int BattleState_FindProtagonistSlot(const BattleState *state)
{
    int i;
    const BattleMember *member;

    if (!state)
    {
        return -1;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        member = BattleState_GetMemberAtSlotConst(state, BATTLE_SIDE_PLAYER, i);
        if (BattleState_IsProtagonistMember(member) &&
            BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, i))
        {
            return i;
        }
    }
    return -1;
}

int BattleState_FindAksilSlot(const BattleState *state)
{
    int i;
    const BattleMember *member;

    if (!state)
    {
        return -1;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        member = BattleState_GetMemberAtSlotConst(state, BATTLE_SIDE_PLAYER, i);
        if (BattleState_IsAksilMember(member) &&
            BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, i))
        {
            return i;
        }
    }
    return -1;
}

int BattleState_FindEnemyTargetPlayerSlot(const BattleState *state)
{
    int slot;

    if (!state)
    {
        return -1;
    }
    slot = BattleState_FindProtagonistSlot(state);
    if (slot >= 0)
    {
        return slot;
    }
    return BattleState_FindAlivePlayerSlot(state, 0);
}

int BattleState_FindAlivePlayerSlot(const BattleState *state, int preferredSlot)
{
    int i;

    if (!state)
    {
        return -1;
    }
    if (BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, preferredSlot) &&
        BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, preferredSlot))
    {
        return preferredSlot;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, i))
        {
            return i;
        }
    }
    return -1;
}

int BattleState_FindRandomAliveAllySlot(
    const BattleState *state,
    int side,
    int actorSlot,
    int excludeSelf)
{
    int candidates[BATTLE_PLAYER_SLOT_COUNT];
    int candidateCount = 0;
    int slotCount;
    int i;
    int pickIndex;

    if (!state)
    {
        return -1;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        slotCount = BATTLE_PLAYER_SLOT_COUNT;
    }
    else if (side == BATTLE_SIDE_ENEMY)
    {
        slotCount = BATTLE_ENEMY_SLOT_COUNT;
    }
    else
    {
        return -1;
    }
    for (i = 0; i < slotCount; i++)
    {
        if (excludeSelf && i == actorSlot)
        {
            continue;
        }
        if (!BattleState_IsAliveOccupiedSlot(state, side, i))
        {
            continue;
        }
        candidates[candidateCount++] = i;
    }
    if (candidateCount <= 0 && excludeSelf)
    {
        return BattleState_FindRandomAliveAllySlot(state, side, actorSlot, 0);
    }
    if (candidateCount <= 0)
    {
        return -1;
    }
    pickIndex = Random_Range(0, candidateCount);
    return candidates[pickIndex];
}

void BattleState_ClearTileEffects(BattleState *state)
{
    int i;

    if (!state)
    {
        return;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        BattleTileEffect_InitEmpty(&state->playerTileEffects[i]);
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        BattleTileEffect_InitEmpty(&state->enemyTileEffects[i]);
    }
}

const BattleTileEffect *BattleState_GetTileEffect(const BattleState *state, int side, int slotIndex)
{
    if (!state || !BattleState_IsSlotValid(side, slotIndex))
    {
        return 0;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        return &state->playerTileEffects[slotIndex];
    }
    return &state->enemyTileEffects[slotIndex];
}

int BattleState_HasTileEffect(const BattleState *state, int side, int slotIndex)
{
    const BattleTileEffect *effect = BattleState_GetTileEffect(state, side, slotIndex);
    if (!effect)
    {
        return 0;
    }
    return effect->effectId != BATTLE_TILE_EFFECT_NONE;
}

void BattleState_SetTileEffect(
    BattleState *state,
    int side,
    int slotIndex,
    BattleTileEffectId effectId,
    int ownerSide,
    int durationTurns)
{
    BattleTileEffect *effect;

    if (!state || !BattleState_IsSlotValid(side, slotIndex))
    {
        return;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        effect = &state->playerTileEffects[slotIndex];
    }
    else
    {
        effect = &state->enemyTileEffects[slotIndex];
    }
    effect->effectId = effectId;
    effect->ownerSide = ownerSide;
    effect->durationTurns = durationTurns;
}

void BattleState_ClearTileEffect(BattleState *state, int side, int slotIndex)
{
    BattleTileEffect *effect;

    if (!state || !BattleState_IsSlotValid(side, slotIndex))
    {
        return;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        effect = &state->playerTileEffects[slotIndex];
    }
    else
    {
        effect = &state->enemyTileEffects[slotIndex];
    }
    BattleTileEffect_InitEmpty(effect);
}

void BattleState_ClearVeilTiles(BattleState *state)
{
    int i;

    if (!state)
    {
        return;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        state->playerVeilTileTurns[i] = 0;
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        state->enemyVeilTileTurns[i] = 0;
    }
}

void BattleState_PlaceRandomVeilTiles(BattleState *state)
{
    int pool[BATTLE_TOTAL_SLOT_COUNT];
    int remaining;
    int i;
    int pick;
    int tileIndex;
    int slot;

    if (!state)
    {
        return;
    }

    BattleState_ClearVeilTiles(state);

    remaining = BATTLE_TOTAL_SLOT_COUNT;
    for (i = 0; i < BATTLE_TOTAL_SLOT_COUNT; i++)
    {
        pool[i] = i;
    }

    for (i = 0; i < BATTLE_VEIL_TILE_COUNT && remaining > 0; i++)
    {
        pick = Random_Range(0, remaining);
        tileIndex = pool[pick];
        remaining--;
        pool[pick] = pool[remaining];

        if (tileIndex < BATTLE_PLAYER_SLOT_COUNT)
        {
            slot = tileIndex;
            state->playerVeilTileTurns[slot] = (unsigned char)BATTLE_VEIL_TILE_DURATION_TURNS;
        }
        else
        {
            slot = tileIndex - BATTLE_PLAYER_SLOT_COUNT;
            state->enemyVeilTileTurns[slot] = (unsigned char)BATTLE_VEIL_TILE_DURATION_TURNS;
        }
    }
}

int BattleState_HasVeilTile(const BattleState *state, int side, int slotIndex)
{
    if (!state || !BattleState_IsSlotValid(side, slotIndex))
    {
        return 0;
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        return state->playerVeilTileTurns[slotIndex] > 0;
    }
    return state->enemyVeilTileTurns[slotIndex] > 0;
}

int BattleState_GetVeilCritStageBonus(
    const BattleState *state,
    int attackerSide,
    int attackerSlot,
    int targetSide,
    int targetSlot)
{
    int bonus = 0;

    if (!state)
    {
        return 0;
    }
    if (BattleState_HasVeilTile(state, attackerSide, attackerSlot))
    {
        bonus++;
    }
    if (BattleState_HasVeilTile(state, targetSide, targetSlot))
    {
        bonus++;
    }
    return bonus;
}

int BattleState_GetVeilCritBonusPermille(
    const BattleState *state,
    int attackerSide,
    int attackerSlot,
    int targetSide,
    int targetSlot)
{
    return BattleState_GetVeilCritStageBonus(
               state,
               attackerSide,
               attackerSlot,
               targetSide,
               targetSlot) *
           BATTLE_VEIL_CRIT_PERMILLE_PER_STAGE;
}

void BattleState_TickVeilTiles(BattleState *state)
{
    int i;

    if (!state)
    {
        return;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (state->playerVeilTileTurns[i] > 0)
        {
            state->playerVeilTileTurns[i]--;
        }
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        if (state->enemyVeilTileTurns[i] > 0)
        {
            state->enemyVeilTileTurns[i]--;
        }
    }
}

int BattleState_TickKoBurrowAnimations(BattleState *state)
{
    int i;
    int any = 0;

    if (!state)
    {
        return 0;
    }
    s_koBurrowDirtyPlayerMask = 0;
    s_koBurrowDirtyEnemyMask = 0;
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (BattleMember_TickKoBurrow(&state->playerSlots[i]))
        {
            s_koBurrowDirtyPlayerMask |= (1u << i);
            any = 1;
        }
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        if (BattleMember_TickKoBurrow(&state->enemySlots[i]))
        {
            s_koBurrowDirtyEnemyMask |= (1u << i);
            any = 1;
        }
    }
    return any;
}

unsigned int BattleState_GetKoBurrowDirtyPlayerMask(const BattleState *state)
{
    (void)state;
    return s_koBurrowDirtyPlayerMask;
}

unsigned int BattleState_GetKoBurrowDirtyEnemyMask(const BattleState *state)
{
    (void)state;
    return s_koBurrowDirtyEnemyMask;
}

void BattleState_ClearKoBurrowDirtyMasks(BattleState *state)
{
    (void)state;
    s_koBurrowDirtyPlayerMask = 0;
    s_koBurrowDirtyEnemyMask = 0;
}

void BattleState_ClearEnemyEffectReactions(BattleState *state)
{
    int i;

    if (!state)
    {
        return;
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        state->enemyEffectReactions[i] = BATTLE_EFFECT_REACTION_NONE;
    }
}

void BattleState_SetEnemyEffectReaction(
    BattleState *state,
    int enemySlot,
    BattleEffectivenessReaction reaction)
{
    if (!state || enemySlot < 0 || enemySlot >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return;
    }
    if ((unsigned int)reaction > (unsigned int)BATTLE_EFFECT_REACTION_CANNOT_INFLICT)
    {
        reaction = BATTLE_EFFECT_REACTION_NONE;
    }
    state->enemyEffectReactions[enemySlot] = reaction;
}

BattleEffectivenessReaction BattleState_GetEnemyEffectReaction(
    const BattleState *state,
    int enemySlot)
{
    BattleEffectivenessReaction reaction;

    if (!state || enemySlot < 0 || enemySlot >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return BATTLE_EFFECT_REACTION_NONE;
    }
    reaction = state->enemyEffectReactions[enemySlot];
    if ((unsigned int)reaction > (unsigned int)BATTLE_EFFECT_REACTION_CANNOT_INFLICT)
    {
        return BATTLE_EFFECT_REACTION_NONE;
    }
    return reaction;
}

void BattleState_ClearPlayerTurnFlags(BattleState *state)
{
    int i;

    if (!state)
    {
        return;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        state->playerSlotActed[i] = 0;
    }
    state->bonusActionPlayerSlot = BATTLE_BONUS_ACTION_SLOT_NONE;
    state->isPlayerBonusCommand = 0;
}

void BattleState_SetPlayerSlotActed(BattleState *state, int slot, int acted)
{
    if (!state || !BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, slot))
    {
        return;
    }
    state->playerSlotActed[slot] = acted ? 1 : 0;
}

void BattleState_MarkPlayerSlotActed(BattleState *state, int slot)
{
    BattleState_SetPlayerSlotActed(state, slot, 1);
}

int BattleState_IsPlayerSlotActed(const BattleState *state, int slot)
{
    if (!state || !BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, slot))
    {
        return 0;
    }
    return state->playerSlotActed[slot] != 0;
}

int BattleState_FindNextLivingUnactedPlayerSlot(const BattleState *state, int startAfterSlot)
{
    int i;
    int startIndex;
    int slot;

    if (!state)
    {
        return -1;
    }
    if (startAfterSlot >= 0 && startAfterSlot < BATTLE_PLAYER_SLOT_COUNT)
    {
        startIndex = (startAfterSlot + 1) % BATTLE_PLAYER_SLOT_COUNT;
    }
    else
    {
        startIndex = 0;
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        slot = (startIndex + i) % BATTLE_PLAYER_SLOT_COUNT;
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
        {
            continue;
        }
        if (BattleState_IsPlayerSlotActed(state, slot))
        {
            continue;
        }
        return slot;
    }
    return -1;
}

/* Slot-based acted flags must move with actors when movement skills relocate a member. */
void BattleState_RekeyPlayerSlotActed(BattleState *state, int fromSlot, int toSlot)
{
    if (!state || fromSlot == toSlot)
    {
        return;
    }
    if (!BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, fromSlot) ||
        !BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, toSlot))
    {
        return;
    }
    state->playerSlotActed[toSlot] = state->playerSlotActed[fromSlot];
    state->playerSlotActed[fromSlot] = 0;
}

void BattleState_SetAllyNextMoveCannotMiss(BattleState *state, int enabled)
{
    if (!state)
    {
        return;
    }
    state->allyNextMoveCannotMiss = enabled ? 1 : 0;
}

int BattleState_HasAllyNextMoveCannotMiss(const BattleState *state)
{
    return state && state->allyNextMoveCannotMiss;
}

int BattleState_TryConsumeAllyNextMoveCannotMiss(BattleState *state)
{
    if (!state || !state->allyNextMoveCannotMiss)
    {
        return 0;
    }
    state->allyNextMoveCannotMiss = 0;
    return 1;
}

void BattleState_ClearInventory(BattleState *state)
{
    int i;

    if (!state)
    {
        return;
    }
    for (i = 0; i < ITEM_COUNT; i++)
    {
        state->itemCounts[i] = 0;
    }
}

int BattleState_GetItemCount(const BattleState *state, ItemId itemId)
{
    if (!state || !ItemData_IsValid(itemId))
    {
        return 0;
    }
    return state->itemCounts[itemId];
}

void BattleState_SetItemCount(BattleState *state, ItemId itemId, int count)
{
    if (!state || !ItemData_IsValid(itemId))
    {
        return;
    }
    if (count < 0)
    {
        count = 0;
    }
    state->itemCounts[itemId] = count;
}

int BattleState_HasAnyItems(const BattleState *state)
{
    int i;

    if (!state)
    {
        return 0;
    }
    for (i = (int)ITEM_POTION; i < (int)ITEM_COUNT; i++)
    {
        if (state->itemCounts[i] > 0)
        {
            return 1;
        }
    }
    return 0;
}

int BattleState_DecrementItem(BattleState *state, ItemId itemId)
{
    if (!state || !ItemData_IsValid(itemId))
    {
        return 0;
    }
    if (state->itemCounts[itemId] <= 0)
    {
        return 0;
    }
    state->itemCounts[itemId]--;
    return 1;
}
