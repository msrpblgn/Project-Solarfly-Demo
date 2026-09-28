#include "battle/battle_resolve.h"
#include "battle/battle_action.h"
#include "battle/battle_draw.h"
#include "battle/battle_enemy_turn.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_result.h"
#include "battle/battle_slot.h"
#include "battle/battle_state.h"
#include "battle/battle_slot.h"
#include "battle/damage_calc.h"
#include "battle_ui/battle_message_queue.h"
#include "core/random.h"
#include "data/element_database.h"
#include "data/item_database.h"
#include "data/status_database.h"

#define RESOLVE_MESSAGE_MAX 48
#define SOLARFLY_DEBUG_EFFECT_REACTION_OVERRIDE 0
/* 31E: production effect executor active; legacy fallback when unsupported. */
#define SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS 1

typedef struct BattleResolveContext {
    BattleState *state;
    BattleAction *action;
    BattleMember *actor;
    BattleMember *target;
    const MoveData *move;
    int useLpBonus;
    int targetSide;
    int targetSlot;
    int hasLastCombatResult;
    CombatResult lastCombatResult;
    BattleMember *statModRollbackTarget;
    BattleStatId statModRollbackStats[MOVE_MAX_EFFECTS];
    int statModRollbackCount;
    int fanOutToAllEnemies;
    int useEffectiveAttackElement;
    BattleElementId effectiveAttackElement;
    int outgoingDamageMultiplierPercent;
} BattleResolveContext;

static char s_resolveMessage[RESOLVE_MESSAGE_MAX];

static void BattleResolve_AppendChar(char *dest, int *cursor, char c)
{
    if (*cursor < RESOLVE_MESSAGE_MAX - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void BattleResolve_AppendString(char *dest, int *cursor, const char *text)
{
    while (*text)
    {
        BattleResolve_AppendChar(dest, cursor, *text);
        text++;
    }
}

static BattleMember *BattleResolve_GetMember(BattleState *state, int side, int slot)
{
    if (side == BATTLE_SIDE_ENEMY)
    {
        if (slot < 0 || slot >= BATTLE_ENEMY_SLOT_COUNT)
        {
            return 0;
        }
        return &state->enemySlots[slot];
    }
    if (slot < 0 || slot >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return 0;
    }
    return &state->playerSlots[slot];
}

static int BattleResolve_GetOpponentSide(int actorSide)
{
    return actorSide == BATTLE_SIDE_PLAYER ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
}

static int BattleResolve_GetSideSlotCount(int side)
{
    if (side == BATTLE_SIDE_ENEMY)
    {
        return BATTLE_ENEMY_SLOT_COUNT;
    }
    return BATTLE_PLAYER_SLOT_COUNT;
}

static int BattleResolve_BuildFanOutSlotOrder(int selectedSlot, int sideSlotCount, int *outSlots)
{
    int count = 0;
    int distance;

    if (!outSlots || sideSlotCount <= 0)
    {
        return 0;
    }
    if (selectedSlot < 0)
    {
        selectedSlot = 0;
    }
    if (selectedSlot >= sideSlotCount)
    {
        selectedSlot = sideSlotCount - 1;
    }

    outSlots[count++] = selectedSlot;
    for (distance = 1; distance < sideSlotCount; distance++)
    {
        if (selectedSlot - distance >= 0)
        {
            outSlots[count++] = selectedSlot - distance;
        }
        if (selectedSlot + distance < sideSlotCount)
        {
            outSlots[count++] = selectedSlot + distance;
        }
    }
    return count;
}

static int BattleResolve_FindOuterEnemySlots(
    BattleState *state,
    int opponentSide,
    int *outLeftSlot,
    int *outRightSlot)
{
    int slot;
    int slotCount;
    int leftSlot = -1;
    int rightSlot = -1;

    if (!state || !outLeftSlot || !outRightSlot)
    {
        return 0;
    }

    slotCount = BattleResolve_GetSideSlotCount(opponentSide);
    for (slot = 0; slot < slotCount; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, opponentSide, slot))
        {
            continue;
        }
        if (leftSlot < 0)
        {
            leftSlot = slot;
        }
        rightSlot = slot;
    }

    *outLeftSlot = leftSlot;
    *outRightSlot = rightSlot;
    return leftSlot >= 0;
}

static void BattleResolve_AppendNumber(char *dest, int *cursor, int value)
{
    char digits[8];
    int count = 0;
    int temp = value;

    if (temp == 0)
    {
        BattleResolve_AppendChar(dest, cursor, '0');
        return;
    }
    while (temp > 0 && count < 8)
    {
        digits[count++] = (char)('0' + (temp % 10));
        temp /= 10;
    }
    while (count > 0)
    {
        BattleResolve_AppendChar(dest, cursor, digits[--count]);
    }
}

static void BattleResolve_EnqueueUsedMessage(const BattleMember *actor, const MoveData *move)
{
    int cursor = 0;

    s_resolveMessage[0] = '\0';
    if (!actor || !move)
    {
        return;
    }
    BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " used ");
    BattleResolve_AppendString(s_resolveMessage, &cursor, move->name);
    BattleResolve_AppendChar(s_resolveMessage, &cursor, '.');
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
}

static void BattleResolve_EnqueueAttackMessage(const BattleMember *actor)
{
    int cursor = 0;

    s_resolveMessage[0] = '\0';
    if (!actor)
    {
        return;
    }
    BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " attacks.");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
}

static void BattleResolve_TryApplySeededHeal(
    BattleMember *attacker,
    const BattleMember *target,
    int actualDamage)
{
    int healAmount;
    int actualHealed;
    int cursor = 0;

    if (!attacker || !target || actualDamage <= 0)
    {
        return;
    }
    if (!BattleMember_HasStatus(target, BATTLE_STATUS_SEEDED))
    {
        return;
    }
    if (!attacker->isAlive || attacker->currentHp <= 0)
    {
        return;
    }
    healAmount = actualDamage / 4;
    if (healAmount <= 0)
    {
        healAmount = 1;
    }
    actualHealed = BattleMember_Heal(attacker, healAmount);
    if (actualHealed <= 0)
    {
        return;
    }
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, attacker->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " drained ");
    BattleResolve_AppendNumber(s_resolveMessage, &cursor, actualHealed);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " HP!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
}

static int BattleResolve_TryMatterStatusBeforeAction(BattleMember *actor)
{
    int cursor = 0;
    int selfDamage;

    if (!actor)
    {
        return 0;
    }
    if (BattleMember_ShouldLoseActionToParalysis(actor))
    {
        s_resolveMessage[0] = '\0';
        BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, " is paralyzed!");
        s_resolveMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        return 1;
    }
    if (BattleMember_ShouldConfusedSelfHit(actor))
    {
        selfDamage = BattleMember_ComputeConfusedSelfDamage(actor);
        s_resolveMessage[0] = '\0';
        cursor = 0;
        BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, " hurt self in confusion!");
        s_resolveMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        if (selfDamage > 0)
        {
            BattleMember_TakeDamage(actor, selfDamage);
        }
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
        return 1;
    }
    return 0;
}

static int BattleResolve_TryInstinctStatusBeforeAction(
    BattleState *state,
    BattleMember *actor)
{
    int cursor = 0;
    int targetSlot;
    BattleMember *panicTarget;
    int damage;
    int hitSelf;

    if (!state || !actor || !actor->isAlive)
    {
        return 0;
    }
    if (!BattleMember_ShouldConfused2HitAlly(actor))
    {
        return 0;
    }
    targetSlot = BattleState_FindRandomAliveAllySlot(state, actor->side, actor->slotIndex, 1);
    if (targetSlot < 0)
    {
        return 0;
    }
    panicTarget = BattleResolve_GetMember(state, actor->side, targetSlot);
    if (!panicTarget)
    {
        return 0;
    }
    damage = BattleMember_ComputeConfusedSelfDamage(actor);
    hitSelf = targetSlot == actor->slotIndex;
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
    if (hitSelf)
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, " lashed out in confusion!");
    }
    else
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, " struck an ally in panic!");
    }
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    if (damage > 0)
    {
        BattleMember_TakeDamage(panicTarget, damage);
    }
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    return 1;
}

static void BattleResolve_ApplyActorTurnRecovery(BattleState *state, BattleMember *actor)
{
    BattleStatusId recovered;
    int cursor = 0;

    (void)state;
    if (!actor)
    {
        return;
    }
    recovered = BattleMember_TryRecoverAfterActorTurn(actor);
    if (recovered == BATTLE_STATUS_NONE)
    {
        return;
    }
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
    if (recovered == BATTLE_STATUS_FROZEN)
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, " thawed out!");
    }
    else
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, " stabilized!");
    }
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL);
}

static void BattleResolve_EnqueueDistanceBoostMessage(
    const BattleMember *attacker,
    const BattleMember *target,
    const MoveData *move)
{
    int distance;

    if (!MoveData_HasDistanceScale(move) || !attacker || !target)
    {
        return;
    }
    distance = BattleSlot_GetDistance(
        attacker->side,
        attacker->slotIndex,
        target->side,
        target->slotIndex);
    if (distance > 0)
    {
        BattleMessageQueue_EnqueuePriority("Distance boosted the hit.", BATTLE_MESSAGE_PRIORITY_LOW);
    }
}

static BattleEffectivenessReaction BattleResolve_MapEffectiveness(ElementEffectiveness effectiveness)
{
    switch (effectiveness)
    {
    case ELEMENT_EFFECT_HIGHLY_EFFECTIVE:
        return BATTLE_EFFECT_REACTION_HIGHLY_EFFECTIVE;
    case ELEMENT_EFFECT_CANNOT_INFLICT:
        return BATTLE_EFFECT_REACTION_CANNOT_INFLICT;
    case ELEMENT_EFFECT_NEUTRAL:
    default:
        return BATTLE_EFFECT_REACTION_NEUTRAL;
    }
}

#if SOLARFLY_DEBUG_EFFECT_REACTION_OVERRIDE
/*
 * Debug-only visual validation hook.
 * Does not change damage, hit logic, messages, or status infliction.
 */
static BattleEffectivenessReaction BattleResolve_GetDebugReactionOverride(
    MoveId moveId,
    BattleEffectivenessReaction normalReaction)
{
    (void)normalReaction;

    switch (moveId)
    {
    case MOVE_LUCKY_SWING_TEST:
        return BATTLE_EFFECT_REACTION_HIGHLY_EFFECTIVE;
    case MOVE_LONG_SHOT_TEST:
        return BATTLE_EFFECT_REACTION_CANNOT_INFLICT;
    default:
        return normalReaction;
    }
}
#endif

static void BattleResolve_StoreEnemyEffectReaction(
    BattleState *state,
    int targetSide,
    int targetSlot,
    const MoveData *move,
    BattleElementId attackElement,
    const BattleMember *defender)
{
    ElementEffectiveness effectiveness;
    BattleEffectivenessReaction reaction;

    if (!state || targetSide != BATTLE_SIDE_ENEMY || !defender)
    {
        return;
    }
    if (targetSlot < 0 || targetSlot >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return;
    }
    if (attackElement == BATTLE_ELEMENT_NONE)
    {
        BattleState_SetEnemyEffectReaction(state, targetSlot, BATTLE_EFFECT_REACTION_NONE);
        return;
    }

    effectiveness = ElementDatabase_GetEffectivenessAgainstRace(
        attackElement,
        BattleMember_GetRace(defender));
    reaction = BattleResolve_MapEffectiveness(effectiveness);
#if SOLARFLY_DEBUG_EFFECT_REACTION_OVERRIDE
    if (move)
    {
        reaction = BattleResolve_GetDebugReactionOverride(move->id, reaction);
    }
#else
    (void)move;
#endif
    BattleState_SetEnemyEffectReaction(state, targetSlot, reaction);
}

