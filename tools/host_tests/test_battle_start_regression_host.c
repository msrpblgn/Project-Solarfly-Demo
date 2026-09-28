/*
 * Battle-start / end-condition regression (production logic).
 *
 * Uses TestBattle_Setup / CustomBattleSetup_ApplyFormationToState and real
 * BattleState_AllEnemiesDefeated / AllPlayersDefeated — not stubs of win/loss.
 *
 * Build (WSL):
 *   gcc -std=c11 -Wall -Wextra -O0 -fsanitize=address,undefined \
 *       -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_battle_start_regression_host \
 *       tools/host_tests/test_battle_start_regression_host.c \
 *       src/battle/battle_state.c \
 *       src/battle/battle_member.c \
 *       src/battle/battle_move.c \
 *       src/battle/battle_action.c \
 *       src/battle/stat_curve.c \
 *       src/battle/test_battle.c \
 *       src/battle/custom_battle_setup.c \
 *       src/battle/battle_slot.c \
 *       tools/host_tests/host_battle_start_stubs.c \
 *       src/data/move_database.c \
 *       src/data/element_database.c \
 *       src/data/race_database.c \
 *       src/data/member_database.c \
 *       src/data/status_database.c \
 *       src/data/item_database.c \
 *       src/core/random.c
 *
 * Untested: hardware, drawing, input, full Battle_Update UI loop.
 */

#include <stdio.h>
#include <string.h>

#include "battle/battle_action.h"
#include "battle/battle_enemy_turn.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_state.h"
#include "battle/custom_battle_setup.h"
#include "battle/damage_calc.h"
#include "battle/test_battle.h"
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

static int CountLiving(const BattleMember *slots, int count)
{
    int i;
    int n = 0;

    for (i = 0; i < count; i++)
    {
        if (slots[i].maxHp > 0 && slots[i].isAlive)
        {
            n++;
        }
    }
    return n;
}

static void DumpSide(const char *label, const BattleMember *slots, int count)
{
    int i;

    printf("--- %s ---\n", label);
    for (i = 0; i < count; i++)
    {
        printf(
            "  slot %d name=%s idSide=%d hp=%d/%d alive=%d moves=%d,%d,%d,%d\n",
            i,
            slots[i].name ? slots[i].name : "(null)",
            slots[i].side,
            slots[i].currentHp,
            slots[i].maxHp,
            slots[i].isAlive,
            (int)slots[i].equippedMoves[0],
            (int)slots[i].equippedMoves[1],
            (int)slots[i].equippedMoves[2],
            (int)slots[i].equippedMoves[3]);
    }
}

static void CheckPetalburgStart(const char *tag)
{
    BattleState state;
    int allyCount;
    int enemyCount;

    printf("\n== %s: Petalburg start ==\n", tag);
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    DumpSide("allies", state.playerSlots, BATTLE_PLAYER_SLOT_COUNT);
    DumpSide("enemies", state.enemySlots, BATTLE_ENEMY_SLOT_COUNT);

    allyCount = CountLiving(state.playerSlots, BATTLE_PLAYER_SLOT_COUNT);
    enemyCount = CountLiving(state.enemySlots, BATTLE_ENEMY_SLOT_COUNT);

    ExpectEqInt(state.phase, BATTLE_PHASE_INTRO_MESSAGE, "phase INTRO_MESSAGE");
    ExpectEqInt(allyCount, 2, "ally living count 2");
    ExpectEqInt(enemyCount, 3, "enemy living count 3");
    ExpectTrue(
        state.playerSlots[PETALBURG_ALLY_AKSIL_SLOT].maxHp > 0
            && state.playerSlots[PETALBURG_ALLY_AKSIL_SLOT].isAlive,
        "Aksil alive positive HP");
    ExpectTrue(
        state.playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT].maxHp > 0
            && state.playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT].isAlive,
        "Protagonist alive positive HP");
    ExpectTrue(
        state.enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT].maxHp > 0
            && state.enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT].isAlive,
        "Tutsil alive positive HP");
    ExpectTrue(
        state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT].maxHp > 0
            && state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT].isAlive,
        "Badger L alive positive HP");
    ExpectTrue(
        state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_RIGHT].maxHp > 0
            && state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_RIGHT].isAlive,
        "Badger R alive positive HP");
    ExpectEqInt(
        state.playerSlots[PETALBURG_ALLY_AKSIL_SLOT].side,
        BATTLE_SIDE_PLAYER,
        "Aksil side player");
    ExpectEqInt(
        state.enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT].side,
        BATTLE_SIDE_ENEMY,
        "Tutsil side enemy");
    ExpectTrue(!BattleState_AllEnemiesDefeated(&state), "victory false at start");
    ExpectTrue(!BattleState_AllPlayersDefeated(&state), "defeat false at start");
    ExpectEqInt(state.activePlayerSlot, PETALBURG_ALLY_PROTAGONIST_SLOT, "active player");
    ExpectEqInt(state.activeEnemySlot, PETALBURG_ENEMY_TUTSIL_SLOT, "active enemy");
}

