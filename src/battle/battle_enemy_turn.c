#include "battle/battle_enemy_turn.h"
#include "battle/battle_action.h"
#include "battle/battle_draw.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_resolve.h"
#include "battle/battle_slot.h"
#include "battle/battle_state.h"
#include "battle_ui/battle_message_queue.h"

#define ENEMY_TURN_MESSAGE_MAX 48
#define BATTLE_ENEMY_EXTRA_ACTIONS_MAX 1

typedef struct EnemyAiMoveChoice {
    MoveId moveId;
    int moveSlot;
    int targetSide;
    int targetSlot;
} EnemyAiMoveChoice;

static char s_enemyTurnMessage[ENEMY_TURN_MESSAGE_MAX];
static unsigned char s_tutsilMoveCycleStep;
static unsigned char s_tutsilVortexCooldown;
static unsigned char s_tutsilYoungBloodDone;
static unsigned char s_badgerUsedCharge[BATTLE_ENEMY_SLOT_COUNT];
/* Cap dual-badger Static Lock spam to one lock per enemy wave. */
static unsigned char s_staticLockUsedThisEnemyTurn;

/* Fallback setup order when Vortex/Long Shot are unavailable (usable slots only). */
static const int s_tutsilCycleMoveSlots[BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT] = { 1, 2, 0 };
static const int s_longShotPowersByDistance[5] = { 10, 16, 22, 28, 34 };
#define TUTSIL_VORTEX_COOLDOWN_TURNS 3
#define TUTSIL_VORTEX_MIN_POWER_GAIN 6

