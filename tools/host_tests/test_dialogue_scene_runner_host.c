/*
 * Milestone 6 host harness: scene validation + runner lifecycle with stubs.
 *
 *   gcc -std=c11 -Wall -Wextra -Iinclude -Itools/host_tests \
 *       -o build/host_dialogue_scene_runner_test.exe \
 *       tools/host_tests/test_dialogue_scene_runner_host.c \
 *       tools/host_tests/host_dialogue_scene_stubs.c \
 *       tools/host_tests/host_video_stub.c \
 *       src/dialogue/dialogue_scene.c \
 *       src/dialogue/dialogue_scene_runner.c \
 *       src/dialogue/dialogue_text_printer.c
 */

#include <stdio.h>
#include <string.h>

#include "dialogue/dialogue_host.h"
#include "dialogue/dialogue_scene.h"
#include "dialogue/dialogue_scene_runner.h"
#include "dialogue/dialogue_text_printer.h"

extern int HostLock_GetLockCount(void);
extern int HostLock_GetUnlockCount(void);
extern void HostLock_ResetCounters(void);
extern void HostLock_BumpLock(void);
extern void HostLock_BumpUnlock(void);

static int g_failures;
static int g_finishCount;
static DialogueResult g_lastResult;
static int g_eventCount;
static u16 g_lastEvent;

static void Expect(int cond, const char *name)
{
    if (!cond)
    {
        printf("FAIL: %s\n", name);
        g_failures++;
    }
}

static void Host_OnLock(void *ctx)
{
    (void)ctx;
    HostLock_BumpLock();
}

static void Host_OnUnlock(void *ctx)
{
    (void)ctx;
    HostLock_BumpUnlock();
}

static void Host_OnEvent(u16 eventId, void *ctx)
{
    (void)ctx;
    g_eventCount++;
    g_lastEvent = eventId;
}

static void Host_OnFinished(DialogueResult result, void *ctx)
{
    (void)ctx;
    g_finishCount++;
    g_lastResult = result;
}

static DialogueHost MakeHost(void)
{
    DialogueHost host;

    host.overlayMode = DIALOGUE_OVERLAY_MODE_DEMO;
    host.lock = Host_OnLock;
    host.unlock = Host_OnUnlock;
    host.on_event = Host_OnEvent;
    host.on_finished = Host_OnFinished;
    host.ctx = 0;
    return host;
}

static void ResetHostStats(void)
{
    g_finishCount = 0;
    g_lastResult = DIALOGUE_RESULT_NONE;
    g_eventCount = 0;
    g_lastEvent = 0;
    HostLock_ResetCounters();
}

static void Test_SharedSceneValid(void)
{
    char err[48];

    Expect(DialogueScene_Validate(DialogueScene_GetSharedConversation(), err, 48),
        "shared scene validates");
    Expect(DialogueScene_Validate(DialogueScene_GetReplacementTest(), err, 48),
        "replacement scene validates");
    Expect(
        DialogueScene_GetSharedConversation() == DialogueScene_GetSharedConversation(),
        "shared scene pointer stable");
}

static void Test_RejectMissingEnd(void)
{
    static const DialogueSceneCommand cmds[] = {
        DIALOGUE_SCENE_CMD_WAIT_FRAMES(1)
    };
    static const DialogueScene scene = {"bad", cmds, 1};
    char err[48];
    DialogueHost host = MakeHost();

    ResetHostStats();
    Expect(!DialogueScene_Validate(&scene, err, 48), "missing END rejected");
    Expect(!Dialogue_Start(&scene, &host, TEXT_SPEED_NORMAL), "invalid start refused");
    Expect(HostLock_GetLockCount() == 0, "refused start does not lock");
    Expect(g_finishCount == 0, "refused start no finish callback");
}

static void Test_RejectOccupiedEnter(void)
{
    static const DialogueSceneCommand cmds[] = {
        DIALOGUE_SCENE_CMD_ENTER_PAIR(0),
        DIALOGUE_SCENE_CMD_ENTER(
            DIALOGUE_PORTRAIT_SIDE_LEFT,
            DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST,
            1),
        DIALOGUE_SCENE_CMD_END()
    };
    static const DialogueScene scene = {"occ", cmds, 3};
    char err[48];

    Expect(!DialogueScene_Validate(&scene, err, 48), "occupied ENTER rejected");
}

