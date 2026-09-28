/*
 * Host test: Tutsil Vortex Escape forces a one-turn skip recovery.
 *
 * Expected sequence:
 *   Vortex Escape → skipNextTurn set (no extra action) →
 *   next enemy turn skipped entirely → recovery cleared → acts normally.
 *
 * Build (WSL):
 *   gcc -std=c11 -Wall -Wextra -O0 \
 *       -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_tutsil_vortex_skip_host \
 *       tools/host_tests/test_tutsil_vortex_skip_host.c \
 *       tools/host_tests/host_tutsil_vortex_stubs.c \
 *       src/battle/battle_state.c \
 *       src/battle/battle_member.c \
 *       src/battle/battle_move.c \
 *       src/battle/battle_action.c \
 *       src/battle/battle_resolve.c \
 *       src/battle/battle_enemy_turn.c \
 *       src/battle/battle_result.c \
 *       src/battle/battle_slot.c \
 *       src/battle/damage_calc.c \
 *       src/battle/stat_curve.c \
 *       src/battle/test_battle.c \
 *       src/battle/custom_battle_setup.c \
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

#include "battle/battle_action.h"
#include "battle/battle_enemy_turn.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_resolve.h"
#include "battle/battle_state.h"
#include "battle/test_battle.h"

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

static int SumLivingPlayerHp(const BattleState *state)
{
    int i;
    int total = 0;

    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        if (state->playerSlots[i].isAlive)
        {
            total += state->playerSlots[i].currentHp;
        }
    }
    return total;
}

static void KnockOutBadgers(BattleState *state)
{
    int slots[2] = {
        PETALBURG_ENEMY_BADGER_SLOT_LEFT,
        PETALBURG_ENEMY_BADGER_SLOT_RIGHT
    };
    int i;

    for (i = 0; i < 2; i++)
    {
        BattleMember *badger = &state->enemySlots[slots[i]];
        badger->currentHp = 0;
        badger->isAlive = 0;
    }
}

static int FindEmptyEnemySlot(const BattleState *state, int excludeSlot)
{
    int slot;

    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        if (slot == excludeSlot)
        {
            continue;
        }
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, slot))
        {
            return slot;
        }
    }
    return -1;
}

static void TestTutsilVortexSetsSkipNotExtraAction(void)
{
    BattleState state;
    BattleAction action;
    BattleMember *tutsil;
    int destSlot;
    int fromSlot;

    printf("\n== Tutsil Vortex sets skip recovery ==\n");
    memset(&state, 0, sizeof(state));
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    BattleEnemyTurn_Init();
    KnockOutBadgers(&state);

    fromSlot = PETALBURG_ENEMY_TUTSIL_SLOT;
    tutsil = &state.enemySlots[fromSlot];
    ExpectTrue(BattleMember_IsTutsil(tutsil), "actor is Tutsil");

    destSlot = FindEmptyEnemySlot(&state, fromSlot);
    ExpectTrue(destSlot >= 0, "empty Vortex destination exists");

    BattleAction_Clear(&action);
    action.type = BATTLE_ACTION_SKILL;
    action.actorSide = BATTLE_SIDE_ENEMY;
    action.actorSlot = fromSlot;
    action.targetSide = BATTLE_SIDE_ENEMY;
    action.targetSlot = destSlot;
    action.moveId = MOVE_VORTEX_ESCAPE;
    action.actorMoveSlot = 0;

    ExpectTrue(BattleResolve_Execute(&state, &action), "Vortex resolve succeeds");

    tutsil = &state.enemySlots[destSlot];
    ExpectTrue(BattleMember_IsTutsil(tutsil), "Tutsil moved to dest slot");
    ExpectEqInt(BattleMember_GetSkipNextTurn(tutsil), 1, "skipNextTurn set after Vortex");
    ExpectEqInt(BattleMember_GetExtraActionsNextTurn(tutsil), 0, "no extraActions after Vortex");
}

static void TestTutsilSkipTurnThenResume(void)
{
    BattleState state;
    BattleAction action;
    BattleMember *tutsil;
    int destSlot;
    int fromSlot;
    int hpBeforeSkip;
    int slotBeforeSkip;
    int hpAfterSkip;
    int hpAfterAttack;
    int playerTarget;

    printf("\n== Tutsil skips one turn then resumes ==\n");
    memset(&state, 0, sizeof(state));
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    BattleEnemyTurn_Init();
    KnockOutBadgers(&state);

    fromSlot = PETALBURG_ENEMY_TUTSIL_SLOT;
    destSlot = FindEmptyEnemySlot(&state, fromSlot);
    ExpectTrue(destSlot >= 0, "empty Vortex destination exists");

    BattleAction_Clear(&action);
    action.type = BATTLE_ACTION_SKILL;
    action.actorSide = BATTLE_SIDE_ENEMY;
    action.actorSlot = fromSlot;
    action.targetSide = BATTLE_SIDE_ENEMY;
    action.targetSlot = destSlot;
    action.moveId = MOVE_VORTEX_ESCAPE;
    action.actorMoveSlot = 0;
    ExpectTrue(BattleResolve_Execute(&state, &action), "Vortex resolve succeeds");

    tutsil = &state.enemySlots[destSlot];
    ExpectEqInt(BattleMember_GetSkipNextTurn(tutsil), 1, "recovery armed");

    /* Recovery turn: Tutsil must not move or attack. */
    hpBeforeSkip = SumLivingPlayerHp(&state);
    slotBeforeSkip = destSlot;
    BattleEnemyTurn_Execute(&state);
    tutsil = &state.enemySlots[slotBeforeSkip];
    hpAfterSkip = SumLivingPlayerHp(&state);

    ExpectTrue(BattleMember_IsTutsil(tutsil) && tutsil->isAlive, "Tutsil still in recovery slot");
    ExpectEqInt(BattleMember_GetSkipNextTurn(tutsil), 0, "recovery cleared after skip turn");
    ExpectEqInt(hpAfterSkip, hpBeforeSkip, "no player damage during skipped turn");

    /* After recovery, Tutsil can act normally again (manual Long Shot). */
    playerTarget = PETALBURG_ALLY_PROTAGONIST_SLOT;
    BattleAction_Clear(&action);
    action.type = BATTLE_ACTION_SKILL;
    action.actorSide = BATTLE_SIDE_ENEMY;
    action.actorSlot = slotBeforeSkip;
    action.targetSide = BATTLE_SIDE_PLAYER;
    action.targetSlot = playerTarget;
    action.moveId = MOVE_LONG_SHOT;
    action.actorMoveSlot = 2;
    ExpectTrue(BattleResolve_Execute(&state, &action), "post-recovery Long Shot succeeds");
    hpAfterAttack = SumLivingPlayerHp(&state);
    ExpectTrue(hpAfterAttack < hpBeforeSkip, "Tutsil deals damage after recovery");
    ExpectEqInt(BattleMember_GetSkipNextTurn(&state.enemySlots[slotBeforeSkip]), 0,
                "skip stays clear after normal action");
}

