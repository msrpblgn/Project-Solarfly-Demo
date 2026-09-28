#ifndef BATTLE_SLOT_H
#define BATTLE_SLOT_H

#define BATTLE_PLAYER_SLOT_COUNT 5
#define BATTLE_ENEMY_SLOT_COUNT 5
#define BATTLE_CENTER_SLOT 2
#define BATTLE_TOTAL_SLOT_COUNT (BATTLE_PLAYER_SLOT_COUNT + BATTLE_ENEMY_SLOT_COUNT)

#define BATTLE_SIDE_PLAYER 0
#define BATTLE_SIDE_ENEMY 1

#define BATTLE_SLOT_DISTANCE_INVALID (-1)

typedef enum BattleTileEffectId {
    BATTLE_TILE_EFFECT_NONE = 0,
    BATTLE_TILE_EFFECT_TRAP_TEST,
    BATTLE_TILE_EFFECT_VEIL_TEST
} BattleTileEffectId;

typedef struct BattleTileEffect {
    BattleTileEffectId effectId;
    int ownerSide;
    int durationTurns;
} BattleTileEffect;

void BattleTileEffect_InitEmpty(BattleTileEffect *effect);

int BattleSlot_GetSlotCount(int side);
int BattleSlot_GetDistance(int sideA, int slotA, int sideB, int slotB);

#endif
