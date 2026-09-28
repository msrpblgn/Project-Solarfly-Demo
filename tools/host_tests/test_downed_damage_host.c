/*
 * Host test: Downed targets take 1.5x only from Dark / Chaos / Light moves.
 *
 * Build (WSL):
 *   gcc -std=c11 -Wall -Wextra -O0 \
 *       -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_downed_damage_host \
 *       tools/host_tests/test_downed_damage_host.c \
 *       tools/host_tests/host_tutsil_vortex_stubs.c \
 *       src/battle/damage_calc.c \
 *       src/battle/battle_member.c \
 *       src/battle/battle_move.c \
 *       src/battle/battle_result.c \
 *       src/battle/battle_slot.c \
 *       src/battle/battle_state.c \
 *       src/battle/battle_action.c \
 *       src/battle/stat_curve.c \
 *       src/data/move_database.c \
 *       src/data/element_database.c \
 *       src/data/race_database.c \
 *       src/data/member_database.c \
 *       src/data/status_database.c \
 *       src/data/item_database.c \
 *       src/core/random.c
 */

#include <stdio.h>
#include <string.h>

#include "battle/battle_element.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_result.h"
#include "battle/damage_calc.h"
#include "battle/test_battle.h"
#include "core/random.h"
#include "data/member_database.h"
#include "data/move_database.h"

static int g_failures;

static void ExpectTrue(int cond, const char *label)
{
    if (!cond)
    {
        printf("FAIL: %s\n", label);
        g_failures++;
    }
    else
    {
        printf("OK:   %s\n", label);
    }
}

static void ExpectEqInt(int got, int want, const char *label)
{
    if (got != want)
    {
        printf("FAIL: %s (got %d want %d)\n", label, got, want);
        g_failures++;
    }
    else
    {
        printf("OK:   %s\n", label);
    }
}

static int ResolveHitDamage(
    BattleMember *attacker,
    BattleMember *defender,
    MoveId moveId,
    int basePower)
{
    CombatResult result;
    const MoveData *move;

    move = MoveData_Get(moveId);
    memset(&result, 0, sizeof(result));
    DamageCalc_Resolve(
        attacker,
        defender,
        move,
        basePower,
        0,
        0, /* no crits */
        1, /* bypass accuracy miss */
        0,
        &result);
    ExpectTrue(result.hit && !result.dodged && !result.missed && !result.defenceNegated,
               "hit lands without dodge/miss/negate");
    ExpectEqInt(result.critical, 0, "crit disabled for deterministic damage");
    return result.damage;
}

static void SetupPair(BattleMember *attacker, BattleMember *defender)
{
    const MemberTemplate *protag;
    const MemberTemplate *tutsil;

    protag = MemberDatabase_Get(MEMBER_TEMPLATE_PROTAGONIST);
    tutsil = MemberDatabase_Get(MEMBER_TEMPLATE_TUTSIL);
    BattleMember_InitFromTemplate(attacker, protag, PETALBURG_ALLY_PROTAGONIST_SLOT, BATTLE_SIDE_PLAYER);
    BattleMember_InitFromTemplate(defender, tutsil, PETALBURG_ENEMY_TUTSIL_SLOT, BATTLE_SIDE_ENEMY);
    BattleMember_ClearStatuses(attacker);
    BattleMember_ClearStatuses(defender);
}

static void ExpectMoveElement(MoveId moveId, BattleElementId element, const char *label)
{
    const MoveData *move = MoveData_Get(moveId);
    ExpectTrue(move != NULL, label);
    ExpectEqInt((int)move->element, (int)element, label);
}

static void TestDownedVulnerableElements(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baselineDark;
    int baselineChaos;
    int baselineLight;

    printf("\n== Downed 1.5x only for Dark / Chaos / Light ==\n");
    Random_Init(1);
    SetupPair(&attacker, &defender);

    ExpectMoveElement(MOVE_DARK_TEST, BATTLE_ELEMENT_DARK, "Dark Test is Dark");
    ExpectMoveElement(MOVE_CHAOS_TEST, BATTLE_ELEMENT_CHAOS, "Chaos Test is Chaos");
    ExpectMoveElement(MOVE_LIGHT_TEST, BATTLE_ELEMENT_LIGHT, "Light Test is Light");

    baselineDark = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    baselineChaos = ResolveHitDamage(&attacker, &defender, MOVE_CHAOS_TEST, 15);
    baselineLight = ResolveHitDamage(&attacker, &defender, MOVE_LIGHT_TEST, 15);
    ExpectTrue(baselineDark > 0 && baselineChaos > 0 && baselineLight > 0,
               "vulnerable-element baselines positive");

    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "apply Downed");
    ExpectTrue(BattleMember_IsDowned(&defender), "defender is Downed");

    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15),
        (baselineDark * 150) / 100,
        "Downed + Dark is baseline * 1.5 (truncate)");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_CHAOS_TEST, 15),
        (baselineChaos * 150) / 100,
        "Downed + Chaos is baseline * 1.5 (truncate)");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_LIGHT_TEST, 15),
        (baselineLight * 150) / 100,
        "Downed + Light is baseline * 1.5 (truncate)");
}

