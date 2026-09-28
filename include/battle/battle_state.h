#ifndef BATTLE_STATE_H
#define BATTLE_STATE_H

#include "battle/battle_member.h"
#include "battle/battle_item.h"
#include "battle/battle_slot.h"

#define BATTLE_BONUS_ACTION_SLOT_NONE (-1)
#define BATTLE_VEIL_TILE_COUNT 3
#define BATTLE_VEIL_TILE_DURATION_TURNS 3
#define BATTLE_VEIL_CRIT_PERMILLE_PER_STAGE 100

typedef enum BattlePhase {
    BATTLE_PHASE_INIT,
    BATTLE_PHASE_INTRO_MESSAGE,
    BATTLE_PHASE_PLAYER_COMMAND,
    BATTLE_PHASE_PARTY_SCREEN,
    BATTLE_PHASE_PLAYER_SKILL,
    BATTLE_PHASE_PLAYER_ITEM,
    BATTLE_PHASE_TARGET_SELECT,
    BATTLE_PHASE_LP_PROMPT,
    BATTLE_PHASE_RESOLVE_ACTION,
    BATTLE_PHASE_MESSAGE_QUEUE,
    BATTLE_PHASE_RUN_FAILED,
    BATTLE_PHASE_ENEMY_TURN,
    BATTLE_PHASE_VICTORY,
    BATTLE_PHASE_DEFEAT,
    BATTLE_PHASE_ESCAPE
} BattlePhase;

typedef struct BattleState {
    BattlePhase phase;
    BattleMember playerSlots[BATTLE_PLAYER_SLOT_COUNT];
    BattleMember enemySlots[BATTLE_ENEMY_SLOT_COUNT];
    BattleTileEffect playerTileEffects[BATTLE_PLAYER_SLOT_COUNT];
    BattleTileEffect enemyTileEffects[BATTLE_ENEMY_SLOT_COUNT];
    /* Active Veil luck spots: 0 = none; >0 = remaining end-of-round ticks. */
    unsigned char playerVeilTileTurns[BATTLE_PLAYER_SLOT_COUNT];
    unsigned char enemyVeilTileTurns[BATTLE_ENEMY_SLOT_COUNT];
    int activePlayerSlot;
    int activeEnemySlot;
    int frameCounter;
    int menuSelection;
    int skillMenuSelection;
    int itemMenuSelection;
    int targetSlot;
    int targetSide;
    int lpPromptSelection;
    BattleEffectivenessReaction enemyEffectReactions[BATTLE_ENEMY_SLOT_COUNT];
    /* Per-player-slot acted flags for the upcoming controllable ally turn scheduler. Data-only until 18C wires turn flow. */
    int playerSlotActed[BATTLE_PLAYER_SLOT_COUNT];
    int bonusActionPlayerSlot;
    unsigned char allyNextMoveCannotMiss;
    unsigned char isPlayerBonusCommand;
    int itemCounts[ITEM_COUNT];
} BattleState;

