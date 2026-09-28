#include "dialogue/dialogue_portrait_controller.h"

#include "assets/dialogue_portraits.h"
#include "core/oam.h"
#include "core/video.h"
#include "dialogue/dialogue_resources.h"

typedef struct PortraitSlot {
    DialoguePortraitActor actor;
    DialoguePortraitSlotState state;
    int x;
    int y;
    int active;
    int tileBase;
    int paletteBank;
    int oamIndex;
} PortraitSlot;

typedef enum TechPhase {
    TECH_NONE = 0,
    TECH_LEFT_EXIT,
    TECH_LEFT_HOLD_OFF,
    TECH_LEFT_ENTER_ALT,
    TECH_LEFT_HOLD_ALT,
    TECH_LEFT_EXIT_ALT,
    TECH_LEFT_RETURN,
    TECH_DUAL_EXIT,
    TECH_DUAL_ENTER,
    TECH_DONE
} TechPhase;

static PortraitSlot s_slots[2];
static DialoguePortraitSceneState s_scene;
static int s_frame;
static int s_focusLeftActive;
static int s_enteringLeftActive;
static TechPhase s_tech;
static int s_active;
static int s_techFinished;

static const int s_focusLeftX[DIALOGUE_PORTRAIT_FOCUS_FRAMES] = {
    -16, -14, -12, -10, -8
};
static const int s_focusLeftY[DIALOGUE_PORTRAIT_FOCUS_FRAMES] = {
    44, 42, 40, 38, 36
};
static const int s_focusRightX[DIALOGUE_PORTRAIT_FOCUS_FRAMES] = {
    116, 118, 120, 122, 124
};
static const int s_focusRightY[DIALOGUE_PORTRAIT_FOCUS_FRAMES] = {
    36, 38, 40, 42, 44
};

static void DialoguePortrait_EnableWindow(void)
{
    /* Exclusive bottom at clip Y: OBJ visible for y in [0, 116). */
    REG_WIN0H = (u16)((0 << 8) | SCREEN_WIDTH);
    REG_WIN0V = (u16)((0 << 8) | DIALOGUE_UI_PORTRAIT_CLIP_Y);
    REG_WININ = (u16)(WIN_BG0 | WIN_BG3 | WIN_OBJ);
    REG_WINOUT = (u16)(WIN_BG0 | WIN_BG3);
}

static void DialoguePortrait_DisableWindow(void)
{
    REG_WIN0H = 0;
    REG_WIN0V = 0;
    REG_WININ = 0;
    REG_WINOUT = 0;
}

static const u16 *DialoguePortrait_ActivePalette(DialoguePortraitActor actor)
{
    if (actor == DIALOGUE_PORTRAIT_ACTOR_AKSIL)
    {
        return gDialoguePortraitAksilActivePalette;
    }
    return gDialoguePortraitProtagonistActivePalette;
}

static const u16 *DialoguePortrait_InactivePalette(DialoguePortraitActor actor)
{
    if (actor == DIALOGUE_PORTRAIT_ACTOR_AKSIL)
    {
        return gDialoguePortraitAksilInactivePalette;
    }
    return gDialoguePortraitProtagonistInactivePalette;
}

static const u32 *DialoguePortrait_Tiles(DialoguePortraitActor actor)
{
    if (actor == DIALOGUE_PORTRAIT_ACTOR_AKSIL)
    {
        return gDialoguePortraitAksilTiles;
    }
    return gDialoguePortraitProtagonistTiles;
}

static void DialoguePortrait_LoadSlotGraphics(PortraitSlot *slot)
{
    const u16 *pal;

    if (slot->actor == DIALOGUE_PORTRAIT_ACTOR_NONE)
    {
        return;
    }
    Video_LoadObjTiles4bpp(
        slot->tileBase,
        DialoguePortrait_Tiles(slot->actor),
        DIALOGUE_PORTRAIT_TILE_WORDS);
    pal = slot->active
        ? DialoguePortrait_ActivePalette(slot->actor)
        : DialoguePortrait_InactivePalette(slot->actor);
    Video_LoadObjPaletteBank(slot->paletteBank, pal, DIALOGUE_PORTRAIT_PALETTE_COUNT);
}

