#ifndef BATTLE_RESOLVE_H
#define BATTLE_RESOLVE_H

#include "battle/battle_state.h"

struct BattleAction;

int BattleResolve_Execute(BattleState *state, struct BattleAction *action);

#endif
