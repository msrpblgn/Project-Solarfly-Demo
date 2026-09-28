#ifndef OVERWORLD_PLAYER_H
#define OVERWORLD_PLAYER_H

#include "core/fixed.h"
#include "overworld/overworld_map.h"

typedef enum OverworldPlayerFacing {
    OVERWORLD_PLAYER_FACING_DOWN = 0,
    OVERWORLD_PLAYER_FACING_DOWN_LEFT,
    OVERWORLD_PLAYER_FACING_LEFT,
    OVERWORLD_PLAYER_FACING_UP_LEFT,
    OVERWORLD_PLAYER_FACING_UP,
    OVERWORLD_PLAYER_FACING_UP_RIGHT,
    OVERWORLD_PLAYER_FACING_RIGHT,
    OVERWORLD_PLAYER_FACING_DOWN_RIGHT
} OverworldPlayerFacing;

/* Cardinal speed: 1.5 px/frame. Diagonal axis component: 272/256 px/frame. */
#define OVERWORLD_PLAYER_SPEED_CARDINAL ((Fixed24_8)384)
#define OVERWORLD_PLAYER_SPEED_DIAGONAL ((Fixed24_8)272)

#define OVERWORLD_PLAYER_SCREEN_FOOT_X 120
#define OVERWORLD_PLAYER_SCREEN_FOOT_Y 96

#define OVERWORLD_PLAYER_MIN_WORLD_X 8
#define OVERWORLD_PLAYER_MAX_WORLD_X 632
#define OVERWORLD_PLAYER_MIN_WORLD_Y 32
#define OVERWORLD_PLAYER_MAX_WORLD_Y 480

#define OVERWORLD_PLAYER_START_WORLD_X 120
#define OVERWORLD_PLAYER_START_WORLD_Y 96

/* Foot collision AABB: 8x8 centered on the foot anchor. */
#define OVERWORLD_PLAYER_COLLISION_HALF_W 4
#define OVERWORLD_PLAYER_COLLISION_H 8
#define OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED \
    Fixed24_8_FromInt(OVERWORLD_PLAYER_COLLISION_HALF_W)
#define OVERWORLD_PLAYER_COLLISION_H_FIXED \
    Fixed24_8_FromInt(OVERWORLD_PLAYER_COLLISION_H)

_Static_assert(sizeof(Fixed24_8) == 4, "Fixed24_8 must remain 32-bit");
_Static_assert(sizeof(u16) == 2, "collision masks use u16");
_Static_assert(sizeof(u8) == 1, "collision profile ids use u8");
_Static_assert(
    OVERWORLD_PLAYER_COLLISION_HALF_W * 2 == 8,
    "player collision width must be 8");
_Static_assert(
    OVERWORLD_PLAYER_COLLISION_H == 8,
    "player collision height must be 8");
_Static_assert(
    (OVERWORLD_PLAYER_COLLISION_HALF_W * 2) % 2 == 0,
    "player collision width must be even");
_Static_assert(
    OVERWORLD_PLAYER_COLLISION_HALF_W * 2 <= 16 &&
        OVERWORLD_PLAYER_COLLISION_H <= 32,
    "collision box must fit inside the 16x32 sprite footprint");
_Static_assert(
    OVERWORLD_PLAYER_SPEED_CARDINAL < (4 << FIXED24_8_SHIFT) &&
        OVERWORLD_PLAYER_SPEED_DIAGONAL < (4 << FIXED24_8_SHIFT),
    "max movement component must stay below one 4px collision subcell");

typedef struct OverworldPlayer {
    Fixed24_8 worldX; /* Foot-anchor world X (center), Q24.8 */
    Fixed24_8 worldY; /* Foot-anchor world Y (bottom), Q24.8 */
    OverworldPlayerFacing facing;
    /* Down-idle blink when that facing has a blink frame; open for blinkOpenFrames,
 * closed for blinkClosedFrames. Upward facings never invent a blink. */
    unsigned char blinkEyesClosed;
    unsigned short blinkTimer;
    unsigned short blinkOpenFrames;
    unsigned char blinkClosedFrames;
} OverworldPlayer;

void OverworldPlayer_Reset(OverworldPlayer *player);
void OverworldPlayer_UpdateFromInput(
    OverworldPlayer *player,
    const OverworldMapDef *map);
void OverworldPlayer_GetScreenTopLeft(
    const OverworldPlayer *player,
    int cameraPixelX,
    int cameraPixelY,
    int *outScreenX,
    int *outScreenY);
void OverworldPlayer_PrepareOam(
    const OverworldPlayer *player,
    int cameraPixelX,
    int cameraPixelY);
int OverworldPlayer_GetIdleSpriteTileIndex(const OverworldPlayer *player);

#endif
