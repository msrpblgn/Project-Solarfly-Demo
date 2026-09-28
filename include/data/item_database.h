#ifndef ITEM_DATABASE_H
#define ITEM_DATABASE_H

#include "battle/battle_item.h"

extern const ItemData g_ItemDatabase[];
extern const int g_ItemDatabaseCount;

const ItemData *ItemData_Get(ItemId id);
int ItemData_IsValid(ItemId id);

#endif
