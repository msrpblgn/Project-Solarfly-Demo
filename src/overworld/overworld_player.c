#include "overworld/overworld_player.h"

#include "assets/overworld_player_sprite.h"
#include "core/input.h"
#include "core/oam.h"
#include "core/random.h"
#include "overworld/overworld_collision.h"

#define OVERWORLD_PLAYER_BLINK_OPEN_MIN 240
#define OVERWORLD_PLAYER_BLINK_OPEN_MAX_EXCLUSIVE 361
#define OVERWORLD_PLAYER_BLINK_CLOSED_MIN 6
#define OVERWORLD_PLAYER_BLINK_CLOSED_MAX_EXCLUSIVE 9

static void OverworldPlayer_RollOpenDuration(OverworldPlayer *player)
{
    player->blinkEyesClosed = 0;
    player->blinkOpenFrames = (unsigned short)Random_Range(
        OVERWORLD_PLAYER_BLINK_OPEN_MIN,
        OVERWORLD_PLAYER_BLINK_OPEN_MAX_EXCLUSIVE);
    player->blinkClosedFrames = (unsigned char)Random_Range(
        OVERWORLD_PLAYER_BLINK_CLOSED_MIN,
        OVERWORLD_PLAYER_BLINK_CLOSED_MAX_EXCLUSIVE);
    player->blinkTimer = player->blinkOpenFrames;
}

static void OverworldPlayer_ResetBlinkToOpen(OverworldPlayer *player)
{
    OverworldPlayer_RollOpenDuration(player);
}

static void OverworldPlayer_UpdateIdleBlink(OverworldPlayer *player)
{
    if (!player)
    {
        return;
    }

    if (player->facing < 0
        || player->facing >= OVERWORLD_PLAYER_FACING_SPRITE_COUNT
        || !gOverworldPlayerFacingHasBlink[player->facing])
    {
        if (player->blinkEyesClosed)
        {
            OverworldPlayer_ResetBlinkToOpen(player);
        }
        return;
    }

    if (player->blinkTimer > 0)
    {
        player->blinkTimer--;
    }

    if (player->blinkTimer > 0)
    {
        return;
    }

    if (!player->blinkEyesClosed)
    {
        player->blinkEyesClosed = 1;
        player->blinkTimer = player->blinkClosedFrames;
        return;
    }

    OverworldPlayer_RollOpenDuration(player);
}

static int OverworldPlayer_EdgeHasSolid(
    const OverworldMapDef *map,
    int edgePixel,
    int orthoStart,
    int orthoEnd,
    int verticalEdge)
{
    int ortho;

    if (orthoEnd < orthoStart)
    {
        return 0;
    }
    for (ortho = orthoStart; ortho <= orthoEnd; ortho++)
    {
        if (verticalEdge)
        {
            if (OverworldCollision_IsSolidAtPixel(map, edgePixel, ortho))
            {
                return 1;
            }
        }
        else if (OverworldCollision_IsSolidAtPixel(map, ortho, edgePixel))
        {
            return 1;
        }
    }
    return 0;
}

static Fixed24_8 OverworldPlayer_ResolveAxisX(
    const OverworldMapDef *map,
    Fixed24_8 currentX,
    Fixed24_8 currentY,
    Fixed24_8 proposedX)
{
    Fixed24_8 left;
    Fixed24_8 rightExclusive;
    Fixed24_8 top;
    Fixed24_8 bottomExclusive;
    int y0;
    int y1;
    int edgePixel;
    int subcellBoundary;

    proposedX = Fixed24_8_Clamp(
        proposedX,
        Fixed24_8_FromInt(OVERWORLD_PLAYER_MIN_WORLD_X),
        Fixed24_8_FromInt(OVERWORLD_PLAYER_MAX_WORLD_X));

    if (proposedX == currentX || !map)
    {
        return proposedX;
    }

    left = proposedX - OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED;
    rightExclusive = proposedX + OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED;
    top = currentY - OVERWORLD_PLAYER_COLLISION_H_FIXED;
    bottomExclusive = currentY;
    y0 = Fixed24_8_FloorToIntNonNeg(top);
    y1 = Fixed24_8_FloorToIntNonNeg(bottomExclusive - 1);

    if (proposedX > currentX)
    {
        edgePixel = Fixed24_8_FloorToIntNonNeg(rightExclusive - 1);
        if (OverworldPlayer_EdgeHasSolid(map, edgePixel, y0, y1, 1))
        {
            subcellBoundary = edgePixel & ~3;
            return Fixed24_8_FromInt(subcellBoundary) -
                OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED;
        }
    }
    else
    {
        edgePixel = Fixed24_8_FloorToIntNonNeg(left);
        if (OverworldPlayer_EdgeHasSolid(map, edgePixel, y0, y1, 1))
        {
            subcellBoundary = (edgePixel & ~3) + 4;
            return Fixed24_8_FromInt(subcellBoundary) +
                OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED;
        }
    }

    return proposedX;
}

