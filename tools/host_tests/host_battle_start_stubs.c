/*
 * Minimal stubs so TestBattle_Setup / CustomBattleSetup can link on host
 * without pulling battle_enemy_turn → resolve → message/draw.
 * Win/loss predicates and formation init remain production code.
 */
#include "battle/battle_enemy_turn.h"
#include "battle/battle_state.h"

void BattleEnemyTurn_Init(void)
{
}

void BattleEnemyTurn_ResetAiState(void)
{
}

void BattleEnemyTurn_Begin(BattleState *state)
{
    (void)state;
}

void BattleEnemyTurn_RekeyEnemySlot(int fromSlot, int toSlot)
{
    (void)fromSlot;
    (void)toSlot;
}

int BattleEnemyTurn_Execute(BattleState *state)
{
    (void)state;
    return 1;
}