static void DialoguePortrait_ApplyPalette(PortraitSlot *slot)
{
    const u16 *pal;

    if (slot->actor == DIALOGUE_PORTRAIT_ACTOR_NONE)
    {
        return;
    }
    pal = slot->active
        ? DialoguePortrait_ActivePalette(slot->actor)
        : DialoguePortrait_InactivePalette(slot->actor);
    Video_LoadObjPaletteBank(slot->paletteBank, pal, DIALOGUE_PORTRAIT_PALETTE_COUNT);
}

static void DialoguePortrait_WriteOam(const PortraitSlot *slot)
{
    if (slot->actor == DIALOGUE_PORTRAIT_ACTOR_NONE
        || slot->state == DIALOGUE_PORTRAIT_HIDDEN)
    {
        Oam_Hide(slot->oamIndex);
        return;
    }
    Oam_SetAffine(
        slot->oamIndex,
        slot->x,
        slot->y,
        slot->tileBase,
        slot->paletteBank,
        DIALOGUE_OBJ_PRIORITY_PORTRAIT,
        DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT,
        OAM_SIZE_SQUARE_64X64,
        1);
}

static int DialoguePortrait_Lerp(int a, int b, int frame, int frames)
{
    if (frames <= 1)
    {
        return b;
    }
    if (frame <= 0)
    {
        return a;
    }
    if (frame >= frames - 1)
    {
        return b;
    }
    return a + ((b - a) * frame) / (frames - 1);
}

static void DialoguePortrait_SettledPositions(int leftActive)
{
    if (leftActive)
    {
        s_slots[0].x = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X;
        s_slots[0].y = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y;
        s_slots[0].active = 1;
        s_slots[0].state = DIALOGUE_PORTRAIT_ACTIVE;
        s_slots[1].x = DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_X;
        s_slots[1].y = DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_Y;
        s_slots[1].active = 0;
        s_slots[1].state = DIALOGUE_PORTRAIT_INACTIVE;
    }
    else
    {
        s_slots[0].x = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X;
        s_slots[0].y = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_Y;
        s_slots[0].active = 0;
        s_slots[0].state = DIALOGUE_PORTRAIT_INACTIVE;
        s_slots[1].x = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X;
        s_slots[1].y = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y;
        s_slots[1].active = 1;
        s_slots[1].state = DIALOGUE_PORTRAIT_ACTIVE;
    }
    DialoguePortrait_ApplyPalette(&s_slots[0]);
    DialoguePortrait_ApplyPalette(&s_slots[1]);
}

void DialoguePortrait_Init(void)
{
    s_active = 1;
    s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
    s_frame = 0;
    s_tech = TECH_NONE;
    s_techFinished = 0;
    s_focusLeftActive = 0;
    s_enteringLeftActive = 0;

    s_slots[0].actor = DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST;
    s_slots[0].state = DIALOGUE_PORTRAIT_HIDDEN;
    s_slots[0].x = DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X;
    s_slots[0].y = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_Y;
    s_slots[0].active = 0;
    s_slots[0].tileBase = DIALOGUE_OBJ_TILE_BASE_LEFT;
    s_slots[0].paletteBank = DIALOGUE_OBJ_PALETTE_BANK_LEFT;
    s_slots[0].oamIndex = DIALOGUE_OAM_INDEX_LEFT_PORTRAIT;

    s_slots[1].actor = DIALOGUE_PORTRAIT_ACTOR_AKSIL;
    s_slots[1].state = DIALOGUE_PORTRAIT_HIDDEN;
    s_slots[1].x = DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X;
    s_slots[1].y = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y;
    s_slots[1].active = 0;
    s_slots[1].tileBase = DIALOGUE_OBJ_TILE_BASE_RIGHT;
    s_slots[1].paletteBank = DIALOGUE_OBJ_PALETTE_BANK_RIGHT;
    s_slots[1].oamIndex = DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT;

    DialoguePortrait_EnableWindow();
    Oam_SetAffineMatrix(
        DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT,
        (s16)OAM_AFFINE_SCALE_2X,
        0,
        0,
        (s16)OAM_AFFINE_SCALE_2X);
    DialoguePortrait_LoadSlotGraphics(&s_slots[0]);
    DialoguePortrait_LoadSlotGraphics(&s_slots[1]);
    Oam_Hide(s_slots[0].oamIndex);
    Oam_Hide(s_slots[1].oamIndex);
}

