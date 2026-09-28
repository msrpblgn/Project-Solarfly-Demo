/*
 * Host test: Downed targets take 1.5x incoming damage in DamageCalc_Resolve.
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

static void TestDownedMultiplierAndRecovery(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baseline;
    int downedDamage;
    int afterRemove;
    int afterExpiry;
    int expectedDowned;

    printf("\n== Downed 1.5x incoming damage ==\n");
    Random_Init(1);
    SetupPair(&attacker, &defender);

    baseline = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 30);
    ExpectTrue(baseline > 0, "baseline damage positive");

    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "apply Downed");
    ExpectTrue(BattleMember_IsDowned(&defender), "defender is Downed");
    downedDamage = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 30);
    expectedDowned = (baseline * 150) / 100;
    ExpectEqInt(downedDamage, expectedDowned, "Downed damage is baseline * 1.5 (truncate)");

    ExpectTrue(BattleMember_RemoveStatus(&defender, BATTLE_STATUS_DOWNED), "clear Downed");
    ExpectTrue(!BattleMember_IsDowned(&defender), "Downed ended via remove");
    afterRemove = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 30);
    ExpectEqInt(afterRemove, baseline, "damage returns to normal after Downed ends");

    /* Also verify end-of-round style expiry restores normal damage. */
    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "re-apply Downed");
    ExpectTrue(BattleMember_TickDownedDuration(&defender), "Downed expires after tick");
    ExpectTrue(!BattleMember_IsDowned(&defender), "Downed ended via expiry tick");
    afterExpiry = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 30);
    ExpectEqInt(afterExpiry, baseline, "damage normal after Downed expiry");
}

static void TestDownedAppliesToPlayerTarget(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baseline;
    int downedDamage;

    printf("\n== Downed multiplier on player target ==\n");
    Random_Init(2);
    SetupPair(&attacker, &defender);
    /* Flip roles: enemy hits Downed player. */
    {
        BattleMember tmp = attacker;
        attacker = defender;
        defender = tmp;
        attacker.side = BATTLE_SIDE_ENEMY;
        attacker.slotIndex = PETALBURG_ENEMY_TUTSIL_SLOT;
        defender.side = BATTLE_SIDE_PLAYER;
        defender.slotIndex = PETALBURG_ALLY_PROTAGONIST_SLOT;
    }

    baseline = ResolveHitDamage(&attacker, &defender, MOVE_JAB, 20);
    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "Down player");
    downedDamage = ResolveHitDamage(&attacker, &defender, MOVE_JAB, 20);
    ExpectEqInt(downedDamage, (baseline * 150) / 100, "player Downed takes 1.5x");
    BattleMember_RemoveStatus(&defender, BATTLE_STATUS_DOWNED);
    ExpectEqInt(
        ResolveHitDamage(&attacker, &defender, MOVE_JAB, 20),
        baseline,
        "player damage normal after standing up");
}

static void TestOddDamageTruncate(void)
{
    BattleMember attacker;
    BattleMember defender;
    int baseline;
    int downedDamage;

    printf("\n== Fractional Downed damage uses truncate ==\n");
    Random_Init(3);
    SetupPair(&attacker, &defender);

    /* Force an odd baseline via ApplyAttackDefence path with known inputs. */
    baseline = DamageCalc_ApplyAttackDefence(7, 5, 3); /* (7*5)/3 = 11 */
    ExpectEqInt(baseline, 11, "odd baseline from atk/def");
    ExpectEqInt((baseline * 150) / 100, 16, "11 * 1.5 truncates to 16");

    /* Drive Resolve with a power that yields the same odd intermediate when possible.
     * Still assert the shared percent convention on the Resolve path. */
    baseline = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 1);
    ExpectTrue(BattleMember_AddStatus(&defender, BATTLE_STATUS_DOWNED), "Downed for truncate check");
    downedDamage = ResolveHitDamage(&attacker, &defender, MOVE_HAMMER_FIST, 1);
    ExpectEqInt(downedDamage, (baseline * 150) / 100, "Resolve uses /100 truncate for 1.5x");
}

int main(void)
{
    g_failures = 0;
    TestDownedMultiplierAndRecovery();
    TestDownedAppliesToPlayerTarget();
    TestOddDamageTruncate();
    printf("\n%s\n", g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return g_failures == 0 ? 0 : 1;
}
