#include "overworld/overworld_collision.h"

int OverworldCollision_IsSolidAtPixel(
    const OverworldMapDef *map,
    s32 worldPixelX,
    s32 worldPixelY)
{
    int metatileX;
    int metatileY;
    int bitIndex;
    u8 profileId;
    u16 mask;

    if (!map || !map->collisionProfileIds || !map->collisionProfileMasks ||
        map->widthMetatiles <= 0 || map->heightMetatiles <= 0 ||
        map->collisionProfileCount <= 0)
    {
        return 1;
    }

    if (worldPixelX < 0 || worldPixelY < 0)
    {
        return 1;
    }

    metatileX = worldPixelX >> 4;
    metatileY = worldPixelY >> 4;
    if (metatileX >= map->widthMetatiles || metatileY >= map->heightMetatiles)
    {
        return 1;
    }

    profileId = map->collisionProfileIds[
        (metatileY * map->widthMetatiles) + metatileX];
    if (profileId == 0)
    {
        return 0;
    }
    if (profileId >= (u8)map->collisionProfileCount)
    {
        /* Invalid profile: treat as solid; never index past the mask table. */
        return 1;
    }

    mask = map->collisionProfileMasks[profileId];
    bitIndex = ((worldPixelY >> 2) & 3) * 4 + ((worldPixelX >> 2) & 3);
    return (mask & (u16)(1u << bitIndex)) != 0;
}
