#ifndef BATTLE_MEMBER_H
#define BATTLE_MEMBER_H

#include "battle/battle_action.h"
#include "battle/battle_move.h"
#include "battle/battle_race.h"
#include "battle/battle_status.h"
#include "data/status_database.h"

/*
 * Move storage vs usable equipped skills:
 * - BATTLE_MEMBER_MOVE_SLOT_COUNT (4): physical storage layout for equippedMoves /
 *   moveSc and compatible formation/RAM copies. Do not shrink in this pass.
 * - BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT (3): authoritative max usable skills.
 *   Only indices 0..2 may be equipped, selected, recovered, or executed.
 *   Storage index 3 is reserved, always empty, zero SC, hidden from UI/AI/Custom.
 * Later physical cleanup of the reserved cell is optional and out of scope here.
 */
#define BATTLE_MEMBER_MOVE_SLOT_COUNT 4
#define BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT 3
#define BATTLE_MEMBER_RESERVED_MOVE_SLOT 3
#define BATTLE_MEMBER_LEVEL_MIN 1
#define BATTLE_MEMBER_LEVEL_MAX 100
#define BATTLE_MEMBER_BRACE_SC_GAIN 2
#define BATTLE_MEMBER_GUARD_DEFENCE_MULTIPLIER_PERCENT 150
#define BATTLE_JAVELIN_SLOW_PERCENT -15
#define BATTLE_STATUS_BURN_ATTACK_MULTIPLIER_PERCENT 80
#define BATTLE_STATUS_CURSE_DEFENCE_MULTIPLIER_PERCENT 80
#define BATTLE_STATUS_PARALYSIS_SKIP_CHANCE_PERCENT 33
#define BATTLE_STATUS_FROZEN_SPEED_MULTIPLIER_PERCENT 50
#define BATTLE_STATUS_CONFUSED_SELF_HIT_CHANCE_PERCENT 25
#define BATTLE_STATUS_CONFUSED2_ALLY_HIT_CHANCE_PERCENT 25
#define BATTLE_STATUS_RECOVER_CHANCE_PERCENT 33
#define BATTLE_STATUS_DURATION_PERMANENT 0
#define BATTLE_STATUS_DOWNED_DURATION_TURNS 1
#define BATTLE_MEMBER_KO_BURROW_FRAMES 6

#define BATTLE_STAT_MOD_SLOT_COUNT 4
#define BATTLE_STAT_MOD_DURATION_BATTLE_LONG 0
#define BATTLE_ENCORE_RETARGET (-1)

typedef enum BattleStatId {
    BATTLE_STAT_NONE = 0,
    BATTLE_STAT_ATTACK,
    BATTLE_STAT_DEFENCE,
    BATTLE_STAT_SPEED,
    BATTLE_STAT_EVASIVENESS
} BattleStatId;

typedef struct BattleStatMod {
    BattleStatId stat;
    signed char percent;
    unsigned char stacks;
    unsigned char maxStacks;
    unsigned char durationTurns;
} BattleStatMod;

typedef struct BattleStats {
    int attack;
    int defence;
    int speed;
    int evasiveness;
    int luck;
} BattleStats;

typedef enum MemberWeaponKind {
    MEMBER_WEAPON_KIND_NONE = 0,
    MEMBER_WEAPON_KIND_SLINGSHOT,
    MEMBER_WEAPON_KIND_SWORD,
    MEMBER_WEAPON_KIND_JAVELIN
} MemberWeaponKind;

typedef struct MemberTemplate MemberTemplate;
typedef struct BattleState BattleState;

