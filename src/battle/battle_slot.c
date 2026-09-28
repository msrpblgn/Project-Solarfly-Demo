#include "battle/battle_slot.h"

void BattleTileEffect_InitEmpty(BattleTileEffect *effect)
{
    if (!effect)
    {
        return;
    }
    effect->effectId = BATTLE_TILE_EFFECT_NONE;
    effect->ownerSide = BATTLE_SIDE_PLAYER;
    effect->durationTurns = 0;
}

int BattleSlot_GetSlotCount(int side)
{
    if (side == BATTLE_SIDE_PLAYER)
    {
        return BATTLE_PLAYER_SLOT_COUNT;
    }
    if (side == BATTLE_SIDE_ENEMY)
    {
        return BATTLE_ENEMY_SLOT_COUNT;
    }
    return 0;
}

static int BattleSlot_IsValidSlotIndex(int side, int slotIndex)
{
    int slotCount = BattleSlot_GetSlotCount(side);
    if (slotCount <= 0)
    {
        return 0;
    }
    return slotIndex >= 0 && slotIndex < slotCount;
}

int BattleSlot_GetDistance(int sideA, int slotA, int sideB, int slotB)
{
    if (!BattleSlot_IsValidSlotIndex(sideA, slotA) || !BattleSlot_IsValidSlotIndex(sideB, slotB))
    {
        return BATTLE_SLOT_DISTANCE_INVALID;
    }
    if (slotA > slotB)
    {
        return slotA - slotB;
    }
    return slotB - slotA;
}