static void CheckCustomStart(const char *tag)
{
    BattleState state;
    CustomBattleFormation formation;
    int enemyCount;

    printf("\n== %s: Custom battle start ==\n", tag);
    CustomBattleSetup_InitFormation(&formation);
    CustomBattleSetup_ResetCurrentFormation();
    ExpectTrue(
        CustomBattleSetup_SetAllySlot(2, MEMBER_TEMPLATE_PROTAGONIST, 16),
        "set custom ally");
    ExpectTrue(
        CustomBattleSetup_SetEnemySlot(2, MEMBER_TEMPLATE_TUTSIL, 24),
        "set custom enemy");
    formation = *CustomBattleSetup_GetCurrentFormationConst();
    CustomBattleSetup_ApplyFormationToState(&state, &formation);
    DumpSide("custom allies", state.playerSlots, BATTLE_PLAYER_SLOT_COUNT);
    DumpSide("custom enemies", state.enemySlots, BATTLE_ENEMY_SLOT_COUNT);

    enemyCount = CountLiving(state.enemySlots, BATTLE_ENEMY_SLOT_COUNT);
    ExpectTrue(enemyCount >= 1, "custom has living enemy");
    ExpectTrue(
        state.enemySlots[2].maxHp > 0 && state.enemySlots[2].isAlive,
        "custom enemy HP positive");
    ExpectTrue(!BattleState_AllEnemiesDefeated(&state), "custom victory false");
    ExpectTrue(!BattleState_AllPlayersDefeated(&state), "custom defeat false");
}

static void CheckActionIntegrity(const char *tag)
{
    BattleState state;
    BattleMember *protag;
    BattleMember *enemy;
    int sc0;
    int sc1;
    int sc2;
    int sc3;
    int hpBefore;
    int enemyHpBefore;
    int allyAlive;
    int enemyAlive;

    printf("\n== %s: ordinary attack integrity ==\n", tag);
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    protag = &state.playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT];
    enemy = &state.enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT];
    sc0 = protag->moveSc[0];
    sc1 = protag->moveSc[1];
    sc2 = protag->moveSc[2];
    sc3 = protag->moveSc[3];
    hpBefore = protag->currentHp;
    enemyHpBefore = enemy->currentHp;
    allyAlive = CountLiving(state.playerSlots, BATTLE_PLAYER_SLOT_COUNT);
    enemyAlive = CountLiving(state.enemySlots, BATTLE_ENEMY_SLOT_COUNT);

    /* Basic-attack path does not spend SC; apply provisional weak hit via TakeDamage. */
    BattleMember_TakeDamage(enemy, DAMAGE_CALC_ATTACK_BASE_POWER);
    ExpectTrue(enemy->currentHp < enemyHpBefore, "enemy HP reduced by attack");
    ExpectEqInt(protag->moveSc[0], sc0, "attack leaves SC0");
    ExpectEqInt(protag->moveSc[1], sc1, "attack leaves SC1");
    ExpectEqInt(protag->moveSc[2], sc2, "attack leaves SC2");
    ExpectEqInt(protag->moveSc[3], sc3, "attack leaves reserved SC");
    ExpectEqInt(protag->currentHp, hpBefore, "attacker HP unchanged");
    ExpectEqInt(
        CountLiving(state.playerSlots, BATTLE_PLAYER_SLOT_COUNT),
        allyAlive,
        "ally count intact after attack");
    ExpectEqInt(
        CountLiving(state.enemySlots, BATTLE_ENEMY_SLOT_COUNT),
        enemyAlive,
        "enemy count intact after nonlethal attack");
    ExpectTrue(!BattleState_AllEnemiesDefeated(&state), "still not victory");

    /* Enemy-side hit against ally without collapsing formation. */
    BattleMember_TakeDamage(protag, 10);
    ExpectTrue(protag->currentHp < hpBefore, "ally HP reduced by enemy hit");
    ExpectTrue(protag->isAlive, "ally remains alive");
    ExpectTrue(!BattleState_AllPlayersDefeated(&state), "still not defeat");
}

static void CheckPositiveControls(const char *tag)
{
    BattleState state;

    printf("\n== %s: victory/defeat positive controls ==\n", tag);
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);

    {
        int i;
        for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
        {
            state.enemySlots[i].isAlive = 0;
            state.enemySlots[i].currentHp = 0;
        }
    }
    ExpectTrue(BattleState_AllEnemiesDefeated(&state), "defeat all enemies => victory");
    ExpectTrue(!BattleState_AllPlayersDefeated(&state), "allies still alive");

    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    {
        int i;
        for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
        {
            state.playerSlots[i].isAlive = 0;
            state.playerSlots[i].currentHp = 0;
        }
    }
    ExpectTrue(BattleState_AllPlayersDefeated(&state), "defeat all allies => defeat");
    ExpectTrue(!BattleState_AllEnemiesDefeated(&state), "enemies still alive");
}

int main(void)
{
    const char *tag = "baseline";

#if defined(BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT)
    tag = "post-migration";
#endif

    printf("battle_start_regression tag=%s\n", tag);
    printf(
        "storage=%d equipped_macro=%s\n",
        BATTLE_MEMBER_MOVE_SLOT_COUNT,
#if defined(BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT)
        "defined"
#else
        "undefined"
#endif
    );

    CheckPetalburgStart(tag);
    CheckCustomStart(tag);
    CheckActionIntegrity(tag);
    CheckPositiveControls(tag);

    if (g_failures)
    {
        printf("\nFAILED: %d checks\n", g_failures);
        return 1;
    }
    printf("\nAll battle-start regression checks passed.\n");
    return 0;
}
