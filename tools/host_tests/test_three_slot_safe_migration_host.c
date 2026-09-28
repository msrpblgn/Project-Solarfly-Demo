/*
 * Safe three-slot migration host checks (storage=4, usable=3).
 *
 *   gcc -std=c11 -Wall -Wextra -O0 -fsanitize=address,undefined -Iinclude \
 *       -o tools/host_tests/test_three_slot_safe_migration_host \
 *       tools/host_tests/test_three_slot_safe_migration_host.c \
 *       tools/host_tests/host_battle_start_stubs.c \
 *       src/battle/battle_member.c src/battle/battle_move.c \
 *       src/battle/battle_action.c src/battle/battle_state.c \
 *       src/battle/battle_slot.c src/battle/custom_battle_setup.c \
 *       src/battle/stat_curve.c src/battle/test_battle.c \
 *       src/core/random.c \
 *       src/data/member_database.c src/data/move_database.c \
 *       src/data/element_database.c src/data/race_database.c \
 *       src/data/status_database.c src/data/item_database.c
 */

#include <stdio.h>
#include <string.h>

#include "battle/battle_action.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_state.h"
#include "battle/custom_battle_setup.h"
#include "battle/damage_calc.h"
#include "battle/test_battle.h"
#include "data/element_database.h"
#include "data/member_database.h"
#include "data/race_database.h"

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

static int FindEquippedMoveSlotUsable(const BattleMember *member, MoveId moveId)
{
    int i;

    if (!member)
    {
        return -1;
    }
    for (i = 0; i < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; i++)
    {
        if (member->equippedMoves[i] == moveId)
        {
            return i;
        }
    }
    return -1;
}

static void TestCapacityAndBounds(void)
{
    BattleMember member;
    int scBefore0;
    int scBefore2;

    ExpectEqInt(BATTLE_MEMBER_MOVE_SLOT_COUNT, 4, "storage capacity is four");
    ExpectEqInt(BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT, 3, "usable equipped limit is three");
    ExpectEqInt(BATTLE_MEMBER_RESERVED_MOVE_SLOT, 3, "reserved storage index is three");

    BattleMember_Init(&member, "Bound", 100, 10, 0, 0);
    BattleMember_EquipMove(&member, 0, MOVE_HOLD_BACK, 5);
    BattleMember_EquipMove(&member, 1, MOVE_HAMMER_FIST, 5);
    BattleMember_EquipMove(&member, 2, MOVE_INFLUENCE, 5);
    ExpectEqInt(member.equippedMoves[3], MOVE_NONE, "reserved empty after equip");
    ExpectEqInt(member.moveSc[3], 0, "reserved SC zero after equip");

    scBefore0 = member.moveSc[0];
    scBefore2 = member.moveSc[2];
    BattleMember_EquipMove(&member, 3, MOVE_RAM, 99);
    ExpectEqInt(member.equippedMoves[0], MOVE_HOLD_BACK, "slot0 unchanged after invalid equip");
    ExpectEqInt(member.equippedMoves[2], MOVE_INFLUENCE, "slot2 unchanged after invalid equip");
    ExpectEqInt(member.equippedMoves[3], MOVE_NONE, "slot3 still empty after rejected equip");
    ExpectEqInt(member.moveSc[0], scBefore0, "slot0 SC unchanged after invalid equip");
    ExpectEqInt(member.moveSc[2], scBefore2, "slot2 SC unchanged after invalid equip");
    ExpectTrue(!BattleMember_IsEquippableMoveSlot(3), "slot 3 not equippable");
    ExpectTrue(!BattleMember_CanUseMoveSlot(&member, 3), "invalid slot 3 not usable");
    ExpectTrue(!BattleMember_SpendMoveSc(&member, 3, 1), "invalid slot 3 spend rejected");
}