typedef struct BattleMember {
    const char *name;
    int currentHp;
    int maxHp;
    int currentLp;
    int maxLp;
    int slotIndex;
    int side;
    BattleElementId element;
    MemberWeaponKind weaponKind;
    BattleRaceId race;
    BattleElementId pendingWeaponAttackElement;
    BattleElementId pendingNextMoveElement;
    unsigned char extraActionsNextTurn;
    /* Tutsil Vortex Escape recovery: skip the entire next turn, then clear. */
    unsigned char skipNextTurn;
    int isAlive;
    /* 1 = KO'd / vacated tile; struct kept for bookkeeping. */
    unsigned char removedFromField;
    unsigned char koBurrowFramesRemaining;
    signed char koBurrowYOffset;
    int luckStage;
    int guardNextHit;
    int bubbleProtectionActive;
    unsigned char javelinSlowActive;
    /* Unused after 41F-E: Veil is tile-based on BattleState. */
    unsigned char veilLuckStacks;
    unsigned char veilDurationTurns;
    unsigned char level;
    unsigned char skillTreePoints;
    unsigned char movementLockTurns;
    unsigned char incomingDamageReductionPercent;
    unsigned char incomingDamageReductionTurns;
    unsigned char pendingNextDamageDouble;
    int pendingAttackHitsAllEnemies;
    BattleActionType lastResolvedActionType;
    MoveId lastResolvedMoveId;
    int lastResolvedMoveSlot;
    int lastResolvedTargetSide;
    int lastResolvedTargetSlot;
    int lastResolvedUseLpBonus;
    unsigned char encoreTurnsRemaining;
    BattleActionType encoredActionType;
    MoveId encoredMoveId;
    int encoredMoveSlot;
    int encoredTargetSide;
    int encoredTargetSlot;
    int encoredUseLpBonus;
    BattleStats stats;
    MoveId equippedMoves[BATTLE_MEMBER_MOVE_SLOT_COUNT];
    int moveSc[BATTLE_MEMBER_MOVE_SLOT_COUNT];
    BattleStatusId statuses[BATTLE_MEMBER_STATUS_SLOT_COUNT];
    int statusDurations[BATTLE_MEMBER_STATUS_SLOT_COUNT];
    BattleStatusId balanceStatus;
    int poisonSeverity;
    BattleStatMod statMods[BATTLE_STAT_MOD_SLOT_COUNT];
} BattleMember;

void BattleMember_Init(BattleMember *member, const char *name, int maxHp, int maxLp, int slotIndex, int side);
void BattleMember_InitEmpty(BattleMember *member, int slotIndex, int side);
void BattleMember_InitFromTemplate(BattleMember *member, const MemberTemplate *template, int slotIndex, int side);
int BattleMember_GetAttack(const BattleMember *member);
int BattleMember_GetEffectiveAttack(const BattleMember *member);
int BattleMember_GetDefence(const BattleMember *member);
int BattleMember_GetSpeed(const BattleMember *member);
int BattleMember_GetEffectiveSpeed(const BattleMember *member);
int BattleMember_ShouldLoseActionToParalysis(const BattleMember *member);
int BattleMember_ShouldConfusedSelfHit(const BattleMember *member);
int BattleMember_ShouldConfused2HitAlly(const BattleMember *member);
int BattleMember_ComputeConfusedSelfDamage(const BattleMember *member);
int BattleMember_GetEvasiveness(const BattleMember *member);
int BattleMember_GetEffectiveEvasiveness(const BattleMember *member);
int BattleMember_GetLuck(const BattleMember *member);
void BattleMember_ClearStatMods(BattleMember *member);
int BattleMember_ApplyStatMod(
    BattleMember *member,
    BattleStatId stat,
    int percent,
    int maxStacks,
    int durationTurns);