static void BattleEnemyTurn_AppendChar(char *dest, int *cursor, char c)
{
    if (*cursor < ENEMY_TURN_MESSAGE_MAX - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void BattleEnemyTurn_AppendString(char *dest, int *cursor, const char *text)
{
    while (*text)
    {
        BattleEnemyTurn_AppendChar(dest, cursor, *text);
        text++;
    }
}

static int BattleEnemyTurn_FindPreferredPlayerTarget(const BattleState *state)
{
    int slot;

    if (!state)
    {
        return -1;
    }
    /* Prefer living Protagonist, then Aksil, then lowest living player slot. */
    slot = BattleState_FindProtagonistSlot(state);
    if (slot >= 0 && BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
    {
        return slot;
    }
    slot = BattleState_FindAksilSlot(state);
    if (slot >= 0 && BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
    {
        return slot;
    }
    /* preferredSlot -1 forces deterministic scan from slot 0 upward. */
    return BattleState_FindAlivePlayerSlot(state, -1);
}

static int BattleEnemyTurn_GetLongShotPowerForSlots(int attackerSlot, int targetSlot)
{
    int distance;

    distance = BattleSlot_GetDistance(
        BATTLE_SIDE_ENEMY,
        attackerSlot,
        BATTLE_SIDE_PLAYER,
        targetSlot);
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

static int BattleEnemyTurn_FindEquippedMoveSlot(const BattleMember *member, MoveId moveId)
{
    int i;

    if (!member || moveId == MOVE_NONE)
    {
        return -1;
    }
    for (i = 0; i < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; i++)
    {
        if (member->equippedMoves[i] == moveId)
        {
            return i;
        }
    }
    return -1;
}

/*
 * Vortex Escape targets an empty destination tile. Do not require that tile
 * to be living/occupied — that rejected every Vortex and left solo Tutsil
 * stuck choosing Vortex forever without ever attacking.
 */
static int BattleEnemyTurn_ChoiceHasValidTarget(
    const BattleState *state,
    const BattleMember *enemy,
    const EnemyAiMoveChoice *choice)
{
    if (!state || !enemy || !choice || choice->moveId == MOVE_NONE)
    {
        return 0;
    }
    if (choice->moveId == MOVE_VORTEX_ESCAPE)
    {
        return BattleState_CanMoveMemberToSlot(
            state,
            enemy->side,
            enemy->slotIndex,
            choice->targetSlot);
    }
    return BattleState_IsAliveOccupiedSlot(
        state,
        choice->targetSide,
        choice->targetSlot);
}

static int BattleEnemyTurn_TryBuildTutsilLongShotChoice(
    const BattleState *state,
    const BattleMember *tutsil,
    EnemyAiMoveChoice *outChoice)
{
    int playerTarget;
    int longShotSlot;

    if (!state || !tutsil || !outChoice)
    {
        return 0;
    }
    longShotSlot = BattleEnemyTurn_FindEquippedMoveSlot(tutsil, MOVE_LONG_SHOT);
    playerTarget = BattleEnemyTurn_FindPreferredPlayerTarget(state);
    if (longShotSlot < 0 || playerTarget < 0)
    {
        return 0;
    }
    outChoice->moveId = MOVE_LONG_SHOT;
    outChoice->moveSlot = longShotSlot;
    outChoice->targetSide = BATTLE_SIDE_PLAYER;
    outChoice->targetSlot = playerTarget;
    return 1;
}

/*
 * Best empty enemy slot for Tutsil Long Shot vs preferred player target.
 * Tie-break: higher power, then farther distance, then lower slot index.
 * Does not consider the current slot as a move destination.
 */
static int BattleEnemyTurn_GetBestTutsilLongShotSlot(
    const BattleState *state,
    int fromSlot,
    int playerTargetSlot,
    int *outPower,
    int *outDistance)
{
    int slot;
    int bestSlot = -1;
    int bestPower = -1;
    int bestDistance = -1;
    int power;
    int distance;

    if (outPower)
    {
        *outPower = 0;
    }
    if (outDistance)
    {
        *outDistance = 0;
    }
    if (!state || fromSlot < 0 || playerTargetSlot < 0)
    {
        return -1;
    }

    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        if (slot == fromSlot)
        {
            continue;
        }
        if (!BattleState_CanMoveMemberToSlot(state, BATTLE_SIDE_ENEMY, fromSlot, slot))
        {
            continue;
        }
        distance = BattleSlot_GetDistance(
            BATTLE_SIDE_ENEMY,
            slot,
            BATTLE_SIDE_PLAYER,
            playerTargetSlot);
        if (distance == BATTLE_SLOT_DISTANCE_INVALID)
        {
            continue;
        }
        power = BattleEnemyTurn_GetLongShotPowerForSlots(slot, playerTargetSlot);
        if (bestSlot < 0 ||
            power > bestPower ||
            (power == bestPower && distance > bestDistance) ||
            (power == bestPower && distance == bestDistance && slot < bestSlot))
        {
            bestSlot = slot;
            bestPower = power;
            bestDistance = distance;
        }
    }

    if (bestSlot >= 0)
    {
        if (outPower)
        {
            *outPower = bestPower;
        }
        if (outDistance)
        {
            *outDistance = bestDistance;
        }
    }
    return bestSlot;
}

static int BattleEnemyTurn_ShouldTutsilVortexForLongShot(
    const BattleState *state,
    int tutsilSlot,
    int playerTargetSlot,
    int *outDestSlot)
{
    int currentPower;
    int bestPower = 0;
    int bestSlot;

    if (outDestSlot)
    {
        *outDestSlot = -1;
    }
    if (!state || tutsilSlot < 0 || playerTargetSlot < 0)
    {
        return 0;
    }

    currentPower = BattleEnemyTurn_GetLongShotPowerForSlots(tutsilSlot, playerTargetSlot);
    bestSlot = BattleEnemyTurn_GetBestTutsilLongShotSlot(
        state,
        tutsilSlot,
        playerTargetSlot,
        &bestPower,
        0);
    if (bestSlot < 0)
    {
        return 0;
    }
    if (bestPower < currentPower + TUTSIL_VORTEX_MIN_POWER_GAIN)
    {
        return 0;
    }
    if (outDestSlot)
    {
        *outDestSlot = bestSlot;
    }
    return 1;
}

static int BattleEnemyTurn_FindBestLongShotDestination(
    const BattleState *state,
    int fromSlot,
    int playerTargetSlot)
{
    return BattleEnemyTurn_GetBestTutsilLongShotSlot(
        state,
        fromSlot,
        playerTargetSlot,
        0,
        0);
}

static int BattleEnemyTurn_FindTutsilSlot(const BattleState *state)
{
    int i;

    if (!state)
    {
        return -1;
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        if (BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, i) &&
            BattleMember_IsTutsil(&state->enemySlots[i]))
        {
            return i;
        }
    }
    return -1;
}

static int BattleEnemyTurn_FindStatusAfflictedAllySlot(const BattleState *state, int preferSlot)
{
    int i;

    if (!state)
    {
        return -1;
    }
    if (preferSlot >= 0 &&
        BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, preferSlot) &&
        BattleMember_HasAnyStatus(&state->enemySlots[preferSlot]))
    {
        return preferSlot;
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        if (BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, i) &&
            BattleMember_HasAnyStatus(&state->enemySlots[i]))
        {
            return i;
        }
    }
    return -1;
}

static int BattleEnemyTurn_BuildSkillAction(
    const BattleMember *actor,
    const EnemyAiMoveChoice *choice,
    BattleAction *outAction)
{
    if (!actor || !choice || !outAction || choice->moveId == MOVE_NONE)
    {
        return 0;
    }

    BattleAction_Clear(outAction);
    outAction->type = BATTLE_ACTION_SKILL;
    outAction->actorSide = actor->side;
    outAction->actorSlot = actor->slotIndex;
    outAction->targetSide = choice->targetSide;
    outAction->targetSlot = choice->targetSlot;
    outAction->moveId = choice->moveId;
    outAction->actorMoveSlot = choice->moveSlot;
    outAction->useLpBonus = 0;
    return 1;
}

static int BattleEnemyTurn_BuildTutsilChoice(const BattleState *state, EnemyAiMoveChoice *outChoice)
{
    int tutsilSlot;
    int playerTarget;
    int vortexSlot;
    int destSlot;
    int youngBloodSlot;
    int tempDiffSlot;
    const BattleMember *tutsil;

    if (!state || !outChoice)
    {
        return 0;
    }

    tutsilSlot = BattleEnemyTurn_FindTutsilSlot(state);
    if (tutsilSlot < 0)
    {
        return 0;
    }
    tutsil = &state->enemySlots[tutsilSlot];
    playerTarget = BattleEnemyTurn_FindPreferredPlayerTarget(state);
    vortexSlot = BattleEnemyTurn_FindEquippedMoveSlot(tutsil, MOVE_VORTEX_ESCAPE);
    youngBloodSlot = BattleEnemyTurn_FindEquippedMoveSlot(tutsil, MOVE_YOUNG_BLOOD);
    tempDiffSlot = BattleEnemyTurn_FindEquippedMoveSlot(tutsil, MOVE_TEMPERATURE_DIFFERENCE);

    /*
     * Priority:
     * 1. Vortex Escape when it meaningfully improves Long Shot.
     * 2. One Young Blood (never spam).
     * 3. Long Shot at preferred/any living player target.
     * 4. Temperature Difference only if no damaging Long Shot is available.
     */
    if (s_tutsilVortexCooldown == 0 &&
        vortexSlot >= 0 &&
        playerTarget >= 0 &&
        BattleEnemyTurn_ShouldTutsilVortexForLongShot(
            state,
            tutsilSlot,
            playerTarget,
            &destSlot))
    {
        outChoice->moveId = MOVE_VORTEX_ESCAPE;
        outChoice->moveSlot = vortexSlot;
        outChoice->targetSide = BATTLE_SIDE_ENEMY;
        outChoice->targetSlot = destSlot;
        return 1;
    }

    if (!s_tutsilYoungBloodDone && youngBloodSlot >= 0)
    {
        outChoice->moveId = MOVE_YOUNG_BLOOD;
        outChoice->moveSlot = youngBloodSlot;
        outChoice->targetSide = BATTLE_SIDE_ENEMY;
        outChoice->targetSlot = tutsilSlot;
        return 1;
    }

    if (BattleEnemyTurn_TryBuildTutsilLongShotChoice(state, tutsil, outChoice))
    {
        return 1;
    }

    if (tempDiffSlot >= 0)
    {
        outChoice->moveId = MOVE_TEMPERATURE_DIFFERENCE;
        outChoice->moveSlot = tempDiffSlot;
        outChoice->targetSide = BATTLE_SIDE_ENEMY;
        outChoice->targetSlot = tutsilSlot;
        return 1;
    }

    /* Legacy cycle fallback if kit is incomplete. Prefer damage over support. */
    if (BattleEnemyTurn_TryBuildTutsilLongShotChoice(state, tutsil, outChoice))
    {
        return 1;
    }
    {
        int moveSlot;
        MoveId moveId;

        moveSlot = s_tutsilCycleMoveSlots[s_tutsilMoveCycleStep % BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT];
        moveId = tutsil->equippedMoves[moveSlot];
        if (moveId == MOVE_NONE)
        {
            return 0;
        }
        if (moveId == MOVE_TEMPERATURE_DIFFERENCE ||
            moveId == MOVE_YOUNG_BLOOD ||
            moveId == MOVE_VORTEX_ESCAPE)
        {
            /* Safety: never loop support forever when any living player exists. */
            if (BattleEnemyTurn_TryBuildTutsilLongShotChoice(state, tutsil, outChoice))
            {
                return 1;
            }
        }
        outChoice->moveId = moveId;
        outChoice->moveSlot = moveSlot;
        outChoice->targetSide = BATTLE_SIDE_ENEMY;
        outChoice->targetSlot = tutsilSlot;
        if (moveId == MOVE_VORTEX_ESCAPE)
        {
            destSlot = BattleEnemyTurn_FindBestLongShotDestination(
                state,
                tutsilSlot,
                playerTarget);
            if (destSlot < 0)
            {
                return BattleEnemyTurn_TryBuildTutsilLongShotChoice(state, tutsil, outChoice);
            }
            outChoice->targetSlot = destSlot;
            return 1;
        }
        if (playerTarget >= 0 && moveId == MOVE_LONG_SHOT)
        {
            outChoice->targetSide = BATTLE_SIDE_PLAYER;
            outChoice->targetSlot = playerTarget;
        }
        return 1;
    }
}

static void BattleEnemyTurn_AdvanceTutsilCycle(void)
{
    s_tutsilMoveCycleStep = (unsigned char)((s_tutsilMoveCycleStep + 1) % 4);
}

static int BattleEnemyTurn_FindStaticLockTarget(const BattleState *state)
{
    int slot;

    if (!state)
    {
        return -1;
    }

    slot = BattleState_FindAksilSlot(state);
    if (slot >= 0 &&
        BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot) &&
        !BattleMember_IsMovementLocked(&state->playerSlots[slot]))
    {
        return slot;
    }

    slot = BattleState_FindProtagonistSlot(state);
    if (slot >= 0 &&
        BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot) &&
        !BattleMember_IsMovementLocked(&state->playerSlots[slot]))
    {
        return slot;
    }

    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        if (BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot) &&
            !BattleMember_IsMovementLocked(&state->playerSlots[slot]))
        {
            return slot;
        }
    }
    return -1;
}