static Fixed24_8 OverworldPlayer_ResolveAxisY(
    const OverworldMapDef *map,
    Fixed24_8 resolvedX,
    Fixed24_8 currentY,
    Fixed24_8 proposedY)
{
    Fixed24_8 left;
    Fixed24_8 rightExclusive;
    Fixed24_8 top;
    Fixed24_8 bottomExclusive;
    int x0;
    int x1;
    int edgePixel;
    int subcellBoundary;

    proposedY = Fixed24_8_Clamp(
        proposedY,
        Fixed24_8_FromInt(OVERWORLD_PLAYER_MIN_WORLD_Y),
        Fixed24_8_FromInt(OVERWORLD_PLAYER_MAX_WORLD_Y));

    if (proposedY == currentY || !map)
    {
        return proposedY;
    }

    left = resolvedX - OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED;
    rightExclusive = resolvedX + OVERWORLD_PLAYER_COLLISION_HALF_W_FIXED;
    top = proposedY - OVERWORLD_PLAYER_COLLISION_H_FIXED;
    bottomExclusive = proposedY;
    x0 = Fixed24_8_FloorToIntNonNeg(left);
    x1 = Fixed24_8_FloorToIntNonNeg(rightExclusive - 1);

    if (proposedY > currentY)
    {
        edgePixel = Fixed24_8_FloorToIntNonNeg(bottomExclusive - 1);
        if (OverworldPlayer_EdgeHasSolid(map, edgePixel, x0, x1, 0))
        {
            subcellBoundary = edgePixel & ~3;
            return Fixed24_8_FromInt(subcellBoundary);
        }
    }
    else
    {
        edgePixel = Fixed24_8_FloorToIntNonNeg(top);
        if (OverworldPlayer_EdgeHasSolid(map, edgePixel, x0, x1, 0))
        {
            subcellBoundary = (edgePixel & ~3) + 4;
            return Fixed24_8_FromInt(subcellBoundary) +
                OVERWORLD_PLAYER_COLLISION_H_FIXED;
        }
    }

    return proposedY;
}

void OverworldPlayer_Reset(OverworldPlayer *player)
{
    if (!player)
    {
        return;
    }
    player->worldX = Fixed24_8_FromInt(OVERWORLD_PLAYER_START_WORLD_X);
    player->worldY = Fixed24_8_FromInt(OVERWORLD_PLAYER_START_WORLD_Y);
    player->facing = OVERWORLD_PLAYER_FACING_DOWN;
    OverworldPlayer_ResetBlinkToOpen(player);
}

