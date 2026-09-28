#ifndef TEST_BATTLE_H
#define TEST_BATTLE_H

#include "battle/battle_state.h"

#define TEST_BATTLE_PROTAG_SLOT BATTLE_CENTER_SLOT
#define TEST_BATTLE_ALLY_SLOT 1

#define PETALBURG_ALLY_AKSIL_SLOT TEST_BATTLE_ALLY_SLOT
#define PETALBURG_ALLY_PROTAGONIST_SLOT TEST_BATTLE_PROTAG_SLOT
#define PETALBURG_ENEMY_BADGER_SLOT_LEFT 1
#define PETALBURG_ENEMY_TUTSIL_SLOT BATTLE_CENTER_SLOT
#define PETALBURG_ENEMY_BADGER_SLOT_RIGHT 3

/* Tutsil boss HP base (Hard = 100%). Normal/Easy scale this at fight start. */
#define PETALBURG_TUTSIL_BOSS_HP 400
#define PETALBURG_TUTSIL_HP_PERCENT_HARD 100
#define PETALBURG_TUTSIL_HP_PERCENT_NORMAL 70
#define PETALBURG_TUTSIL_HP_PERCENT_EASY 50
#define PETALBURG_ELECTRIC_ARMOR_BADGER_HP_EASY 160
#define PETALBURG_ELECTRIC_ARMOR_BADGER_HP 200
#define PETALBURG_ELECTRIC_ARMOR_BADGER_HP_HARD 240

typedef enum BattleDemoId {
    BATTLE_DEMO_PETALBURG_SHOWDOWN = 0,
    BATTLE_DEMO_DUMMY_TEST,
    BATTLE_DEMO_CUSTOM_BATTLE
} BattleDemoId;

typedef enum TutsilDifficulty {
    TUTSIL_DIFFICULTY_EASY = 0,
    TUTSIL_DIFFICULTY_NORMAL,
    TUTSIL_DIFFICULTY_HARD,
    TUTSIL_DIFFICULTY_COUNT
} TutsilDifficulty;

void TestBattle_SetupDummyTestBattle(BattleState *state);
void TestBattle_SetupPetalburgShowdown(BattleState *state, TutsilDifficulty difficulty);
void TestBattle_Setup(
    BattleState *state,
    BattleDemoId demoId,
    TutsilDifficulty difficulty);
const char *TestBattle_GetIntroMessage(BattleDemoId demoId);

#endif