static int BattleEnemyTurn_BuildBadgerChoice(
    BattleState *state,
    BattleMember *enemy,
    EnemyAiMoveChoice *outChoice)
{
    int tutsilSlot;
    int statusSlot;
    int playerTarget;
    int lockTarget;
    int slotIndex;
    int staticLockSlot;

    if (!state || !enemy || !outChoice)
    {
        return 0;
    }

    slotIndex = enemy->slotIndex;
    if (slotIndex < 0 || slotIndex >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return 0;
    }

    tutsilSlot = BattleEnemyTurn_FindTutsilSlot(state);
    statusSlot = BattleEnemyTurn_FindStatusAfflictedAllySlot(state, tutsilSlot);

    if (statusSlot >= 0)
    {
        outChoice->moveId = MOVE_ELECTRO_THERAPY;
        outChoice->moveSlot = 1;
        outChoice->targetSide = BATTLE_SIDE_ENEMY;
        outChoice->targetSlot = statusSlot;
        return 1;
    }

    if (tutsilSlot >= 0 && !s_badgerUsedCharge[slotIndex])
    {
        s_badgerUsedCharge[slotIndex] = 1;
        outChoice->moveId = MOVE_CHARGE;
        outChoice->moveSlot = 2;
        outChoice->targetSide = BATTLE_SIDE_ENEMY;
        outChoice->targetSlot = tutsilSlot;
        return 1;
    }

    staticLockSlot = BattleEnemyTurn_FindEquippedMoveSlot(enemy, MOVE_STATIC_LOCK);
    lockTarget = BattleEnemyTurn_FindStaticLockTarget(state);
    if (staticLockSlot >= 0 &&
        lockTarget >= 0 &&
        !s_staticLockUsedThisEnemyTurn)
    {
        s_staticLockUsedThisEnemyTurn = 1;
        outChoice->moveId = MOVE_STATIC_LOCK;
        outChoice->moveSlot = staticLockSlot;
        outChoice->targetSide = BATTLE_SIDE_PLAYER;
        outChoice->targetSlot = lockTarget;
        return 1;
    }

    playerTarget = BattleEnemyTurn_FindPreferredPlayerTarget(state);
    if (playerTarget < 0)
    {
        return 0;
    }
    outChoice->moveId = MOVE_HEADBUTT;
    outChoice->moveSlot = 0;
    outChoice->targetSide = BATTLE_SIDE_PLAYER;
    outChoice->targetSlot = playerTarget;
    return 1;
}

