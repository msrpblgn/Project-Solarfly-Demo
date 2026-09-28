#ifndef BATTLE_LP_PROMPT_H
#define BATTLE_LP_PROMPT_H

#include "battle/battle_state.h"

struct BattleAction;

void BattleLpPrompt_Init(void);
void BattleLpPrompt_Update(BattleState *state, struct BattleAction *action);
void BattleLpPrompt_Draw(const BattleState *state, const struct BattleAction *action);

#endif