void DialoguePortrait_Shutdown(void)
{
    if (!s_active)
    {
        return;
    }
    Oam_Hide(DIALOGUE_OAM_INDEX_LEFT_PORTRAIT);
    Oam_Hide(DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT);
    DialoguePortrait_DisableWindow();
    s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
    s_tech = TECH_NONE;
    s_active = 0;
}

void DialoguePortrait_BeginEntrance(int leftActive)
{
    int endLx;
    int endLy;
    int endRx;
    int endRy;

    s_enteringLeftActive = leftActive ? 1 : 0;
    s_focusLeftActive = s_enteringLeftActive;
    if (leftActive)
    {
        endLx = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X;
        endLy = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y;
        endRx = DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_X;
        endRy = DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_Y;
        s_slots[0].active = 1;
        s_slots[1].active = 0;
    }
    else
    {
        endLx = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X;
        endLy = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_Y;
        endRx = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X;
        endRy = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y;
        s_slots[0].active = 0;
        s_slots[1].active = 1;
    }
    (void)endLx;
    (void)endLy;
    (void)endRx;
    (void)endRy;

    s_slots[0].actor = DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST;
    s_slots[1].actor = DIALOGUE_PORTRAIT_ACTOR_AKSIL;
    DialoguePortrait_LoadSlotGraphics(&s_slots[0]);
    DialoguePortrait_LoadSlotGraphics(&s_slots[1]);
    DialoguePortrait_ApplyPalette(&s_slots[0]);
    DialoguePortrait_ApplyPalette(&s_slots[1]);

    s_slots[0].x = DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X;
    s_slots[0].y = endLy;
    s_slots[0].state = DIALOGUE_PORTRAIT_ENTERING;
    s_slots[1].x = DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X;
    s_slots[1].y = endRy;
    s_slots[1].state = DIALOGUE_PORTRAIT_ENTERING;
    s_scene = DIALOGUE_PORTRAIT_SCENE_ENTERING;
    s_frame = 0;
}

void DialoguePortrait_BeginFocus(int leftActive)
{
    if (DialoguePortrait_IsBusy())
    {
        return;
    }
    if ((leftActive && s_focusLeftActive) || (!leftActive && !s_focusLeftActive))
    {
        DialoguePortrait_SettledPositions(leftActive);
        return;
    }
    s_focusLeftActive = leftActive ? 1 : 0;
    s_scene = DIALOGUE_PORTRAIT_SCENE_FOCUS;
    s_frame = 0;
}

void DialoguePortrait_BeginExitBoth(void)
{
    if (DialoguePortrait_IsBusy())
    {
        return;
    }
    s_slots[0].state = DIALOGUE_PORTRAIT_EXITING;
    s_slots[1].state = DIALOGUE_PORTRAIT_EXITING;
    s_scene = DIALOGUE_PORTRAIT_SCENE_EXITING;
    s_frame = 0;
}

void DialoguePortrait_BeginTechTest(void)
{
    if (DialoguePortrait_IsBusy())
    {
        return;
    }
    s_scene = DIALOGUE_PORTRAIT_SCENE_TECH_TEST;
    s_tech = TECH_LEFT_EXIT;
    s_frame = 0;
    /* Right slot must remain Aksil settled for the left-replacement portion. */
    s_slots[1].actor = DIALOGUE_PORTRAIT_ACTOR_AKSIL;
    s_slots[1].active = 1;
    s_slots[1].x = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X;
    s_slots[1].y = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y;
    s_slots[1].state = DIALOGUE_PORTRAIT_ACTIVE;
    DialoguePortrait_ApplyPalette(&s_slots[1]);
}

