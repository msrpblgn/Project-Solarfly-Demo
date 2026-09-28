#ifndef BATTLE_SKILLS_PANEL_H
#define BATTLE_SKILLS_PANEL_H

#include "battle/battle_state.h"

/*
 * Production Skills panel ownership (ROM + host integration).
 * Used by Battle_Draw when phase is BATTLE_PHASE_PLAYER_SKILL and PANEL is dirty.
 */
void Battle_DrawPlayerSkillPanel(const BattleState *state);

#endif