static int BattleResolve_CanActivateBalanceStatus(const BattleMember *target, BattleStatusId status)
{
    if (status != BATTLE_STATUS_DARK
        && status != BATTLE_STATUS_LIGHT
        && status != BATTLE_STATUS_CHAOS)
    {
        return 1;
    }
    if (!target)
    {
        return 0;
    }
    return BattleMember_HasAllPrimaryStatusTrios(target);
}

static int BattleResolve_TryApplyStatusDirect(BattleMember *target, BattleStatusId status)
{
    int cursor = 0;

    if (!target || !target->isAlive || status == BATTLE_STATUS_NONE)
    {
        return 0;
    }
    if (BattleMember_HasStatus(target, status))
    {
        return 0;
    }
    if (!BattleResolve_CanActivateBalanceStatus(target, status))
    {
        BattleMessageQueue_Enqueue("Needs Pain/Matter/Instinct.");
        return 0;
    }
    if (!BattleMember_AddStatus(target, status))
    {
        return 0;
    }
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " was ");
    BattleResolve_AppendString(s_resolveMessage, &cursor, StatusDatabase_GetInflictPastTense(status));
    BattleResolve_AppendChar(s_resolveMessage, &cursor, '!');
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    return 1;
}

static void BattleResolve_EnqueueDebugTrioSelection(const char *trioLabel, BattleStatusId status)
{
    int cursor = 0;

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, trioLabel);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " cycle: ");
    BattleResolve_AppendString(s_resolveMessage, &cursor, StatusDatabase_GetName(status));
    BattleResolve_AppendChar(s_resolveMessage, &cursor, '.');
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_LOW);
}

static void BattleResolve_ApplyDebugStatusCycle(BattleMember *target, MoveId moveId)
{
    static const BattleStatusId s_MatterStatuses[] = {
        BATTLE_STATUS_PARALYSIS,
        BATTLE_STATUS_FROZEN,
        BATTLE_STATUS_CONFUSED
    };
    static const BattleStatusId s_InstinctStatuses[] = {
        BATTLE_STATUS_SEEDED,
        BATTLE_STATUS_CONFUSED2,
        BATTLE_STATUS_DOWNED
    };
    static const BattleStatusId s_BalanceStatuses[] = {
        BATTLE_STATUS_DARK,
        BATTLE_STATUS_LIGHT,
        BATTLE_STATUS_CHAOS
    };
    static int s_MatterCycleIndex;
    static int s_InstinctCycleIndex;
    static int s_BalanceCycleIndex;

    const BattleStatusId *statuses = 0;
    int *cycleIndex = 0;
    const char *trioLabel = 0;
    int count = 3;
    BattleStatusId status;

    if (!target)
    {
        return;
    }
    if (moveId == MOVE_MATTER_STATUS_TEST)
    {
        statuses = s_MatterStatuses;
        cycleIndex = &s_MatterCycleIndex;
        trioLabel = "Matter";
    }
    else if (moveId == MOVE_INSTINCT_STATUS_TEST)
    {
        statuses = s_InstinctStatuses;
        cycleIndex = &s_InstinctCycleIndex;
        trioLabel = "Instinct";
    }
    else if (moveId == MOVE_BALANCE_STATUS_TEST)
    {
        statuses = s_BalanceStatuses;
        cycleIndex = &s_BalanceCycleIndex;
        trioLabel = "Balance";
    }
    else
    {
        return;
    }

    status = statuses[*cycleIndex % count];
    *cycleIndex = (*cycleIndex + 1) % count;
    BattleResolve_EnqueueDebugTrioSelection(trioLabel, status);
    BattleResolve_TryApplyStatusDirect(target, status);
}

static void BattleResolve_ApplyDebugClearStatus(BattleMember *target)
{
    int cursor = 0;

    if (!target)
    {
        return;
    }
    if (!BattleMember_HasAnyStatus(target))
    {
        s_resolveMessage[0] = '\0';
        BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, " has no status.");
        s_resolveMessage[cursor] = '\0';
        BattleMessageQueue_Enqueue(s_resolveMessage);
        return;
    }
    BattleMember_ClearStatuses(target);
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " statuses cleared.");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
}

static void BattleResolve_TryInflictStatusWithParams(
    BattleMember *target,
    const MoveData *move,
    BattleStatusId status,
    int inflictChancePercent,
    BattleElementId attackElement,
    const CombatResult *result)
{
    ElementRaceInteraction interaction;
    int targetIsDowned;
    int cursor = 0;

    if (!target || !move || !result || !result->hit)
    {
        return;
    }
    if (status == BATTLE_STATUS_NONE || inflictChancePercent <= 0)
    {
        return;
    }
    if (!target->isAlive)
    {
        return;
    }
    targetIsDowned = BattleMember_IsDowned(target);
    interaction = ElementDatabase_GetInteraction(
        attackElement,
        BattleMember_GetRace(target));
    if (interaction.statusRule == ELEMENT_STATUS_RULE_CANNOT && !targetIsDowned)
    {
        s_resolveMessage[0] = '\0';
        cursor = 0;
        BattleResolve_AppendString(s_resolveMessage, &cursor, "No effect on ");
        BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
        BattleResolve_AppendChar(s_resolveMessage, &cursor, '.');
        s_resolveMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        return;
    }
    if (interaction.statusRule == ELEMENT_STATUS_RULE_NONE)
    {
        /* 1.5 dmg rows are damage-only: no status infliction from generic effectiveness. */
        if (interaction.damageMultiplierPercent != 100)
        {
            return;
        }
        if (!result->critical && !targetIsDowned)
        {
            return;
        }
    }
    if (BattleMember_HasStatus(target, status))
    {
        return;
    }
    if (interaction.statusRule == ELEMENT_STATUS_RULE_ALWAYS)
    {
        BattleMessageQueue_Enqueue("Highly effective!");
    }
    if (inflictChancePercent < 100 && !Random_Percent(inflictChancePercent))
    {
        return;
    }
    if (!BattleResolve_CanActivateBalanceStatus(target, status))
    {
        return;
    }
    if (!BattleMember_AddStatus(target, status))
    {
        return;
    }
    s_resolveMessage[0] = '\0';
    cursor = 0;
    BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " was ");
    BattleResolve_AppendString(s_resolveMessage, &cursor, StatusDatabase_GetInflictPastTense(status));
    BattleResolve_AppendChar(s_resolveMessage, &cursor, '!');
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
}

static void BattleResolve_ExecuteSkillLegacy(BattleState *state, BattleAction *action)
{
    BattleMember *actor;
    const MoveData *move;

    move = MoveData_Get(action->moveId);
    actor = BattleResolve_GetMember(state, action->actorSide, action->actorSlot);
    if (!move || !actor)
    {
        return;
    }

    BattleResolve_EnqueueUsedMessage(actor, move);
#if SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS
    BattleMessageQueue_Enqueue("The move had no effect.");
#else
    BattleMessageQueue_Enqueue("Move effects are unavailable.");
#endif
}

static BattleElementId BattleResolve_GetResolvedAttackElement(
    const MoveData *move,
    BattleElementId effectiveAttackElement,
    int useEffectiveAttackElement)
{
    if (useEffectiveAttackElement)
    {
        return effectiveAttackElement;
    }
    if (move)
    {
        return move->element;
    }
    return BATTLE_ELEMENT_NONE;
}

/*
 * Generic rule: normal non-balance elements keep their prior behavior unless
 * the target is Downed. Downed targets become broadly susceptible to mapped
 * element statuses unless the race table says CANNOT.
 */
static void BattleResolve_TryApplyGenericElementStatusFromDamage(
    BattleMember *target,
    BattleElementId attackElement,
    const CombatResult *result)
{
    ElementRaceInteraction interaction;
    BattleStatusId status;
    int targetIsDowned;
    int cursor = 0;

    if (!target || !result || !result->hit || !target->isAlive)
    {
        return;
    }
    if (attackElement == BATTLE_ELEMENT_NONE)
    {
        return;
    }

    if (ElementDatabase_IsBalanceElement(attackElement))
    {
        return;
    }
    interaction = ElementDatabase_GetInteraction(
        attackElement,
        BattleMember_GetRace(target));
    status = ElementDatabase_GetStatusForElement(attackElement);
    if (status == BATTLE_STATUS_NONE)
    {
        return;
    }
    if (interaction.statusRule == ELEMENT_STATUS_RULE_CANNOT)
    {
        return;
    }
    targetIsDowned = BattleMember_IsDowned(target);
    if (!targetIsDowned && interaction.statusRule == ELEMENT_STATUS_RULE_NONE)
    {
        /* 1.5x dmg rows stay damage-only; neutral rows still need a crit. */
        if (interaction.damageMultiplierPercent != 100 || !result->critical)
        {
            return;
        }
    }
    if (BattleMember_HasStatus(target, status))
    {
        return;
    }
    if (!BattleResolve_CanActivateBalanceStatus(target, status))
    {
        return;
    }
    if (interaction.statusRule == ELEMENT_STATUS_RULE_ALWAYS)
    {
        BattleMessageQueue_Enqueue("Highly effective!");
    }
    if (!BattleMember_AddStatus(target, status))
    {
        return;
    }

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " was ");
    BattleResolve_AppendString(
        s_resolveMessage,
        &cursor,
        StatusDatabase_GetInflictPastTense(status));
    BattleResolve_AppendChar(s_resolveMessage, &cursor, '!');
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
}

static int BattleResolve_ActionQualifiesAsJavelinWeaponAttack(
    const BattleMember *actor,
    const BattleAction *action,
    const MoveData *move)
{
    (void)move;
    if (!actor || !action)
    {
        return 0;
    }
    if (!BattleMember_IsMaren(actor) || actor->weaponKind != MEMBER_WEAPON_KIND_JAVELIN)
    {
        return 0;
    }
    if (action->type == BATTLE_ACTION_ATTACK)
    {
        return 1;
    }
    if (action->type == BATTLE_ACTION_SKILL && action->moveId == MOVE_WIDE_THROW)
    {
        return 1;
    }
    return 0;
}

static void BattleResolve_TryApplyJavelinPassiveSlow(
    BattleMember *attacker,
    BattleMember *target,
    const BattleAction *action,
    const MoveData *move)
{
    if (!attacker || !target || !target->isAlive)
    {
        return;
    }
    if (attacker->side == target->side)
    {
        return;
    }
    if (!BattleResolve_ActionQualifiesAsJavelinWeaponAttack(attacker, action, move))
    {
        return;
    }
    if (BattleMember_HasJavelinSlow(target))
    {
        return;
    }
    if (!BattleMember_ApplyStatMod(
            target,
            BATTLE_STAT_SPEED,
            BATTLE_JAVELIN_SLOW_PERCENT,
            1,
            BATTLE_STAT_MOD_DURATION_BATTLE_LONG))
    {
        return;
    }
    BattleMember_SetJavelinSlow(target, 1);
    BattleMessageQueue_Enqueue("Javelin slowed the foe!");
}

