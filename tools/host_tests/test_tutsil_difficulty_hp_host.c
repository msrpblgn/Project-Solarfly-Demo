/*
 * Host test: Petalburg Tutsil start/max HP scales with Options difficulty.
 *
 * Hard 100% / Normal 70% / Easy 50% of PETALBURG_TUTSIL_BOSS_HP.
 * Badger HP table must stay on its existing constants.
 *
 * Build (WSL):
 *   gcc -std=c11 -Wall -Wextra -O0 \
 *       -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_tutsil_difficulty_hp_host \
 *       tools/host_tests/test_tutsil_difficulty_hp_host.c \
 *       tools/host_tests/host_battle_start_stubs.c \
 *       src/battle/battle_state.c \
 *       src/battle/battle_member.c \
 *       src/battle/battle_move.c \
 *       src/battle/battle_action.c \
 *       src/battle/stat_curve.c \
 *       src/battle/test_battle.c \
 *       src/battle/custom_battle_setup.c \
 *       src/battle/battle_slot.c \
 *       src/data/move_database.c \
 *       src/data/element_database.c \
 *       src/data/race_database.c \
 *       src/data/member_database.c \
 *       src/data/status_database.c \
 *       src/data/item_database.c \
 *       src/core/random.c
 */

#include <stdio.h>

#include "battle/battle_member.h"
#include "battle/battle_state.h"
#include "battle/test_battle.h"

static int g_failures;

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

static int ExpectedTutsilHp(TutsilDifficulty difficulty)
{
    int percent;

    switch (difficulty)
    {
    case TUTSIL_DIFFICULTY_EASY:
        percent = PETALBURG_TUTSIL_HP_PERCENT_EASY;
        break;
    case TUTSIL_DIFFICULTY_HARD:
        percent = PETALBURG_TUTSIL_HP_PERCENT_HARD;
        break;
    case TUTSIL_DIFFICULTY_NORMAL:
    default:
        percent = PETALBURG_TUTSIL_HP_PERCENT_NORMAL;
        break;
    }
    return (PETALBURG_TUTSIL_BOSS_HP * percent) / 100;
}

static int ExpectedBadgerHp(TutsilDifficulty difficulty)
{
    switch (difficulty)
    {
    case TUTSIL_DIFFICULTY_EASY:
        return PETALBURG_ELECTRIC_ARMOR_BADGER_HP_EASY;
    case TUTSIL_DIFFICULTY_HARD:
        return PETALBURG_ELECTRIC_ARMOR_BADGER_HP_HARD;
    case TUTSIL_DIFFICULTY_NORMAL:
    default:
        return PETALBURG_ELECTRIC_ARMOR_BADGER_HP;
    }
}

static void CheckDifficulty(TutsilDifficulty difficulty, const char *label)
{
    BattleState state;
    BattleMember *tutsil;
    BattleMember *badgerL;
    BattleMember *badgerR;
    int wantTutsil;
    int wantBadger;

    printf("\n== %s ==\n", label);
    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, difficulty);
    tutsil = &state.enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT];
    badgerL = &state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT];
    badgerR = &state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_RIGHT];
    wantTutsil = ExpectedTutsilHp(difficulty);
    wantBadger = ExpectedBadgerHp(difficulty);

    ExpectTrue(BattleMember_IsTutsil(tutsil), "slot is Tutsil");
    ExpectEqInt(tutsil->maxHp, wantTutsil, "Tutsil maxHp");
    ExpectEqInt(tutsil->currentHp, wantTutsil, "Tutsil starts at full HP");
    ExpectEqInt(badgerL->maxHp, wantBadger, "badger L HP unchanged by new rule");
    ExpectEqInt(badgerR->maxHp, wantBadger, "badger R HP unchanged by new rule");
    printf("  Tutsil HP = %d / %d\n", tutsil->currentHp, tutsil->maxHp);
}

int main(void)
{
    g_failures = 0;

    ExpectEqInt(PETALBURG_TUTSIL_BOSS_HP, 400, "base HP remains 400");
    ExpectEqInt(ExpectedTutsilHp(TUTSIL_DIFFICULTY_HARD), 400, "Hard expected 400");
    ExpectEqInt(ExpectedTutsilHp(TUTSIL_DIFFICULTY_NORMAL), 280, "Normal expected 280");
    ExpectEqInt(ExpectedTutsilHp(TUTSIL_DIFFICULTY_EASY), 200, "Easy expected 200");

    CheckDifficulty(TUTSIL_DIFFICULTY_HARD, "Hard fight start");
    CheckDifficulty(TUTSIL_DIFFICULTY_NORMAL, "Normal fight start");
    CheckDifficulty(TUTSIL_DIFFICULTY_EASY, "Easy fight start");

    printf("\n%s\n", g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return g_failures == 0 ? 0 : 1;
}
