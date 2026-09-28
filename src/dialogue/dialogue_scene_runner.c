#include "dialogue/dialogue_scene_runner.h"

#include "dialogue/dialogue_overlay.h"
#include "dialogue/dialogue_panel.h"
#include "dialogue/dialogue_portrait_controller.h"
#include "dialogue/dialogue_text_printer.h"

typedef enum RunnerState {
    RUNNER_INACTIVE = 0,
    RUNNER_RUNNING,
    RUNNER_WAIT_PORTRAIT,
    RUNNER_WAIT_TEXT,
    RUNNER_WAIT_SAY_ADVANCE,
    RUNNER_WAIT_FRAMES,
    RUNNER_CLOSING
} RunnerState;

typedef enum WaitKind {
    WAIT_NONE = 0,
    WAIT_ENTER,
    WAIT_FOCUS,
    WAIT_EXIT,
    WAIT_TECH,
    WAIT_SAY_FOCUS
} WaitKind;

static int s_active;
static RunnerState s_state;
static WaitKind s_waitKind;
static DialogueHost s_host;
static const DialogueScene *s_scene;
static int s_cmdIndex;
static TextSpeed s_baseSpeed;
static TextSpeed s_nextMessageSpeed;
static int s_waitFrames;
static int s_sayNeedsAdvance;
static int s_locked;
static int s_finishedNotified;
static DialogueResult s_pendingResult;
static char s_lastError[48];

static void Dialogue_ClearPrinterCallbacks(void)
{
    DialogueTextPrinterCallbacks empty;

    empty.clear_line = 0;
    empty.draw_char = 0;
    empty.sfx = 0;
    empty.ctx = 0;
    DialogueTextPrinter_SetCallbacks(&empty);
}

static void DialogueRunner_PanelClearLine(int lineIndex, void *ctx)
{
    (void)ctx;
    DialoguePanel_ClearTextLine(lineIndex);
}

static void DialogueRunner_PanelDrawChar(int x, int y, char ch, int colorRole, void *ctx)
{
    int colorIndex = DialogueTextPrinter_ColorIndexForRole((DialogueTextColorRole)colorRole);

    (void)ctx;
    DialoguePanel_DrawTextCharColor(x, y, ch, colorIndex);
}

static void DialogueRunner_BindPrinter(void)
{
    DialogueTextPrinterCallbacks cb;

    cb.clear_line = DialogueRunner_PanelClearLine;
    cb.draw_char = DialogueRunner_PanelDrawChar;
    cb.sfx = 0;
    cb.ctx = 0;
    DialogueTextPrinter_SetCallbacks(&cb);
}

static void DialogueRunner_ApplyNameplate(DialoguePortraitSide side, DialoguePortraitActor actor)
{
    const char *name = "";

    if (actor == DIALOGUE_PORTRAIT_ACTOR_AKSIL)
    {
        name = "AKSIL";
    }
    else if (actor == DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST)
    {
        name = "PROTAGONIST";
    }
    if (side == DIALOGUE_PORTRAIT_SIDE_LEFT)
    {
        DialoguePanel_SetNameplate(DIALOGUE_NAMEPLATE_LEFT, name);
    }
    else
    {
        DialoguePanel_SetNameplate(DIALOGUE_NAMEPLATE_RIGHT, name);
    }
}

static void DialogueRunner_NotifyFinished(DialogueResult result)
{
    if (s_finishedNotified)
    {
        return;
    }
    s_finishedNotified = 1;
    s_pendingResult = result;
    if (s_host.on_finished)
    {
        s_host.on_finished(result, s_host.ctx);
    }
}

static void DialogueRunner_Cleanup(DialogueResult result)
{
    DialogueTextPrinter_Cancel();
    DialoguePortrait_Cancel();
    DialoguePanel_SetPromptVisible(0);
    DialogueOverlay_Shutdown();
    Dialogue_ClearPrinterCallbacks();

    if (s_locked && s_host.unlock)
    {
        s_host.unlock(s_host.ctx);
    }
    s_locked = 0;
    s_active = 0;
    s_state = RUNNER_INACTIVE;
    s_waitKind = WAIT_NONE;
    s_scene = 0;
    s_cmdIndex = 0;
    DialogueRunner_NotifyFinished(result);
}

static int DialogueRunner_BeginSay(const DialogueSceneCommand *cmd)
{
    int leftActive = (cmd->side == DIALOGUE_PORTRAIT_SIDE_LEFT) ? 1 : 0;

    if (DialoguePortrait_GetActor(cmd->side) != (DialoguePortraitActor)cmd->actor)
    {
        return 0;
    }

    DialogueTextPrinter_Cancel();
    DialoguePanel_SetPromptVisible(0);
    DialoguePanel_ClearTextLine(0);
    DialoguePanel_ClearTextLine(1);
    DialoguePanel_FlushDirty();

    DialoguePortrait_BeginFocus(leftActive);
    s_waitKind = WAIT_SAY_FOCUS;
    s_state = RUNNER_WAIT_PORTRAIT;
    /* Stash say payload in cmdIndex current; text starts after focus. */
    return 1;
}

