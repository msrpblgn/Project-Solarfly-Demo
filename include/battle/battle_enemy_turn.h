#ifndef BATTLE_ENEMY_TURN_H
#define BATTLE_ENEMY_TURN_H

#include "battle/battle_state.h"

void BattleEnemyTurn_Init(void);
void BattleEnemyTurn_ResetAiState(void);
void BattleEnemyTurn_RekeyEnemySlot(int fromSlot, int toSlot);
void BattleEnemyTurn_Begin(BattleState *state);
int BattleEnemyTurn_Execute(BattleState *state);

#endif
