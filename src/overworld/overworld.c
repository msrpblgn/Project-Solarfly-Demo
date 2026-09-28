#include "overworld/overworld.h"

#include "assets/overworld_player_sprite.h"
#include "core/fixed.h"
#include "core/input.h"
#include "core/oam.h"
#include "core/random.h"
#include "core/video.h"
#include "overworld/overworld_map.h"
#include "overworld/overworld_player.h"
#include "overworld/overworld_streamer.h"

#define OVERWORLD_CAMERA_MAX_X 400
#define OVERWORLD_CAMERA_MAX_Y 320

static const OverworldMapDef *s_activeMap;
static OverworldBg3Streamer s_bg3Streamer;
static OverworldPlayer s_player;
static Fixed24_8 s_cameraX;
static Fixed24_8 s_cameraY;
static int s_battleTriggerLatched;
static int s_dialogueLock;
static int s_dialogueInteractLatch;

static void Overworld_FollowCameraFromPlayer(void)
{
    Fixed24_8 desiredCameraX =
        s_player.worldX - Fixed24_8_FromInt(OVERWORLD_PLAYER_SCREEN_FOOT_X);
    Fixed24_8 desiredCameraY =
        s_player.worldY - Fixed24_8_FromInt(OVERWORLD_PLAYER_SCREEN_FOOT_Y);

    s_cameraX = Fixed24_8_Clamp(
        desiredCameraX,
        Fixed24_8_FromInt(0),
        Fixed24_8_FromInt(OVERWORLD_CAMERA_MAX_X));
    s_cameraY = Fixed24_8_Clamp(
        desiredCameraY,
        Fixed24_8_FromInt(0),
        Fixed24_8_FromInt(OVERWORLD_CAMERA_MAX_Y));
}

static int Overworld_IsFootInBattleTrigger(void)
{
    Fixed24_8 triggerX = s_player.worldX;
    Fixed24_8 triggerY = s_player.worldY - 1;

    return triggerX >= Fixed24_8_FromInt(OVERWORLD_TRIGGER_LEFT)
        && triggerX < Fixed24_8_FromInt(OVERWORLD_TRIGGER_RIGHT)
        && triggerY >= Fixed24_8_FromInt(OVERWORLD_TRIGGER_TOP)
        && triggerY < Fixed24_8_FromInt(OVERWORLD_TRIGGER_BOTTOM);
}

static int Overworld_IsFootInDialogueTrigger(void)
{
    Fixed24_8 triggerX = s_player.worldX;
    Fixed24_8 triggerY = s_player.worldY - 1;

    return triggerX >= Fixed24_8_FromInt(OVERWORLD_DIALOGUE_TRIGGER_LEFT)
        && triggerX < Fixed24_8_FromInt(OVERWORLD_DIALOGUE_TRIGGER_RIGHT)
        && triggerY >= Fixed24_8_FromInt(OVERWORLD_DIALOGUE_TRIGGER_TOP)
        && triggerY < Fixed24_8_FromInt(OVERWORLD_DIALOGUE_TRIGGER_BOTTOM);
}

static void Overworld_PreparePlayerPresentation(void)
{
    int cameraPixelX;
    int cameraPixelY;

    Video_LoadObjPaletteBank(
        OVERWORLD_PLAYER_SPRITE_PALETTE_BANK,
        gOverworldPlayerSpritePalette,
        OVERWORLD_PLAYER_SPRITE_PALETTE_COUNT);
    {
        int facing;

        for (facing = 0; facing < OVERWORLD_PLAYER_FACING_SPRITE_COUNT; facing++)
        {
            Video_LoadObjTiles4bpp(
                OVERWORLD_PLAYER_SPRITE_IDLE_OBJ_TILE_INDEX(facing),
                gOverworldPlayerIdleTiles[facing],
                OVERWORLD_PLAYER_SPRITE_TILE_WORDS);
            Video_LoadObjTiles4bpp(
                OVERWORLD_PLAYER_SPRITE_BLINK_OBJ_TILE_INDEX(facing),
                gOverworldPlayerBlinkTiles[facing],
                OVERWORLD_PLAYER_SPRITE_TILE_WORDS);
        }
    }

    cameraPixelX = Fixed24_8_FloorToIntNonNeg(s_cameraX);
    cameraPixelY = Fixed24_8_FloorToIntNonNeg(s_cameraY);
    OverworldBg3Streamer_SetCamera(&s_bg3Streamer, cameraPixelX, cameraPixelY);
    OverworldPlayer_PrepareOam(&s_player, cameraPixelX, cameraPixelY);
    Oam_Commit();
}

