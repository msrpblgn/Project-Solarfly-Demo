#ifndef RACE_DATABASE_H
#define RACE_DATABASE_H

#include "battle/battle_race.h"

const char *RaceDatabase_GetName(BattleRaceId race);
const char *RaceDatabase_GetShortName(BattleRaceId race);
int RaceDatabase_IsValid(BattleRaceId race);

#endif