static void DialogueRunner_StartSayText(const DialogueSceneCommand *cmd)
{
    DialogueRunner_ApplyNameplate((DialoguePortraitSide)cmd->side, (DialoguePortraitActor)cmd->actor);
    DialogueRunner_BindPrinter();
    if (!DialogueTextPrinter_StartCommands(
            cmd->textCommands,
            (int)cmd->textCommandCount,
            s_nextMessageSpeed))
    {
        DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
        return;
    }
    s_baseSpeed = s_nextMessageSpeed;
    DialoguePanel_SetPromptVisible(0);
    DialoguePanel_FlushDirty();
    s_sayNeedsAdvance = 0;
    s_state = RUNNER_WAIT_TEXT;
    s_waitKind = WAIT_NONE;
}

static void DialogueRunner_AdvanceCommand(void);

static int DialogueRunner_ExecuteOne(const DialogueSceneCommand *cmd)
{
    switch (cmd->op)
    {
    case DIALOGUE_SCENE_OP_ENTER_PAIR:
        DialoguePortrait_BeginEntrance(cmd->arg ? 1 : 0);
        s_waitKind = WAIT_ENTER;
        s_state = RUNNER_WAIT_PORTRAIT;
        s_cmdIndex++;
        return 1;
    case DIALOGUE_SCENE_OP_EXIT_BOTH:
        DialoguePortrait_BeginExitBoth();
        s_waitKind = WAIT_EXIT;
        s_state = RUNNER_WAIT_PORTRAIT;
        s_cmdIndex++;
        return 1;
    case DIALOGUE_SCENE_OP_FOCUS:
        DialoguePortrait_BeginFocus(cmd->side == DIALOGUE_PORTRAIT_SIDE_LEFT ? 1 : 0);
        s_waitKind = WAIT_FOCUS;
        s_state = RUNNER_WAIT_PORTRAIT;
        s_cmdIndex++;
        return 1;
    case DIALOGUE_SCENE_OP_SAY:
        if (!DialogueRunner_BeginSay(cmd))
        {
            DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
            return 1;
        }
        /* Do not increment cmdIndex until SAY fully completes. */
        return 1;
    case DIALOGUE_SCENE_OP_SET_EXPRESSION:
        if (cmd->arg != DIALOGUE_EXPRESSION_NEUTRAL)
        {
            DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
            return 1;
        }
        s_cmdIndex++;
        return 0;
    case DIALOGUE_SCENE_OP_WAIT_FRAMES:
        if (cmd->arg == 0)
        {
            s_cmdIndex++;
            return 0;
        }
        s_waitFrames = (int)cmd->arg;
        s_state = RUNNER_WAIT_FRAMES;
        s_cmdIndex++;
        return 1;
    case DIALOGUE_SCENE_OP_EVENT:
        if (s_host.on_event)
        {
            s_host.on_event((u16)cmd->arg, s_host.ctx);
        }
        s_cmdIndex++;
        return 0;
    case DIALOGUE_SCENE_OP_TECH_REPLACE:
        DialoguePortrait_BeginTechTest();
        s_waitKind = WAIT_TECH;
        s_state = RUNNER_WAIT_PORTRAIT;
        s_cmdIndex++;
        return 1;
    case DIALOGUE_SCENE_OP_END:
        s_cmdIndex++;
        DialogueRunner_Cleanup(DIALOGUE_RESULT_COMPLETED);
        return 1;
    case DIALOGUE_SCENE_OP_ENTER:
    case DIALOGUE_SCENE_OP_EXIT:
    case DIALOGUE_SCENE_OP_REPLACE:
        /* Individual enter/exit/replace reserved; shared scenes use pair/tech. */
        DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
        return 1;
    default:
        DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
        return 1;
    }
}

static void DialogueRunner_AdvanceCommand(void)
{
    int steps = 0;

    while (s_active && s_state == RUNNER_RUNNING && steps < DIALOGUE_SCENE_MAX_IMMEDIATE_STEPS)
    {
        const DialogueSceneCommand *cmd;

        steps++;
        if (!s_scene || s_cmdIndex >= s_scene->commandCount)
        {
            DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
            return;
        }
        cmd = &s_scene->commands[s_cmdIndex];
        if (DialogueRunner_ExecuteOne(cmd))
        {
            return;
        }
    }
    if (steps >= DIALOGUE_SCENE_MAX_IMMEDIATE_STEPS && s_state == RUNNER_RUNNING)
    {
        DialogueRunner_Cleanup(DIALOGUE_RESULT_ERROR);
    }
}

