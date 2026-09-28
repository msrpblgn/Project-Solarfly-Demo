#include "assets/sprite_assets.h"
#include "assets/aksil_front_sprite.h"
#include "assets/maren_front_sprite.h"
#include "assets/protagonist_front_sprite.h"
#include "assets/tutsil_front_sprite.h"
#include "assets/place_holder_enemy_sprite.h"
#include "assets/electric_armor_badger_sprite.h"
#include "core/video.h"
#include "core/gba_types.h"

void SpriteAssets_DrawPlaceholder(SpriteAssetId id, int x, int y)
{
    u16 color;
    switch (id)
    {
    case SPRITE_ASSET_PROTAGONIST_PLACEHOLDER:
        color = RGB15(8, 16, 31);
        Video_DrawRect(x, y, 48, 48, color);
        Video_DrawRect(x + 16, y + 8, 16, 16, RGB15(31, 24, 16));
        break;
    case SPRITE_ASSET_ENEMY_PLACEHOLDER:
        color = RGB15(24, 8, 8);
        Video_DrawRect(x, y, 56, 40, color);
        Video_DrawRect(x + 8, y + 8, 12, 12, RGB15(31, 31, 0));
        Video_DrawRect(x + 36, y + 8, 12, 12, RGB15(31, 31, 0));
        break;
    case SPRITE_ASSET_TARGET_CURSOR:
        color = RGB15(31, 31, 0);
        Video_DrawRect(x, y, 16, 4, color);
        Video_DrawRect(x, y + 12, 16, 4, color);
        Video_DrawRect(x, y, 4, 16, color);
        Video_DrawRect(x + 12, y, 4, 16, color);
        break;
    case SPRITE_ASSET_UI_CURSOR:
        color = RGB15(31, 28, 0);
        Video_DrawRect(x, y, 8, 8, color);
        break;
    default:
        break;
    }
}

void SpriteAssets_DrawAksilFront(int x, int y)
{
    Video_DrawBitmapKeyColor(
        x,
        y,
        AKSIL_FRONT_SPRITE_WIDTH,
        AKSIL_FRONT_SPRITE_HEIGHT,
        aksil_front_spriteBitmap,
        AKSIL_FRONT_SPRITE_TRANSPARENT_COLOR);
}

void SpriteAssets_DrawMarenFront(int x, int y)
{
    Video_DrawBitmapKeyColor(
        x,
        y,
        MAREN_FRONT_SPRITE_WIDTH,
        MAREN_FRONT_SPRITE_HEIGHT,
        maren_front_spriteBitmap,
        MAREN_FRONT_SPRITE_TRANSPARENT_COLOR);
}

void SpriteAssets_DrawProtagonistFront(int x, int y)
{
    Video_DrawBitmapKeyColor(
        x,
        y,
        PROTAGONIST_FRONT_SPRITE_WIDTH,
        PROTAGONIST_FRONT_SPRITE_HEIGHT,
        protagonist_front_spriteBitmap,
        PROTAGONIST_FRONT_SPRITE_TRANSPARENT_COLOR);
}

void SpriteAssets_DrawTutsilFront(int x, int y)
{
    Video_DrawBitmapKeyColor(
        x,
        y,
        TUTSIL_FRONT_SPRITE_WIDTH,
        TUTSIL_FRONT_SPRITE_HEIGHT,
        tutsil_front_spriteBitmap,
        TUTSIL_FRONT_SPRITE_TRANSPARENT_COLOR);
}

void SpriteAssets_DrawPlaceholderEnemyFront(int x, int y)
{
    Video_DrawBitmapKeyColor(
        x,
        y,
        PLACE_HOLDER_ENEMY_SPRITE_WIDTH,
        PLACE_HOLDER_ENEMY_SPRITE_HEIGHT,
        place_holder_enemy_spriteBitmap,
        PLACE_HOLDER_ENEMY_SPRITE_TRANSPARENT_COLOR);
}

void SpriteAssets_DrawElectricArmorBadgerFront(int x, int y)
{
    Video_DrawBitmapKeyColor(
        x,
        y,
        ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH,
        ELECTRIC_ARMOR_BADGER_SPRITE_HEIGHT,
        electric_armor_badger_spriteBitmap,
        ELECTRIC_ARMOR_BADGER_SPRITE_TRANSPARENT_COLOR);
}
