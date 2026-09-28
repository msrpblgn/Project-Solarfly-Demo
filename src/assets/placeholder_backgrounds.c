#include "assets/bg_assets.h"
#include "assets/petalburg_battle_background.h"
#include "core/video.h"
#include "core/gba_types.h"

void BgAssets_DrawPlaceholder(BgAssetId id)
{
    (void)id;
    Video_Clear(RGB15(12, 20, 8));
    Video_DrawRect(0, 0, SCREEN_WIDTH, 112, RGB15(16, 24, 12));
}

void BgAssets_DrawField(BgAssetId id)
{
    (void)id;
    Video_DrawRect(0, 0, SCREEN_WIDTH, 112, RGB15(16, 24, 12));
}

void BgAssets_DrawFieldRegion(BgAssetId id, int x, int y, int w, int h)
{
    (void)id;
    if (w <= 0 || h <= 0)
    {
        return;
    }
    Video_DrawRect(x, y, w, h, RGB15(16, 24, 12));
}

void BgAssets_DrawPetalburgBattleBackground(void)
{
    Video_DrawBitmap(
        0,
        0,
        PETALBURG_BATTLE_BACKGROUND_WIDTH,
        PETALBURG_BATTLE_BACKGROUND_HEIGHT,
        gPetalburgBattleBackgroundPixels);
}

void BgAssets_DrawPetalburgBattleBackgroundRegion(int x, int y, int w, int h)
{
    int x2;
    int y2;

    if (w <= 0 || h <= 0)
    {
        return;
    }
    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    x2 = x + w;
    y2 = y + h;
    if (x2 > PETALBURG_BATTLE_BACKGROUND_WIDTH)
    {
        x2 = PETALBURG_BATTLE_BACKGROUND_WIDTH;
    }
    if (y2 > PETALBURG_BATTLE_BACKGROUND_HEIGHT)
    {
        y2 = PETALBURG_BATTLE_BACKGROUND_HEIGHT;
    }
    w = x2 - x;
    h = y2 - y;
    if (w <= 0 || h <= 0)
    {
        return;
    }
    Video_BlitBitmapRegion(
        x,
        y,
        w,
        h,
        gPetalburgBattleBackgroundPixels,
        PETALBURG_BATTLE_BACKGROUND_WIDTH,
        PETALBURG_BATTLE_BACKGROUND_HEIGHT,
        x,
        y);
}

