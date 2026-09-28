#include "battle_ui/battle_slot_layout.h"
#include "battle_ui/hud_layout.h"
#include "battle/battle_member.h"
#include "assets/protagonist_front_sprite.h"
#include "assets/aksil_front_sprite.h"
#include "assets/maren_front_sprite.h"
#include "assets/tutsil_front_sprite.h"
#include "assets/electric_armor_badger_sprite.h"

#define BATTLE_SLOT_TILE_W 36
#define BATTLE_SLOT_UNIT_W 22
#define BATTLE_SLOT_UNIT_H 14

static const int s_enemyTileX[BATTLE_ENEMY_SLOT_COUNT] = {14, 58, 102, 146, 190};
static const int s_enemyTileY[BATTLE_ENEMY_SLOT_COUNT] = {24, 30, 36, 42, 48};
static const int s_allyTileX[BATTLE_PLAYER_SLOT_COUNT] = {0, 46, 92, 138, 184};
static const int s_allyTileY[BATTLE_PLAYER_SLOT_COUNT] = {61, 67, 73, 79, 85};

static BattleSlotLayout s_enemyLayouts[BATTLE_ENEMY_SLOT_COUNT];
static BattleSlotLayout s_allyLayouts[BATTLE_PLAYER_SLOT_COUNT];
static int s_layoutInitialized;

static void BattleSlotLayout_InitEntry(BattleSlotLayout *layout, int tileX, int baseTileY, int cursorYOffset)
{
    int tileY = baseTileY + BATTLE_FIELD_TILE_OFFSET_Y;

    layout->tileX = tileX;
    layout->tileY = tileY;
    layout->tileW = BATTLE_SLOT_TILE_W;
    layout->tileH = BATTLE_SLOT_TILE_H;
    layout->spriteX = tileX + 7;
    layout->spriteY = tileY + 3;
    layout->spriteW = BATTLE_SLOT_UNIT_W;
    layout->spriteH = BATTLE_SLOT_UNIT_H;
    layout->statX = tileX + 2;
    layout->statY = tileY + 14;
    layout->cursorX = tileX + 10;
    layout->cursorY = tileY + cursorYOffset;
}

static void BattleSlotLayout_InitTables(void)
{
    int i;

    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        BattleSlotLayout_InitEntry(&s_enemyLayouts[i], s_enemyTileX[i], s_enemyTileY[i], -2);
    }
    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        BattleSlotLayout_InitEntry(&s_allyLayouts[i], s_allyTileX[i], s_allyTileY[i], 0);
    }
    s_layoutInitialized = 1;
}

const BattleSlotLayout *BattleSlotLayout_Get(int side, int slotIndex)
{
    if (!s_layoutInitialized)
    {
        BattleSlotLayout_InitTables();
    }
    if (side == BATTLE_SIDE_ENEMY)
    {
        if (slotIndex < 0 || slotIndex >= BATTLE_ENEMY_SLOT_COUNT)
        {
            return 0;
        }
        return &s_enemyLayouts[slotIndex];
    }
    if (side == BATTLE_SIDE_PLAYER)
    {
        if (slotIndex < 0 || slotIndex >= BATTLE_PLAYER_SLOT_COUNT)
        {
            return 0;
        }
        return &s_allyLayouts[slotIndex];
    }
    return 0;
}

static int BattleSlotLayout_GetFrontSpriteHeight(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    if (BattleMember_IsProtagonist(member))
    {
        return PROTAGONIST_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsAksil(member))
    {
        return AKSIL_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsMaren(member))
    {
        return MAREN_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsTutsil(member))
    {
        return TUTSIL_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsElectricArmorBadger(member))
    {
        return ELECTRIC_ARMOR_BADGER_VISUAL_HEIGHT;
    }
    return 0;
}

static int BattleSlotLayout_GetFrontSpriteCanvasHeight(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    if (BattleMember_IsProtagonist(member))
    {
        return PROTAGONIST_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsAksil(member))
    {
        return AKSIL_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsMaren(member))
    {
        return MAREN_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsTutsil(member))
    {
        return TUTSIL_FRONT_SPRITE_HEIGHT;
    }
    if (BattleMember_IsElectricArmorBadger(member))
    {
        return ELECTRIC_ARMOR_BADGER_SPRITE_HEIGHT;
    }
    return 0;
}

int BattleLayout_GetUnitDrawY(
    int side,
    const BattleMember *member,
    const BattleSlotLayout *layout)
{
    int canvasHeight;

    (void)side;
    if (!layout)
    {
        return 0;
    }
    canvasHeight = BattleSlotLayout_GetFrontSpriteCanvasHeight(member);
    if (canvasHeight > 0)
    {
        return layout->tileY + layout->tileH - canvasHeight;
    }
    return layout->spriteY;
}

int BattleLayout_GetUnitTopY(
    int side,
    const BattleMember *member,
    const BattleSlotLayout *layout)
{
    int spriteHeight;

    (void)side;
    if (!layout)
    {
        return 0;
    }
    spriteHeight = BattleSlotLayout_GetFrontSpriteHeight(member);
    if (spriteHeight > 0)
    {
        return layout->tileY + layout->tileH - spriteHeight;
    }
    return layout->spriteY;
}
