/*
 * Host test: eight-facing idle tiles + blink cadence for blinkable facings.
 *
 * Build (WSL):
 *   gcc -std=c11 -Wall -Wextra -O0 -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_overworld_blink_host \
 *       tools/host_tests/test_overworld_blink_host.c \
 *       src/overworld/overworld_player.c \
 *       src/assets/overworld_player_sprite.c \
 *       src/core/random.c
 */

#include <stdio.h>
#include <string.h>

#include "assets/overworld_player_sprite.h"
#include "core/input.h"
#include "core/oam.h"
#include "core/random.h"
#include "overworld/overworld_collision.h"
#include "overworld/overworld_map.h"
#include "overworld/overworld_player.h"

static int g_failures;
static u16 s_keysHeld;

void Input_Poll(void)
{
}

int Input_IsPressed(u16 keys)
{
    return (s_keysHeld & keys) == keys;
}

int Input_JustPressed(u16 keys)
{
    return Input_IsPressed(keys);
}

void Oam_SetRegular(
    int index,
    int screenX,
    int screenY,
    int tileIndex,
    int paletteBank,
    int priority,
    u16 shapeAttr0,
    int sizeCode,
    int hFlip,
    int vFlip)
{
    (void)index;
    (void)screenX;
    (void)screenY;
    (void)tileIndex;
    (void)paletteBank;
    (void)priority;
    (void)shapeAttr0;
    (void)sizeCode;
    (void)hFlip;
    (void)vFlip;
}

int OverworldCollision_IsSolidAtPixel(const OverworldMapDef *map, int x, int y)
{
    (void)map;
    (void)x;
    (void)y;
    return 0;
}

static void ExpectTrue(int cond, const char *label)
{
    if (!cond)
    {
        printf("FAIL: %s\n", label);
        g_failures++;
    }
    else
    {
        printf("OK:   %s\n", label);
    }
}

static void ExpectEqInt(int got, int want, const char *label)
{
    if (got != want)
    {
        printf("FAIL: %s (got %d want %d)\n", label, got, want);
        g_failures++;
    }
    else
    {
        printf("OK:   %s\n", label);
    }
}

static void TestEightFacingIdleBanks(void)
{
    int facing;
    int i;
    int downDiffers = 0;

    printf("\n== Eight facing idle banks ==\n");
    ExpectEqInt(OVERWORLD_PLAYER_FACING_SPRITE_COUNT, 8, "eight facing sprites");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[0], 1, "down blinks");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[1], 1, "down-left blinks");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[2], 1, "left blinks");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[3], 0, "up-left no blink");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[4], 0, "up no blink");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[5], 0, "up-right no blink");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[6], 1, "right blinks");
    ExpectEqInt(gOverworldPlayerFacingHasBlink[7], 1, "down-right blinks");

    for (facing = 0; facing < OVERWORLD_PLAYER_FACING_SPRITE_COUNT; facing++)
    {
        int opaqueWords = 0;
        for (i = 0; i < OVERWORLD_PLAYER_SPRITE_TILE_WORDS; i++)
        {
            if (gOverworldPlayerIdleTiles[facing][i] != 0)
            {
                opaqueWords++;
            }
        }
        ExpectTrue(opaqueWords > 0, "facing idle has tile data");
    }

    for (i = 0; i < OVERWORLD_PLAYER_SPRITE_TILE_WORDS; i++)
    {
        if (gOverworldPlayerIdleTiles[0][i] != gOverworldPlayerBlinkTiles[0][i])
        {
            downDiffers = 1;
            break;
        }
    }
    ExpectTrue(downDiffers, "down blink tiles differ from open");
}

static void TestDownIdleBlinkCycle(void)
{
    OverworldPlayer player;
    int frames;
    int sawClosed = 0;
    int closedFrames = 0;
    int openBefore = 0;

    printf("\n== Down idle blink cycle ==\n");
    Random_Init(1);
    memset(&player, 0, sizeof(player));
    OverworldPlayer_Reset(&player);
    ExpectEqInt(player.facing, OVERWORLD_PLAYER_FACING_DOWN, "starts facing down");
    ExpectEqInt(player.blinkEyesClosed, 0, "starts eyes open");
    ExpectTrue(
        player.blinkOpenFrames >= 240 && player.blinkOpenFrames <= 360,
        "open duration in 240..360");
    ExpectTrue(
        player.blinkClosedFrames >= 6 && player.blinkClosedFrames <= 8,
        "closed duration in 6..8");

    s_keysHeld = 0;
    openBefore = player.blinkOpenFrames;
    for (frames = 0; frames < openBefore + 20; frames++)
    {
        OverworldPlayer_UpdateFromInput(&player, 0);
        if (player.blinkEyesClosed)
        {
            sawClosed = 1;
            closedFrames++;
            ExpectEqInt(
                OverworldPlayer_GetIdleSpriteTileIndex(&player),
                OVERWORLD_PLAYER_SPRITE_BLINK_OBJ_TILE_INDEX(0),
                "closed uses blink tiles");
        }
        else if (!sawClosed)
        {
            ExpectEqInt(
                OverworldPlayer_GetIdleSpriteTileIndex(&player),
                OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(0),
                "open uses idle tiles");
        }
        else if (sawClosed && !player.blinkEyesClosed)
        {
            break;
        }
    }
    ExpectTrue(sawClosed, "eventually blinks");
    ExpectTrue(closedFrames >= 6 && closedFrames <= 8, "blink lasts 6..8 frames");
    ExpectEqInt(player.blinkEyesClosed, 0, "returns to eyes open after blink");
}