static void BattleResolve_ApplyCombatDamage(
    BattleState *state,
    int targetSide,
    int targetSlot,
    BattleMember *attacker,
    BattleMember *target,
    const MoveData *move,
    int basePower,
    int useLpBonus,
    int hitCritOverridePermille,
    BattleElementId effectiveAttackElement,
    int useEffectiveAttackElement,
    int outgoingDamageMultiplierPercent,
    const BattleAction *sourceAction,
    CombatResult *outResult)
{
    CombatResult result;
    int hadGuard;
    int actualDamage = 0;
    BattleElementId attackElement;
    int bypassAccuracyMiss = 0;
    int veilCritBonusPermille = 0;

    if (outResult)
    {
        CombatResult_Clear(outResult);
    }
    hadGuard = BattleMember_HasGuardNextHit(target);

    /* Team Focus: one-shot ally accuracy lock. Consume when this hit rolls accuracy. */
    if (state &&
        attacker &&
        attacker->side == BATTLE_SIDE_PLAYER &&
        move &&
        move->accuracyPercent > 0 &&
        move->accuracyPercent < 100 &&
        BattleState_TryConsumeAllyNextMoveCannotMiss(state))
    {
        bypassAccuracyMiss = 1;
    }

    veilCritBonusPermille = 0;
    if (state && attacker)
    {
        veilCritBonusPermille = BattleState_GetVeilCritBonusPermille(
            state,
            attacker->side,
            attacker->slotIndex,
            targetSide,
            targetSlot);
    }

    DamageCalc_Resolve(
        attacker,
        target,
        move,
        basePower,
        useLpBonus,
        hitCritOverridePermille,
        bypassAccuracyMiss,
        veilCritBonusPermille,
        &result);
    attackElement = BattleResolve_GetResolvedAttackElement(
        move,
        effectiveAttackElement,
        useEffectiveAttackElement);

    /* Apply 1.5x damage (and any other percent multipliers) from the
     * element-vs-race source table, using the runtime resolved element. */
    {
        u16 damageMultiplierPercent = ElementDatabase_GetDamageMultiplierPercent(
            attackElement,
            BattleMember_GetRace(target));
        if (result.damage > 0 && damageMultiplierPercent != 100)
        {
            result.damage = (result.damage * damageMultiplierPercent) / 100;
        }
    }

    /* Immense Support / similar: multiply after element, before Bubble. */
    if (outgoingDamageMultiplierPercent <= 0)
    {
        outgoingDamageMultiplierPercent = 100;
    }
    if (result.damage > 0 && outgoingDamageMultiplierPercent != 100)
    {
        result.damage = (result.damage * outgoingDamageMultiplierPercent) / 100;
    }

    BattleResolve_StoreEnemyEffectReaction(
        state,
        targetSide,
        targetSlot,
        move,
        attackElement,
        target);
    CombatResult_ApplyLpRewards(attacker, target, &result);
    if (result.damage > 0)
    {
        if (BattleMember_ApplyBubbleDamageConversion(target, result.damage))
        {
            BattleMessageQueue_Enqueue("Bubble converted damage!");
            result.damage = 0;
        }
        else
        {
            int hpBefore = target->currentHp;

            BattleMember_TakeDamage(target, result.damage);
            actualDamage = hpBefore - target->currentHp;
            if (actualDamage < 0)
            {
                actualDamage = 0;
            }
            if (BattleMember_IsKoBurrowing(target) || BattleMember_IsRemovedFromField(target))
            {
                BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
            }
        }
    }
    CombatMessages_EnqueueOutcome(attacker, target, hadGuard, &result);
    BattleResolve_TryApplySeededHeal(attacker, target, actualDamage);
    if (actualDamage > 0)
    {
        /*
         * Phantom Phist uses a dedicated main-hit crit Curse path in
         * ExecuteDamageEffect — skip generic element status to avoid
         * non-crit / splash-adjacent application.
         */
        if (!(move && move->id == MOVE_PHANTOM_PHIST))
        {
            BattleResolve_TryApplyGenericElementStatusFromDamage(
                target,
                attackElement,
                &result);
        }
        BattleResolve_TryApplyJavelinPassiveSlow(
            attacker,
            target,
            sourceAction,
            move);
    }
    if (outResult)
    {
        *outResult = result;
    }
}

static void BattleResolve_ApplyCombat(
    BattleState *state,
    int targetSide,
    int targetSlot,
    BattleMember *attacker,
    BattleMember *target,
    const MoveData *move,
    int basePower,
    int useLpBonus,
    BattleElementId effectiveAttackElement,
    int useEffectiveAttackElement,
    int outgoingDamageMultiplierPercent,
    const BattleAction *sourceAction)
{
    BattleResolve_ApplyCombatDamage(
        state,
        targetSide,
        targetSlot,
        attacker,
        target,
        move,
        basePower,
        useLpBonus,
        DAMAGE_CALC_HIT_CRIT_DEFAULT,
        effectiveAttackElement,
        useEffectiveAttackElement,
        outgoingDamageMultiplierPercent,
        sourceAction,
        0);
}

static int BattleResolve_VertigoWaveGoesLeftToRight(int anchorSlot, int sideSlotCount)
{
    if (sideSlotCount <= 0)
    {
        return 1;
    }
    if (anchorSlot < sideSlotCount / 2)
    {
        return 1;
    }
    if (anchorSlot > sideSlotCount / 2)
    {
        return 0;
    }
    return 1;
}

static int BattleResolve_ExecuteVertigoWaveDamageEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int opponentSide;
    int sideSlotCount;
    int leftToRight;
    int livingSlots[BATTLE_ENEMY_SLOT_COUNT];
    int livingCount = 0;
    int slot;
    int i;
    const int *wavePowers;
    static const int s_wavePowersNormal[] = { 35, 30, 25, 20, 15 };
    static const int s_wavePowersLp[] = { 50, 40, 30, 20, 10 };

    (void)effect;
    if (!ctx || !ctx->state || !ctx->actor || !ctx->move || !ctx->action)
    {
        return 0;
    }

    wavePowers = (ctx->useLpBonus && ctx->move->optionalLpCost > 0)
        ? s_wavePowersLp
        : s_wavePowersNormal;

    opponentSide = ctx->targetSide;
    sideSlotCount = BattleResolve_GetSideSlotCount(opponentSide);
    leftToRight = BattleResolve_VertigoWaveGoesLeftToRight(ctx->targetSlot, sideSlotCount);

    if (leftToRight)
    {
        for (slot = 0; slot < sideSlotCount; slot++)
        {
            if (BattleState_IsAliveOccupiedSlot(ctx->state, opponentSide, slot))
            {
                livingSlots[livingCount++] = slot;
            }
        }
    }
    else
    {
        for (slot = sideSlotCount - 1; slot >= 0; slot--)
        {
            if (BattleState_IsAliveOccupiedSlot(ctx->state, opponentSide, slot))
            {
                livingSlots[livingCount++] = slot;
            }
        }
    }

    if (livingCount <= 0)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    for (i = 0; i < livingCount; i++)
    {
        BattleMember *waveTarget;
        int basePower;
        int hitCritOverridePermille = DAMAGE_CALC_HIT_CRIT_DEFAULT;

        slot = livingSlots[i];
        waveTarget = BattleResolve_GetMember(ctx->state, opponentSide, slot);
        if (!waveTarget)
        {
            continue;
        }

        basePower = wavePowers[i];
        if (basePower <= 0)
        {
            basePower = wavePowers[4];
        }
        if (i >= livingCount - 2)
        {
            hitCritOverridePermille = 0;
        }

        BattleResolve_ApplyCombatDamage(
            ctx->state,
            opponentSide,
            slot,
            ctx->actor,
            waveTarget,
            ctx->move,
            basePower,
            ctx->useLpBonus,
            hitCritOverridePermille,
            ctx->effectiveAttackElement,
            ctx->useEffectiveAttackElement,
            ctx->outgoingDamageMultiplierPercent,
            ctx->action,
            &ctx->lastCombatResult);
        ctx->hasLastCombatResult = 1;
    }
    return 1;
}

#if SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS

static int BattleResolve_MoveHasProductionEffects(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind != MOVE_EFFECT_KIND_NONE)
        {
            return 1;
        }
    }
    return 0;
}

static int BattleResolve_IsProductionEffectKindSupported(MoveEffectKind kind)
{
    switch (kind)
    {
    case MOVE_EFFECT_KIND_NONE:
    case MOVE_EFFECT_KIND_DAMAGE:
    case MOVE_EFFECT_KIND_INFLICT_STATUS:
    case MOVE_EFFECT_KIND_CLEAR_STATUS:
    case MOVE_EFFECT_KIND_MOVE_TO_SLOT:
    case MOVE_EFFECT_KIND_MOVEMENT_LOCK:
    case MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE:
    case MOVE_EFFECT_KIND_BONUS_ACTION:
    case MOVE_EFFECT_KIND_INCOMING_DAMAGE_REDUCTION:
    case MOVE_EFFECT_KIND_PENDING_ATTACK_ALL_ENEMIES:
    case MOVE_EFFECT_KIND_PENDING_WEAPON_ATTACK_ELEMENT:
    case MOVE_EFFECT_KIND_PENDING_NEXT_MOVE_ELEMENT:
    case MOVE_EFFECT_KIND_EXTRA_ACTION_NEXT_TURN:
    case MOVE_EFFECT_KIND_ENCORE:
    case MOVE_EFFECT_KIND_DISTANCE_SCALE:
    case MOVE_EFFECT_KIND_VERTIGO_WAVE_DAMAGE:
    case MOVE_EFFECT_KIND_BUBBLE_PROTECTION:
    case MOVE_EFFECT_KIND_VEIL_LUCK_BUFF:
    case MOVE_EFFECT_KIND_SWAP_WITH_ALLY:
    case MOVE_EFFECT_KIND_TEAM_FOCUS:
    case MOVE_EFFECT_KIND_LAST_CHANCE:
    case MOVE_EFFECT_KIND_ON_DAMAGE_SELF_STAT_MOD:
    case MOVE_EFFECT_KIND_PENDING_NEXT_DAMAGE_DOUBLE:
    case MOVE_EFFECT_KIND_AUTO_PUSH_ON_DAMAGE:
    case MOVE_EFFECT_KIND_DEBUG_STATUS_CYCLE:
    case MOVE_EFFECT_KIND_STAT_MODIFIER:
        return 1;
    default:
        return 0;
    }
}

static int BattleResolve_IsProductionSkillPathSupported(const MoveData *move)
{
    int i;

    if (!BattleResolve_MoveHasProductionEffects(move))
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (!BattleResolve_IsProductionEffectKindSupported(move->effects[i].kind))
        {
            return 0;
        }
    }
    return 1;
}

static int BattleResolve_MoveHasDamageEffect(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_DAMAGE)
        {
            return 1;
        }
    }
    return 0;
}

static int BattleResolve_MoveQualifiesForPendingAllEnemiesAttack(const MoveData *move)
{
    if (!move)
    {
        return 0;
    }
    if (move->category != MOVE_CATEGORY_PHYSICAL && move->category != MOVE_CATEGORY_SPECIAL)
    {
        return 0;
    }
    return BattleResolve_MoveHasDamageEffect(move);
}

static int BattleResolve_TryConsumePendingAllEnemiesAttack(BattleMember *actor, int qualifies)
{
    if (!qualifies || !actor)
    {
        return 0;
    }
    if (!BattleMember_HasPendingAttackHitsAllEnemies(actor))
    {
        return 0;
    }
    BattleMember_ClearPendingAttackHitsAllEnemies(actor);
    BattleMessageQueue_Enqueue("The attack hits everyone!");
    return 1;
}

static int BattleResolve_MoveQualifiesForPendingNextDamageDouble(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        switch (move->effects[i].kind)
        {
        case MOVE_EFFECT_KIND_DAMAGE:
        case MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE:
        case MOVE_EFFECT_KIND_VERTIGO_WAVE_DAMAGE:
            return 1;
        default:
            break;
        }
    }
    return 0;
}