static int BattleEnemyTurn_BuildAiChoice(
    BattleState *state,
    BattleMember *enemy,
    EnemyAiMoveChoice *outChoice)
{
    if (!state || !enemy || !outChoice)
    {
        return 0;
    }
    if (BattleMember_IsTutsil(enemy))
    {
        return BattleEnemyTurn_BuildTutsilChoice(state, outChoice);
    }
    if (BattleMember_IsElectricArmorBadger(enemy))
    {
        return BattleEnemyTurn_BuildBadgerChoice(state, enemy, outChoice);
    }
    return 0;
}

static int BattleEnemyTurn_TryMatterStatusBeforeAction(BattleMember *enemy)
{
    int cursor = 0;
    int selfDamage;

    if (!enemy)
    {
        return 0;
    }
    if (BattleMember_ShouldLoseActionToParalysis(enemy))
    {
        s_enemyTurnMessage[0] = '\0';
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, enemy->name);
        BattleEnemyTurn_AppendString(
            s_enemyTurnMessage,
            &cursor,
            " is paralyzed!");
        s_enemyTurnMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_enemyTurnMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        return 1;
    }
    if (BattleMember_ShouldConfusedSelfHit(enemy))
    {
        selfDamage = BattleMember_ComputeConfusedSelfDamage(enemy);
        s_enemyTurnMessage[0] = '\0';
        cursor = 0;
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, enemy->name);
        BattleEnemyTurn_AppendString(
            s_enemyTurnMessage,
            &cursor,
            " hurt self in confusion!");
        s_enemyTurnMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_enemyTurnMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        if (selfDamage > 0)
        {
            BattleMember_TakeDamage(enemy, selfDamage);
        }
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
        return 1;
    }
    return 0;
}

