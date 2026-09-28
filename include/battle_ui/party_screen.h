#ifndef PARTY_SCREEN_H
#define PARTY_SCREEN_H

#include "battle/battle_state.h"

void PartyScreen_Init(void);
void PartyScreen_Open(void);
int PartyScreen_Update(BattleState *state);
void PartyScreen_Draw(const BattleState *state);

#endif
