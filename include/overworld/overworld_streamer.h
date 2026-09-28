#ifndef OVERWORLD_STREAMER_H
#define OVERWORLD_STREAMER_H

#include "overworld/overworld_map.h"

typedef struct OverworldBg3Streamer {
    const OverworldMapDef *map;
    int residentWorldTileX;
    int residentWorldTileY;
} OverworldBg3Streamer;

void OverworldBg3Streamer_Init(
    OverworldBg3Streamer *streamer,
    const OverworldMapDef *map);
void OverworldBg3Streamer_SetCamera(
    OverworldBg3Streamer *streamer,
    int cameraX,
    int cameraY);

#endif
