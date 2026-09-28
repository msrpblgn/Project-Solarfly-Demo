#include "data/item_database.h"

const ItemData g_ItemDatabase[] = {
    {
        ITEM_NONE,
        "None",
        ITEM_TARGET_NONE,
        0,
        0,
        ""
    },
    {
        ITEM_POTION,
        "Potion",
        ITEM_TARGET_ALLY,
        30,
        0,
        "Restores 30 HP."
    },
    {
        ITEM_CURE,
        "Cure",
        ITEM_TARGET_ALLY,
        0,
        1,
        "Removes status conditions."
    }
};

const int g_ItemDatabaseCount = 3;

int ItemData_IsValid(ItemId id)
{
    return id > ITEM_NONE && id < ITEM_COUNT;
}

const ItemData *ItemData_Get(ItemId id)
{
    int i;

    if (!ItemData_IsValid(id))
    {
        return 0;
    }
    for (i = 0; i < g_ItemDatabaseCount; i++)
    {
        if (g_ItemDatabase[i].id == id)
        {
            return &g_ItemDatabase[i];
        }
    }
    return 0;
}