static void TestSparseDuplicateAndSc(void)
{
    BattleMember member;

    BattleMember_Init(&member, "Sparse", 100, 10, 0, 0);
    BattleMember_EquipMove(&member, 0, MOVE_NONE, 0);
    BattleMember_EquipMove(&member, 1, MOVE_HOLD_BACK, 4);
    BattleMember_EquipMove(&member, 2, MOVE_HOLD_BACK, 7);

    ExpectTrue(!BattleMember_CanUseMoveSlot(&member, 0), "empty slot unusable");
    ExpectTrue(BattleMember_CanUseMoveSlot(&member, 1), "dup slot1 usable");
    ExpectTrue(BattleMember_CanUseMoveSlot(&member, 2), "dup slot2 usable");
    ExpectEqInt(member.moveSc[1], 4, "dup slot1 independent SC");
    ExpectEqInt(member.moveSc[2], 7, "dup slot2 independent SC");

    ExpectTrue(BattleMember_SpendMoveSc(&member, 1, 1), "spend slot1");
    ExpectEqInt(member.moveSc[1], 3, "slot1 SC after spend");
    ExpectEqInt(member.moveSc[2], 7, "slot2 SC untouched");

    BattleMember_BraceRecoverAllMoves(&member, 1);
    ExpectEqInt(member.moveSc[0], 0, "empty slot SC stays 0 after brace");
    ExpectEqInt(member.moveSc[3], 0, "reserved SC stays 0 after brace");
    ExpectTrue(member.moveSc[1] >= 3, "slot1 recovered within usable");
}

static void TestPetalburgKitsAndLongShot(void)
{
    BattleState state;
    const BattleMember *protag;
    const BattleMember *aksil;
    const BattleMember *tutsil;
    const BattleMember *badger;
    int longShotSlot;

    TestBattle_Setup(&state, BATTLE_DEMO_PETALBURG_SHOWDOWN, TUTSIL_DIFFICULTY_NORMAL);
    aksil = &state.playerSlots[PETALBURG_ALLY_AKSIL_SLOT];
    protag = &state.playerSlots[PETALBURG_ALLY_PROTAGONIST_SLOT];
    tutsil = &state.enemySlots[PETALBURG_ENEMY_TUTSIL_SLOT];
    badger = &state.enemySlots[PETALBURG_ENEMY_BADGER_SLOT_LEFT];

    ExpectEqInt(protag->equippedMoves[0], MOVE_HOLD_BACK, "protag slot0 Hold Back");
    ExpectEqInt(protag->equippedMoves[1], MOVE_HAMMER_FIST, "protag slot1 Hammer Fist");
    ExpectEqInt(protag->equippedMoves[2], MOVE_INFLUENCE, "protag slot2 Influence");
    ExpectEqInt(protag->equippedMoves[3], MOVE_NONE, "protag reserved empty");
    ExpectTrue(protag->equippedMoves[0] != MOVE_PHANTOM_PHIST, "no early Phantom Phist");

    ExpectEqInt(aksil->equippedMoves[0], MOVE_VORTEX_ESCAPE, "aksil slot0 Vortex");
    ExpectEqInt(aksil->equippedMoves[1], MOVE_FAKIE, "aksil slot1 Fakie");
    ExpectEqInt(aksil->equippedMoves[2], MOVE_ALLY_SWAP, "aksil slot2 Ally Swap");
    ExpectEqInt(aksil->equippedMoves[3], MOVE_NONE, "aksil reserved empty");

    ExpectEqInt(tutsil->equippedMoves[0], MOVE_VORTEX_ESCAPE, "tutsil Vortex");
    ExpectEqInt(tutsil->equippedMoves[1], MOVE_YOUNG_BLOOD, "tutsil Young Blood");
    ExpectEqInt(tutsil->equippedMoves[2], MOVE_LONG_SHOT, "tutsil Long Shot at slot2");
    ExpectEqInt(tutsil->equippedMoves[3], MOVE_NONE, "tutsil reserved empty");
    longShotSlot = FindEquippedMoveSlotUsable(tutsil, MOVE_LONG_SHOT);
    ExpectEqInt(longShotSlot, 2, "AI usable-slot lookup finds Long Shot");

    ExpectEqInt(badger->equippedMoves[0], MOVE_HEADBUTT, "badger Headbutt");
    ExpectEqInt(badger->equippedMoves[1], MOVE_CHARGE, "badger Charge");
    ExpectEqInt(badger->equippedMoves[2], MOVE_STATIC_LOCK, "badger Static Lock");

    ExpectEqInt(
        BattleMember_GetBasicAttackElement(protag),
        BATTLE_ELEMENT_MIGHT,
        "protag basic Might");
    ExpectEqInt(
        BattleMember_GetBasicAttackElement(aksil),
        BATTLE_ELEMENT_FIRE,
        "aksil basic Fire");
    ExpectEqInt(DAMAGE_CALC_ATTACK_BASE_POWER, 20, "player attack base power preserved");
}

