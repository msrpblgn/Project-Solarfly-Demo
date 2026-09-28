#include "dialogue/dialogue_demo.h"

#include "core/gba_types.h"
#include "core/input.h"
#include "core/system.h"
#include "core/video.h"
#include "dialogue/dialogue_host.h"
#include "dialogue/dialogue_overlay.h"
#include "dialogue/dialogue_panel.h"
#include "dialogue/dialogue_scene.h"
#include "dialogue/dialogue_scene_runner.h"

typedef enum DemoHostPhase {
    DEMO_HOST_PLAYING = 0,
    DEMO_HOST_COMPLETE
} DemoHostPhase;

static int s_active;
static int s_entryLatch;
static DemoHostPhase s_phase;
static TextSpeed s_selectedSpeed;
static int s_eventCount;
static DialogueResult s_lastResult;

static void DialogueDemo_WaitEntryRelease(void)
{
    while (Input_IsPressed(KEY_A) || Input_IsPressed(KEY_B) || Input_IsPressed(KEY_START)
        || Input_IsPressed(KEY_L) || Input_IsPressed(KEY_R) || Input_IsPressed(KEY_SELECT))
    {
        System_WaitForVBlank();
        Input_Update();
    }
}

static void DialogueDemo_OnLock(void *ctx)
{
    (void)ctx;
}

static void DialogueDemo_OnUnlock(void *ctx)
{
    (void)ctx;
}

static void DialogueDemo_OnEvent(u16 eventId, void *ctx)
{
    (void)ctx;
    if (eventId == DIALOGUE_EVENT_TEST_COMPLETE)
    {
        s_eventCount++;
    }
}

static void DialogueDemo_OnFinished(DialogueResult result, void *ctx)
{
    (void)ctx;
    s_lastResult = result;
    if (result == DIALOGUE_RESULT_COMPLETED)
    {
        s_phase = DEMO_HOST_COMPLETE;
    }
    else
    {
        /* Cancel/error: host exits to menu via Update return. */
        s_phase = DEMO_HOST_COMPLETE;
    }
}

static int DialogueDemo_StartScene(const DialogueScene *scene)
{
    DialogueHost host;

    host.overlayMode = DIALOGUE_OVERLAY_MODE_DEMO;
    host.lock = DialogueDemo_OnLock;
    host.unlock = DialogueDemo_OnUnlock;
    host.on_event = DialogueDemo_OnEvent;
    host.on_finished = DialogueDemo_OnFinished;
    host.ctx = 0;
    s_phase = DEMO_HOST_PLAYING;
    s_lastResult = DIALOGUE_RESULT_NONE;
    return Dialogue_Start(scene, &host, s_selectedSpeed);
}

static void DialogueDemo_ShowCompleteHint(void)
{
    DialogueOverlay_Init(DIALOGUE_OVERLAY_MODE_DEMO);
    DialoguePanel_BeginSample(
        DIALOGUE_NAMEPLATE_NONE,
        "",
        "A:REPLAY SEL:REPLACE START:MENU");
    DialoguePanel_SetText("CONVERSATION COMPLETE.", "");
    DialoguePanel_SetPromptVisible(0);
    DialoguePanel_FlushDirty();
}

void DialogueDemo_Enter(TextSpeed initialSpeed)
{
    if (initialSpeed >= TEXT_SPEED_COUNT)
    {
        initialSpeed = TEXT_SPEED_NORMAL;
    }
    s_selectedSpeed = initialSpeed;
    Dialogue_SetNextMessageSpeed(initialSpeed);
    s_eventCount = 0;
    s_active = 1;
    s_entryLatch = 1;
    DialogueDemo_WaitEntryRelease();
    s_entryLatch = 0;
    if (!DialogueDemo_StartScene(DialogueScene_GetSharedConversation()))
    {
        s_active = 0;
    }
}

int DialogueDemo_Update(void)
{
    DialogueResult result;

    if (!s_active)
    {
        return 1;
    }

    if (s_entryLatch)
    {
        return 0;
    }

    if (Input_JustPressed(KEY_START))
    {
        if (Dialogue_IsActive())
        {
            Dialogue_Cancel();
        }
        Dialogue_TakeResult();
        return 1;
    }

    if (s_phase == DEMO_HOST_PLAYING && Dialogue_IsActive())
    {
        if (Input_JustPressed(KEY_L))
        {
            s_selectedSpeed = (TextSpeed)(
                (s_selectedSpeed + TEXT_SPEED_COUNT - 1) % TEXT_SPEED_COUNT);
            Dialogue_SetNextMessageSpeed(s_selectedSpeed);
        }
        else if (Input_JustPressed(KEY_R))
        {
            s_selectedSpeed = (TextSpeed)((s_selectedSpeed + 1) % TEXT_SPEED_COUNT);
            Dialogue_SetNextMessageSpeed(s_selectedSpeed);
        }

        /* A advances; B held reveals text, B edge also advances at prompt. */
        Dialogue_Update(
            Input_JustPressed(KEY_A) || Input_JustPressed(KEY_B),
            Input_IsPressed(KEY_B),
            0);
        result = Dialogue_TakeResult();
        if (result == DIALOGUE_RESULT_CANCELLED || result == DIALOGUE_RESULT_ERROR)
        {
            return 1;
        }
        if (result == DIALOGUE_RESULT_COMPLETED)
        {
            DialogueDemo_ShowCompleteHint();
        }
        return 0;
    }

    /* Completion idle: A replay, SELECT replacement, START already handled. */
    if (s_phase == DEMO_HOST_COMPLETE && !Dialogue_IsActive())
    {
        if (Input_JustPressed(KEY_A))
        {
            DialogueOverlay_Shutdown();
            Dialogue_SetNextMessageSpeed(s_selectedSpeed);
            DialogueDemo_StartScene(DialogueScene_GetSharedConversation());
            return 0;
        }
        if (Input_JustPressed(KEY_SELECT))
        {
            DialogueOverlay_Shutdown();
            Dialogue_SetNextMessageSpeed(s_selectedSpeed);
            DialogueDemo_StartScene(DialogueScene_GetReplacementTest());
            return 0;
        }
    }

    return 0;
}

void DialogueDemo_Exit(void)
{
    if (Dialogue_IsActive())
    {
        Dialogue_Cancel();
        Dialogue_TakeResult();
    }
    if (DialogueOverlay_IsActive())
    {
        DialogueOverlay_Shutdown();
    }
    Video_EnterMode3Bitmap();
    s_active = 0;
    s_entryLatch = 0;
    s_phase = DEMO_HOST_PLAYING;
}
