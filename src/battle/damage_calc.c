#include "battle/damage_calc.h"
#include "battle/battle_slot.h"
#include "core/random.h"

#define DEFAULT_CRIT_PERMILLE 50
#define DAMAGE_DISTANCE_SCALE_PER_SLOT_PERCENT 25
/* Incoming damage while Downed: 1.5x via existing percent truncate (/ 100). */
#define DOWNED_INCOMING_DAMAGE_MULTIPLIER_PERCENT 150

static int DamageCalc_EffectiveDefenceValue(int defence)
{
    if (defence <= 0)
    {
        return 1;
    }
    return defence;
}

static int DamageCalc_ClampPercent(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 100)
    {
        return 100;
    }
    return value;
}

static int DamageCalc_ClampPermille(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 1000)
    {
        return 1000;
    }
    return value;
}

/*
 * Defence negation is chance-based, not automatic when def > atk.
 * Gate: only if defender defence >= 2x attacker attack (Godot parity).
 * Base ~12% at luck stage 0, doubles per luck stage, plus flat Luck %.
 * Veil tile bonuses must never enter this formula (crit-only).
 */
static int DamageCalc_ComputeDefenceNegationChancePercent(
    const BattleMember *attacker,
    const BattleMember *defender,
    int attackerAttack,
    int effectiveDefence)
{
    int chancePercent;
    int stage;
    int i;

    if (!attacker || !defender || attackerAttack <= 0)
    {
        return 0;
    }
    if (effectiveDefence < attackerAttack * 2)
    {
        return 0;
    }

    stage = defender->luckStage;
    if (stage < 0)
    {
        stage = 0;
    }
    if (stage > 6)
    {
        stage = 6;
    }

    /* 0.125 ≈ 12%; doubles each luck stage (12, 25, 50, 100...). */
    chancePercent = 12;
    for (i = 0; i < stage; i++)
    {
        chancePercent *= 2;
    }
    chancePercent += BattleMember_GetLuck(defender);
    return DamageCalc_ClampPercent(chancePercent);
}

static void DamageCalc_ClearDefenderGuard(BattleMember *defender)
{
    if (defender && BattleMember_HasGuardNextHit(defender))
    {
        BattleMember_ClearGuardNextHit(defender);
    }
}

static int DamageCalc_ComputeCritPermille(
    const BattleMember *attacker,
    const MoveData *move,
    int hitCritOverridePermille,
    int veilCritBonusPermille)
{
    int baseCritPermille;
    int critChancePermille;

    if (hitCritOverridePermille >= 0)
    {
        baseCritPermille = hitCritOverridePermille;
    }
    else if (move && move->critChanceOverridePermille >= 0)
    {
        baseCritPermille = move->critChanceOverridePermille;
    }
    else
    {
        baseCritPermille = DEFAULT_CRIT_PERMILLE;
    }
    critChancePermille = baseCritPermille;
    if (move)
    {
        critChancePermille += move->critChanceBonusPermille;
    }
    if (attacker)
    {
        critChancePermille += BattleMember_GetLuck(attacker) * 10;
    }
    if (veilCritBonusPermille > 0)
    {
        critChancePermille += veilCritBonusPermille;
    }
    return DamageCalc_ClampPermille(critChancePermille);
}

static int DamageCalc_ApplyDistanceScale(
    int damage,
    const BattleMember *attacker,
    const BattleMember *defender,
    const MoveData *move)
{
    int distance;
    int multiplierPercent;

    if (!move || !MoveData_HasDistanceScale(move) || !attacker || !defender)
    {
        return damage;
    }
    distance = BattleSlot_GetDistance(
        attacker->side,
        attacker->slotIndex,
        defender->side,
        defender->slotIndex);
    if (distance == BATTLE_SLOT_DISTANCE_INVALID || distance <= 0)
    {
        return damage;
    }
    multiplierPercent = 100 + (distance * DAMAGE_DISTANCE_SCALE_PER_SLOT_PERCENT);
    return (damage * multiplierPercent) / 100;
}

