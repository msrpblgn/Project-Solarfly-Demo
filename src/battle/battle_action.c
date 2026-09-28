#include "battle/battle_action.h"

void BattleAction_Clear(BattleAction *action)
{
    action->type = BATTLE_ACTION_NONE;
    action->actorSlot = 0;
    action->actorSide = 0;
    action->targetSlot = 0;
    action->targetSide = 0;
    action->moveId = MOVE_NONE;
    action->itemId = ITEM_NONE;
    action->actorMoveSlot = 0;
    action->useLpBonus = 0;
}