static void TestAksilVortexDoesNotSkip(void)
{
    BattleState state;
    BattleAction action;
    BattleMember *aksil;
    int destSlot;
    int fromSlot;

    printf("\n== Aksil Vortex unchanged (no skip) ==\n");
    memset(&state, 0, sizeof(state));
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    BattleEnemyTurn_Init();

    fromSlot = PETALBURG_ALLY_AKSIL_SLOT;
    aksil = &state.playerSlots[fromSlot];
    ExpectTrue(!BattleMember_IsTutsil(aksil), "actor is Aksil not Tutsil");

    destSlot = 0;
    if (BattleState_IsAliveOccupiedSlot(&state, BATTLE_SIDE_PLAYER, destSlot))
    {
        destSlot = 3;
    }
    ExpectTrue(
        !BattleState_IsAliveOccupiedSlot(&state, BATTLE_SIDE_PLAYER, destSlot),
        "empty ally destination for Aksil Vortex");

    BattleAction_Clear(&action);
    action.type = BATTLE_ACTION_SKILL;
    action.actorSide = BATTLE_SIDE_PLAYER;
    action.actorSlot = fromSlot;
    action.targetSide = BATTLE_SIDE_PLAYER;
    action.targetSlot = destSlot;
    action.moveId = MOVE_VORTEX_ESCAPE;
    action.actorMoveSlot = 0;

    ExpectTrue(BattleResolve_Execute(&state, &action), "Aksil Vortex resolve succeeds");
    aksil = &state.playerSlots[destSlot];
    ExpectEqInt(BattleMember_GetSkipNextTurn(aksil), 0, "Aksil skipNextTurn stays clear");
    ExpectEqInt(BattleMember_GetExtraActionsNextTurn(aksil), 0, "Aksil no next-turn extraActions");
}

int main(void)
{
    g_failures = 0;
    TestTutsilVortexSetsSkipNotExtraAction();
    TestTutsilSkipTurnThenResume();
    TestAksilVortexDoesNotSkip();

    printf("\n%s\n", g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return g_failures == 0 ? 0 : 1;
}
