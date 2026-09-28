#ifndef ELEMENT_DATABASE_H
#define ELEMENT_DATABASE_H

#include "battle/battle_element.h"
#include "battle/battle_race.h"
#include "battle/battle_status.h"
#include "core/gba_types.h"

const char *ElementDatabase_GetName(BattleElementId element);
const char *ElementDatabase_GetShortName(BattleElementId element);
u16 ElementDatabase_GetIconFillColor(BattleElementId element);
u16 ElementDatabase_GetIconBorderColor(BattleElementId element);
ElementEffectiveness ElementDatabase_GetEffectiveness(
    BattleElementId attacker,
    BattleElementId defender);
ElementEffectiveness ElementDatabase_GetEffectivenessAgainstRace(
    BattleElementId element,
    BattleRaceId race);
int ElementDatabase_IsBalanceElement(BattleElementId element);
BattleStatusId ElementDatabase_GetBalanceStatusForElement(BattleElementId element);

typedef enum ElementStatusRule {
    ELEMENT_STATUS_RULE_NONE = 0,
    ELEMENT_STATUS_RULE_ALWAYS,
    ELEMENT_STATUS_RULE_CANNOT
} ElementStatusRule;

typedef struct ElementRaceInteraction {
    BattleStatusId statusId;
    ElementStatusRule statusRule;
    u16 damageMultiplierPercent;
} ElementRaceInteraction;
/*
 * Maps an attack element to the status used by generic damaging-hit logic.
 * Unmapped elements return BATTLE_STATUS_NONE.
 */
BattleStatusId ElementDatabase_GetStatusForElement(BattleElementId element);

ElementRaceInteraction ElementDatabase_GetInteraction(
    BattleElementId element,
    BattleRaceId race);

u16 ElementDatabase_GetDamageMultiplierPercent(
    BattleElementId element,
    BattleRaceId race);

int ElementDatabase_IsBalanceElementHighlyEffectiveAgainstRace(
    BattleElementId element,
    BattleRaceId race);
int ElementDatabase_IsValid(BattleElementId element);

#endif
