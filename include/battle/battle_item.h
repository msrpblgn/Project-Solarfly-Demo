#ifndef BATTLE_ITEM_H
#define BATTLE_ITEM_H

typedef enum ItemId {
    ITEM_NONE = 0,
    ITEM_POTION,
    ITEM_CURE,
    ITEM_COUNT
} ItemId;

typedef enum ItemTargetType {
    ITEM_TARGET_NONE = 0,
    ITEM_TARGET_ALLY
} ItemTargetType;

typedef struct ItemData {
    ItemId id;
    const char *name;
    ItemTargetType targetType;
    int healAmount;
    int curesStatus;
    const char *description;
} ItemData;

#endif