static void TestDownedUnaffectedElements(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baselineNone;
    int baselineFire;
    int baselineCurse;
    int baselineMight;

    printf("\n== Downed does not amplify other elements ==\n");
    Random_Init(2);
    SetupPair(&attacker, &defender);

    ExpectMoveElement(MOVE_HAMMER_FIST, BATTLE_ELEMENT_NONE, "Hammer Fist is untyped");
    ExpectMoveElement(MOVE_BURN_TEST, BATTLE_ELEMENT_FIRE, "Burn Test is Fire");
    ExpectMoveElement(MOVE_PHANTOM_PHIST, BATTLE_ELEMENT_CURSE, "Phantom Phist is Curse");
    ExpectMoveElement(MOVE_DIRECT_ATTACK, BATTLE_ELEMENT_MIGHT, "Direct Attack is Might");

    baselineNone = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 30);
    baselineFire = ResolveHitDamage(&attacker, &defender, MOVE_BURN_TEST, 15);
    baselineCurse = ResolveHitDamage(&attacker, &defender, MOVE_PHANTOM_PHIST, 30);
    baselineMight = ResolveHitDamage(&attacker, &defender, MOVE_DIRECT_ATTACK, 20);

    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "apply Downed");

    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 30),
        baselineNone,
        "Downed + untyped stays normal");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_BURN_TEST, 15),
        baselineFire,
        "Downed + Fire stays normal");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_PHANTOM_PHIST, 30),
        baselineCurse,
        "Downed + Curse stays normal");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_DIRECT_ATTACK, 20),
        baselineMight,
        "Downed + Might stays normal");
}

static void TestStandingNoVulnerableBonus(void)
{
    BattleMember attacker;
    BattleMember defender;
    int darkA;
    int darkB;
    int chaosA;
    int lightA;

    printf("\n== Standing targets ignore Dark/Chaos/Light Downed bonus ==\n");
    Random_Init(3);
    SetupPair(&attacker, &defender);

    ExpectTrue(!BattleMember_IsDowned(&defender), "defender starts standing");
    darkA = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    chaosA = ResolveHitDamage(&attacker, &defender, MOVE_CHAOS_TEST, 15);
    lightA = ResolveHitDamage(&attacker, &defender, MOVE_LIGHT_TEST, 15);

    darkB = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    ExpectEqInt(darkB, darkA, "standing Dark damage is stable");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_CHAOS_TEST, 15),
        chaosA,
        "standing Chaos damage is stable");
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_LIGHT_TEST, 15),
        lightA,
        "standing Light damage is stable");
}

static void TestDownedRecoveryClearsBonus(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baseline;
    int downedDamage;
    int afterRemove;
    int afterExpiry;

    printf("\n== Downed Dark bonus ends with status ==\n");
    Random_Init(4);
    SetupPair(&attacker, &defender);

    baseline = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "apply Downed");
    downedDamage = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    ExpectEqInt(downedDamage, (baseline * 150) / 100, "Downed Dark amplified");

    ExpectTrue(BattleMember_RemoveStatus(&defender, BATTLE_STATUS_DOWNED), "clear Downed");
    afterRemove = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    ExpectEqInt(afterRemove, baseline, "Dark damage normal after remove");

    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "re-apply Downed");
    ExpectTrue(BattleMember_TickDownedDuration(&defender), "Downed expires after tick");
    ExpectTrue(!BattleMember_IsDowned(&defender), "Downed ended via expiry tick");
    afterExpiry = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    ExpectEqInt(afterExpiry, baseline, "Dark damage normal after expiry");
}

static void TestDownedAppliesToPlayerTarget(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baseline;
    int downedDamage;

    printf("\n== Downed Dark multiplier on player target ==\n");
    Random_Init(5);
    SetupPair(&attacker, &defender);
    {
        BattleMember tmp = attacker;
        attacker = defender;
        defender = tmp;
        attacker.side = BATTLE_SIDE_ENEMY;
        attacker.slotIndex = PETALBURG_ENEMY_TUTSIL_SLOT;
        defender.side = BATTLE_SIDE_PLAYER;
        defender.slotIndex = PETALBURG_ALLY_PROTAGONIST_SLOT;
    }

    baseline = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
    {
        int jabBaseline = ResolveHitDamage(&attacker, &defender, MOVE_JAB, 20);

        ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "Down player");
        downedDamage = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15);
        ExpectEqInt(downedDamage, (baseline * 150) / 100, "player Downed takes 1.5x from Dark");
        ExpectEqInt(
            ResolveHitDamage(&attacker, &defender, MOVE_JAB, 20),
            jabBaseline,
            "player Downed Jab stays unamplified");
    }
    BattleMember_RemoveStatus(&defender, BATTLE_STATUS_DOWNED);
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 15),
        baseline,
        "player Dark damage normal after standing up");
}

static void TestOddDamageTruncate(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baseline;
    int downedDamage;

    printf("\n== Fractional Downed Dark damage uses truncate ==\n");
    Random_Init(6);
    SetupPair(&attacker, &defender);

    baseline = DamageCalc_ApplyAttackDefence(7, 5, 3); /* (7*5)/3 = 11 */
    ExpectEqInt(baseline, 11, "odd baseline from atk/def");
    ExpectEqInt((baseline * 150) / 100, 16, "11 * 1.5 truncates to 16");

    baseline = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 1);
    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "Downed for truncate check");
    downedDamage = ResolveHitDamage(&attacker, &defender, MOVE_DARK_TEST, 1);
    ExpectEqInt(downedDamage, (baseline * 150) / 100, "Resolve uses /100 truncate for Dark 1.5x");
}

int main(void)
{
    g_failures = 0;
    TestDownedVulnerableElements();
    TestDownedUnaffectedElements();
    TestStandingNoVulnerableBonus();
    TestDownedRecoveryClearsBonus();
    TestDownedAppliesToPlayerTarget();
    TestOddDamageTruncate();
    printf("\n%s\n", g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return g_failures == 0 ? 0 : 1;
}