static int BattleEnemyTurn_TryInstinctStatusBeforeAction(BattleState *state, BattleMember *enemy)
{
    int cursor = 0;
    int targetSlot;
    BattleMember *panicTarget;
    int damage;
    int hitSelf;

    if (!state || !enemy || !enemy->isAlive)
    {
        return 0;
    }
    if (!BattleMember_ShouldConfused2HitAlly(enemy))
    {
        return 0;
    }
    targetSlot = BattleState_FindRandomAliveAllySlot(state, enemy->side, enemy->slotIndex, 1);
    if (targetSlot < 0)
    {
        return 0;
    }
    panicTarget = BattleState_GetMemberAtSlot(state, enemy->side, targetSlot);
    if (!panicTarget)
    {
        return 0;
    }
    damage = BattleMember_ComputeConfusedSelfDamage(enemy);
    hitSelf = targetSlot == enemy->slotIndex;
    s_enemyTurnMessage[0] = '\0';
    BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, enemy->name);
    if (hitSelf)
    {
        BattleEnemyTurn_AppendString(
            s_enemyTurnMessage,
            &cursor,
            " lashed out in confusion!");
    }
    else
    {
        BattleEnemyTurn_AppendString(
            s_enemyTurnMessage,
            &cursor,
            " struck an ally in panic!");
    }
    s_enemyTurnMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_enemyTurnMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    if (damage > 0)
    {
        BattleMember_TakeDamage(panicTarget, damage);
    }
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    return 1;
}

