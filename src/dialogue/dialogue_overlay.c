#include "dialogue/dialogue_overlay.h"

#include "core/gba_types.h"
#include "core/oam.h"
#include "core/video.h"
#include "dialogue/dialogue_panel.h"
#include "dialogue/dialogue_portrait_controller.h"
#include "dialogue/dialogue_resources.h"

#define DIALOGUE_DEMO_BG_TILE_COUNT 1
#define DIALOGUE_DEMO_BG_TILE_WORDS (DIALOGUE_DEMO_BG_TILE_COUNT * 8)

static int s_overlayActive;
static DialogueOverlayMode s_mode;

static const u16 s_demoBgPalette[16] = {
    RGB15(0, 0, 0),
    RGB15(6, 20, 8),
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
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
    RGB15(0, 0, 0)
};

static const u32 s_demoBgTiles[DIALOGUE_DEMO_BG_TILE_WORDS] = {
    0x11111111u, 0x11111111u, 0x11111111u, 0x11111111u,
    0x11111111u, 0x11111111u, 0x11111111u, 0x11111111u
};

static void DialogueOverlay_FillBg3Solid(void)
{
    volatile u16 *map = Video_GetMode0Bg3Screenbase();
    int i;

    for (i = 0; i < 32 * 32; i++)
    {
        map[i] = 0;
    }
}

void DialogueOverlay_Init(DialogueOverlayMode mode)
{
    s_mode = mode;
    s_overlayActive = 1;

    if (mode == DIALOGUE_OVERLAY_MODE_DEMO)
    {
        Video_BeginMode0FieldBg3(
            s_demoBgPalette,
            16,
            s_demoBgTiles,
            DIALOGUE_DEMO_BG_TILE_WORDS);
        DialogueOverlay_FillBg3Solid();
        Video_SetMode0Bg3Scroll(0, 0);
        Oam_InitHidden();
    }
    else
    {
        /* FIELD: keep BG3 map; suppress host player sprite while overlay owns OBJ. */
        Oam_Hide(OAM_INDEX_OVERWORLD_PLAYER);
    }

    DialoguePanel_Init();
    DialoguePanel_ShowFrame();
    DialoguePortrait_Init();

    if (mode == DIALOGUE_OVERLAY_MODE_DEMO)
    {
        REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_WIN0;
    }
    else
    {
        REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_WIN0;
    }
    DialoguePortrait_Commit();
}

void DialogueOverlay_Shutdown(void)
{
    if (!s_overlayActive)
    {
        return;
    }
    DialoguePortrait_Cancel();
    DialoguePortrait_Shutdown();
    DialoguePanel_Close();
    REG_WIN0H = 0;
    REG_WIN0V = 0;
    REG_WININ = 0;
    REG_WINOUT = 0;
    REG_BG0CNT = 0;
    if (s_mode == DIALOGUE_OVERLAY_MODE_DEMO)
    {
        REG_BG3CNT = 0;
        Oam_InitHidden();
        Oam_Commit();
    }
    else
    {
        Oam_Hide(DIALOGUE_OAM_INDEX_LEFT_PORTRAIT);
        Oam_Hide(DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT);
        /* Field host restores player OAM / DISPCNT without WIN0. */
        REG_DISPCNT = DCNT_MODE0_FIELD_BG3_OBJ;
        Oam_Commit();
    }
    s_overlayActive = 0;
}

void DialogueOverlay_Commit(void)
{
    if (!s_overlayActive)
    {
        return;
    }
    if (s_mode == DIALOGUE_OVERLAY_MODE_FIELD)
    {
        Oam_Hide(OAM_INDEX_OVERWORLD_PLAYER);
    }
    DialoguePortrait_Commit();
}

int DialogueOverlay_IsActive(void)
{
    return s_overlayActive;
}
