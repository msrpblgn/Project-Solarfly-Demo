#ifndef OVERWORLD_MAP_H
#define OVERWORLD_MAP_H

#include "core/gba_types.h"

typedef struct OverworldMetatileDef {
    u16 topLeft;
    u16 topRight;
    u16 bottomLeft;
    u16 bottomRight;
} OverworldMetatileDef;

typedef struct OverworldMapDef {
    int widthMetatiles;
    int heightMetatiles;
    const u16 *metatileIds;
    const OverworldMetatileDef *metatileDefs;
    int metatileDefCount;
    u16 fallbackMetatileId;
    const u8 *collisionProfileIds;
    const u16 *collisionProfileMasks;
    int collisionProfileCount;
} OverworldMapDef;

const OverworldMapDef *OverworldMap_GetDiagnostic(void);
const u16 *OverworldMap_GetDiagnosticPalette(void);
const u32 *OverworldMap_GetDiagnosticTiles(void);
int OverworldMap_GetDiagnosticTileWordCount(void);
int OverworldMap_GetPixelWidth(const OverworldMapDef *map);
int OverworldMap_GetPixelHeight(const OverworldMapDef *map);
u16 OverworldMap_GetScreenEntryAtWorldTile(
    const OverworldMapDef *map,
    int worldTileX,
    int worldTileY);

#endif