void DialoguePortrait_Cancel(void)
{
    s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
    s_tech = TECH_NONE;
    s_frame = 0;
    s_slots[0].state = DIALOGUE_PORTRAIT_HIDDEN;
    s_slots[1].state = DIALOGUE_PORTRAIT_HIDDEN;
    Oam_Hide(s_slots[0].oamIndex);
    Oam_Hide(s_slots[1].oamIndex);
}

static void DialoguePortrait_UpdateEntering(void)
{
    int endLx;
    int endLy;
    int endRx;
    int endRy;

    if (s_enteringLeftActive)
    {
        endLx = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X;
        endLy = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y;
        endRx = DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_X;
        endRy = DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_Y;
    }
    else
    {
        endLx = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X;
        endLy = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_Y;
        endRx = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X;
        endRy = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y;
    }

    s_slots[0].x = DialoguePortrait_Lerp(
        DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
        endLx,
        s_frame,
        DIALOGUE_PORTRAIT_SLIDE_FRAMES);
    s_slots[0].y = endLy;
    s_slots[1].x = DialoguePortrait_Lerp(
        DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X,
        endRx,
        s_frame,
        DIALOGUE_PORTRAIT_SLIDE_FRAMES);
    s_slots[1].y = endRy;

    s_frame++;
    if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
    {
        DialoguePortrait_SettledPositions(s_enteringLeftActive);
        s_focusLeftActive = s_enteringLeftActive;
        s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
        s_frame = 0;
    }
}

static void DialoguePortrait_UpdateFocus(void)
{
    int f;
    int towardLeft = s_focusLeftActive;

    /*
     * Table frame 0 = right (Aksil) active; frame 4 = left (Protagonist) active.
     * Toward left plays forward; toward right plays reverse.
     */
    if (towardLeft)
    {
        f = s_frame;
    }
    else
    {
        f = (DIALOGUE_PORTRAIT_FOCUS_FRAMES - 1) - s_frame;
    }

    s_slots[0].x = s_focusLeftX[f];
    s_slots[0].y = s_focusLeftY[f];
    s_slots[1].x = s_focusRightX[f];
    s_slots[1].y = s_focusRightY[f];

    if (towardLeft)
    {
        s_slots[0].active = (s_frame >= 2) ? 1 : 0;
        s_slots[1].active = (s_frame >= 2) ? 0 : 1;
    }
    else
    {
        s_slots[0].active = (s_frame >= 2) ? 0 : 1;
        s_slots[1].active = (s_frame >= 2) ? 1 : 0;
    }
    DialoguePortrait_ApplyPalette(&s_slots[0]);
    DialoguePortrait_ApplyPalette(&s_slots[1]);
    s_slots[0].state = s_slots[0].active ? DIALOGUE_PORTRAIT_ACTIVE : DIALOGUE_PORTRAIT_INACTIVE;
    s_slots[1].state = s_slots[1].active ? DIALOGUE_PORTRAIT_ACTIVE : DIALOGUE_PORTRAIT_INACTIVE;

    s_frame++;
    if (s_frame >= DIALOGUE_PORTRAIT_FOCUS_FRAMES)
    {
        DialoguePortrait_SettledPositions(s_focusLeftActive);
        s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
        s_frame = 0;
    }
}

