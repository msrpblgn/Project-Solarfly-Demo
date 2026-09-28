#ifndef BATTLE_ACTION_H
#define BATTLE_ACTION_H

#include "battle/battle_move.h"
#include "battle/battle_item.h"

typedef enum BattleActionType {
    BATTLE_ACTION_NONE = 0,
    BATTLE_ACTION_ATTACK,
    BATTLE_ACTION_SKILL,
    BATTLE_ACTION_BRACE,
    BATTLE_ACTION_ITEM,
    BATTLE_ACTION_RUN
} BattleActionType;

typedef struct BattleAction {
    BattleActionType type;
    int actorSlot;
    int actorSide;
    int targetSlot;
    int targetSide;
    MoveId moveId;
    ItemId itemId;
    int actorMoveSlot;
    int useLpBonus;
} BattleAction;

void BattleAction_Clear(BattleAction *action);

#endif