static int BattleResolve_TryConsumePendingNextDamageDouble(BattleMember *actor, int qualifies)
{
    if (!qualifies || !actor)
    {
        return 0;
    }
    if (!BattleMember_HasPendingNextDamageDouble(actor))
    {
        return 0;
    }
    BattleMember_ClearPendingNextDamageDouble(actor);
    BattleMessageQueue_Enqueue("Damage doubled!");
    return 1;
}

static int BattleResolve_ActionQualifiesForPendingWeaponAttack(
    const BattleMember *actor,
    const BattleAction *action,
    const MoveData *move)
{
    if (!actor || !action)
    {
        return 0;
    }
    if (!BattleMember_IsAksil(actor) || actor->weaponKind != MEMBER_WEAPON_KIND_SLINGSHOT)
    {
        return 0;
    }
    if (action->type == BATTLE_ACTION_ATTACK)
    {
        return 1;
    }
    if (action->type != BATTLE_ACTION_SKILL || !move)
    {
        return 0;
    }
    if (action->moveId != MOVE_SNIPE_DOWN)
    {
        return 0;
    }
    return move->weaponKind == MOVE_WEAPON_KIND_SLINGSHOT
        && (move->category == MOVE_CATEGORY_PHYSICAL
            || move->category == MOVE_CATEGORY_SPECIAL)
        && BattleResolve_MoveHasDamageEffect(move);
}

static int BattleResolve_ActionQualifiesForPendingNextMoveElement(
    const BattleAction *action,
    const MoveData *move)
{
    if (!action)
    {
        return 0;
    }
    if (action->type == BATTLE_ACTION_ATTACK)
    {
        return 1;
    }
    if (action->type != BATTLE_ACTION_SKILL || !move)
    {
        return 0;
    }
    if (move->category != MOVE_CATEGORY_PHYSICAL && move->category != MOVE_CATEGORY_SPECIAL)
    {
        return 0;
    }
    return BattleResolve_MoveHasDamageEffect(move);
}

static int BattleResolve_TryConsumePendingWeaponAttackElement(
    BattleMember *actor,
    const BattleAction *action,
    const MoveData *move,
    BattleElementId *outElement)
{
    BattleElementId pending;

    if (!actor || !BattleMember_HasPendingWeaponAttackElement(actor))
    {
        return 0;
    }
    if (!BattleResolve_ActionQualifiesForPendingWeaponAttack(actor, action, move))
    {
        return 0;
    }
    pending = BattleMember_GetPendingWeaponAttackElement(actor);
    BattleMember_ClearPendingWeaponAttackElement(actor);
    if (outElement)
    {
        *outElement = pending;
    }
    return 1;
}

static int BattleResolve_TryConsumePendingNextMoveElement(
    BattleMember *actor,
    const BattleAction *action,
    const MoveData *move,
    BattleElementId *outElement)
{
    BattleElementId pending;

    if (!actor || !BattleMember_HasPendingNextMoveElement(actor))
    {
        return 0;
    }
    if (!BattleResolve_ActionQualifiesForPendingNextMoveElement(action, move))
    {
        return 0;
    }
    pending = BattleMember_GetPendingNextMoveElement(actor);
    BattleMember_ClearPendingNextMoveElement(actor);
    if (outElement)
    {
        *outElement = pending;
    }
    return 1;
}

static int BattleResolve_TryConsumePendingAttackElement(
    BattleMember *actor,
    const BattleAction *action,
    const MoveData *move,
    BattleElementId *outElement)
{
    if (BattleResolve_TryConsumePendingNextMoveElement(actor, action, move, outElement))
    {
        return 1;
    }
    return BattleResolve_TryConsumePendingWeaponAttackElement(
        actor,
        action,
        move,
        outElement);
}

static int BattleResolve_GetClosestEnemyDistance(
    const BattleState *state,
    int actorSide,
    int actorSlot)
{
    int opponentSide;
    int slotCount;
    int slot;
    int distance;
    int closest = -1;

    if (!state)
    {
        return -1;
    }
    opponentSide = BattleResolve_GetOpponentSide(actorSide);
    slotCount = BattleResolve_GetSideSlotCount(opponentSide);
    for (slot = 0; slot < slotCount; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, opponentSide, slot))
        {
            continue;
        }
        distance = BattleSlot_GetDistance(actorSide, actorSlot, opponentSide, slot);
        if (distance == BATTLE_SLOT_DISTANCE_INVALID)
        {
            continue;
        }
        if (closest < 0 || distance < closest)
        {
            closest = distance;
        }
    }
    return closest;
}

static int BattleResolve_IsClosestEnemyTarget(
    const BattleState *state,
    int actorSide,
    int actorSlot,
    int targetSide,
    int targetSlot)
{
    int closest;
    int distance;

    if (!state ||
        targetSide != BattleResolve_GetOpponentSide(actorSide) ||
        !BattleState_IsAliveOccupiedSlot(state, targetSide, targetSlot))
    {
        return 0;
    }
    closest = BattleResolve_GetClosestEnemyDistance(state, actorSide, actorSlot);
    if (closest < 0)
    {
        return 0;
    }
    distance = BattleSlot_GetDistance(actorSide, actorSlot, targetSide, targetSlot);
    return distance == closest;
}

/*
 * Long Shot: |slotIndex| distance maps to power.
 * distance 0/1/2/3/4+ -> 10/16/22/28/34 (closest to furthest).
 * Softened from 10-50 so max-range shots threaten Petalburg allies
 * without routinely one-shotting Aksil before mobility plays.
 */
static int BattleResolve_GetLongShotPower(
    const BattleMember *attacker,
    const BattleMember *target)
{
    int distance;
    static const int s_longShotPowersByDistance[5] = { 10, 16, 22, 28, 34 };

    if (!attacker || !target)
    {
        return 10;
    }
    distance = BattleSlot_GetDistance(
        attacker->side,
        attacker->slotIndex,
        target->side,
        target->slotIndex);
    if (distance == BATTLE_SLOT_DISTANCE_INVALID || distance < 0)
    {
        return 10;
    }
    if (distance > 4)
    {
        distance = 4;
    }
    return s_longShotPowersByDistance[distance];
}

static void BattleResolve_ExecuteDamageEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int basePower;
    int hitCritOverridePermille = DAMAGE_CALC_HIT_CRIT_DEFAULT;

    if (!ctx || !effect || !ctx->state || !ctx->actor)
    {
        return;
    }
    basePower = effect->paramA;
    if (basePower <= 0 && ctx->move)
    {
        basePower = MoveData_GetDamagePower(ctx->move);
    }
    if (effect->paramB > 0)
    {
        hitCritOverridePermille = effect->paramB;
    }

    if (!ctx->target)
    {
        return;
    }
    if (ctx->move && ctx->move->id == MOVE_FIERY_STRIKE)
    {
        if (!BattleResolve_IsClosestEnemyTarget(
                ctx->state,
                ctx->action ? ctx->action->actorSide : ctx->actor->side,
                ctx->action ? ctx->action->actorSlot : ctx->actor->slotIndex,
                ctx->targetSide,
                ctx->targetSlot))
        {
            BattleMessageQueue_Enqueue("Only the closest enemy can be hit!");
            return;
        }
    }
    if (ctx->move && ctx->move->id == MOVE_LONG_SHOT)
    {
        basePower = BattleResolve_GetLongShotPower(ctx->actor, ctx->target);
    }
    BattleResolve_ApplyCombatDamage(
        ctx->state,
        ctx->targetSide,
        ctx->targetSlot,
        ctx->actor,
        ctx->target,
        ctx->move,
        basePower,
        ctx->useLpBonus,
        hitCritOverridePermille,
        ctx->effectiveAttackElement,
        ctx->useEffectiveAttackElement,
        ctx->outgoingDamageMultiplierPercent,
        ctx->action,
        &ctx->lastCombatResult);
    ctx->hasLastCombatResult = 1;

    /*
     * Phantom Phist v1: Curse only the main DAMAGE target on crit with real
     * residual damage. Splash uses ApplyCombatDamage separately and never
     * reaches this hook.
     */
    if (ctx->move &&
        ctx->move->id == MOVE_PHANTOM_PHIST &&
        ctx->target &&
        ctx->target->isAlive &&
        ctx->lastCombatResult.hit &&
        ctx->lastCombatResult.critical &&
        ctx->lastCombatResult.damage > 0)
    {
        BattleResolve_TryApplyStatusDirect(ctx->target, BATTLE_STATUS_CURSE);
    }
}

static void BattleResolve_ExecuteStatusEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    if (!ctx || !effect || !ctx->target || !ctx->move)
    {
        return;
    }
    if (!ctx->hasLastCombatResult)
    {
        CombatResult_Clear(&ctx->lastCombatResult);
    }
    BattleResolve_TryInflictStatusWithParams(
        ctx->target,
        ctx->move,
        effect->status,
        effect->paramA,
        BattleResolve_GetResolvedAttackElement(
            ctx->move,
            ctx->effectiveAttackElement,
            ctx->useEffectiveAttackElement),
        &ctx->lastCombatResult);
}

static void BattleResolve_ExecuteClearStatusEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    (void)effect;
    if (!ctx || !ctx->target)
    {
        return;
    }
    BattleResolve_ApplyDebugClearStatus(ctx->target);
}

static void BattleResolve_ApplyMemberMove(
    BattleState *state,
    BattleAction *action,
    int side,
    int fromSlot,
    int toSlot)
{
    const BattleMember *member;
    int cursor = 0;

    if (!state || !action)
    {
        return;
    }
    if (!BattleState_CanMoveMemberToSlot(state, side, fromSlot, toSlot))
    {
        return;
    }
    BattleState_MoveMemberToSlot(state, side, fromSlot, toSlot);
    if (side == BATTLE_SIDE_PLAYER && action->actorSide == BATTLE_SIDE_PLAYER)
    {
        BattleState_RekeyPlayerSlotActed(state, fromSlot, toSlot);
        state->activePlayerSlot = toSlot;
        action->actorSlot = toSlot;
    }
    else if (side == BATTLE_SIDE_ENEMY && action->actorSide == BATTLE_SIDE_ENEMY)
    {
        action->actorSlot = toSlot;
    }
    member = BattleState_GetMemberAtSlot(state, side, toSlot);
    s_resolveMessage[0] = '\0';
    if (member)
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, member->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, " moved.");
    }
    else
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, "Moved.");
    }
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
}

static int BattleResolve_GetMainDamagePower(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_DAMAGE && move->effects[i].paramA > 0)
        {
            return move->effects[i].paramA;
        }
    }
    if (move->defaultPower > 0)
    {
        return move->defaultPower;
    }
    return 0;
}

static int BattleResolve_ExecuteSideSplashDamageEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int splashPercent;
    int mainPower;
    int splashPower;
    int appliedSplash = 0;
    static const int s_sideOffsets[] = { -1, 1 };
    int i;

    if (!ctx || !effect || !ctx->state || !ctx->actor || !ctx->move)
    {
        return 0;
    }

    splashPercent = effect->paramA;
    if (splashPercent <= 0)
    {
        return 1;
    }

    mainPower = BattleResolve_GetMainDamagePower(ctx->move);
    if (mainPower <= 0)
    {
        return 1;
    }

    splashPower = (mainPower * splashPercent) / 100;
    if (splashPower < 1)
    {
        splashPower = 1;
    }

    for (i = 0; i < 2; i++)
    {
        int sideSlot = ctx->targetSlot + s_sideOffsets[i];
        BattleMember *sideTarget;

        if (!BattleState_IsAliveOccupiedSlot(ctx->state, ctx->targetSide, sideSlot))
        {
            continue;
        }
        sideTarget = BattleResolve_GetMember(ctx->state, ctx->targetSide, sideSlot);
        if (!sideTarget || BattleMember_IsDowned(sideTarget))
        {
            continue;
        }
        BattleResolve_ApplyCombatDamage(
            ctx->state,
            ctx->targetSide,
            sideSlot,
            ctx->actor,
            sideTarget,
            ctx->move,
            splashPower,
            ctx->useLpBonus,
            DAMAGE_CALC_HIT_CRIT_DEFAULT,
            ctx->effectiveAttackElement,
            ctx->useEffectiveAttackElement,
            ctx->outgoingDamageMultiplierPercent,
            ctx->action,
            0);
        appliedSplash = 1;
    }

    if (appliedSplash)
    {
        BattleMessageQueue_Enqueue("The attack splashed outward!");
    }

    return 1;
}

