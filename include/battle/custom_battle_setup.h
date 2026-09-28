#ifndef CUSTOM_BATTLE_SETUP_H
#define CUSTOM_BATTLE_SETUP_H

#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_state.h"
#include "data/member_database.h"

typedef struct CustomBattleFormationSlot {
    int occupied;
    MemberTemplateId memberTemplateId;
    int level;
    MoveId moveIds[BATTLE_MEMBER_MOVE_SLOT_COUNT];
    unsigned int flags;
} CustomBattleFormationSlot;

typedef struct CustomBattleFormation {
    CustomBattleFormationSlot allySlots[BATTLE_PLAYER_SLOT_COUNT];
    CustomBattleFormationSlot enemySlots[BATTLE_ENEMY_SLOT_COUNT];
} CustomBattleFormation;

#define CUSTOM_BATTLE_SAVE_SLOT_COUNT 3

int CustomBattleSetup_IsAllySlotOccupied(int slotIndex);
const CustomBattleFormationSlot *CustomBattleSetup_GetAllySlot(int slotIndex);
int CustomBattleSetup_SetAllySlot(
    int slotIndex,
    MemberTemplateId memberTemplateId,
    int level);
int CustomBattleSetup_SetAllySlotWithMoves(
    int slotIndex,
    MemberTemplateId memberTemplateId,
    int level,
    const MoveId *moveIds);
void CustomBattleSetup_ClearAllySlot(int slotIndex);
int CustomBattleSetup_IsEnemySlotOccupied(int slotIndex);
const CustomBattleFormationSlot *CustomBattleSetup_GetEnemySlot(int slotIndex);
int CustomBattleSetup_SetEnemySlot(
    int slotIndex,
    MemberTemplateId memberTemplateId,
    int level);
void CustomBattleSetup_ClearEnemySlot(int slotIndex);
int CustomBattleSetup_SaveCurrentToSlot(int saveSlot);
int CustomBattleSetup_LoadSlotToCurrent(int saveSlot);
int CustomBattleSetup_IsSaveSlotOccupied(int saveSlot);
void CustomBattleSetup_GetSaveSlotSummary(
    int saveSlot,
    int *outAllyCount,
    int *outEnemyCount);
void CustomBattleSetup_InitFormation(CustomBattleFormation *formation);
void CustomBattleSetup_ResetCurrentFormation(void);
CustomBattleFormation *CustomBattleSetup_GetCurrentFormation(void);
const CustomBattleFormation *CustomBattleSetup_GetCurrentFormationConst(void);
void CustomBattleSetup_ApplyFormationToState(
    BattleState *state,
    const CustomBattleFormation *formation);
const char *CustomBattleSetup_GetIntroMessage(void);

#endif