static void DialoguePortrait_UpdateTech(void)
{
    PortraitSlot *left = &s_slots[0];
    PortraitSlot *right = &s_slots[1];

    switch (s_tech)
    {
    case TECH_LEFT_EXIT:
        left->state = DIALOGUE_PORTRAIT_EXITING;
        left->y = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y;
        left->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X,
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        s_frame++;
        if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
        {
            left->state = DIALOGUE_PORTRAIT_HIDDEN;
            left->x = DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X;
            Oam_Hide(left->oamIndex);
            /* Swap left actor to Aksil for non-canonical dual-Aksil test. */
            left->actor = DIALOGUE_PORTRAIT_ACTOR_AKSIL;
            DialoguePortrait_LoadSlotGraphics(left);
            left->active = 1;
            DialoguePortrait_ApplyPalette(left);
            s_tech = TECH_LEFT_ENTER_ALT;
            s_frame = 0;
        }
        break;
    case TECH_LEFT_ENTER_ALT:
        left->state = DIALOGUE_PORTRAIT_ENTERING;
        left->y = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y;
        left->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
            DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        s_frame++;
        if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
        {
            left->x = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X;
            left->state = DIALOGUE_PORTRAIT_ACTIVE;
            s_tech = TECH_LEFT_HOLD_ALT;
            s_frame = 0;
        }
        break;
    case TECH_LEFT_HOLD_ALT:
        s_frame++;
        if (s_frame >= 12)
        {
            s_tech = TECH_LEFT_EXIT_ALT;
            s_frame = 0;
        }
        break;
    case TECH_LEFT_EXIT_ALT:
        left->state = DIALOGUE_PORTRAIT_EXITING;
        left->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X,
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        s_frame++;
        if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
        {
            left->actor = DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST;
            DialoguePortrait_LoadSlotGraphics(left);
            left->active = 1;
            DialoguePortrait_ApplyPalette(left);
            s_tech = TECH_LEFT_RETURN;
            s_frame = 0;
        }
        break;
    case TECH_LEFT_RETURN:
        left->state = DIALOGUE_PORTRAIT_ENTERING;
        left->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
            DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        s_frame++;
        if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
        {
            left->x = DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X;
            left->state = DIALOGUE_PORTRAIT_ACTIVE;
            s_tech = TECH_DUAL_EXIT;
            s_frame = 0;
        }
        break;
    case TECH_DUAL_EXIT:
        left->state = DIALOGUE_PORTRAIT_EXITING;
        right->state = DIALOGUE_PORTRAIT_EXITING;
        left->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X,
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        right->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X,
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        s_frame++;
        if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
        {
            left->state = DIALOGUE_PORTRAIT_HIDDEN;
            right->state = DIALOGUE_PORTRAIT_HIDDEN;
            s_tech = TECH_DUAL_ENTER;
            s_frame = 0;
        }
        break;
    case TECH_DUAL_ENTER:
        if (s_frame == 0)
        {
            left->actor = DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST;
            right->actor = DIALOGUE_PORTRAIT_ACTOR_AKSIL;
            DialoguePortrait_LoadSlotGraphics(left);
            DialoguePortrait_LoadSlotGraphics(right);
            left->active = 0;
            right->active = 1;
            DialoguePortrait_ApplyPalette(left);
            DialoguePortrait_ApplyPalette(right);
            left->state = DIALOGUE_PORTRAIT_ENTERING;
            right->state = DIALOGUE_PORTRAIT_ENTERING;
            left->y = DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_Y;
            right->y = DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y;
        }
        left->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X,
            DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        right->x = DialoguePortrait_Lerp(
            DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X,
            DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X,
            s_frame,
            DIALOGUE_PORTRAIT_SLIDE_FRAMES);
        s_frame++;
        if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
        {
            DialoguePortrait_SettledPositions(0);
            s_focusLeftActive = 0;
            s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
            s_tech = TECH_NONE;
            s_techFinished = 1;
            s_frame = 0;
        }
        break;
    default:
        s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
        s_tech = TECH_NONE;
        break;
    }
}

int DialoguePortrait_TakeTechTestFinished(void)
{
    int v = s_techFinished;
    s_techFinished = 0;
    return v;
}