static int BattleResolve_TryGrantPlayerBonusAction(
    BattleResolveContext *ctx,
    BattleMember *actor)
{
    if (!ctx || !ctx->state || !ctx->action || !actor)
    {
        return 0;
    }
    if (ctx->action->actorSide != BATTLE_SIDE_PLAYER)
    {
        return 0;
    }
    /* Only one immediate bonus command; do not chain from a bonus turn. */
    if (ctx->state->isPlayerBonusCommand)
    {
        return 0;
    }
    ctx->state->bonusActionPlayerSlot = ctx->action->actorSlot;
    return 1;
}

static int BattleResolve_ExecuteBonusActionEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    BattleMember *actor;
    int cursor = 0;

    if (!ctx || !ctx->state || !ctx->action || !effect)
    {
        return 0;
    }
    if (effect->paramA == MOVE_EFFECT_BONUS_ACTION_REQUIRES_LP && !ctx->useLpBonus)
    {
        return 1;
    }

    actor = BattleResolve_GetMember(ctx->state, ctx->action->actorSide, ctx->action->actorSlot);
    if (!actor)
    {
        return 0;
    }
    if (!BattleResolve_TryGrantPlayerBonusAction(ctx, actor))
    {
        return 1;
    }

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " can act again!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    return 1;
}

static int BattleResolve_ExecuteLastChanceEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    BattleMember *actor;
    int targetHp;
    int grantedBonus;

    (void)effect;
    if (!ctx || !ctx->state || !ctx->action || !ctx->actor || !ctx->move)
    {
        return 0;
    }
    if (ctx->action->actorSide != BATTLE_SIDE_PLAYER)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }
    actor = ctx->actor;
    if (!actor->isAlive || actor->maxHp <= 0)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    targetHp = (actor->maxHp * 5) / 100;
    if (targetHp < 1)
    {
        targetHp = 1;
    }
    /* Decrease to 5% when above; leave HP if already at or below. */
    if (actor->currentHp > targetHp)
    {
        actor->currentHp = targetHp;
    }

    BattleMessageQueue_Enqueue("Last Chance!");
    grantedBonus = BattleResolve_TryGrantPlayerBonusAction(ctx, actor);
    if (grantedBonus)
    {
        BattleMessageQueue_Enqueue("Can act again!");
    }
    return 1;
}

static int BattleResolve_ExecuteIncomingDamageReductionEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int percent;
    int durationTurns;

    if (!ctx || !effect || !ctx->actor || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }

    percent = effect->paramA;
    durationTurns = effect->paramB;
    if (percent <= 0 || durationTurns <= 0)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    BattleMember_SetIncomingDamageReduction(ctx->actor, percent, durationTurns);
    BattleMessageQueue_Enqueue("Damage taken fell!");
    return 1;
}

static int BattleResolve_ExecuteTeamFocusEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    (void)effect;
    if (!ctx || !ctx->state || !ctx->actor || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->action->actorSide != BATTLE_SIDE_PLAYER)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }

    BattleState_SetAllyNextMoveCannotMiss(ctx->state, 1);
    BattleMessageQueue_Enqueue("Team Focus!");
    return 1;
}

static int BattleResolve_ExecuteBubbleProtectionEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int cursor = 0;

    (void)effect;
    if (!ctx || !ctx->target || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SINGLE_ALLY)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide)
    {
        return 0;
    }
    if (!BattleState_IsAliveOccupiedSlot(
            ctx->state, ctx->action->targetSide, ctx->action->targetSlot))
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    BattleMember_SetBubbleProtection(ctx->target, 1);
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, ctx->target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " is shielded by Bubble!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    return 1;
}

static int BattleResolve_ExecuteVeilLuckBuffEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    (void)effect;
    if (!ctx || !ctx->state || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }

    BattleState_PlaceRandomVeilTiles(ctx->state);
    BattleMessageQueue_Enqueue("Veil!");
    BattleMessageQueue_Enqueue("Luck spots appear!");
    return 1;
}

static int BattleResolve_ExecutePendingAttackAllEnemiesEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    (void)effect;
    if (!ctx || !ctx->actor || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }

    BattleMember_SetPendingAttackHitsAllEnemies(ctx->actor, 1);
    BattleMessageQueue_Enqueue("Next attack will hit all enemies!");
    return 1;
}

static int BattleResolve_ExecuteExtraActionNextTurnEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int extraActions;

    if (!ctx || !effect || !ctx->actor)
    {
        return 0;
    }

    extraActions = effect->paramA;
    if (extraActions <= 0)
    {
        extraActions = MOVE_EFFECT_EXTRA_ACTION_NEXT_TURN_DEFAULT;
    }
    BattleMember_SetExtraActionsNextTurn(ctx->actor, extraActions);
    BattleMessageQueue_Enqueue("Will act twice next turn!");
    return 1;
}

static int BattleResolve_ExecutePendingNextMoveElementEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    BattleElementId element;

    if (!ctx || !effect || !ctx->actor || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }

    element = (BattleElementId)effect->paramA;
    if (!ElementDatabase_IsValid(element) || element == BATTLE_ELEMENT_NONE)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    BattleMember_SetPendingNextMoveElement(ctx->actor, element);
    BattleMessageQueue_Enqueue("Next move will be Wind!");
    return 1;
}

static int BattleResolve_ExecutePendingWeaponAttackElementEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    BattleElementId element;

    if (!ctx || !effect || !ctx->actor || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }

    element = (BattleElementId)effect->paramA;
    if (!ElementDatabase_IsValid(element) || element == BATTLE_ELEMENT_NONE)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    BattleMember_SetPendingWeaponAttackElement(ctx->actor, element);
    BattleMessageQueue_Enqueue("Next slingshot attack will be Fire!");
    return 1;
}

static int BattleResolve_ExecuteEncoreEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int durationTurns;
    int cursor = 0;

    if (!ctx || !effect || !ctx->target || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SINGLE_ENEMY)
    {
        return 0;
    }
    if (!BattleState_IsAliveOccupiedSlot(
            ctx->state, ctx->action->targetSide, ctx->action->targetSlot))
    {
        return 0;
    }
    if (ctx->target->side != BATTLE_SIDE_ENEMY)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    durationTurns = effect->paramA;
    if (durationTurns <= 0)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    if (ctx->useLpBonus && ctx->move && ctx->move->optionalLpCost > 0)
    {
        durationTurns += ctx->move->optionalLpCost;
    }

    if (!BattleMember_HasQualifyingLastAction(ctx->target) ||
        !BattleMember_LastActionSupportsEncoreReplayV1(ctx->target))
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    if (!BattleMember_ApplyEncoreFromLastAction(ctx->target, durationTurns))
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, ctx->target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " is influenced!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    return 1;
}

static int BattleResolve_ExecuteMovementEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int actorSide;

    (void)effect;
    if (!ctx || !ctx->state || !ctx->action || !ctx->actor)
    {
        return 0;
    }
    if (BattleMember_IsMovementLocked(ctx->actor))
    {
        BattleMessageQueue_Enqueue("Cannot move!");
        return 0;
    }
    actorSide = ctx->action->actorSide;
    if (ctx->action->targetSide != actorSide)
    {
        BattleMessageQueue_Enqueue("Cannot move!");
        return 0;
    }
    if (!BattleState_CanMoveMemberToSlot(
            ctx->state,
            actorSide,
            ctx->action->actorSlot,
            ctx->targetSlot))
    {
        BattleMessageQueue_Enqueue("Cannot move!");
        return 0;
    }
    BattleResolve_ApplyMemberMove(
        ctx->state,
        ctx->action,
        actorSide,
        ctx->action->actorSlot,
        ctx->targetSlot);
    ctx->actor = BattleResolve_GetMember(ctx->state, actorSide, ctx->action->actorSlot);

    /* Boss Tutsil Vortex Escape special: next turn is skipped. Player Aksil unchanged. */
    if (ctx->move &&
        ctx->move->id == MOVE_VORTEX_ESCAPE &&
        BattleMember_IsTutsil(ctx->actor))
    {
        BattleMember_SetSkipNextTurn(ctx->actor, 1);
        BattleMessageQueue_Enqueue("Must recover next turn!");
    }
    return 1;
}

static int BattleResolve_ExecuteSwapWithAllyEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int actorSide;
    int actorSlot;
    int allySlot;
    int cursor = 0;

    (void)effect;
    if (!ctx || !ctx->state || !ctx->action || !ctx->actor || !ctx->move)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SINGLE_ALLY)
    {
        return 0;
    }

    actorSide = ctx->action->actorSide;
    actorSlot = ctx->action->actorSlot;
    allySlot = ctx->targetSlot;
    if (ctx->action->targetSide != actorSide)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    if (allySlot == actorSlot)
    {
        BattleMessageQueue_Enqueue("Cannot swap with self!");
        return 0;
    }
    if (!BattleState_CanSwapMembersOnSide(ctx->state, actorSide, actorSlot, allySlot))
    {
        BattleMessageQueue_Enqueue("Cannot swap!");
        return 0;
    }
    if (!BattleState_SwapMembersOnSide(ctx->state, actorSide, actorSlot, allySlot))
    {
        BattleMessageQueue_Enqueue("Cannot swap!");
        return 0;
    }

    if (actorSide == BATTLE_SIDE_PLAYER)
    {
        if (ctx->state->activePlayerSlot == actorSlot)
        {
            ctx->state->activePlayerSlot = allySlot;
        }
        else if (ctx->state->activePlayerSlot == allySlot)
        {
            ctx->state->activePlayerSlot = actorSlot;
        }
    }
    ctx->action->actorSlot = allySlot;
    ctx->actor = BattleResolve_GetMember(ctx->state, actorSide, allySlot);
    ctx->target = BattleResolve_GetMember(ctx->state, actorSide, actorSlot);
    ctx->targetSlot = actorSlot;

    s_resolveMessage[0] = '\0';
    if (ctx->actor)
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, ctx->actor->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, " swapped places!");
    }
    else
    {
        BattleResolve_AppendString(s_resolveMessage, &cursor, "Swapped places!");
    }
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    return 1;
}

static void BattleResolve_EnqueueMovementLockMessage(const BattleMember *member)
{
    int cursor = 0;

    if (!member)
    {
        BattleMessageQueue_Enqueue("Target cannot move!");
        return;
    }

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, member->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " cannot move!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
}