static void BattleEnemyTurn_ApplyActorTurnRecovery(BattleState *state, BattleMember *actor)
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
    s_enemyTurnMessage[0] = '\0';
    BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, actor->name);
    if (recovered == BATTLE_STATUS_FROZEN)
    {
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, " thawed out!");
    }
    else
    {
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, " stabilized!");
    }
    s_enemyTurnMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_enemyTurnMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL);
}

static int BattleEnemyTurn_TryBuildEncoreAction(
    BattleState *state,
    BattleMember *enemy,
    BattleAction *outAction)
{
    int cursor = 0;

    if (!BattleMember_BuildEncoreForcedAction(enemy, state, outAction))
    {
        s_enemyTurnMessage[0] = '\0';
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, enemy->name);
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, " has no target!");
        s_enemyTurnMessage[cursor] = '\0';
        BattleMessageQueue_Enqueue(s_enemyTurnMessage);
        return 0;
    }
    if (outAction->type != BATTLE_ACTION_ATTACK &&
        outAction->type != BATTLE_ACTION_SKILL)
    {
        BattleMessageQueue_Enqueue("The move had no effect.");
        return 0;
    }
    return 1;
}

static int BattleEnemyTurn_ExecuteMemberAction(BattleState *state, BattleMember *enemy, int hadEncore)
{
    BattleAction action;
    EnemyAiMoveChoice choice;
    int cursor = 0;

    if (!enemy || !enemy->isAlive)
    {
        return 0;
    }

    if (BattleEnemyTurn_TryMatterStatusBeforeAction(enemy) ||
        BattleEnemyTurn_TryInstinctStatusBeforeAction(state, enemy))
    {
        BattleEnemyTurn_ApplyActorTurnRecovery(state, enemy);
        return 1;
    }

    if (hadEncore)
    {
        if (!BattleEnemyTurn_TryBuildEncoreAction(state, enemy, &action))
        {
            return 0;
        }
    }
    else if (BattleEnemyTurn_BuildAiChoice(state, enemy, &choice))
    {
        if (!BattleEnemyTurn_ChoiceHasValidTarget(state, enemy, &choice))
        {
            /*
             * Solo Tutsil previously selected Vortex (empty dest) then failed
             * the living-occupied check and skipped forever. Fall back to Long Shot.
             */
            if (BattleMember_IsTutsil(enemy) &&
                BattleEnemyTurn_TryBuildTutsilLongShotChoice(state, enemy, &choice) &&
                BattleEnemyTurn_ChoiceHasValidTarget(state, enemy, &choice))
            {
                /* Use Long Shot fallback below. */
            }
            else
            {
                s_enemyTurnMessage[0] = '\0';
                BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, enemy->name);
                BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, " has no target!");
                s_enemyTurnMessage[cursor] = '\0';
                BattleMessageQueue_Enqueue(s_enemyTurnMessage);
                return 0;
            }
        }
        if (!BattleEnemyTurn_BuildSkillAction(enemy, &choice, &action))
        {
            return 0;
        }
        if (BattleMember_IsTutsil(enemy))
        {
            BattleEnemyTurn_AdvanceTutsilCycle();
        }
    }
    else
    {
        int targetSlot = BattleEnemyTurn_FindPreferredPlayerTarget(state);

        if (targetSlot < 0)
        {
            return 0;
        }
        BattleAction_Clear(&action);
        action.type = BATTLE_ACTION_ATTACK;
        action.actorSide = BATTLE_SIDE_ENEMY;
        action.actorSlot = enemy->slotIndex;
        action.targetSide = BATTLE_SIDE_PLAYER;
        action.targetSlot = targetSlot;
    }

    BattleResolve_Execute(state, &action);

    if (BattleMember_IsTutsil(enemy) && action.type == BATTLE_ACTION_SKILL)
    {
        if (action.moveId == MOVE_VORTEX_ESCAPE)
        {
            s_tutsilVortexCooldown = TUTSIL_VORTEX_COOLDOWN_TURNS;
        }
        else if (action.moveId == MOVE_YOUNG_BLOOD)
        {
            s_tutsilYoungBloodDone = 1;
        }
    }
    return 1;
}