void BattleMember_TickStatMods(BattleMember *member);
int BattleMember_GetStatModPercent(const BattleMember *member, BattleStatId stat);
int BattleMember_CanApplyStatMod(const BattleMember *member, BattleStatId stat, int maxStacks);
void BattleMember_RemoveStatModForStat(BattleMember *member, BattleStatId stat);
BattleElementId BattleMember_GetElement(const BattleMember *member);
/* Element used by the weapon/basic ATTACK command (not an equipped skill). */
BattleElementId BattleMember_GetBasicAttackElement(const BattleMember *member);
BattleRaceId BattleMember_GetRace(const BattleMember *member);
BattleStatusId BattleMember_GetStatusAt(const BattleMember *member, int slot);
BattleStatusId BattleMember_GetBalanceStatus(const BattleMember *member);
int BattleMember_HasPrimaryTrioStatus(const BattleMember *member, StatusTrio trio);
int BattleMember_HasAllPrimaryStatusTrios(const BattleMember *member);
int BattleMember_HasBalanceStatus(const BattleMember *member);
int BattleMember_IsAksil(const BattleMember *member);
int BattleMember_IsMaren(const BattleMember *member);
int BattleMember_IsProtagonist(const BattleMember *member);
int BattleMember_IsTutsil(const BattleMember *member);
int BattleMember_IsElectricArmorBadger(const BattleMember *member);
int BattleMember_GetLevel(const BattleMember *member);
void BattleMember_SetLevel(BattleMember *member, int level);
int BattleMember_GetSkillTreePoints(const BattleMember *member);
void BattleMember_FormatNameWithLevel(
    const BattleMember *member,
    char *dest,
    int destSize);
int BattleMember_GetStatusSlotForTrio(StatusTrio trio);
int BattleMember_GetStatusDurationAt(const BattleMember *member, int slot);
void BattleMember_SetStatusDurationAt(BattleMember *member, int slot, int duration);
void BattleMember_ClearStatusSlotMetadata(BattleMember *member, int slot);
int BattleMember_GetDurationForNewStatus(BattleStatusId status);
int BattleMember_SetStatusAt(BattleMember *member, int slot, BattleStatusId status);
void BattleMember_ClearStatuses(BattleMember *member);
int BattleMember_HasStatus(const BattleMember *member, BattleStatusId status);
int BattleMember_IsDowned(const BattleMember *member);
int BattleMember_TickDownedDuration(BattleMember *member);
BattleStatusId BattleMember_TryRecoverAfterActorTurn(BattleMember *member);
int BattleMember_HasAnyStatus(const BattleMember *member);
int BattleMember_GetFirstEmptyStatusSlot(const BattleMember *member);
int BattleMember_AddStatus(BattleMember *member, BattleStatusId status);
int BattleMember_RemoveStatus(BattleMember *member, BattleStatusId status);
int BattleMember_GetEffectiveDefence(const BattleMember *member);
void BattleMember_SetGuardNextHit(BattleMember *member, int enabled);
int BattleMember_HasGuardNextHit(const BattleMember *member);
void BattleMember_ClearGuardNextHit(BattleMember *member);
void BattleMember_SetBubbleProtection(BattleMember *member, int enabled);
int BattleMember_HasBubbleProtection(const BattleMember *member);
void BattleMember_ClearBubbleProtection(BattleMember *member);
int BattleMember_ApplyBubbleDamageConversion(BattleMember *member, int damage);
int BattleMember_HasJavelinSlow(const BattleMember *member);
void BattleMember_SetJavelinSlow(BattleMember *member, int active);
int BattleMember_IsMovementLocked(const BattleMember *member);
void BattleMember_SetMovementLock(BattleMember *member, int turns);
void BattleMember_TickMovementLock(BattleMember *member);
void BattleMember_ClearMovementLock(BattleMember *member);
void BattleMember_SetIncomingDamageReduction(BattleMember *member, int percent, int turns);
int BattleMember_GetIncomingDamageReductionPercent(const BattleMember *member);
void BattleMember_TickIncomingDamageReduction(BattleMember *member);
void BattleMember_ClearIncomingDamageReduction(BattleMember *member);
void BattleMember_SetPendingAttackHitsAllEnemies(BattleMember *member, int enabled);
int BattleMember_HasPendingAttackHitsAllEnemies(const BattleMember *member);
void BattleMember_ClearPendingAttackHitsAllEnemies(BattleMember *member);
void BattleMember_SetPendingNextDamageDouble(BattleMember *member, int enabled);
int BattleMember_HasPendingNextDamageDouble(const BattleMember *member);
void BattleMember_ClearPendingNextDamageDouble(BattleMember *member);
void BattleMember_SetPendingWeaponAttackElement(BattleMember *member, BattleElementId element);
int BattleMember_HasPendingWeaponAttackElement(const BattleMember *member);
BattleElementId BattleMember_GetPendingWeaponAttackElement(const BattleMember *member);
void BattleMember_ClearPendingWeaponAttackElement(BattleMember *member);
void BattleMember_SetPendingNextMoveElement(BattleMember *member, BattleElementId element);
int BattleMember_HasPendingNextMoveElement(const BattleMember *member);
BattleElementId BattleMember_GetPendingNextMoveElement(const BattleMember *member);
void BattleMember_ClearPendingNextMoveElement(BattleMember *member);
void BattleMember_SetExtraActionsNextTurn(BattleMember *member, int count);
int BattleMember_GetExtraActionsNextTurn(const BattleMember *member);
void BattleMember_ClearExtraActionsNextTurn(BattleMember *member);
void BattleMember_SetSkipNextTurn(BattleMember *member, int enabled);
int BattleMember_GetSkipNextTurn(const BattleMember *member);
void BattleMember_ClearSkipNextTurn(BattleMember *member);
int BattleMember_HasQualifyingLastAction(const BattleMember *member);
int BattleMember_LastActionSupportsEncoreReplayV1(const BattleMember *member);
void BattleMember_RecordResolvedAction(BattleMember *member, const BattleAction *action);
int BattleMember_HasEncore(const BattleMember *member);
void BattleMember_ClearEncore(BattleMember *member);
int BattleMember_ApplyEncoreFromLastAction(BattleMember *member, int durationTurns);
void BattleMember_TickEncoreAfterAction(BattleMember *member);
int BattleMember_BuildEncoreForcedAction(
    const BattleMember *member,
    const BattleState *state,
    BattleAction *outAction);
