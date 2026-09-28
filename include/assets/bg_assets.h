#ifndef BG_ASSETS_H
#define BG_ASSETS_H

#include "assets/asset_ids.h"

void BgAssets_DrawPlaceholder(BgAssetId id);
void BgAssets_DrawField(BgAssetId id);
void BgAssets_DrawFieldRegion(BgAssetId id, int x, int y, int w, int h);
void BgAssets_DrawPetalburgBattleBackground(void);
void BgAssets_DrawPetalburgBattleBackgroundRegion(int x, int y, int w, int h);

#endif