static void TestCustomDefaultsAndLooksGood(void)
{
    const CustomBattleFormationSlot *ally;
    const CustomBattleFormationSlot *enemy;
    int looksGood = BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT;

    ExpectEqInt(looksGood, 3, "LOOKS GOOD index is 3");
    ExpectTrue(
        looksGood == BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT,
        "LOOKS GOOD derived from usable limit");
    ExpectTrue(
        looksGood >= BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT,
        "LOOKS GOOD cannot index usable move array");

    CustomBattleSetup_ResetCurrentFormation();
    ExpectTrue(
        CustomBattleSetup_SetAllySlot(0, MEMBER_TEMPLATE_PROTAGONIST, 5),
        "set protag ally");
    ExpectTrue(
        CustomBattleSetup_SetAllySlot(1, MEMBER_TEMPLATE_AKSIL, 5),
        "set aksil ally");
    ExpectTrue(
        CustomBattleSetup_SetEnemySlot(0, MEMBER_TEMPLATE_TUTSIL, 5),
        "set tutsil enemy");
    ExpectTrue(
        CustomBattleSetup_SetEnemySlot(1, MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER, 5),
        "set badger enemy");

    ally = CustomBattleSetup_GetAllySlot(0);
    ExpectEqInt(ally->moveIds[0], MOVE_HOLD_BACK, "custom protag Hold Back");
    ExpectEqInt(ally->moveIds[1], MOVE_HAMMER_FIST, "custom protag Hammer Fist");
    ExpectEqInt(ally->moveIds[2], MOVE_INFLUENCE, "custom protag Influence");
    ExpectEqInt(ally->moveIds[3], MOVE_NONE, "custom protag reserved empty");

    ally = CustomBattleSetup_GetAllySlot(1);
    ExpectEqInt(ally->moveIds[0], MOVE_VORTEX_ESCAPE, "custom aksil Vortex");
    ExpectEqInt(ally->moveIds[1], MOVE_FAKIE, "custom aksil Fakie");
    ExpectEqInt(ally->moveIds[2], MOVE_ALLY_SWAP, "custom aksil Ally Swap");

    enemy = CustomBattleSetup_GetEnemySlot(0);
    ExpectEqInt(enemy->moveIds[2], MOVE_LONG_SHOT, "custom tutsil Long Shot");
    enemy = CustomBattleSetup_GetEnemySlot(1);
    ExpectEqInt(enemy->moveIds[2], MOVE_STATIC_LOCK, "custom badger Static Lock");

    ExpectTrue(MoveData_Get(MOVE_HAMMER_FIST) != 0, "Hammer Fist move definition exists");
    ExpectTrue(MoveData_Get(MOVE_DIRECT_ATTACK) != 0, "Direct Attack remains defined");
    ExpectTrue(MoveData_Get(MOVE_PHANTOM_PHIST) != 0, "Phantom Phist remains defined");

    ExpectTrue(CustomBattleSetup_SaveCurrentToSlot(0), "RAM save formation");
    CustomBattleSetup_ResetCurrentFormation();
    ExpectTrue(CustomBattleSetup_LoadSlotToCurrent(0), "RAM load formation");
    ally = CustomBattleSetup_GetAllySlot(0);
    ExpectEqInt(ally->moveIds[1], MOVE_HAMMER_FIST, "loaded Hammer Fist survives");
    ExpectEqInt(ally->moveIds[3], MOVE_NONE, "loaded reserved empty");

    {
        MoveId customMoves[BATTLE_MEMBER_MOVE_SLOT_COUNT] = {
            MOVE_DIRECT_ATTACK, MOVE_PHANTOM_PHIST, MOVE_RAM, MOVE_HOLD_BACK
        };
        ExpectTrue(
            CustomBattleSetup_SetAllySlotWithMoves(
                0, MEMBER_TEMPLATE_PROTAGONIST, 5, customMoves),
            "explicit custom moves");
        ally = CustomBattleSetup_GetAllySlot(0);
        ExpectEqInt(ally->moveIds[0], MOVE_DIRECT_ATTACK, "explicit config preserved");
        ExpectEqInt(ally->moveIds[1], MOVE_PHANTOM_PHIST, "sandbox Phantom preserved");
        ExpectEqInt(ally->moveIds[2], MOVE_RAM, "explicit slot2 preserved");
        ExpectEqInt(ally->moveIds[3], MOVE_NONE, "slot3 assignment rejected/cleared");
    }
}