void BattleMember_ClearReservedMoveSlot(BattleMember *member);
void BattleMember_EquipMove(BattleMember *member, int moveSlot, MoveId moveId, int startingSc);
int BattleMember_IsEquippableMoveSlot(int moveSlot);
int BattleMember_CanUseMoveSlot(const BattleMember *member, int moveSlot);
int BattleMember_SpendMoveSc(BattleMember *member, int moveSlot, int amount);
int BattleMember_SpendLp(BattleMember *member, int amount);
int BattleMember_GainLp(BattleMember *member, int amount);
void BattleMember_MaxAllMoveSc(BattleMember *member);
void BattleMember_BraceRecoverAllMoves(BattleMember *member, int amount);
void BattleMember_TakeDamage(BattleMember *member, int amount);
int BattleMember_Heal(BattleMember *member, int amount);
int BattleMember_IsRemovedFromField(const BattleMember *member);
int BattleMember_IsKoBurrowing(const BattleMember *member);
void BattleMember_StartKoBurrow(BattleMember *member);
int BattleMember_TickKoBurrow(BattleMember *member);

/*
 * Compile-time layout guards for the storage-vs-usable split.
 * Storage stays 4; usable equipped skills are 3; reserved index is 3.
 */
typedef char solarfly_move_storage_is_four[
    (BATTLE_MEMBER_MOVE_SLOT_COUNT == 4) ? 1 : -1];
typedef char solarfly_move_equipped_is_three[
    (BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT == 3) ? 1 : -1];
typedef char solarfly_move_reserved_is_index_three[
    (BATTLE_MEMBER_RESERVED_MOVE_SLOT == 3) ? 1 : -1];
typedef char solarfly_move_equipped_lt_storage[
    (BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT < BATTLE_MEMBER_MOVE_SLOT_COUNT)
        ? 1
        : -1];

#endif
