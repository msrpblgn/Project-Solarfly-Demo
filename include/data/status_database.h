#ifndef STATUS_DATABASE_H
#define STATUS_DATABASE_H

#include "battle/battle_status.h"
#include "core/gba_types.h"

typedef enum StatusTrio {
    STATUS_TRIO_NONE = 0,
    STATUS_TRIO_PAIN,
    STATUS_TRIO_MATTER,
    STATUS_TRIO_INSTINCT,
    STATUS_TRIO_BALANCE
} StatusTrio;

const char *StatusDatabase_GetName(BattleStatusId status);
const char *StatusDatabase_GetShortName(BattleStatusId status);
const char *StatusDatabase_GetInflictPastTense(BattleStatusId status);
StatusTrio StatusDatabase_GetTrio(BattleStatusId status);
u16 StatusDatabase_GetIconFillColor(BattleStatusId status);
u16 StatusDatabase_GetIconBorderColor(BattleStatusId status);
int StatusDatabase_GetTickDamage(BattleStatusId status);
int StatusDatabase_IsValid(BattleStatusId status);

#endif
