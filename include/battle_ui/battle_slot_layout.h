#ifndef BATTLE_SLOT_LAYOUT_H
#define BATTLE_SLOT_LAYOUT_H

#include "battle/battle_slot.h"
#include "battle/battle_member.h"

typedef struct BattleSlotLayout {
    int tileX;
    int tileY;
    int tileW;
    int tileH;
    int spriteX;
    int spriteY;
    int spriteW;
    int spriteH;
    int statX;
    int statY;
    int cursorX;
    int cursorY;
} BattleSlotLayout;

const BattleSlotLayout *BattleSlotLayout_Get(int side, int slotIndex);

int BattleLayout_GetUnitTopY(
    int side,
    const BattleMember *member,
    const BattleSlotLayout *layout);

int BattleLayout_GetUnitDrawY(
    int side,
    const BattleMember *member,
    const BattleSlotLayout *layout);

#endif
