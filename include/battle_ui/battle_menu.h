#ifndef BATTLE_MENU_H
#define BATTLE_MENU_H

#include "battle/battle_state.h"

void BattleMenu_Init(void);
void BattleMenu_ClearSkillCardSlide(void);
int BattleMenu_IsSkillCardSlideActive(void);
void BattleMenu_Update(BattleState *state);
void BattleMenu_Draw(const BattleState *state);
void BattleMenu_DrawCommandSelectionChange(int oldIndex, int newIndex);

int BattleMenu_SkillCardSlideOffsetPx(int frame, int travelPx);

#endif