static int BattleResolve_ExecuteMovementLockEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int duration;

    if (!ctx || !effect || !ctx->target)
    {
        return 0;
    }
    if (!ctx->target->isAlive || BattleMember_IsDowned(ctx->target))
    {
        return 1;
    }

    duration = effect->paramA;
    if (duration <= 0)
    {
        duration = 1;
    }
    BattleMember_SetMovementLock(ctx->target, duration);
    if (ctx->move && ctx->move->id == MOVE_STATIC_LOCK)
    {
        BattleMessageQueue_Enqueue("Static Lock!");
        BattleMessageQueue_Enqueue("Movement locked!");
    }
    else
    {
        BattleResolve_EnqueueMovementLockMessage(ctx->target);
    }
    return 1;
}

static void BattleResolve_EnqueueStatModMessage(const BattleMember *member, BattleStatId stat, int percent)
{
    int cursor = 0;

    if (!member)
    {
        return;
    }

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, member->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, "'s ");
    switch (stat)
    {
    case BATTLE_STAT_ATTACK:
        if (percent < 0)
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "attack fell!");
        }
        else
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "attack rose!");
        }
        break;
    case BATTLE_STAT_DEFENCE:
        if (percent < 0)
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "defence fell!");
        }
        else
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "defence rose!");
        }
        break;
    case BATTLE_STAT_SPEED:
        if (percent < 0)
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "speed fell!");
        }
        else
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "speed rose!");
        }
        break;
    case BATTLE_STAT_EVASIVENESS:
        if (percent < 0)
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "evasiveness fell!");
        }
        else
        {
            BattleResolve_AppendString(s_resolveMessage, &cursor, "evasiveness rose!");
        }
        break;
    default:
        BattleResolve_AppendString(s_resolveMessage, &cursor, "stats changed!");
        break;
    }
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
}

static int BattleResolve_ApplyStatModToAllOpponents(
    BattleState *state,
    int actorSide,
    BattleStatId stat,
    int percent,
    int maxStacks,
    int durationTurns)
{
    int opponentSide;
    int slot;
    int slotCount;
    BattleMember *members;
    int applied = 0;

    if (!state)
    {
        return 0;
    }

    opponentSide = actorSide == BATTLE_SIDE_PLAYER ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
    if (opponentSide == BATTLE_SIDE_ENEMY)
    {
        slotCount = BATTLE_ENEMY_SLOT_COUNT;
        members = state->enemySlots;
    }
    else
    {
        slotCount = BATTLE_PLAYER_SLOT_COUNT;
        members = state->playerSlots;
    }

    for (slot = 0; slot < slotCount; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, opponentSide, slot))
        {
            continue;
        }
        if (BattleMember_ApplyStatMod(&members[slot], stat, percent, maxStacks, durationTurns))
        {
            applied++;
        }
    }
    return applied;
}

static int BattleResolve_IsAdjacentAllySlot(int side, int actorSlot, int allySlot)
{
    if (allySlot == actorSlot)
    {
        return 0;
    }
    return BattleSlot_GetDistance(side, actorSlot, side, allySlot) == 1;
}

static int BattleResolve_ApplyStatModToAdjacentAllies(
    BattleState *state,
    int actorSide,
    int actorSlot,
    BattleStatId stat,
    int percent,
    int maxStacks,
    int durationTurns)
{
    int slot;
    int slotCount;
    BattleMember *members;
    int applied = 0;

    if (!state)
    {
        return 0;
    }

    if (actorSide == BATTLE_SIDE_ENEMY)
    {
        slotCount = BATTLE_ENEMY_SLOT_COUNT;
        members = state->enemySlots;
    }
    else
    {
        slotCount = BATTLE_PLAYER_SLOT_COUNT;
        members = state->playerSlots;
    }

    for (slot = 0; slot < slotCount; slot++)
    {
        if (!BattleResolve_IsAdjacentAllySlot(actorSide, actorSlot, slot))
        {
            continue;
        }
        if (!BattleState_IsAliveOccupiedSlot(state, actorSide, slot))
        {
            continue;
        }
        if (!BattleMember_CanApplyStatMod(&members[slot], stat, maxStacks))
        {
            continue;
        }
        if (BattleMember_ApplyStatMod(
                &members[slot], stat, percent, maxStacks, durationTurns))
        {
            applied++;
        }
    }
    return applied;
}

static void BattleResolve_RollbackPendingStatMods(BattleResolveContext *ctx)
{
    int i;

    if (!ctx || !ctx->statModRollbackTarget)
    {
        return;
    }
    for (i = 0; i < ctx->statModRollbackCount; i++)
    {
        BattleMember_RemoveStatModForStat(
            ctx->statModRollbackTarget,
            ctx->statModRollbackStats[i]);
    }
    ctx->statModRollbackCount = 0;
    ctx->statModRollbackTarget = 0;
}

static void BattleResolve_RecordStatModRollback(
    BattleResolveContext *ctx,
    BattleMember *target,
    BattleStatId stat)
{
    if (!ctx || !target || ctx->statModRollbackCount >= MOVE_MAX_EFFECTS)
    {
        return;
    }
    if (!ctx->statModRollbackTarget)
    {
        ctx->statModRollbackTarget = target;
    }
    ctx->statModRollbackStats[ctx->statModRollbackCount++] = stat;
}

static int BattleResolve_HasLaterStatModifierEffect(const MoveData *move, int afterIndex)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = afterIndex + 1; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_STAT_MODIFIER)
        {
            return 1;
        }
    }
    return 0;
}

static int BattleResolve_ExecuteStatModifierEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect,
    int effectIndex)
{
    BattleStatId stat;
    int percent;
    int durationTurns;
    int maxStacks;
    int applied;

    if (!ctx || !effect || !ctx->move || !ctx->actor || !ctx->action || !ctx->state)
    {
        return 0;
    }

    stat = (BattleStatId)effect->paramA;
    percent = effect->paramB;
    durationTurns = effect->paramC & 0xFF;
    maxStacks = (effect->paramC >> 8) & 0xFF;
    if (maxStacks <= 0)
    {
        maxStacks = 1;
    }

    if (ctx->move->targetPattern == MOVE_TARGET_ALL_ENEMIES)
    {
        applied = BattleResolve_ApplyStatModToAllOpponents(
            ctx->state,
            ctx->action->actorSide,
            stat,
            percent,
            maxStacks,
            durationTurns);
        if (applied <= 0)
        {
            BattleMessageQueue_Enqueue("The move had no effect.");
            return 0;
        }
        if (stat == BATTLE_STAT_SPEED && percent < 0)
        {
            BattleMessageQueue_Enqueue("Enemy speed fell!");
        }
        else
        {
            BattleMessageQueue_Enqueue("Opponents' stats changed!");
        }
        return 1;
    }

    if (ctx->move->targetPattern == MOVE_TARGET_ADJACENT_ALLIES)
    {
        if (ctx->action->targetSide != ctx->action->actorSide ||
            ctx->action->targetSlot != ctx->action->actorSlot)
        {
            return 0;
        }
        applied = BattleResolve_ApplyStatModToAdjacentAllies(
            ctx->state,
            ctx->action->actorSide,
            ctx->action->actorSlot,
            stat,
            percent,
            maxStacks,
            durationTurns);
        if (applied <= 0)
        {
            BattleMessageQueue_Enqueue("The move had no effect.");
            return 0;
        }
        if (stat == BATTLE_STAT_SPEED && percent > 0)
        {
            BattleMessageQueue_Enqueue("Adjacent allies sped up!");
        }
        else
        {
            BattleMessageQueue_Enqueue("Adjacent allies' stats changed!");
        }
        return 1;
    }

    if (ctx->move->targetPattern == MOVE_TARGET_SINGLE_ALLY)
    {
        BattleMember *recipient = ctx->target;

        if (!recipient || ctx->action->targetSide != ctx->action->actorSide)
        {
            return 0;
        }
        if (!BattleState_IsAliveOccupiedSlot(
                ctx->state, ctx->action->targetSide, ctx->action->targetSlot))
        {
            return 0;
        }
        if (!BattleMember_CanApplyStatMod(recipient, stat, maxStacks))
        {
            BattleMessageQueue_Enqueue("The move had no effect.");
            return 0;
        }
        if (!BattleMember_ApplyStatMod(
                recipient, stat, percent, maxStacks, durationTurns))
        {
            BattleMessageQueue_Enqueue("The move had no effect.");
            return 0;
        }
        BattleResolve_RecordStatModRollback(ctx, recipient, stat);
        if (!BattleResolve_HasLaterStatModifierEffect(ctx->move, effectIndex))
        {
            BattleMessageQueue_Enqueue("Stats shifted!");
        }
        return 1;
    }

    if (ctx->move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide ||
        ctx->action->targetSlot != ctx->action->actorSlot)
    {
        return 0;
    }

    if (!BattleMember_ApplyStatMod(ctx->actor, stat, percent, maxStacks, durationTurns))
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    BattleResolve_EnqueueStatModMessage(ctx->actor, stat, percent);
    return 1;
}

static int BattleResolve_ExecuteOnDamageSelfStatModEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    BattleStatId stat;
    int percent;
    int durationTurns;
    int maxStacks;

    if (!ctx || !effect || !ctx->actor || !ctx->action)
    {
        return 0;
    }

    /*
     * Only buff when a preceding damage effect actually damaged an opponent.
     * Miss / dodge / defence negation / zero damage / Bubble leave damage at 0.
     */
    if (!ctx->hasLastCombatResult ||
        !ctx->lastCombatResult.hit ||
        ctx->lastCombatResult.damage <= 0)
    {
        return 1;
    }
    if (ctx->targetSide == ctx->action->actorSide)
    {
        return 1;
    }
    if (!ctx->target)
    {
        return 1;
    }
    if (!ctx->actor->isAlive || ctx->actor->currentHp <= 0)
    {
        return 1;
    }

    stat = (BattleStatId)effect->paramA;
    percent = effect->paramB;
    durationTurns = effect->paramC & 0xFF;
    maxStacks = (effect->paramC >> 8) & 0xFF;
    if (maxStacks <= 0)
    {
        maxStacks = 1;
    }
    if (!BattleMember_CanApplyStatMod(ctx->actor, stat, maxStacks))
    {
        return 1;
    }
    if (!BattleMember_ApplyStatMod(ctx->actor, stat, percent, maxStacks, durationTurns))
    {
        return 1;
    }
    BattleResolve_EnqueueStatModMessage(ctx->actor, stat, percent);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    return 1;
}

static int BattleResolve_ComputeAutoPushDestSlot(int actorSlot, int targetSlot)
{
    if (targetSlot == actorSlot)
    {
        if (targetSlot <= 2)
        {
            return targetSlot + 1;
        }
        return targetSlot - 1;
    }
    if (targetSlot > actorSlot)
    {
        return targetSlot + 1;
    }
    return targetSlot - 1;
}

