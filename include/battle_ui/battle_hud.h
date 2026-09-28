#ifndef BATTLE_HUD_H
#define BATTLE_HUD_H

#include "battle/battle_state.h"
#include "battle/battle_action.h"
#include "battle_ui/hud_layout.h"

void BattleHud_Init(void);
void BattleHud_SetBorderColor(BorderColor borderColor);
u16 BattleHud_GetBorderColorRgb(void);
void BattleHud_DrawCombatants(const BattleState *state, const BattleAction *action);
/* Mode-3 Emerald-style faint: restore/draw only KO dirty rects, not the full field. */
void BattleHud_DrawKoBurrowDirtyRects(
    const BattleState *state,
    const BattleAction *action,
    void (*restoreRect)(int x, int y, int w, int h));
void BattleHud_DrawPanelChrome(const BattleState *state);

#endif