int DamageCalc_ApplyAttackDefence(int basePower, int attackerAttack, int defenderDefence)
{
    int effectiveDefence;
    int damage;

    if (basePower <= 0)
    {
        return 0;
    }
    effectiveDefence = DamageCalc_EffectiveDefenceValue(defenderDefence);
    damage = (basePower * attackerAttack) / effectiveDefence;
    if (damage < 1)
    {
        damage = 1;
    }
    return damage;
}

void DamageCalc_Resolve(
    const BattleMember *attacker,
    BattleMember *defender,
    const MoveData *move,
    int basePower,
    int useLpBonus,
    int hitCritOverridePermille,
    int bypassAccuracyMiss,
    int veilCritBonusPermille,
    CombatResult *result)
{
    int dodgeChancePercent;
    int effectiveDefence;
    int attackerAttack;
    int defenceNegationChancePercent;
    int damage;
    int critChancePermille;

    if (!attacker || !defender || !result)
    {
        return;
    }

    CombatResult_Clear(result);

    if (basePower <= 0 && move)
    {
        basePower = MoveData_GetDamagePower(move);
    }
    if (basePower <= 0)
    {
        return;
    }

    /* Move accuracy (e.g. Hail Mary 67% / ~1/3 miss). 100 or unset = always pass.
     * Team Focus can force a pass via bypassAccuracyMiss (accuracy only, not dodge). */
    if (move && move->accuracyPercent > 0 && move->accuracyPercent < 100)
    {
        if (!bypassAccuracyMiss && !Random_Percent(move->accuracyPercent))
        {
            result->missed = 1;
            result->hit = 0;
            result->damage = 0;
            DamageCalc_ClearDefenderGuard(defender);
            return;
        }
    }

    dodgeChancePercent = DamageCalc_ClampPercent(BattleMember_GetEffectiveEvasiveness(defender));
    if (BattleMember_IsDowned(defender))
    {
        dodgeChancePercent = 0;
    }
    if (dodgeChancePercent > 0 && Random_Percent(dodgeChancePercent))
    {
        result->dodged = 1;
        result->hit = 0;
        result->damage = 0;
        DamageCalc_ClearDefenderGuard(defender);
        return;
    }

    effectiveDefence = BattleMember_GetEffectiveDefence(defender);
    attackerAttack = BattleMember_GetEffectiveAttack(attacker);
    defenceNegationChancePercent = DamageCalc_ComputeDefenceNegationChancePercent(
        attacker,
        defender,
        attackerAttack,
        effectiveDefence);
    if (defenceNegationChancePercent > 0 && Random_Percent(defenceNegationChancePercent))
    {
        result->defenceNegated = 1;
        result->hit = 0;
        result->damage = 0;
        DamageCalc_ClearDefenderGuard(defender);
        return;
    }

    damage = DamageCalc_ApplyAttackDefence(
        basePower,
        attackerAttack,
        effectiveDefence);
    damage = DamageCalc_ApplyDistanceScale(damage, attacker, defender, move);
    result->hit = 1;

    critChancePermille = DamageCalc_ComputeCritPermille(
        attacker,
        move,
        hitCritOverridePermille,
        veilCritBonusPermille);
    if (Random_Permille(critChancePermille))
    {
        result->critical = 1;
        damage *= 2;
    }

    if (useLpBonus && move && move->lpBonusDamageMultiplierPercent > 0)
    {
        damage = (damage * move->lpBonusDamageMultiplierPercent) / 100;
    }

    /* All combatants: while Downed, take 1.5x damage from any attack. */
    if (BattleMember_IsDowned(defender))
    {
        damage = (damage * DOWNED_INCOMING_DAMAGE_MULTIPLIER_PERCENT) / 100;
    }

    {
        int reductionPercent = BattleMember_GetIncomingDamageReductionPercent(defender);

        if (reductionPercent > 0)
        {
            damage = (damage * (100 - reductionPercent)) / 100;
        }
    }

    if (damage < 1)
    {
        damage = 1;
    }
    result->damage = damage;
    DamageCalc_ClearDefenderGuard(defender);
}
