#include "overworld/overworld_map.h"
#include "assets/maps/overworld_test_map.h"

#define OVERWORLD_DIAGNOSTIC_TILE_COUNT 3
#define OVERWORLD_DIAGNOSTIC_TILE_WORD_COUNT (OVERWORLD_DIAGNOSTIC_TILE_COUNT * 8)

#define OW_SCREEN_ENTRY(tileId, palette, hFlip, vFlip) \
    ((u16)((tileId) | ((hFlip) ? 0x0400 : 0) | ((vFlip) ? 0x0800 : 0) | (((palette) & 15) << 12)))

/* Index 1 = green, 2 = red battle trigger, 3 = blue dialogue marker. */
static const u16 s_overworldDiagnosticPalette[16] = {
    RGB15(0, 0, 0),
    RGB15(6, 20, 8),
    RGB15(29, 3, 3),
    RGB15(4, 12, 28),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0)
};

/* Tile 0 green, 1 red, 2 blue (palette indices 1/2/3). */
static const u32 s_overworldDiagnosticTiles[OVERWORLD_DIAGNOSTIC_TILE_WORD_COUNT] = {
    0x11111111, 0x11111111, 0x11111111, 0x11111111,
    0x11111111, 0x11111111, 0x11111111, 0x11111111,

    0x22222222, 0x22222222, 0x22222222, 0x22222222,
    0x22222222, 0x22222222, 0x22222222, 0x22222222,

    0x33333333, 0x33333333, 0x33333333, 0x33333333,
    0x33333333, 0x33333333, 0x33333333, 0x33333333
};

static const OverworldMetatileDef s_overworldDiagnosticMetatiles[] = {
    {OW_SCREEN_ENTRY(0, 0, 0, 0), OW_SCREEN_ENTRY(0, 0, 0, 0),
     OW_SCREEN_ENTRY(0, 0, 0, 0), OW_SCREEN_ENTRY(0, 0, 0, 0)},
    {OW_SCREEN_ENTRY(1, 0, 0, 0), OW_SCREEN_ENTRY(1, 0, 0, 0),
     OW_SCREEN_ENTRY(1, 0, 0, 0), OW_SCREEN_ENTRY(1, 0, 0, 0)},
    {OW_SCREEN_ENTRY(2, 0, 0, 0), OW_SCREEN_ENTRY(2, 0, 0, 0),
     OW_SCREEN_ENTRY(2, 0, 0, 0), OW_SCREEN_ENTRY(2, 0, 0, 0)}
};

_Static_assert(
    (sizeof(s_overworldDiagnosticMetatiles) / sizeof(s_overworldDiagnosticMetatiles[0]))
        == gOverworldTestMap_TILESET_METATILE_COUNT,
    "diagnostic metatile def count must match TSX tilecount");

static const OverworldMapDef s_overworldDiagnosticMap = {
    gOverworldTestMap_WIDTH_METATILES,
    gOverworldTestMap_HEIGHT_METATILES,
    gOverworldTestMapMetatileIds,
    s_overworldDiagnosticMetatiles,
    sizeof(s_overworldDiagnosticMetatiles) / sizeof(s_overworldDiagnosticMetatiles[0]),
    gOverworldTestMap_FALLBACK_METATILE_ID,
    gOverworldTestMapCollisionProfileIds,
    gOverworldTestMapCollisionProfileMasks,
    gOverworldTestMap_COLLISION_PROFILE_COUNT
};

_Static_assert(
    gOverworldTestMap_CELL_COUNT == 1200,
    "diagnostic map cell count must be 1200");
_Static_assert(
    gOverworldTestMap_COLLISION_PROFILE_COUNT == 6,
    "diagnostic collision profile count must be 6");
_Static_assert(
    gOverworldTestMap_TILESET_METATILE_COUNT == 3,
    "OW-6 ground tileset has green+red+blue metatiles");

static const OverworldMetatileDef *OverworldMap_GetMetatileDefSafe(
    const OverworldMapDef *map,
    u16 metatileId)
{
    u16 safeId = 0;

    if (!map || !map->metatileDefs || map->metatileDefCount <= 0)
    {
        return 0;
    }
    if (map->fallbackMetatileId < map->metatileDefCount)
    {
        safeId = map->fallbackMetatileId;
    }
    if (metatileId >= (u16)map->metatileDefCount)
    {
        metatileId = safeId;
    }
    return &map->metatileDefs[metatileId];
}

const OverworldMapDef *OverworldMap_GetDiagnostic(void)
{
    return &s_overworldDiagnosticMap;
}

const u16 *OverworldMap_GetDiagnosticPalette(void)
{
    return s_overworldDiagnosticPalette;
}

const u32 *OverworldMap_GetDiagnosticTiles(void)
{
    return s_overworldDiagnosticTiles;
}

int OverworldMap_GetDiagnosticTileWordCount(void)
{
    return OVERWORLD_DIAGNOSTIC_TILE_WORD_COUNT;
}

int OverworldMap_GetPixelWidth(const OverworldMapDef *map)
{
    if (!map || map->widthMetatiles <= 0)
    {
        return 0;
    }
    return map->widthMetatiles * 16;
}

int OverworldMap_GetPixelHeight(const OverworldMapDef *map)
{
    if (!map || map->heightMetatiles <= 0)
    {
        return 0;
    }
    return map->heightMetatiles * 16;
}

u16 OverworldMap_GetScreenEntryAtWorldTile(
    const OverworldMapDef *map,
    int worldTileX,
    int worldTileY)
{
    int metatileX;
    int metatileY;
    int localX;
    int localY;
    const OverworldMetatileDef *def;
    u16 metatileId;

    if (!map)
    {
        return 0;
    }
    metatileX = worldTileX >> 1;
    metatileY = worldTileY >> 1;
    localX = worldTileX & 1;
    localY = worldTileY & 1;
    def = OverworldMap_GetMetatileDefSafe(map, map->fallbackMetatileId);
    if (!def)
    {
        return 0;
    }
    if (metatileX < 0 || metatileY < 0 ||
        metatileX >= map->widthMetatiles || metatileY >= map->heightMetatiles ||
        !map->metatileIds)
    {
        metatileId = map->fallbackMetatileId;
    }
    else
    {
        metatileId = map->metatileIds[(metatileY * map->widthMetatiles) + metatileX];
    }
    def = OverworldMap_GetMetatileDefSafe(map, metatileId);
    if (!def)
    {
        return 0;
    }
    if (localY == 0)
    {
        return localX == 0 ? def->topLeft : def->topRight;
    }
    return localX == 0 ? def->bottomLeft : def->bottomRight;
}