int Dialogue_Start(
    const DialogueScene *scene,
    const DialogueHost *host,
    TextSpeed baseSpeed)
{
    char error[48];

    if (s_active)
    {
        return 0;
    }
    if (!scene || !host)
    {
        return 0;
    }
    if (!DialogueScene_Validate(scene, error, (int)sizeof(error)))
    {
        int i;
        for (i = 0; i < 47 && error[i]; i++)
        {
            s_lastError[i] = error[i];
        }
        s_lastError[i] = '\0';
        return 0;
    }

    s_host = *host;
    s_scene = scene;
    s_cmdIndex = 0;
    s_baseSpeed = (baseSpeed >= TEXT_SPEED_COUNT) ? TEXT_SPEED_NORMAL : baseSpeed;
    s_nextMessageSpeed = s_baseSpeed;
    s_waitFrames = 0;
    s_sayNeedsAdvance = 0;
    s_finishedNotified = 0;
    s_pendingResult = DIALOGUE_RESULT_NONE;
    s_waitKind = WAIT_NONE;
    s_lastError[0] = '\0';

    if (s_host.lock)
    {
        s_host.lock(s_host.ctx);
    }
    s_locked = 1;

    DialogueTextPrinter_Init();
    DialogueRunner_BindPrinter();
    DialogueOverlay_Init(s_host.overlayMode);

    s_active = 1;
    s_state = RUNNER_RUNNING;
    DialogueRunner_AdvanceCommand();
    return 1;
}

void Dialogue_Update(int aPressed, int bHeld, int startCancel)
{
    if (!s_active)
    {
        return;
    }

    if (startCancel)
    {
        DialogueRunner_Cleanup(DIALOGUE_RESULT_CANCELLED);
        return;
    }

    DialoguePortrait_Update();
    DialogueOverlay_Commit();

    if (s_state == RUNNER_WAIT_PORTRAIT)
    {
        if (s_waitKind == WAIT_TECH)
        {
            if (DialoguePortrait_TakeTechTestFinished() || DialoguePortrait_IsSettled())
            {
                s_state = RUNNER_RUNNING;
                s_waitKind = WAIT_NONE;
                DialogueRunner_AdvanceCommand();
            }
            return;
        }
        if (DialoguePortrait_IsSettled())
        {
            if (s_waitKind == WAIT_SAY_FOCUS)
            {
                const DialogueSceneCommand *cmd = &s_scene->commands[s_cmdIndex];
                DialogueRunner_StartSayText(cmd);
            }
            else
            {
                s_state = RUNNER_RUNNING;
                s_waitKind = WAIT_NONE;
                DialogueRunner_AdvanceCommand();
            }
        }
        return;
    }

    if (s_state == RUNNER_WAIT_FRAMES)
    {
        if (s_waitFrames > 0)
        {
            s_waitFrames--;
        }
        if (s_waitFrames > 0)
        {
            return;
        }
        s_state = RUNNER_RUNNING;
        DialogueRunner_AdvanceCommand();
        return;
    }

    if (s_state == RUNNER_WAIT_TEXT)
    {
        if (DialogueTextPrinter_NeedsUpdate())
        {
            int accelerate = DialogueTextPrinter_IsPrinting() && bHeld;
            int aForPrinter = (!DialogueTextPrinter_IsPrinting() && aPressed) ? 1 : 0;
            DialogueTextPrinter_Update(accelerate, aForPrinter);
            DialoguePanel_SetPromptVisible(DialogueTextPrinter_ShowsMarker());
            DialoguePanel_FlushDirty();
        }
        if (DialogueTextPrinter_IsComplete())
        {
            DialoguePanel_SetPromptVisible(1);
            DialoguePanel_FlushDirty();
            s_state = RUNNER_WAIT_SAY_ADVANCE;
            s_sayNeedsAdvance = 1;
            /* A from this update cannot complete SAY. */
        }
        return;
    }

    if (s_state == RUNNER_WAIT_SAY_ADVANCE)
    {
        /* A or B edge advances to the next line; B does not cancel the scene. */
        if (aPressed && s_sayNeedsAdvance)
        {
            s_sayNeedsAdvance = 0;
            DialoguePanel_SetPromptVisible(0);
            DialogueTextPrinter_Cancel();
            s_cmdIndex++;
            s_state = RUNNER_RUNNING;
            DialogueRunner_AdvanceCommand();
        }
        return;
    }

    if (s_state == RUNNER_RUNNING)
    {
        DialogueRunner_AdvanceCommand();
    }
}

int Dialogue_IsActive(void)
{
    return s_active;
}

void Dialogue_Cancel(void)
{
    if (!s_active)
    {
        return;
    }
    DialogueRunner_Cleanup(DIALOGUE_RESULT_CANCELLED);
}

DialogueResult Dialogue_TakeResult(void)
{
    DialogueResult r = s_pendingResult;
    s_pendingResult = DIALOGUE_RESULT_NONE;
    return r;
}

void Dialogue_SetNextMessageSpeed(TextSpeed speed)
{
    if (speed < TEXT_SPEED_COUNT)
    {
        s_nextMessageSpeed = speed;
    }
}

TextSpeed Dialogue_GetNextMessageSpeed(void)
{
    return s_nextMessageSpeed;
}
