#include "battle/test_battle.h"
#include "battle/battle_enemy_turn.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_slot.h"
#include "data/member_database.h"
#include "battle/battle_item.h"

#define SOLARFLY_DEBUG_START_WITH_TEST_STATUS 0
#define SOLARFLY_DEBUG_START_WITH_PROTAGONIST_LP 0
#define DUMMY_TEST_DUMMY_HP 999

static void TestBattle_ApplyProtagonistMoves(BattleMember *member)
{
    BattleMember_EquipMove(member, 0, MOVE_HOLD_BACK, 16);
    BattleMember_EquipMove(member, 1, MOVE_HAMMER_FIST, 8);
    BattleMember_EquipMove(member, 2, MOVE_INFLUENCE, 8);
    BattleMember_ClearReservedMoveSlot(member);
}

static void TestBattle_ApplyAksilMoves(BattleMember *member)
{
    BattleMember_EquipMove(member, 0, MOVE_VORTEX_ESCAPE, 6);
    BattleMember_EquipMove(member, 1, MOVE_FAKIE, 4);
    BattleMember_EquipMove(member, 2, MOVE_ALLY_SWAP, 8);
    BattleMember_ClearReservedMoveSlot(member);
}

static void TestBattle_ApplyTutsilMoveset(BattleMember *member)
{
    BattleMember_EquipMove(member, 0, MOVE_VORTEX_ESCAPE, 6);
    BattleMember_EquipMove(member, 1, MOVE_YOUNG_BLOOD, 2);
    BattleMember_EquipMove(member, 2, MOVE_LONG_SHOT, 12);
    BattleMember_ClearReservedMoveSlot(member);
}

static void TestBattle_ApplyElectricArmorBadgerMoveset(BattleMember *member)
{
    BattleMember_EquipMove(member, 0, MOVE_HEADBUTT, 20);
    BattleMember_EquipMove(member, 1, MOVE_CHARGE, 16);
    BattleMember_EquipMove(member, 2, MOVE_STATIC_LOCK, 8);
    BattleMember_ClearReservedMoveSlot(member);
}

static void TestBattle_OverrideHp(BattleMember *member, int hp)
{
    if (!member)
    {
        return;
    }
    member->maxHp = hp;
    member->currentHp = hp;
}

static int TestBattle_GetPetalburgTutsilHp(TutsilDifficulty difficulty)
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
    /* Percent scale with existing truncate convention (same as damage_calc). */
    return (PETALBURG_TUTSIL_BOSS_HP * percent) / 100;
}

static int TestBattle_GetPetalburgBadgerHp(TutsilDifficulty difficulty)
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

void TestBattle_SetupPetalburgShowdown(BattleState *state, TutsilDifficulty difficulty)
{
    const MemberTemplate *protagonistTemplate;
    const MemberTemplate *aksilTemplate;
    const MemberTemplate *electricArmorBadgerTemplate;
    const MemberTemplate *tutsilTemplate;
    int tutsilHp;
    int badgerHp;

    /*
     * Petalburg Showdown demo setup (2v3).
     * Ally Lv16: Aksil slot 1, Protagonist slot 2.
     * Enemy Lv15 badgers in slots 1 and 3; Tutsil boss Lv24 in slot 2.
     * Battle stats come from documented Level 50 bases via stat_curve.
     * Petalburg uses boss-demo HP overrides only; other stats stay on the curve.
     * Options Tutsil Difficulty scales Tutsil start/max HP only (Hard 100% /
     * Normal 70% / Easy 50% of PETALBURG_TUTSIL_BOSS_HP). Badger HP table unchanged.
     * Enemies use real movesets with simple deterministic v1 AI; enemies do not consume SC.
     */
    protagonistTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_PROTAGONIST);
    aksilTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_AKSIL);
    electricArmorBadgerTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER);
    tutsilTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_TUTSIL);
    tutsilHp = TestBattle_GetPetalburgTutsilHp(difficulty);
    badgerHp = TestBattle_GetPetalburgBadgerHp(difficulty);

    BattleMember_InitFromTemplate(
        &state->playerSlots[PETALBURG_ALLY_AKSIL_SLOT],
        aksilTemplate,
        PETALBURG_ALLY_AKSIL_SLOT,
        BATTLE_SIDE_PLAYER);
    BattleMember_InitFromTemplate(
        &state->playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT],
        protagonistTemplate,
        PETALBURG_ALLY_PROTAGONIST_SLOT,
        BATTLE_SIDE_PLAYER);