void OverworldPlayer_UpdateFromInput(
    OverworldPlayer *player,
    const OverworldMapDef *map)
{
    int axisX = 0;
    int axisY = 0;
    Fixed24_8 velocityX = 0;
    Fixed24_8 velocityY = 0;
    Fixed24_8 proposedX;
    Fixed24_8 proposedY;
    OverworldPlayerFacing previousFacing;

    if (!player)
    {
        return;
    }

    previousFacing = player->facing;

    if (Input_IsPressed(KEY_RIGHT))
    {
        axisX += 1;
    }
    if (Input_IsPressed(KEY_LEFT))
    {
        axisX -= 1;
    }
    if (Input_IsPressed(KEY_DOWN))
    {
        axisY += 1;
    }
    if (Input_IsPressed(KEY_UP))
    {
        axisY -= 1;
    }

    if (axisX == 0 && axisY == 0)
    {
        OverworldPlayer_UpdateIdleBlink(player);
        return;
    }

    /* Any movement input: snap back to eyes-open and restart blink cadence. */
    OverworldPlayer_ResetBlinkToOpen(player);

    if (axisX != 0 && axisY != 0)
    {
        velocityX = (axisX > 0)
            ? OVERWORLD_PLAYER_SPEED_DIAGONAL
            : -OVERWORLD_PLAYER_SPEED_DIAGONAL;
        velocityY = (axisY > 0)
            ? OVERWORLD_PLAYER_SPEED_DIAGONAL
            : -OVERWORLD_PLAYER_SPEED_DIAGONAL;
    }
    else if (axisX != 0)
    {
        velocityX = (axisX > 0)
            ? OVERWORLD_PLAYER_SPEED_CARDINAL
            : -OVERWORLD_PLAYER_SPEED_CARDINAL;
    }
    else
    {
        velocityY = (axisY > 0)
            ? OVERWORLD_PLAYER_SPEED_CARDINAL
            : -OVERWORLD_PLAYER_SPEED_CARDINAL;
    }

    if (axisX < 0 && axisY > 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_DOWN_LEFT;
    }
    else if (axisX < 0 && axisY < 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_UP_LEFT;
    }
    else if (axisX > 0 && axisY > 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_DOWN_RIGHT;
    }
    else if (axisX > 0 && axisY < 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_UP_RIGHT;
    }
    else if (axisX < 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_LEFT;
    }
    else if (axisX > 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_RIGHT;
    }
    else if (axisY < 0)
    {
        player->facing = OVERWORLD_PLAYER_FACING_UP;
    }
    else
    {
        player->facing = OVERWORLD_PLAYER_FACING_DOWN;
    }

    if (player->facing != previousFacing)
    {
        OverworldPlayer_ResetBlinkToOpen(player);
    }

    proposedX = player->worldX + velocityX;
    proposedY = player->worldY + velocityY;
    player->worldX = OverworldPlayer_ResolveAxisX(
        map,
        player->worldX,
        player->worldY,
        proposedX);
    player->worldY = OverworldPlayer_ResolveAxisY(
        map,
        player->worldX,
        player->worldY,
        proposedY);
}

void OverworldPlayer_GetScreenTopLeft(
    const OverworldPlayer *player,
    int cameraPixelX,
    int cameraPixelY,
    int *outScreenX,
    int *outScreenY)
{
    int worldPixelX;
    int worldPixelY;

    if (!player || !outScreenX || !outScreenY)
    {
        return;
    }

    worldPixelX = Fixed24_8_FloorToIntNonNeg(player->worldX);
    worldPixelY = Fixed24_8_FloorToIntNonNeg(player->worldY);
    *outScreenX = worldPixelX - cameraPixelX - (OVERWORLD_PLAYER_SPRITE_WIDTH / 2);
    *outScreenY = worldPixelY - cameraPixelY - OVERWORLD_PLAYER_SPRITE_HEIGHT;
}

void OverworldPlayer_PrepareOam(
    const OverworldPlayer *player,
    int cameraPixelX,
    int cameraPixelY)
{
    int screenX;
    int screenY;
    int tileIndex;

    if (!player)
    {
        return;
    }

    OverworldPlayer_GetScreenTopLeft(
        player,
        cameraPixelX,
        cameraPixelY,
        &screenX,
        &screenY);
    tileIndex = OverworldPlayer_GetIdleSpriteTileIndex(player);
    Oam_SetRegular(
        OAM_INDEX_OVERWORLD_PLAYER,
        screenX,
        screenY,
        tileIndex,
        OVERWORLD_PLAYER_SPRITE_PALETTE_BANK,
        0,
        OAM_ATTR0_SHAPE_TALL,
        OAM_SIZE_TALL_16X32,
        0,
        0);
}

int OverworldPlayer_GetIdleSpriteTileIndex(const OverworldPlayer *player)
{
    int facing;

    if (!player)
    {
        return OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(0);
    }
    facing = (int)player->facing;
    if (facing < 0 || facing >= OVERWORLD_PLAYER_FACING_SPRITE_COUNT)
    {
        facing = 0;
    }
    if (player->blinkEyesClosed && gOverworldPlayerFacingHasBlink[facing])
    {
        return OVERWORLD_PLAYER_SPRITE_BLINK_OBJ_TILE_INDEX(facing);
    }
    return OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(facing);
}
