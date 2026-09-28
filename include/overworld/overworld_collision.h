#ifndef OVERWORLD_COLLISION_H
#define OVERWORLD_COLLISION_H

#include "core/gba_types.h"
#include "overworld/overworld_map.h"

int OverworldCollision_IsSolidAtPixel(
    const OverworldMapDef *map,
    s32 worldPixelX,
    s32 worldPixelY);

#endif