static int BattleResolve_ExecuteAutoPushOnDamageEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int fromSlot;
    int destSlot;
    int actorSlot;

    (void)effect;
    if (!ctx || !ctx->state || !ctx->actor || !ctx->action)
    {
        return 0;
    }

    /* Push only after real damage to a living opposing unit. */
    if (!ctx->hasLastCombatResult ||
        !ctx->lastCombatResult.hit ||
        ctx->lastCombatResult.damage <= 0)
    {
        return 1;
    }
    if (ctx->targetSide == ctx->action->actorSide)
    {
        return 1;
    }
    if (!ctx->target ||
        !BattleState_IsAliveOccupiedSlot(ctx->state, ctx->targetSide, ctx->targetSlot))
    {
        return 1;
    }

    fromSlot = ctx->targetSlot;
    actorSlot = ctx->action->actorSlot;
    destSlot = BattleResolve_ComputeAutoPushDestSlot(actorSlot, fromSlot);

    if (!BattleState_IsSlotValid(ctx->targetSide, destSlot) ||
        !BattleState_CanMoveMemberToSlot(ctx->state, ctx->targetSide, fromSlot, destSlot))
    {
        BattleMessageQueue_Enqueue("No room to push!");
        return 1;
    }

    if (!BattleState_MoveMemberToSlot(ctx->state, ctx->targetSide, fromSlot, destSlot))
    {
        BattleMessageQueue_Enqueue("No room to push!");
        return 1;
    }

    if (ctx->targetSide == BATTLE_SIDE_ENEMY)
    {
        BattleEffectivenessReaction reaction;

        reaction = BattleState_GetEnemyEffectReaction(ctx->state, fromSlot);
        BattleState_SetEnemyEffectReaction(ctx->state, destSlot, reaction);
        BattleState_SetEnemyEffectReaction(
            ctx->state,
            fromSlot,
            BATTLE_EFFECT_REACTION_NONE);
        BattleEnemyTurn_RekeyEnemySlot(fromSlot, destSlot);
        if (ctx->state->activeEnemySlot == fromSlot)
        {
            ctx->state->activeEnemySlot = destSlot;
        }
    }

    ctx->targetSlot = destSlot;
    ctx->action->targetSlot = destSlot;
    ctx->target = BattleResolve_GetMember(ctx->state, ctx->targetSide, destSlot);

    BattleMessageQueue_Enqueue("Foe was pushed!");
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    return 1;
}

static int BattleResolve_ExecutePendingNextDamageDoubleEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    int cursor = 0;

    (void)effect;
    if (!ctx || !ctx->target || !ctx->move || !ctx->action)
    {
        return 0;
    }
    if (ctx->move->targetPattern != MOVE_TARGET_SINGLE_ALLY)
    {
        return 0;
    }
    if (ctx->action->targetSide != ctx->action->actorSide)
    {
        return 0;
    }
    if (!BattleState_IsAliveOccupiedSlot(
            ctx->state, ctx->action->targetSide, ctx->action->targetSlot))
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }

    BattleMember_SetPendingNextDamageDouble(ctx->target, 1);
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, ctx->target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, "'s next attack doubles!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
    return 1;
}

static void BattleResolve_ExecuteDebugStatusCycleEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect)
{
    MoveId moveId;

    if (!ctx || !effect || !ctx->target)
    {
        return;
    }
    switch (effect->paramA)
    {
    case 0:
        moveId = MOVE_MATTER_STATUS_TEST;
        break;
    case 1:
        moveId = MOVE_INSTINCT_STATUS_TEST;
        break;
    case 2:
        moveId = MOVE_BALANCE_STATUS_TEST;
        break;
    default:
        return;
    }
    BattleResolve_ApplyDebugStatusCycle(ctx->target, moveId);
}

static int BattleResolve_ExecuteMoveEffect(
    BattleResolveContext *ctx,
    const MoveEffectData *effect,
    int effectIndex)
{
    if (!ctx || !effect)
    {
        return 0;
    }
    if (effect->kind == MOVE_EFFECT_KIND_NONE)
    {
        return 1;
    }
    if (!BattleResolve_IsProductionEffectKindSupported(effect->kind))
    {
        return 0;
    }

    switch (effect->kind)
    {
    case MOVE_EFFECT_KIND_DAMAGE:
        BattleResolve_ExecuteDamageEffect(ctx, effect);
        return 1;
    case MOVE_EFFECT_KIND_INFLICT_STATUS:
        BattleResolve_ExecuteStatusEffect(ctx, effect);
        return 1;
    case MOVE_EFFECT_KIND_CLEAR_STATUS:
        BattleResolve_ExecuteClearStatusEffect(ctx, effect);
        return 1;
    case MOVE_EFFECT_KIND_MOVE_TO_SLOT:
        return BattleResolve_ExecuteMovementEffect(ctx, effect);
    case MOVE_EFFECT_KIND_MOVEMENT_LOCK:
        return BattleResolve_ExecuteMovementLockEffect(ctx, effect);
    case MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE:
        return BattleResolve_ExecuteSideSplashDamageEffect(ctx, effect);
    case MOVE_EFFECT_KIND_BONUS_ACTION:
        return BattleResolve_ExecuteBonusActionEffect(ctx, effect);
    case MOVE_EFFECT_KIND_INCOMING_DAMAGE_REDUCTION:
        return BattleResolve_ExecuteIncomingDamageReductionEffect(ctx, effect);
    case MOVE_EFFECT_KIND_PENDING_ATTACK_ALL_ENEMIES:
        return BattleResolve_ExecutePendingAttackAllEnemiesEffect(ctx, effect);
    case MOVE_EFFECT_KIND_PENDING_WEAPON_ATTACK_ELEMENT:
        return BattleResolve_ExecutePendingWeaponAttackElementEffect(ctx, effect);
    case MOVE_EFFECT_KIND_PENDING_NEXT_MOVE_ELEMENT:
        return BattleResolve_ExecutePendingNextMoveElementEffect(ctx, effect);
    case MOVE_EFFECT_KIND_EXTRA_ACTION_NEXT_TURN:
        return BattleResolve_ExecuteExtraActionNextTurnEffect(ctx, effect);
    case MOVE_EFFECT_KIND_ENCORE:
        return BattleResolve_ExecuteEncoreEffect(ctx, effect);
    case MOVE_EFFECT_KIND_DISTANCE_SCALE:
        /* Multiplier applied in DamageCalc via MoveData_HasDistanceScale; message sent earlier. */
        return 1;
    case MOVE_EFFECT_KIND_VERTIGO_WAVE_DAMAGE:
        return BattleResolve_ExecuteVertigoWaveDamageEffect(ctx, effect);
    case MOVE_EFFECT_KIND_BUBBLE_PROTECTION:
        return BattleResolve_ExecuteBubbleProtectionEffect(ctx, effect);
    case MOVE_EFFECT_KIND_VEIL_LUCK_BUFF:
        return BattleResolve_ExecuteVeilLuckBuffEffect(ctx, effect);
    case MOVE_EFFECT_KIND_SWAP_WITH_ALLY:
        return BattleResolve_ExecuteSwapWithAllyEffect(ctx, effect);
    case MOVE_EFFECT_KIND_TEAM_FOCUS:
        return BattleResolve_ExecuteTeamFocusEffect(ctx, effect);
    case MOVE_EFFECT_KIND_LAST_CHANCE:
        return BattleResolve_ExecuteLastChanceEffect(ctx, effect);
    case MOVE_EFFECT_KIND_ON_DAMAGE_SELF_STAT_MOD:
        return BattleResolve_ExecuteOnDamageSelfStatModEffect(ctx, effect);
    case MOVE_EFFECT_KIND_PENDING_NEXT_DAMAGE_DOUBLE:
        return BattleResolve_ExecutePendingNextDamageDoubleEffect(ctx, effect);
    case MOVE_EFFECT_KIND_AUTO_PUSH_ON_DAMAGE:
        return BattleResolve_ExecuteAutoPushOnDamageEffect(ctx, effect);
    case MOVE_EFFECT_KIND_DEBUG_STATUS_CYCLE:
        BattleResolve_ExecuteDebugStatusCycleEffect(ctx, effect);
        return 1;
    case MOVE_EFFECT_KIND_STAT_MODIFIER:
        return BattleResolve_ExecuteStatModifierEffect(ctx, effect, effectIndex);
    default:
        return 0;
    }
}

static int BattleResolve_ExecuteMoveEffectsForCurrentTarget(BattleResolveContext *ctx)
{
    int i;

    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (!BattleResolve_ExecuteMoveEffect(ctx, &ctx->move->effects[i], i))
        {
            BattleResolve_RollbackPendingStatMods(ctx);
            return 0;
        }
    }
    return 1;
}

static int BattleResolve_ExecuteMoveEffects(BattleResolveContext *ctx)
{
    if (!ctx || !ctx->move)
    {
        return 0;
    }

    ctx->hasLastCombatResult = 0;
    CombatResult_Clear(&ctx->lastCombatResult);
    ctx->statModRollbackTarget = 0;
    ctx->statModRollbackCount = 0;

    if (ctx->move->targetPattern == MOVE_TARGET_OUTER_ENEMIES)
    {
        int leftSlot;
        int rightSlot;
        int outerSlots[2];
        int outerCount = 0;
        int i;
        int originalTargetSide = ctx->targetSide;

        if (!BattleResolve_FindOuterEnemySlots(
                ctx->state, originalTargetSide, &leftSlot, &rightSlot))
        {
            BattleMessageQueue_Enqueue("The move had no effect.");
            return 0;
        }

        outerSlots[outerCount++] = leftSlot;
        if (rightSlot != leftSlot)
        {
            outerSlots[outerCount++] = rightSlot;
        }

        for (i = 0; i < outerCount; i++)
        {
            int slot = outerSlots[i];

            if (!BattleState_IsAliveOccupiedSlot(ctx->state, originalTargetSide, slot))
            {
                continue;
            }
            ctx->target = BattleResolve_GetMember(ctx->state, originalTargetSide, slot);
            if (!ctx->target)
            {
                continue;
            }
            ctx->targetSide = originalTargetSide;
            ctx->targetSlot = slot;
            if (!BattleResolve_ExecuteMoveEffectsForCurrentTarget(ctx))
            {
                return 0;
            }
        }
        return 1;
    }

    if (ctx->fanOutToAllEnemies)
    {
        int slotOrder[BATTLE_ENEMY_SLOT_COUNT];
        int orderCount;
        int i;
        int originalTargetSide = ctx->targetSide;
        int originalTargetSlot = ctx->targetSlot;
        int sideSlotCount = BattleResolve_GetSideSlotCount(originalTargetSide);

        orderCount = BattleResolve_BuildFanOutSlotOrder(
            originalTargetSlot,
            sideSlotCount,
            slotOrder);
        for (i = 0; i < orderCount; i++)
        {
            int slot = slotOrder[i];

            if (!BattleState_IsAliveOccupiedSlot(ctx->state, originalTargetSide, slot))
            {
                continue;
            }
            ctx->target = BattleResolve_GetMember(ctx->state, originalTargetSide, slot);
            if (!ctx->target)
            {
                continue;
            }
            ctx->targetSide = originalTargetSide;
            ctx->targetSlot = slot;
            if (!BattleResolve_ExecuteMoveEffectsForCurrentTarget(ctx))
            {
                return 0;
            }
        }
        return 1;
    }

    return BattleResolve_ExecuteMoveEffectsForCurrentTarget(ctx);
}

#endif /* SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS */

static int BattleResolve_GetAttackBasePowerForSide(int actorSide)
{
    if (actorSide == BATTLE_SIDE_ENEMY)
    {
        return DAMAGE_CALC_ENEMY_ATTACK_BASE_POWER;
    }
    return DAMAGE_CALC_ATTACK_BASE_POWER;
}