static void DialoguePortrait_UpdateExiting(void)
{
    int startLx = s_slots[0].x;
    int startRx = s_slots[1].x;
    int endLx = DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X;
    int endRx = DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X;

    /* Capture starts on first frame. */
    if (s_frame == 0)
    {
        /* keep current x as start via local above — re-read each frame from lerp base */
    }

    s_slots[0].x = DialoguePortrait_Lerp(
        (s_slots[0].active
            ? DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X
            : DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X),
        endLx,
        s_frame,
        DIALOGUE_PORTRAIT_SLIDE_FRAMES);
    s_slots[1].x = DialoguePortrait_Lerp(
        (s_slots[1].active
            ? DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X
            : DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_X),
        endRx,
        s_frame,
        DIALOGUE_PORTRAIT_SLIDE_FRAMES);
    (void)startLx;
    (void)startRx;

    s_frame++;
    if (s_frame >= DIALOGUE_PORTRAIT_SLIDE_FRAMES)
    {
        s_slots[0].state = DIALOGUE_PORTRAIT_HIDDEN;
        s_slots[1].state = DIALOGUE_PORTRAIT_HIDDEN;
        s_slots[0].actor = DIALOGUE_PORTRAIT_ACTOR_NONE;
        s_slots[1].actor = DIALOGUE_PORTRAIT_ACTOR_NONE;
        Oam_Hide(s_slots[0].oamIndex);
        Oam_Hide(s_slots[1].oamIndex);
        s_scene = DIALOGUE_PORTRAIT_SCENE_IDLE;
        s_frame = 0;
    }
}

void DialoguePortrait_Update(void)
{
    if (!s_active)
    {
        return;
    }
    if (s_scene == DIALOGUE_PORTRAIT_SCENE_ENTERING)
    {
        DialoguePortrait_UpdateEntering();
    }
    else if (s_scene == DIALOGUE_PORTRAIT_SCENE_FOCUS)
    {
        DialoguePortrait_UpdateFocus();
    }
    else if (s_scene == DIALOGUE_PORTRAIT_SCENE_EXITING)
    {
        DialoguePortrait_UpdateExiting();
    }
    else if (s_scene == DIALOGUE_PORTRAIT_SCENE_TECH_TEST)
    {
        DialoguePortrait_UpdateTech();
    }
}

void DialoguePortrait_Commit(void)
{
    if (!s_active)
    {
        return;
    }
    Oam_SetAffineMatrix(
        DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT,
        (s16)OAM_AFFINE_SCALE_2X,
        0,
        0,
        (s16)OAM_AFFINE_SCALE_2X);
    DialoguePortrait_WriteOam(&s_slots[0]);
    DialoguePortrait_WriteOam(&s_slots[1]);
    Oam_Commit();
}

int DialoguePortrait_IsBusy(void)
{
    return s_scene == DIALOGUE_PORTRAIT_SCENE_ENTERING
        || s_scene == DIALOGUE_PORTRAIT_SCENE_FOCUS
        || s_scene == DIALOGUE_PORTRAIT_SCENE_EXITING
        || s_scene == DIALOGUE_PORTRAIT_SCENE_TECH_TEST;
}

int DialoguePortrait_IsSettled(void)
{
    return s_active && s_scene == DIALOGUE_PORTRAIT_SCENE_IDLE;
}

DialoguePortraitSceneState DialoguePortrait_GetSceneState(void)
{
    return s_scene;
}

DialoguePortraitActor DialoguePortrait_GetActor(DialoguePortraitSide side)
{
    if (side != DIALOGUE_PORTRAIT_SIDE_LEFT && side != DIALOGUE_PORTRAIT_SIDE_RIGHT)
    {
        return DIALOGUE_PORTRAIT_ACTOR_NONE;
    }
    return s_slots[side].actor;
}

int DialoguePortrait_IsLeftActive(void)
{
    return s_focusLeftActive;
}

void DialoguePortrait_GetPresentationPos(DialoguePortraitSide side, int *x, int *y)
{
    if (side != DIALOGUE_PORTRAIT_SIDE_LEFT && side != DIALOGUE_PORTRAIT_SIDE_RIGHT)
    {
        return;
    }
    if (x)
    {
        *x = s_slots[side].x;
    }
    if (y)
    {
        *y = s_slots[side].y;
    }
}
