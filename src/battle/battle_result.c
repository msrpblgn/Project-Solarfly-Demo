#include "battle/battle_result.h"
#include "battle_ui/battle_message_queue.h"

#define COMBAT_MESSAGE_MAX 48

static char s_combatMessage[COMBAT_MESSAGE_MAX];

static void CombatMessages_AppendChar(char *dest, int *cursor, char c)
{
    if (*cursor < COMBAT_MESSAGE_MAX - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void CombatMessages_AppendString(char *dest, int *cursor, const char *text)
{
    while (*text)
    {
        CombatMessages_AppendChar(dest, cursor, *text);
        text++;
    }
}

static void CombatMessages_AppendNumber(char *dest, int *cursor, int value)
{
    char digits[8];
    int count = 0;
    int temp = value;

    if (temp == 0)
    {
        CombatMessages_AppendChar(dest, cursor, '0');
        return;
    }
    while (temp > 0 && count < 8)
    {
        digits[count++] = (char)('0' + (temp % 10));
        temp /= 10;
    }
    while (count > 0)
    {
        CombatMessages_AppendChar(dest, cursor, digits[--count]);
    }
}

static void CombatMessages_EnqueueLpGained(const BattleMember *member, int amount)
{
    int cursor = 0;

    if (!member || amount <= 0)
    {
        return;
    }
    s_combatMessage[0] = '\0';
    CombatMessages_AppendString(s_combatMessage, &cursor, member->name);
    CombatMessages_AppendString(s_combatMessage, &cursor, " gained ");
    CombatMessages_AppendNumber(s_combatMessage, &cursor, amount);
    CombatMessages_AppendString(s_combatMessage, &cursor, " LP.");
    s_combatMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_combatMessage);
}

static void CombatMessages_EnqueueDamage(const BattleMember *target, int damage)
{
    int cursor = 0;

    s_combatMessage[0] = '\0';
    if (!target)
    {
        return;
    }
    CombatMessages_AppendString(s_combatMessage, &cursor, target->name);
    CombatMessages_AppendString(s_combatMessage, &cursor, " took ");
    CombatMessages_AppendNumber(s_combatMessage, &cursor, damage);
    CombatMessages_AppendString(s_combatMessage, &cursor, " damage.");
    s_combatMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_combatMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
}

void BattleResult_Clear(BattleResult *result)
{
    result->outcome = BATTLE_OUTCOME_NONE;
    result->turnsElapsed = 0;
}

void CombatResult_Clear(CombatResult *result)
{
    if (!result)
    {
        return;
    }
    result->hit = 0;
    result->dodged = 0;
    result->missed = 0;
    result->defenceNegated = 0;
    result->critical = 0;
    result->damage = 0;
    result->attackerLpGained = 0;
    result->defenderLpGained = 0;
}

void CombatResult_ApplyLpRewards(
    BattleMember *attacker,
    BattleMember *defender,
    CombatResult *result)
{
    if (!result)
    {
        return;
    }
    if (result->dodged || result->missed || result->defenceNegated)
    {
        if (defender)
        {
            result->defenderLpGained = BattleMember_GainLp(defender, 1);
        }
        return;
    }
    if (result->critical && attacker)
    {
        result->attackerLpGained = BattleMember_GainLp(attacker, 1);
    }
}

void CombatMessages_EnqueueOutcome(
    const BattleMember *attacker,
    const BattleMember *defender,
    int hadGuard,
    const CombatResult *result)
{
    int cursor = 0;

    (void)attacker;
    if (!defender || !result)
    {
        return;
    }
    if (result->dodged)
    {
        s_combatMessage[0] = '\0';
        cursor = 0;
        CombatMessages_AppendString(s_combatMessage, &cursor, defender->name);
        CombatMessages_AppendString(s_combatMessage, &cursor, " dodged the hit!");
        s_combatMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_combatMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        if (result->defenderLpGained > 0)
        {
            CombatMessages_EnqueueLpGained(defender, result->defenderLpGained);
        }
        return;
    }
    if (result->missed)
    {
        BattleMessageQueue_EnqueuePriority("The attack missed!", BATTLE_MESSAGE_PRIORITY_HIGH);
        return;
    }
    if (result->defenceNegated)
    {
        s_combatMessage[0] = '\0';
        cursor = 0;
        CombatMessages_AppendString(s_combatMessage, &cursor, defender->name);
        CombatMessages_AppendString(s_combatMessage, &cursor, " negated the hit.");
        s_combatMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_combatMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        if (result->defenderLpGained > 0)
        {
            CombatMessages_EnqueueLpGained(defender, result->defenderLpGained);
        }
        return;
    }
    if (hadGuard)
    {
        s_combatMessage[0] = '\0';
        cursor = 0;
        CombatMessages_AppendString(s_combatMessage, &cursor, defender->name);
        CombatMessages_AppendString(s_combatMessage, &cursor, " braced for the hit!");
        s_combatMessage[cursor] = '\0';
        BattleMessageQueue_Enqueue(s_combatMessage);
    }
    if (result->critical)
    {
        BattleMessageQueue_EnqueuePriority("Critical hit!", BATTLE_MESSAGE_PRIORITY_HIGH);
    }
    if (result->damage > 0)
    {
        CombatMessages_EnqueueDamage(defender, result->damage);
    }
    if (result->attackerLpGained > 0 && attacker)
    {
        CombatMessages_EnqueueLpGained(attacker, result->attackerLpGained);
    }
}
