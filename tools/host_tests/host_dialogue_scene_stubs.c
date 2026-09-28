/*
 * Host stubs for Dialogue_ scene runner tests (no GBA hardware).
 * Linked instead of overlay/panel/portrait implementations.
 */

#include "core/gba_types.h"
#include "dialogue/dialogue_host.h"
#include "dialogue/dialogue_overlay.h"
#include "dialogue/dialogue_panel.h"
#include "dialogue/dialogue_portrait_controller.h"

static int s_overlayActive;
static DialogueOverlayMode s_overlayMode;

static int s_portraitActive;
static DialoguePortraitSceneState s_portraitScene;
static DialoguePortraitActor s_actors[2];
static int s_focusLeft;
static int s_busyFrames;
static int s_techFinished;
static int s_lockCount;
static int s_unlockCount;

int HostPortrait_GetBusyFrames(void)
{
    return s_busyFrames;
}

int HostLock_GetLockCount(void)
{
    return s_lockCount;
}

int HostLock_GetUnlockCount(void)
{
    return s_unlockCount;
}

void HostLock_ResetCounters(void)
{
    s_lockCount = 0;
    s_unlockCount = 0;
}

void HostLock_BumpLock(void)
{
    s_lockCount++;
}

void HostLock_BumpUnlock(void)
{
    s_unlockCount++;
}

void DialogueOverlay_Init(DialogueOverlayMode mode)
{
    s_overlayMode = mode;
    s_overlayActive = 1;
    DialoguePanel_Init();
    DialoguePortrait_Init();
}

void DialogueOverlay_Shutdown(void)
{
    DialoguePortrait_Cancel();
    DialoguePortrait_Shutdown();
    DialoguePanel_Close();
    s_overlayActive = 0;
}

void DialogueOverlay_Commit(void)
{
    DialoguePortrait_Commit();
}

int DialogueOverlay_IsActive(void)
{
    return s_overlayActive;
}

DialogueOverlayMode HostOverlay_GetMode(void)
{
    return s_overlayMode;
}

void DialoguePanel_Init(void)
{
}

void DialoguePanel_ShowFrame(void)
{
}

void DialoguePanel_Close(void)
{
}

void DialoguePanel_ClearTextLine(int lineIndex)
{
    (void)lineIndex;
}

void DialoguePanel_DrawTextCharColor(int x, int y, char ch, int colorIndex)
{
    (void)x;
    (void)y;
    (void)ch;
    (void)colorIndex;
}

void DialoguePanel_SetNameplate(DialogueNameplateSide side, const char *name)
{
    (void)side;
    (void)name;
}

void DialoguePanel_SetPromptVisible(int visible)
{
    (void)visible;
}

void DialoguePanel_FlushDirty(void)
{
}

void DialoguePanel_BeginSample(DialogueNameplateSide side, const char *name, const char *demoHint)
{
    (void)side;
    (void)name;
    (void)demoHint;
}

void DialoguePanel_SetText(const char *line1, const char *line2)
{
    (void)line1;
    (void)line2;
}

void DialoguePanel_ClearContent(void)
{
}

int DialoguePanel_CountDirty(void)
{
    return 0;
}

void DialoguePortrait_Init(void)
{
    s_portraitActive = 1;
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_IDLE;
    s_actors[0] = DIALOGUE_PORTRAIT_ACTOR_NONE;
    s_actors[1] = DIALOGUE_PORTRAIT_ACTOR_NONE;
    s_focusLeft = 0;
    s_busyFrames = 0;
    s_techFinished = 0;
}

void DialoguePortrait_Shutdown(void)
{
    s_portraitActive = 0;
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_IDLE;
}

void DialoguePortrait_BeginEntrance(int leftActive)
{
    s_actors[0] = DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST;
    s_actors[1] = DIALOGUE_PORTRAIT_ACTOR_AKSIL;
    s_focusLeft = leftActive ? 1 : 0;
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_ENTERING;
    s_busyFrames = 2;
}

void DialoguePortrait_BeginFocus(int leftActive)
{
    if ((leftActive && s_focusLeft) || (!leftActive && !s_focusLeft))
    {
        s_portraitScene = DIALOGUE_PORTRAIT_SCENE_IDLE;
        s_busyFrames = 0;
        return;
    }
    s_focusLeft = leftActive ? 1 : 0;
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_FOCUS;
    s_busyFrames = 2;
}

void DialoguePortrait_BeginExitBoth(void)
{
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_EXITING;
    s_busyFrames = 2;
}

void DialoguePortrait_BeginTechTest(void)
{
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_TECH_TEST;
    s_busyFrames = 2;
    s_techFinished = 0;
}

void DialoguePortrait_Cancel(void)
{
    s_portraitScene = DIALOGUE_PORTRAIT_SCENE_IDLE;
    s_busyFrames = 0;
    s_actors[0] = DIALOGUE_PORTRAIT_ACTOR_NONE;
    s_actors[1] = DIALOGUE_PORTRAIT_ACTOR_NONE;
}

void DialoguePortrait_Update(void)
{
    if (s_busyFrames > 0)
    {
        s_busyFrames--;
        if (s_busyFrames == 0)
        {
            if (s_portraitScene == DIALOGUE_PORTRAIT_SCENE_EXITING)
            {
                s_actors[0] = DIALOGUE_PORTRAIT_ACTOR_NONE;
                s_actors[1] = DIALOGUE_PORTRAIT_ACTOR_NONE;
            }
            if (s_portraitScene == DIALOGUE_PORTRAIT_SCENE_TECH_TEST)
            {
                s_techFinished = 1;
            }
            s_portraitScene = DIALOGUE_PORTRAIT_SCENE_IDLE;
        }
    }
}

void DialoguePortrait_Commit(void)
{
}

int DialoguePortrait_IsBusy(void)
{
    return s_busyFrames > 0;
}

int DialoguePortrait_IsSettled(void)
{
    return s_portraitActive && s_portraitScene == DIALOGUE_PORTRAIT_SCENE_IDLE && s_busyFrames == 0;
}

DialoguePortraitSceneState DialoguePortrait_GetSceneState(void)
{
    return s_portraitScene;
}

DialoguePortraitActor DialoguePortrait_GetActor(DialoguePortraitSide side)
{
    if (side != DIALOGUE_PORTRAIT_SIDE_LEFT && side != DIALOGUE_PORTRAIT_SIDE_RIGHT)
    {
        return DIALOGUE_PORTRAIT_ACTOR_NONE;
    }
    return s_actors[side];
}

int DialoguePortrait_IsLeftActive(void)
{
    return s_focusLeft;
}

int DialoguePortrait_TakeTechTestFinished(void)
{
    int v = s_techFinished;
    s_techFinished = 0;
    return v;
}

void DialoguePortrait_GetPresentationPos(DialoguePortraitSide side, int *x, int *y)
{
    (void)side;
    if (x)
    {
        *x = 0;
    }
    if (y)
    {
        *y = 0;
    }
}