static void Overworld_LoadFieldGraphicsAndCommit(void)
{
    Oam_InitHidden();
    Oam_Commit();

    OverworldBg3Streamer_Init(&s_bg3Streamer, s_activeMap);
    Overworld_PreparePlayerPresentation();
    Video_EndMode0FieldBg3();
}

void Overworld_Init(void)
{
    s_activeMap = OverworldMap_GetDiagnostic();
    Random_Init(0x4F564257u);
    Video_BeginMode0FieldBg3(
        OverworldMap_GetDiagnosticPalette(),
        16,
        OverworldMap_GetDiagnosticTiles(),
        OverworldMap_GetDiagnosticTileWordCount());

    OverworldPlayer_Reset(&s_player);
    s_cameraX = Fixed24_8_FromInt(0);
    s_cameraY = Fixed24_8_FromInt(0);
    s_battleTriggerLatched = 0;
    s_dialogueLock = 0;
    s_dialogueInteractLatch = 0;
    Overworld_LoadFieldGraphicsAndCommit();
}

void Overworld_ResumeAfterBattle(void)
{
    s_activeMap = OverworldMap_GetDiagnostic();
    Video_BeginMode0FieldBg3(
        OverworldMap_GetDiagnosticPalette(),
        16,
        OverworldMap_GetDiagnosticTiles(),
        OverworldMap_GetDiagnosticTileWordCount());
    Overworld_FollowCameraFromPlayer();
    /* Keep player position/facing and consumed trigger latch. */
    Overworld_LoadFieldGraphicsAndCommit();
}

void Overworld_LockForDialogue(void)
{
    if (s_dialogueLock)
    {
        return;
    }
    s_dialogueLock = 1;
    /* Camera/player world state preserved; overlay hides player OAM. */
}

void Overworld_UnlockAfterDialogue(void)
{
    int cameraPixelX;
    int cameraPixelY;

    if (!s_dialogueLock)
    {
        return;
    }
    s_dialogueLock = 0;
    /* Hold-A through close must not instantly restart dialogue. */
    s_dialogueInteractLatch = 1;

    /* Restore field DISPCNT/BG3 already done by overlay; rebuild player OAM. */
    cameraPixelX = Fixed24_8_FloorToIntNonNeg(s_cameraX);
    cameraPixelY = Fixed24_8_FloorToIntNonNeg(s_cameraY);
    OverworldBg3Streamer_SetCamera(&s_bg3Streamer, cameraPixelX, cameraPixelY);
    Overworld_PreparePlayerPresentation();
    Video_EndMode0FieldBg3();
}

int Overworld_IsDialogueLocked(void)
{
    return s_dialogueLock;
}

int Overworld_Update(void)
{
    int cameraPixelX;
    int cameraPixelY;
    int inBattleTrigger;
    int inDialogueTrigger;

    if (s_dialogueLock)
    {
        return OVERWORLD_UPDATE_NONE;
    }

    if (Input_JustPressed(KEY_B))
    {
        return OVERWORLD_UPDATE_EXIT_MENU;
    }

    OverworldPlayer_UpdateFromInput(&s_player, s_activeMap);
    Overworld_FollowCameraFromPlayer();

    inDialogueTrigger = Overworld_IsFootInDialogueTrigger();
    if (!Input_IsPressed(KEY_A))
    {
        s_dialogueInteractLatch = 0;
    }
    if (inDialogueTrigger
        && Input_JustPressed(KEY_A)
        && !s_dialogueInteractLatch)
    {
        /* Consume this A for dialogue start; movement already applied this frame. */
        return OVERWORLD_UPDATE_START_DIALOGUE;
    }

    inBattleTrigger = Overworld_IsFootInBattleTrigger();
    if (inBattleTrigger)
    {
        if (!s_battleTriggerLatched)
        {
            s_battleTriggerLatched = 1;
            return OVERWORLD_UPDATE_ENTER_BATTLE;
        }
    }
    else
    {
        s_battleTriggerLatched = 0;
    }

    cameraPixelX = Fixed24_8_FloorToIntNonNeg(s_cameraX);
    cameraPixelY = Fixed24_8_FloorToIntNonNeg(s_cameraY);
    OverworldBg3Streamer_SetCamera(&s_bg3Streamer, cameraPixelX, cameraPixelY);
    OverworldPlayer_PrepareOam(&s_player, cameraPixelX, cameraPixelY);
    Oam_Commit();
    return OVERWORLD_UPDATE_NONE;
}

void Overworld_Shutdown(void)
{
    Oam_InitHidden();
    Oam_Commit();
    s_activeMap = 0;
    s_cameraX = Fixed24_8_FromInt(0);
    s_cameraY = Fixed24_8_FromInt(0);
    s_battleTriggerLatched = 0;
    s_dialogueLock = 0;
    s_dialogueInteractLatch = 0;
    OverworldPlayer_Reset(&s_player);
}
