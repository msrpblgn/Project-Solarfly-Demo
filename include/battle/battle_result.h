#ifndef BATTLE_RESULT_H
#define BATTLE_RESULT_H

#include "battle/battle_member.h"

typedef enum BattleOutcome {
    BATTLE_OUTCOME_NONE = 0,
    BATTLE_OUTCOME_VICTORY,
    BATTLE_OUTCOME_DEFEAT,
    BATTLE_OUTCOME_ESCAPE
} BattleOutcome;

typedef struct BattleResult {
    BattleOutcome outcome;
    int turnsElapsed;
} BattleResult;

typedef struct CombatResult {
    int hit;
    int dodged;
    int missed;
    int defenceNegated;
    int critical;
    int damage;
    int attackerLpGained;
    int defenderLpGained;
} CombatResult;

void BattleResult_Clear(BattleResult *result);
void CombatResult_Clear(CombatResult *result);
void CombatMessages_EnqueueOutcome(
    const BattleMember *attacker,
    const BattleMember *defender,
    int hadGuard,
    const CombatResult *result);
void CombatResult_ApplyLpRewards(
    BattleMember *attacker,
    BattleMember *defender,
    CombatResult *result);

#endif
