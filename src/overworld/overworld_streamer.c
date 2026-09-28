#include "overworld/overworld_streamer.h"

#include "core/video.h"

#define OVERWORLD_STREAMER_BUFFER_TILES 32
#define OVERWORLD_STREAMER_TILE_SIZE 8

static void OverworldBg3Streamer_WriteEntry(int destX, int destY, u16 entry)
{
    volatile u16 *screenBase = Video_GetMode0Bg3Screenbase();

    screenBase[(destY * OVERWORLD_STREAMER_BUFFER_TILES) + destX] = entry;
}

static void OverworldBg3Streamer_WriteColumn(
    OverworldBg3Streamer *streamer,
    int worldTileX)
{
    int row;
    int destX = worldTileX & (OVERWORLD_STREAMER_BUFFER_TILES - 1);

    for (row = 0; row < OVERWORLD_STREAMER_BUFFER_TILES; row++)
    {
        int worldTileY = streamer->residentWorldTileY + row;
        int destY = worldTileY & (OVERWORLD_STREAMER_BUFFER_TILES - 1);
        u16 entry = OverworldMap_GetScreenEntryAtWorldTile(
            streamer->map,
            worldTileX,
            worldTileY);

        OverworldBg3Streamer_WriteEntry(destX, destY, entry);
    }
}

static void OverworldBg3Streamer_WriteRow(
    OverworldBg3Streamer *streamer,
    int worldTileY)
{
    int col;
    int destY = worldTileY & (OVERWORLD_STREAMER_BUFFER_TILES - 1);

    for (col = 0; col < OVERWORLD_STREAMER_BUFFER_TILES; col++)
    {
        int worldTileX = streamer->residentWorldTileX + col;
        int destX = worldTileX & (OVERWORLD_STREAMER_BUFFER_TILES - 1);
        u16 entry = OverworldMap_GetScreenEntryAtWorldTile(
            streamer->map,
            worldTileX,
            worldTileY);

        OverworldBg3Streamer_WriteEntry(destX, destY, entry);
    }
}

static void OverworldBg3Streamer_RefillAll(
    OverworldBg3Streamer *streamer,
    int worldTileX,
    int worldTileY)
{
    int row;
    int col;

    streamer->residentWorldTileX = worldTileX;
    streamer->residentWorldTileY = worldTileY;
    for (row = 0; row < OVERWORLD_STREAMER_BUFFER_TILES; row++)
    {
        for (col = 0; col < OVERWORLD_STREAMER_BUFFER_TILES; col++)
        {
            int srcTileX = worldTileX + col;
            int srcTileY = worldTileY + row;
            int destX = srcTileX & (OVERWORLD_STREAMER_BUFFER_TILES - 1);
            int destY = srcTileY & (OVERWORLD_STREAMER_BUFFER_TILES - 1);
            u16 entry = OverworldMap_GetScreenEntryAtWorldTile(
                streamer->map,
                srcTileX,
                srcTileY);

            OverworldBg3Streamer_WriteEntry(destX, destY, entry);
        }
    }
}

void OverworldBg3Streamer_Init(
    OverworldBg3Streamer *streamer,
    const OverworldMapDef *map)
{
    if (!streamer)
    {
        return;
    }
    streamer->map = map;
    OverworldBg3Streamer_RefillAll(streamer, 0, 0);
}

void OverworldBg3Streamer_SetCamera(
    OverworldBg3Streamer *streamer,
    int cameraX,
    int cameraY)
{
    int targetWorldTileX;
    int targetWorldTileY;

    if (!streamer || !streamer->map)
    {
        return;
    }

    targetWorldTileX = cameraX / OVERWORLD_STREAMER_TILE_SIZE;
    targetWorldTileY = cameraY / OVERWORLD_STREAMER_TILE_SIZE;

    if (targetWorldTileX - streamer->residentWorldTileX >= OVERWORLD_STREAMER_BUFFER_TILES ||
        streamer->residentWorldTileX - targetWorldTileX >= OVERWORLD_STREAMER_BUFFER_TILES ||
        targetWorldTileY - streamer->residentWorldTileY >= OVERWORLD_STREAMER_BUFFER_TILES ||
        streamer->residentWorldTileY - targetWorldTileY >= OVERWORLD_STREAMER_BUFFER_TILES)
    {
        OverworldBg3Streamer_RefillAll(streamer, targetWorldTileX, targetWorldTileY);
    }
    else
    {
        while (streamer->residentWorldTileX < targetWorldTileX)
        {
            OverworldBg3Streamer_WriteColumn(
                streamer,
                streamer->residentWorldTileX + OVERWORLD_STREAMER_BUFFER_TILES);
            streamer->residentWorldTileX++;
        }
        while (streamer->residentWorldTileX > targetWorldTileX)
        {
            streamer->residentWorldTileX--;
            OverworldBg3Streamer_WriteColumn(streamer, streamer->residentWorldTileX);
        }
        while (streamer->residentWorldTileY < targetWorldTileY)
        {
            OverworldBg3Streamer_WriteRow(
                streamer,
                streamer->residentWorldTileY + OVERWORLD_STREAMER_BUFFER_TILES);
            streamer->residentWorldTileY++;
        }
        while (streamer->residentWorldTileY > targetWorldTileY)
        {
            streamer->residentWorldTileY--;
            OverworldBg3Streamer_WriteRow(streamer, streamer->residentWorldTileY);
        }
    }

    Video_SetMode0Bg3Scroll(
        cameraX & ((OVERWORLD_STREAMER_BUFFER_TILES * OVERWORLD_STREAMER_TILE_SIZE) - 1),
        cameraY & ((OVERWORLD_STREAMER_BUFFER_TILES * OVERWORLD_STREAMER_TILE_SIZE) - 1));
}