void BattleState_Init(BattleState *state);
void BattleState_SetPhase(BattleState *state, BattlePhase phase);
int BattleState_GetMeanOpponentSpeed(const BattleState *state, int runnerSide);
int BattleState_ComputeRunChancePercent(const BattleState *state, int runnerSide, int runnerSlot);
int BattleState_AllEnemiesDefeated(const BattleState *state);
int BattleState_AllPlayersDefeated(const BattleState *state);
int BattleState_IsSlotValid(int side, int slotIndex);
BattleMember *BattleState_GetMemberAtSlot(BattleState *state, int side, int slotIndex);
const BattleMember *BattleState_GetMemberAtSlotConst(const BattleState *state, int side, int slotIndex);
int BattleState_IsSlotOccupied(const BattleState *state, int side, int slotIndex);
int BattleState_IsAliveOccupiedSlot(const BattleState *state, int side, int slotIndex);
int BattleState_FindFirstEmptySlot(const BattleState *state, int side);
int BattleState_CanMoveMemberToSlot(const BattleState *state, int side, int fromSlot, int toSlot);
int BattleState_MoveMemberToSlot(BattleState *state, int side, int fromSlot, int toSlot);
int BattleState_CanSwapMembersOnSide(const BattleState *state, int side, int slotA, int slotB);
int BattleState_SwapMembersOnSide(BattleState *state, int side, int slotA, int slotB);
int BattleState_DebugValidateSlotInvariants(const BattleState *state);
int BattleState_IsProtagonistMember(const BattleMember *member);
int BattleState_FindProtagonistSlot(const BattleState *state);
int BattleState_FindAksilSlot(const BattleState *state);
int BattleState_FindEnemyTargetPlayerSlot(const BattleState *state);
int BattleState_FindAlivePlayerSlot(const BattleState *state, int preferredSlot);
int BattleState_FindRandomAliveAllySlot(
    const BattleState *state,
    int side,
    int actorSlot,
    int excludeSelf);
void BattleState_ClearTileEffects(BattleState *state);
const BattleTileEffect *BattleState_GetTileEffect(const BattleState *state, int side, int slotIndex);
int BattleState_HasTileEffect(const BattleState *state, int side, int slotIndex);
void BattleState_SetTileEffect(BattleState *state, int side, int slotIndex, BattleTileEffectId effectId, int ownerSide, int durationTurns);
void BattleState_ClearTileEffect(BattleState *state, int side, int slotIndex);
void BattleState_ClearVeilTiles(BattleState *state);
void BattleState_PlaceRandomVeilTiles(BattleState *state);
int BattleState_HasVeilTile(const BattleState *state, int side, int slotIndex);
int BattleState_GetVeilCritStageBonus(
    const BattleState *state,
    int attackerSide,
    int attackerSlot,
    int targetSide,
    int targetSlot);
int BattleState_GetVeilCritBonusPermille(
    const BattleState *state,
    int attackerSide,
    int attackerSlot,
    int targetSide,
    int targetSlot);
void BattleState_TickVeilTiles(BattleState *state);
int BattleState_TickKoBurrowAnimations(BattleState *state);
unsigned int BattleState_GetKoBurrowDirtyPlayerMask(const BattleState *state);
unsigned int BattleState_GetKoBurrowDirtyEnemyMask(const BattleState *state);
void BattleState_ClearKoBurrowDirtyMasks(BattleState *state);
void BattleState_ClearEnemyEffectReactions(BattleState *state);
void BattleState_SetEnemyEffectReaction(
    BattleState *state,
    int enemySlot,
    BattleEffectivenessReaction reaction);
BattleEffectivenessReaction BattleState_GetEnemyEffectReaction(
    const BattleState *state,
    int enemySlot);
void BattleState_ClearPlayerTurnFlags(BattleState *state);
void BattleState_SetPlayerSlotActed(BattleState *state, int slot, int acted);
void BattleState_MarkPlayerSlotActed(BattleState *state, int slot);
int BattleState_IsPlayerSlotActed(const BattleState *state, int slot);
int BattleState_FindNextLivingUnactedPlayerSlot(const BattleState *state, int startAfterSlot);
void BattleState_RekeyPlayerSlotActed(BattleState *state, int fromSlot, int toSlot);
void BattleState_SetAllyNextMoveCannotMiss(BattleState *state, int enabled);
int BattleState_HasAllyNextMoveCannotMiss(const BattleState *state);
int BattleState_TryConsumeAllyNextMoveCannotMiss(BattleState *state);
void BattleState_ClearInventory(BattleState *state);
int BattleState_GetItemCount(const BattleState *state, ItemId itemId);
void BattleState_SetItemCount(BattleState *state, ItemId itemId, int count);
int BattleState_HasAnyItems(const BattleState *state);
int BattleState_DecrementItem(BattleState *state, ItemId itemId);

#endif
