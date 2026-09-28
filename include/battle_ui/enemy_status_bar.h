#ifndef ENEMY_STATUS_BAR_H
#define ENEMY_STATUS_BAR_H

#include "battle/battle_state.h"

void EnemyStatusBar_DrawAll(const BattleState *state);
void EnemyStatusBar_DrawForSlot(const BattleState *state, int enemySlot);

#endif