static int BattleEnemyTurn_ExecuteMemberTurn(BattleState *state, BattleMember *enemy)
{
    int extraActions;
    int actionsThisTurn;
    int actionIndex;
    int hadEncore;
    int cursor = 0;

    if (!state || !enemy || !enemy->isAlive)
    {
        return 0;
    }

    /*
     * Boss Tutsil Vortex Escape recovery: skip the entire next turn
     * (no move, attack, or other action), then clear and resume normally.
     */
    if (BattleMember_IsTutsil(enemy) && BattleMember_GetSkipNextTurn(enemy))
    {
        BattleMember_ClearSkipNextTurn(enemy);
        if (s_tutsilVortexCooldown > 0)
        {
            s_tutsilVortexCooldown--;
        }
        s_enemyTurnMessage[0] = '\0';
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, enemy->name);
        BattleEnemyTurn_AppendString(s_enemyTurnMessage, &cursor, " is recovering!");
        s_enemyTurnMessage[cursor] = '\0';
        BattleMessageQueue_Enqueue(s_enemyTurnMessage);
        return 1;
    }

    hadEncore = BattleMember_HasEncore(enemy);
    if (BattleMember_IsTutsil(enemy))
    {
        if (s_tutsilVortexCooldown > 0)
        {
            s_tutsilVortexCooldown--;
        }
    }

    extraActions = BattleMember_GetExtraActionsNextTurn(enemy);
    if (extraActions > BATTLE_ENEMY_EXTRA_ACTIONS_MAX)
    {
        extraActions = BATTLE_ENEMY_EXTRA_ACTIONS_MAX;
    }
    BattleMember_ClearExtraActionsNextTurn(enemy);

    if (hadEncore)
    {
        actionsThisTurn = 1;
    }
    else
    {
        actionsThisTurn = 1 + extraActions;
    }

    for (actionIndex = 0; actionIndex < actionsThisTurn; actionIndex++)
    {
        if (!enemy->isAlive)
        {
            break;
        }
        BattleEnemyTurn_ExecuteMemberAction(state, enemy, hadEncore);
        if (hadEncore)
        {
            BattleMember_TickEncoreAfterAction(enemy);
            break;
        }
    }

    return 1;
}

void BattleEnemyTurn_Init(void)
{
    BattleEnemyTurn_ResetAiState();
}

void BattleEnemyTurn_ResetAiState(void)
{
    int i;

    s_tutsilMoveCycleStep = 0;
    s_tutsilVortexCooldown = 0;
    s_tutsilYoungBloodDone = 0;
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        s_badgerUsedCharge[i] = 0;
    }
    s_staticLockUsedThisEnemyTurn = 0;
}

void BattleEnemyTurn_RekeyEnemySlot(int fromSlot, int toSlot)
{
    unsigned char usedCharge;

    if (fromSlot < 0 || fromSlot >= BATTLE_ENEMY_SLOT_COUNT ||
        toSlot < 0 || toSlot >= BATTLE_ENEMY_SLOT_COUNT ||
        fromSlot == toSlot)
    {
        return;
    }
    usedCharge = s_badgerUsedCharge[fromSlot];
    s_badgerUsedCharge[fromSlot] = 0;
    s_badgerUsedCharge[toSlot] = usedCharge;
}

void BattleEnemyTurn_Begin(BattleState *state)
{
    int targetSlot;

    targetSlot = BattleEnemyTurn_FindPreferredPlayerTarget(state);
    if (targetSlot >= 0)
    {
        state->activePlayerSlot = targetSlot;
    }
    BattleState_SetPhase(state, BATTLE_PHASE_ENEMY_TURN);
}

int BattleEnemyTurn_Execute(BattleState *state)
{
    int slot;

    if (!state)
    {
        return 0;
    }

    s_staticLockUsedThisEnemyTurn = 0;

    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, slot))
        {
            continue;
        }
        state->activeEnemySlot = slot;
        BattleEnemyTurn_ExecuteMemberTurn(state, &state->enemySlots[slot]);
        if (BattleState_AllPlayersDefeated(state))
        {
            return 1;
        }
    }

    return BattleState_AllPlayersDefeated(state);
}