static void TestElementResolutionPath(void)
{
    BattleMember protag;
    BattleMember aksil;
    BattleElementId protagEl;
    BattleElementId aksilEl;
    u16 fireVsHuman;
    u16 mightVsHuman;

    BattleMember_InitFromTemplate(
        &protag, MemberDatabase_Get(MEMBER_TEMPLATE_PROTAGONIST), 0, 0);
    BattleMember_InitFromTemplate(
        &aksil, MemberDatabase_Get(MEMBER_TEMPLATE_AKSIL), 1, 0);

    protagEl = BattleMember_GetBasicAttackElement(&protag);
    aksilEl = BattleMember_GetBasicAttackElement(&aksil);
    ExpectEqInt(protagEl, BATTLE_ELEMENT_MIGHT, "weapon path Might");
    ExpectEqInt(aksilEl, BATTLE_ELEMENT_FIRE, "weapon path Fire");

    fireVsHuman = ElementDatabase_GetDamageMultiplierPercent(
        BATTLE_ELEMENT_FIRE, BATTLE_RACE_HUMAN);
    mightVsHuman = ElementDatabase_GetDamageMultiplierPercent(
        BATTLE_ELEMENT_MIGHT, BATTLE_RACE_HUMAN);
    ExpectTrue(
        fireVsHuman == ElementDatabase_GetDamageMultiplierPercent(aksilEl, BATTLE_RACE_HUMAN),
        "aksil Fire reaches effectiveness table");
    ExpectTrue(
        mightVsHuman == ElementDatabase_GetDamageMultiplierPercent(protagEl, BATTLE_RACE_HUMAN),
        "protag Might reaches effectiveness table");
}

static void TestZeroToThreeUsable(void)
{
    BattleMember member;
    int i;

    BattleMember_Init(&member, "Zero", 100, 10, 0, 0);
    for (i = 0; i < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; i++)
    {
        ExpectTrue(!BattleMember_CanUseMoveSlot(&member, i), "zero skills unusable");
    }

    BattleMember_EquipMove(&member, 1, MOVE_HOLD_BACK, 3);
    ExpectTrue(BattleMember_CanUseMoveSlot(&member, 1), "one usable skill");
    ExpectEqInt(member.equippedMoves[3], MOVE_NONE, "reserved empty with sparse kit");
}

int main(void)
{
    TestCapacityAndBounds();
    TestSparseDuplicateAndSc();
    TestPetalburgKitsAndLongShot();
    TestCustomDefaultsAndLooksGood();
    TestElementResolutionPath();
    TestZeroToThreeUsable();

    if (g_failures)
    {
        printf("\n%d failure(s)\n", g_failures);
        return 1;
    }
    printf("\nAll safe three-slot migration host checks passed.\n");
    return 0;
}