static void TestUpFacingNeverBlinks(void)
{
    OverworldPlayer player;
    int i;

    printf("\n== Up facing never blinks ==\n");
    Random_Init(3);
    memset(&player, 0, sizeof(player));
    OverworldPlayer_Reset(&player);
    player.facing = OVERWORLD_PLAYER_FACING_UP;
    player.blinkTimer = 1;
    player.blinkEyesClosed = 0;
    s_keysHeld = 0;
    for (i = 0; i < 20; i++)
    {
        OverworldPlayer_UpdateFromInput(&player, 0);
    }
    ExpectEqInt(player.blinkEyesClosed, 0, "up stays eyes open");
    ExpectEqInt(
        OverworldPlayer_GetIdleSpriteTileIndex(&player),
        OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(OVERWORLD_PLAYER_FACING_UP),
        "up uses idle bank only");
}

static void TestDiagonalFacingTileIndex(void)
{
    OverworldPlayer player;

    printf("\n== Diagonal facing tile indices ==\n");
    memset(&player, 0, sizeof(player));
    OverworldPlayer_Reset(&player);

    player.facing = OVERWORLD_PLAYER_FACING_DOWN_LEFT;
    player.blinkEyesClosed = 0;
    ExpectEqInt(
        OverworldPlayer_GetIdleSpriteTileIndex(&player),
        OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(OVERWORLD_PLAYER_FACING_DOWN_LEFT),
        "down-left idle tile");
    player.blinkEyesClosed = 1;
    ExpectEqInt(
        OverworldPlayer_GetIdleSpriteTileIndex(&player),
        OVERWORLD_PLAYER_SPRITE_BLINK_OBJ_TILE_INDEX(OVERWORLD_PLAYER_FACING_DOWN_LEFT),
        "down-left blink tile");

    player.facing = OVERWORLD_PLAYER_FACING_UP_RIGHT;
    player.blinkEyesClosed = 1;
    ExpectEqInt(
        OverworldPlayer_GetIdleSpriteTileIndex(&player),
        OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(OVERWORLD_PLAYER_FACING_UP_RIGHT),
        "up-right ignores closed flag");
}

static void TestMoveResetsBlink(void)
{
    OverworldPlayer player;
    int i;

    printf("\n== Move / facing change resets blink ==\n");
    Random_Init(2);
    memset(&player, 0, sizeof(player));
    OverworldPlayer_Reset(&player);
    player.blinkTimer = 1;
    s_keysHeld = 0;
    OverworldPlayer_UpdateFromInput(&player, 0);
    ExpectEqInt(player.blinkEyesClosed, 1, "forced into blink");

    s_keysHeld = KEY_RIGHT;
    OverworldPlayer_UpdateFromInput(&player, 0);
    ExpectEqInt(player.blinkEyesClosed, 0, "movement restores open eyes");
    ExpectEqInt(player.facing, OVERWORLD_PLAYER_FACING_RIGHT, "faces right");
    ExpectEqInt(
        OverworldPlayer_GetIdleSpriteTileIndex(&player),
        OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(OVERWORLD_PLAYER_FACING_RIGHT),
        "moving uses open tiles");

    s_keysHeld = 0;
    for (i = 0; i < 3; i++)
    {
        OverworldPlayer_UpdateFromInput(&player, 0);
    }
    ExpectEqInt(player.blinkEyesClosed, 0, "non-down idle clears closed eyes");
}

int main(void)
{
    g_failures = 0;
    TestEightFacingIdleBanks();
    TestDownIdleBlinkCycle();
    TestUpFacingNeverBlinks();
    TestDiagonalFacingTileIndex();
    TestMoveResetsBlink();
    if (g_failures)
    {
        printf("\n%d failure(s)\n", g_failures);
        return 1;
    }
    printf("\nALL TESTS PASSED\n");
    return 0;
}