static void BattleResolve_ExecuteAttack(BattleState *state, BattleAction *action)
{
    BattleMember *actor;
    BattleMember *target;
    int fanOut;
    int opponentSide;
    int slotCount;
    int slot;
    int basePower;
    BattleElementId effectiveAttackElement = BATTLE_ELEMENT_NONE;
    int useEffectiveAttackElement = 0;
    int outgoingDamageMultiplierPercent = 100;

    actor = BattleResolve_GetMember(state, action->actorSide, action->actorSlot);
    target = BattleResolve_GetMember(state, action->targetSide, action->targetSlot);
    if (!actor || !target)
    {
        return;
    }

    basePower = BattleResolve_GetAttackBasePowerForSide(action->actorSide);
    useEffectiveAttackElement = BattleResolve_TryConsumePendingAttackElement(
        actor,
        action,
        0,
        &effectiveAttackElement);
    if (!useEffectiveAttackElement)
    {
        /* Weapon/basic ATTACK element from weapon kind (sword=Might, slingshot=Fire). */
        effectiveAttackElement = BattleMember_GetBasicAttackElement(actor);
        useEffectiveAttackElement = (effectiveAttackElement != BATTLE_ELEMENT_NONE);
    }
    fanOut = BattleResolve_TryConsumePendingAllEnemiesAttack(actor, 1);
    if (BattleResolve_TryConsumePendingNextDamageDouble(actor, 1))
    {
        outgoingDamageMultiplierPercent = 200;
    }

    BattleResolve_EnqueueAttackMessage(actor);
    if (fanOut)
    {
        int slotOrder[BATTLE_ENEMY_SLOT_COUNT];
        int orderCount;
        int i;

        opponentSide = BattleResolve_GetOpponentSide(action->actorSide);
        slotCount = BattleResolve_GetSideSlotCount(opponentSide);
        orderCount = BattleResolve_BuildFanOutSlotOrder(
            action->targetSlot,
            slotCount,
            slotOrder);
        for (i = 0; i < orderCount; i++)
        {
            slot = slotOrder[i];
            if (!BattleState_IsAliveOccupiedSlot(state, opponentSide, slot))
            {
                continue;
            }
            target = BattleResolve_GetMember(state, opponentSide, slot);
            if (!target)
            {
                continue;
            }
            BattleResolve_ApplyCombat(
                state,
                opponentSide,
                slot,
                actor,
                target,
                0,
                basePower,
                0,
                effectiveAttackElement,
                useEffectiveAttackElement,
                outgoingDamageMultiplierPercent,
                action);
        }
        BattleMember_RecordResolvedAction(actor, action);
        return;
    }

    BattleResolve_ApplyCombat(
        state,
        action->targetSide,
        action->targetSlot,
        actor,
        target,
        0,
        basePower,
        0,
        effectiveAttackElement,
        useEffectiveAttackElement,
        outgoingDamageMultiplierPercent,
        action);
    BattleMember_RecordResolvedAction(actor, action);
}

#if SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS

static int BattleResolve_ExecuteSkillProduction(BattleState *state, BattleAction *action)
{
    BattleResolveContext ctx;
    const MoveData *move;
    BattleMember *actor;
    BattleMember *target;

    move = MoveData_Get(action->moveId);
    actor = BattleResolve_GetMember(state, action->actorSide, action->actorSlot);
    target = BattleResolve_GetMember(state, action->targetSide, action->targetSlot);
    if (!move || !actor || !target)
    {
        return 0;
    }

    ctx.state = state;
    ctx.action = action;
    ctx.actor = actor;
    ctx.target = target;
    ctx.move = move;
    ctx.useLpBonus = action->useLpBonus;
    ctx.targetSide = action->targetSide;
    ctx.targetSlot = action->targetSlot;
    ctx.hasLastCombatResult = 0;
    CombatResult_Clear(&ctx.lastCombatResult);
    ctx.statModRollbackTarget = 0;
    ctx.statModRollbackCount = 0;
    ctx.effectiveAttackElement = BATTLE_ELEMENT_NONE;
    ctx.useEffectiveAttackElement = BattleResolve_TryConsumePendingAttackElement(
        actor,
        action,
        move,
        &ctx.effectiveAttackElement);
    ctx.fanOutToAllEnemies = BattleResolve_TryConsumePendingAllEnemiesAttack(
        actor,
        BattleResolve_MoveQualifiesForPendingAllEnemiesAttack(move));
    ctx.outgoingDamageMultiplierPercent = 100;
    if (BattleResolve_TryConsumePendingNextDamageDouble(
            actor,
            BattleResolve_MoveQualifiesForPendingNextDamageDouble(move)))
    {
        ctx.outgoingDamageMultiplierPercent = 200;
    }

    BattleResolve_EnqueueUsedMessage(actor, move);
    BattleResolve_EnqueueDistanceBoostMessage(actor, target, move);
    if (action->useLpBonus && move->optionalLpCost > 0)
    {
        BattleMessageQueue_Enqueue("LP bonus!");
    }
    return BattleResolve_ExecuteMoveEffects(&ctx);
}

#endif /* SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS */

static void BattleResolve_ExecuteSkill(BattleState *state, BattleAction *action)
{
    BattleMember *actor;
    BattleMember *target;
    const MoveData *move;

    move = MoveData_Get(action->moveId);
    actor = BattleResolve_GetMember(state, action->actorSide, action->actorSlot);
    target = BattleResolve_GetMember(state, action->targetSide, action->targetSlot);
    if (!move || !actor || !target)
    {
        return;
    }
    /* Reserved storage index 3 must never execute even if malformed action supplies it. */
    if (!BattleMember_IsEquippableMoveSlot(action->actorMoveSlot))
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return;
    }

#if SOLARFLY_USE_PRODUCTION_MOVE_EFFECTS
    if (BattleResolve_IsProductionSkillPathSupported(move))
    {
        if (BattleResolve_ExecuteSkillProduction(state, action))
        {
            if (action->actorSide != BATTLE_SIDE_ENEMY)
            {
                BattleMember_SpendMoveSc(actor, action->actorMoveSlot, 1);
                if (action->useLpBonus && move->optionalLpCost > 0)
                {
                    BattleMember_SpendLp(actor, move->optionalLpCost);
                }
                /* Last Chance: max all equipped SC after the 1 SC spent. */
                if (action->moveId == MOVE_LAST_CHANCE)
                {
                    BattleMember_MaxAllMoveSc(actor);
                    BattleMessageQueue_Enqueue("SC maxed!");
                }
            }
            BattleMember_RecordResolvedAction(actor, action);
        }
        return;
    }
#endif

    if (action->actorSide != BATTLE_SIDE_ENEMY)
    {
        BattleMember_SpendMoveSc(actor, action->actorMoveSlot, 1);
        if (action->useLpBonus && move->optionalLpCost > 0)
        {
            BattleMember_SpendLp(actor, move->optionalLpCost);
        }
    }
    BattleResolve_ExecuteSkillLegacy(state, action);
    BattleMember_RecordResolvedAction(actor, action);
}

static void BattleResolve_ExecuteBrace(BattleState *state, BattleAction *action)
{
    BattleMember *actor;
    int cursor = 0;

    actor = BattleResolve_GetMember(state, action->actorSide, action->actorSlot);
    if (!actor)
    {
        return;
    }

    BattleMember_BraceRecoverAllMoves(actor, BATTLE_MEMBER_BRACE_SC_GAIN);
    BattleMember_SetGuardNextHit(actor, 1);

    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, actor->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " braced!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);

    cursor = 0;
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, "Restored ");
    BattleResolve_AppendNumber(s_resolveMessage, &cursor, BATTLE_MEMBER_BRACE_SC_GAIN);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " SC!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_resolveMessage);
}

static int BattleResolve_ExecutePotionItem(BattleState *state, BattleAction *action)
{
    BattleMember *target;
    const ItemData *item;
    int healed;
    int cursor = 0;

    item = ItemData_Get(action->itemId);
    if (!item || action->itemId != ITEM_POTION)
    {
        return 0;
    }
    if (BattleState_GetItemCount(state, ITEM_POTION) <= 0)
    {
        BattleMessageQueue_Enqueue("No Potion left.");
        return 0;
    }
    if (action->targetSide != BATTLE_SIDE_PLAYER ||
        !BattleState_IsAliveOccupiedSlot(state, action->targetSide, action->targetSlot))
    {
        return 0;
    }
    target = BattleResolve_GetMember(state, action->targetSide, action->targetSlot);
    if (!target)
    {
        return 0;
    }

    healed = BattleMember_Heal(target, item->healAmount);
    if (healed <= 0)
    {
        s_resolveMessage[0] = '\0';
        BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, "'s HP is full!");
        s_resolveMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        return 0;
    }

    BattleState_DecrementItem(state, ITEM_POTION);
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " recovered ");
    BattleResolve_AppendNumber(s_resolveMessage, &cursor, healed);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " HP!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    return 1;
}

static int BattleResolve_ExecuteCureItem(BattleState *state, BattleAction *action)
{
    BattleMember *target;
    const ItemData *item;
    int cursor = 0;

    item = ItemData_Get(action->itemId);
    if (!item || action->itemId != ITEM_CURE)
    {
        return 0;
    }
    if (BattleState_GetItemCount(state, ITEM_CURE) <= 0)
    {
        BattleMessageQueue_Enqueue("No Cure left.");
        return 0;
    }
    if (action->targetSide != BATTLE_SIDE_PLAYER ||
        !BattleState_IsAliveOccupiedSlot(state, action->targetSide, action->targetSlot))
    {
        return 0;
    }
    target = BattleResolve_GetMember(state, action->targetSide, action->targetSlot);
    if (!target)
    {
        return 0;
    }
    if (!BattleMember_HasAnyStatus(target))
    {
        s_resolveMessage[0] = '\0';
        BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
        BattleResolve_AppendString(s_resolveMessage, &cursor, " has no status.");
        s_resolveMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        return 0;
    }

    BattleMember_ClearStatuses(target);
    BattleState_DecrementItem(state, ITEM_CURE);
    s_resolveMessage[0] = '\0';
    BattleResolve_AppendString(s_resolveMessage, &cursor, target->name);
    BattleResolve_AppendString(s_resolveMessage, &cursor, " was cured!");
    s_resolveMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_resolveMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    return 1;
}

static int BattleResolve_ExecuteItem(BattleState *state, BattleAction *action)
{
    if (action->itemId == ITEM_POTION)
    {
        return BattleResolve_ExecutePotionItem(state, action);
    }
    if (action->itemId == ITEM_CURE)
    {
        return BattleResolve_ExecuteCureItem(state, action);
    }
    return 0;
}

int BattleResolve_Execute(BattleState *state, BattleAction *action)
{
    BattleMember *actor;
    int consumed;

    if (!action || !state)
    {
        return 0;
    }
    if (!BattleState_IsAliveOccupiedSlot(state, action->actorSide, action->actorSlot))
    {
        BattleMessageQueue_EnqueuePriority("Can't act!", BATTLE_MESSAGE_PRIORITY_HIGH);
        return 1;
    }
    actor = BattleResolve_GetMember(state, action->actorSide, action->actorSlot);
    if (BattleResolve_TryMatterStatusBeforeAction(actor))
    {
        BattleResolve_ApplyActorTurnRecovery(state, actor);
        return 1;
    }
    if (BattleResolve_TryInstinctStatusBeforeAction(state, actor))
    {
        BattleResolve_ApplyActorTurnRecovery(state, actor);
        return 1;
    }
    if (action->type == BATTLE_ACTION_ATTACK)
    {
        BattleResolve_ExecuteAttack(state, action);
        BattleResolve_ApplyActorTurnRecovery(state, actor);
        return 1;
    }
    if (action->type == BATTLE_ACTION_SKILL)
    {
        BattleResolve_ExecuteSkill(state, action);
        BattleResolve_ApplyActorTurnRecovery(state, actor);
        return 1;
    }
    if (action->type == BATTLE_ACTION_BRACE)
    {
        BattleResolve_ExecuteBrace(state, action);
        BattleResolve_ApplyActorTurnRecovery(state, actor);
        return 1;
    }
    if (action->type == BATTLE_ACTION_ITEM)
    {
        consumed = BattleResolve_ExecuteItem(state, action);
        if (consumed)
        {
            BattleResolve_ApplyActorTurnRecovery(state, actor);
        }
        return consumed;
    }
    return 1;
}