#if SOLARFLY_DEBUG_START_WITH_PROTAGONIST_LP
    state->playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT].currentLp = 1;
#endif

    TestBattle_ApplyAksilMoves(&state->playerSlots[PETALBURG_ALLY_AKSIL_SLOT]);
    TestBattle_ApplyProtagonistMoves(&state->playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT]);

    BattleMember_InitFromTemplate(
        &state->enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT],
        electricArmorBadgerTemplate,
        PETALBURG_ENEMY_BADGER_SLOT_LEFT,
        BATTLE_SIDE_ENEMY);
    TestBattle_OverrideHp(
        &state->enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT],
        badgerHp);
    BattleMember_InitFromTemplate(
        &state->enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT],
        tutsilTemplate,
        PETALBURG_ENEMY_TUTSIL_SLOT,
        BATTLE_SIDE_ENEMY);
    TestBattle_OverrideHp(
        &state->enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT],
        tutsilHp);
    BattleMember_InitFromTemplate(
        &state->enemySlots[PETALBURG_ENEMY_BADGER_SLOT_RIGHT],
        electricArmorBadgerTemplate,
        PETALBURG_ENEMY_BADGER_SLOT_RIGHT,
        BATTLE_SIDE_ENEMY);
    TestBattle_OverrideHp(
        &state->enemySlots[PETALBURG_ENEMY_BADGER_SLOT_RIGHT],
        badgerHp);

    TestBattle_ApplyTutsilMoveset(&state->enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT]);
    TestBattle_ApplyElectricArmorBadgerMoveset(
        &state->enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT]);
    TestBattle_ApplyElectricArmorBadgerMoveset(
        &state->enemySlots[PETALBURG_ENEMY_BADGER_SLOT_RIGHT]);

    BattleEnemyTurn_ResetAiState();

    state->activePlayerSlot = PETALBURG_ALLY_PROTAGONIST_SLOT;
    state->activeEnemySlot = PETALBURG_ENEMY_TUTSIL_SLOT;
    state->phase = BATTLE_PHASE_INTRO_MESSAGE;

    BattleState_SetItemCount(state, ITEM_POTION, 3);
    BattleState_SetItemCount(state, ITEM_CURE, 3);
}

void TestBattle_SetupDummyTestBattle(BattleState *state)
{
    const MemberTemplate *protagonistTemplate;
    const MemberTemplate *testDummyTemplate;

    protagonistTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_PROTAGONIST);
    testDummyTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_TEST_DUMMY);

    BattleMember_InitFromTemplate(
        &state->playerSlots[TEST_BATTLE_PROTAG_SLOT],
        protagonistTemplate,
        TEST_BATTLE_PROTAG_SLOT,
        BATTLE_SIDE_PLAYER);
    TestBattle_ApplyProtagonistMoves(&state->playerSlots[TEST_BATTLE_PROTAG_SLOT]);

    BattleMember_InitFromTemplate(
        &state->enemySlots[BATTLE_CENTER_SLOT],
        testDummyTemplate,
        BATTLE_CENTER_SLOT,
        BATTLE_SIDE_ENEMY);
    TestBattle_OverrideHp(&state->enemySlots[BATTLE_CENTER_SLOT], DUMMY_TEST_DUMMY_HP);

    BattleEnemyTurn_ResetAiState();

    state->activePlayerSlot = TEST_BATTLE_PROTAG_SLOT;
    state->activeEnemySlot = BATTLE_CENTER_SLOT;
    state->phase = BATTLE_PHASE_INTRO_MESSAGE;
    BattleState_SetItemCount(state, ITEM_POTION, 3);
    BattleState_SetItemCount(state, ITEM_CURE, 3);
}

void TestBattle_Setup(
    BattleState *state,
    BattleDemoId demoId,
    TutsilDifficulty difficulty)
{
    BattleState_Init(state);

    switch (demoId)
    {
    case BATTLE_DEMO_DUMMY_TEST:
        TestBattle_SetupDummyTestBattle(state);
        break;
    case BATTLE_DEMO_PETALBURG_SHOWDOWN:
    default:
        TestBattle_SetupPetalburgShowdown(state, difficulty);
        break;
    }
}

const char *TestBattle_GetIntroMessage(BattleDemoId demoId)
{
    switch (demoId)
    {
    case BATTLE_DEMO_CUSTOM_BATTLE:
        return "Custom Battle!";
    case BATTLE_DEMO_DUMMY_TEST:
        return "Dummy Test Battle!";
    case BATTLE_DEMO_PETALBURG_SHOWDOWN:
    default:
        return "Petalburg Showdown!";
    }
}