static void Test_BusyStartLeavesActive(void)
{
    DialogueHost host = MakeHost();
    const DialogueScene *scene = DialogueScene_GetSharedConversation();

    ResetHostStats();
    Expect(Dialogue_Start(scene, &host, TEXT_SPEED_FAST), "first start ok");
    Expect(Dialogue_IsActive(), "active after start");
    Expect(!Dialogue_Start(scene, &host, TEXT_SPEED_FAST), "busy start fails");
    Expect(HostLock_GetLockCount() == 1, "busy start did not re-lock");
    Dialogue_Cancel();
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_CANCELLED, "cancel result");
    Expect(g_finishCount == 1, "finish once on cancel");
    Expect(HostLock_GetUnlockCount() == 1, "unlock once");
    Dialogue_Cancel();
    Expect(g_finishCount == 1, "repeat cancel harmless");
}

static void Test_NaturalEndAndEvent(void)
{
    DialogueHost host = MakeHost();
    int frames;

    ResetHostStats();
    Expect(Dialogue_Start(DialogueScene_GetSharedConversation(), &host, TEXT_SPEED_FAST),
        "shared start");
    for (frames = 0; frames < 4000 && Dialogue_IsActive(); frames++)
    {
        /* Advance waits / pages aggressively with A every frame after a settle. */
        Dialogue_Update(1, 1, 0);
    }
    Expect(!Dialogue_IsActive(), "shared finished");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_COMPLETED, "completed result");
    Expect(g_finishCount == 1, "finish exactly once");
    Expect(g_eventCount == 1 && g_lastEvent == DIALOGUE_EVENT_TEST_COMPLETE,
        "EVENT once");
    Expect(HostLock_GetLockCount() == 1 && HostLock_GetUnlockCount() == 1,
        "lock/unlock once");
}

static void Test_SayNeedsFinalA(void)
{
    static const DialogueTextCommand text[] = {
        DIALOGUE_CMD_TEXT("HI."),
        DIALOGUE_CMD_END()
    };
    static const DialogueSceneCommand cmds[] = {
        DIALOGUE_SCENE_CMD_ENTER_PAIR(0),
        DIALOGUE_SCENE_CMD_SAY(
            DIALOGUE_PORTRAIT_SIDE_RIGHT,
            DIALOGUE_PORTRAIT_ACTOR_AKSIL,
            text,
            2),
        DIALOGUE_SCENE_CMD_END()
    };
    static const DialogueScene scene = {"say", cmds, 3};
    DialogueHost host = MakeHost();
    int i;

    ResetHostStats();
    Expect(Dialogue_Start(&scene, &host, TEXT_SPEED_FAST), "say scene start");
    for (i = 0; i < 80 && Dialogue_IsActive(); i++)
    {
        /* No A until text should be complete. */
        Dialogue_Update(0, 1, 0);
    }
    /* Printer should be waiting for final A (still active). */
    Expect(Dialogue_IsActive(), "still active awaiting SAY advance");
    Dialogue_Update(1, 0, 0);
    for (i = 0; i < 40 && Dialogue_IsActive(); i++)
    {
        Dialogue_Update(1, 0, 0);
    }
    Expect(!Dialogue_IsActive(), "completed after final A");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_COMPLETED, "say completed");
}

static void Test_ReplacementScene(void)
{
    DialogueHost host = MakeHost();
    int i;

    ResetHostStats();
    Expect(Dialogue_Start(DialogueScene_GetReplacementTest(), &host, TEXT_SPEED_NORMAL),
        "replace start");
    for (i = 0; i < 40 && Dialogue_IsActive(); i++)
    {
        Dialogue_Update(0, 0, 0);
    }
    Expect(!Dialogue_IsActive(), "replace finished");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_COMPLETED, "replace completed");
}

static void Test_CancelFromWait(void)
{
    DialogueHost host = MakeHost();

    ResetHostStats();
    Expect(Dialogue_Start(DialogueScene_GetSharedConversation(), &host, TEXT_SPEED_NORMAL),
        "cancel-path start");
    Dialogue_Update(0, 0, 0);
    Dialogue_Update(0, 0, 1);
    Expect(!Dialogue_IsActive(), "cancelled");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_CANCELLED, "cancelled result");
    Expect(g_finishCount == 1, "cancel finish once");
}

