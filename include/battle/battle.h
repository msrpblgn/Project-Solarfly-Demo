#ifndef BATTLE_H
#define BATTLE_H

#include "battle/battle_move.h"
#include "battle/battle_item.h"
#include "battle/battle_state.h"
#include "battle/custom_battle_setup.h"
#include "battle/test_battle.h"
#include "battle_ui/battle_message_queue.h"

void Battle_Init(void);
void Battle_InitWithDemo(BattleDemoId demoId);
void Battle_InitWithDemoAndDifficulty(
    BattleDemoId demoId,
    TutsilDifficulty difficulty);
void Battle_InitWithDemoAndSettings(
    BattleDemoId demoId,
    TutsilDifficulty difficulty,
    TextSpeed textSpeed,
    BorderColor borderColor);
void Battle_InitWithCustomFormationAndSettings(
    const CustomBattleFormation *formation,
    TextSpeed textSpeed,
    BorderColor borderColor);
void Battle_Update(void);
void Battle_Draw(void);
void Battle_BeginSkillTargetSelect(BattleState *state, MoveId moveId, int moveSlot);
void Battle_BeginAttackTargetSelect(BattleState *state);
void Battle_BeginBraceResolve(BattleState *state);
void Battle_BeginPartyScreen(BattleState *state);
void Battle_BeginItemMenu(BattleState *state);
void Battle_BeginItemTargetSelect(BattleState *state, ItemId itemId);
void Battle_AttemptRun(BattleState *state);
void Battle_BeginResolveAction(BattleState *state);
int Battle_CanActivePlayerAct(const BattleState *state);
int Battle_IsFinished(void);
BattleDemoId Battle_GetActiveDemoId(void);

#endif
