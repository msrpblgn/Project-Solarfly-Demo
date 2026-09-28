#ifndef DAMAGE_CALC_H
#define DAMAGE_CALC_H

#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_result.h"

#define DAMAGE_CALC_ATTACK_BASE_POWER 20
#define DAMAGE_CALC_ENEMY_ATTACK_BASE_POWER 10
/* Use move-level crit rules (see MoveData crit fields). */
#define DAMAGE_CALC_HIT_CRIT_DEFAULT (-1)

int DamageCalc_ApplyAttackDefence(int basePower, int attackerAttack, int defenderDefence);
void DamageCalc_Resolve(
    const BattleMember *attacker,
    BattleMember *defender,
    const MoveData *move,
    int basePower,
    int useLpBonus,
    int hitCritOverridePermille,
    int bypassAccuracyMiss,
    int veilCritBonusPermille,
    CombatResult *result);

#endif