static void Test_BAcceleratesWhilePrinting(void)
{
    static const DialogueTextCommand text[] = {
        DIALOGUE_CMD_TEXT("ABCDEFGHIJKLMNOP."),
        DIALOGUE_CMD_END()
    };
    static const DialogueSceneCommand cmds[] = {
        DIALOGUE_SCENE_CMD_ENTER_PAIR(0),
        DIALOGUE_SCENE_CMD_SAY(
            DIALOGUE_PORTRAIT_SIDE_RIGHT,
            DIALOGUE_PORTRAIT_ACTOR_AKSIL,
            text,
            2),
        DIALOGUE_SCENE_CMD_END()
    };
    static const DialogueScene scene = {"accel", cmds, 3};
    DialogueHost host = MakeHost();
    int slowFrames;
    int fastFrames;
    int i;

    ResetHostStats();
    Expect(Dialogue_Start(&scene, &host, TEXT_SPEED_SLOW), "slow start");
    for (i = 0; i < 200 && Dialogue_IsActive(); i++)
    {
        Dialogue_Update(0, 0, 0);
        if (DialogueTextPrinter_IsComplete())
        {
            break;
        }
    }
    slowFrames = i;
    Expect(Dialogue_IsActive(), "awaiting advance after slow print");
    Dialogue_Cancel();
    Dialogue_TakeResult();

    ResetHostStats();
    Expect(Dialogue_Start(&scene, &host, TEXT_SPEED_SLOW), "accel start");
    for (i = 0; i < 200 && Dialogue_IsActive(); i++)
    {
        /* bHeld=1 reveals remaining printables (does not cancel scene). */
        Dialogue_Update(0, 1, 0);
        if (DialogueTextPrinter_IsComplete())
        {
            break;
        }
    }
    fastFrames = i;
    Expect(Dialogue_IsActive(), "still active after B reveal");
    Expect(fastFrames < slowFrames, "B held reveals line faster than idle");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_NONE, "B did not cancel mid-print");
    Dialogue_Cancel();
    Dialogue_TakeResult();
}

static void Test_BAdvancesAtPrompt(void)
{
    static const DialogueTextCommand line1[] = {
        DIALOGUE_CMD_TEXT("FIRST."),
        DIALOGUE_CMD_END()
    };
    static const DialogueTextCommand line2[] = {
        DIALOGUE_CMD_TEXT("SECOND."),
        DIALOGUE_CMD_END()
    };
    static const DialogueSceneCommand cmds[] = {
        DIALOGUE_SCENE_CMD_ENTER_PAIR(0),
        DIALOGUE_SCENE_CMD_SAY(
            DIALOGUE_PORTRAIT_SIDE_RIGHT,
            DIALOGUE_PORTRAIT_ACTOR_AKSIL,
            line1,
            2),
        DIALOGUE_SCENE_CMD_SAY(
            DIALOGUE_PORTRAIT_SIDE_RIGHT,
            DIALOGUE_PORTRAIT_ACTOR_AKSIL,
            line2,
            2),
        DIALOGUE_SCENE_CMD_END()
    };
    static const DialogueScene scene = {"b-advance", cmds, 4};
    DialogueHost host = MakeHost();
    int i;

    ResetHostStats();
    Expect(Dialogue_Start(&scene, &host, TEXT_SPEED_FAST), "b-advance start");
    for (i = 0; i < 80 && Dialogue_IsActive(); i++)
    {
        Dialogue_Update(0, 0, 0);
    }
    Expect(Dialogue_IsActive(), "at press-A prompt after first line");
    /* Call sites OR JustPressed(B) into aPressed — simulate B edge here. */
    Dialogue_Update(1, 0, 0);
    Expect(Dialogue_IsActive(), "advanced to second line, scene continues");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_NONE, "B did not exit scene");
    for (i = 0; i < 80 && Dialogue_IsActive(); i++)
    {
        Dialogue_Update(0, 0, 0);
    }
    Dialogue_Update(1, 0, 0);
    for (i = 0; i < 40 && Dialogue_IsActive(); i++)
    {
        Dialogue_Update(1, 0, 0);
    }
    Expect(!Dialogue_IsActive(), "completed via B-equivalent advances");
    Expect(Dialogue_TakeResult() == DIALOGUE_RESULT_COMPLETED, "natural complete");
}

int main(void)
{
    DialogueTextPrinter_Init();
    Test_SharedSceneValid();
    Test_RejectMissingEnd();
    Test_RejectOccupiedEnter();
    Test_BusyStartLeavesActive();
    Test_NaturalEndAndEvent();
    Test_SayNeedsFinalA();
    Test_ReplacementScene();
    Test_CancelFromWait();
    Test_BAcceleratesWhilePrinting();
    Test_BAdvancesAtPrompt();

    if (g_failures)
    {
        printf("%d host dialogue scene runner tests failed\n", g_failures);
        return 1;
    }
    printf("all host dialogue scene runner tests passed\n");
    return 0;
}
