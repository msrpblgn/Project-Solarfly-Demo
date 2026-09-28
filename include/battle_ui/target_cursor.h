#ifndef TARGET_CURSOR_H
#define TARGET_CURSOR_H

#include "battle/battle_state.h"

void TargetCursor_Init(void);
void TargetCursor_Update(BattleState *state);
void TargetCursor_DrawSprite(const BattleState *state);
void TargetCursor_DrawPanel(const BattleState *state);

#endif
