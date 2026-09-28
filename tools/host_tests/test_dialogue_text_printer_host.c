/*
 * Milestone 3+4 host harness: printer command stream + panel dirty tiles.
 *
 * Printer build:
 *   gcc -std=c11 -Wall -Wextra -Iinclude -Itools/host_tests \
 *       -o build/host_dialogue_text_printer_test.exe \
 *       tools/host_tests/test_dialogue_text_printer_host.c \
 *       tools/host_tests/host_video_stub.c \
 *       src/dialogue/dialogue_text_printer.c
 *
 * Panel dirty build:
 *   gcc -std=c11 -Wall -Wextra -DDIALOGUE_HOST_TEST -Iinclude -Itools/host_tests \
 *       -o build/host_dialogue_panel_dirty_test.exe \
 *       tools/host_tests/test_dialogue_panel_dirty_host.c \
 *       tools/host_tests/host_vram_shim.c \
 *       tools/host_tests/host_video_stub.c \
 *       src/dialogue/dialogue_panel.c \
 *       src/assets/dialogue_frame.c
 */

#include <stdio.h>
#include <string.h>

#include "dialogue/dialogue_demo_plan.h"
#include "dialogue/dialogue_text_printer.h"

typedef struct FakeSurface {
    char cells[2][40];
    int colors[2][40];
    int drawCount;
    int clearCount[2];
    int xs[128];
    int ys[128];
    char chars[128];
    int colorRoles[128];
    int charUpdateIndex[128];
    int sfxCount;
    u16 sfxIds[8];
} FakeSurface;

static FakeSurface s_surface;
static int s_updateIndex;
static int g_failures;

static void Fake_ClearLine(int lineIndex, void *ctx)
{
    FakeSurface *surface = (FakeSurface *)ctx;
    int i;

    if (lineIndex < 0 || lineIndex > 1)
    {
        return;
    }
    surface->clearCount[lineIndex]++;
    for (i = 0; i < 40; i++)
    {
        surface->cells[lineIndex][i] = '\0';
        surface->colors[lineIndex][i] = 0;
    }
}

static void Fake_DrawChar(int x, int y, char ch, int colorRole, void *ctx)
{
    FakeSurface *surface = (FakeSurface *)ctx;
    int line;
    int col;

    if (surface->drawCount < 128)
    {
        surface->xs[surface->drawCount] = x;
        surface->ys[surface->drawCount] = y;
        surface->chars[surface->drawCount] = ch;
        surface->colorRoles[surface->drawCount] = colorRole;
        surface->charUpdateIndex[surface->drawCount] = s_updateIndex;
    }
    surface->drawCount++;

    line = (y == 144) ? 1 : 0;
    col = (x - 12) / 6;
    if (col >= 0 && col < 39)
    {
        surface->cells[line][col] = ch;
        surface->colors[line][col] = colorRole;
    }
}

static void Fake_Sfx(u16 cueId, void *ctx)
{
    FakeSurface *surface = (FakeSurface *)ctx;

    if (surface->sfxCount < 8)
    {
        surface->sfxIds[surface->sfxCount] = cueId;
    }
    surface->sfxCount++;
}

static void Fake_Reset(void)
{
    memset(&s_surface, 0, sizeof(s_surface));
    s_updateIndex = 0;
}

static void ExpectTrue(int cond, const char *name)
{
    if (!cond)
    {
        printf("FAIL: %s\n", name);
        g_failures++;
    }
    else
    {
        printf("ok: %s\n", name);
    }
}

static void BindFake(void)
{
    DialogueTextPrinterCallbacks cb;

    cb.clear_line = Fake_ClearLine;
    cb.draw_char = Fake_DrawChar;
    cb.sfx = Fake_Sfx;
    cb.ctx = &s_surface;
    DialogueTextPrinter_SetCallbacks(&cb);
}

static int RunUntilComplete(int accelerateAlways, int maxUpdates)
{
    int i;

    for (i = 0; i < maxUpdates; i++)
    {
        s_updateIndex = i + 1;
        DialogueTextPrinter_Update(accelerateAlways, 0);
        if (DialogueTextPrinter_IsComplete())
        {
            return i + 1;
        }
    }
    return -1;
}

static int CountPrintable(const char *text)
{
    int n = 0;
    int i;

    for (i = 0; text[i] != '\0'; i++)
    {
        if (text[i] != '\n')
        {
            n++;
        }
    }
    return n;
}

static void TestCoreTiming(void)
{
    const char *sampleA = "YOU THOUGHT THAT WOULD WORK?\nCOME ON. ONE MORE TRY.";
    const char *sampleB = "IT ALMOST DID.";
    int nA = CountPrintable(sampleA);
    int nB = CountPrintable(sampleB);
    int updates;

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_Start(sampleA, TEXT_SPEED_SLOW);
    updates = RunUntilComplete(0, 512);
    ExpectTrue(updates == 1 + (nA - 1) * 4, "sample A slow timing");

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_Start(sampleA, TEXT_SPEED_NORMAL);
    updates = RunUntilComplete(0, 512);
    ExpectTrue(updates == 1 + (nA - 1) * 2, "sample A normal timing");

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_Start(sampleA, TEXT_SPEED_FAST);
    updates = RunUntilComplete(0, 512);
    ExpectTrue(updates == 1 + (nA - 1) * 1, "sample A fast timing");

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_Start(sampleB, TEXT_SPEED_FAST);
    updates = RunUntilComplete(0, 64);
    ExpectTrue(updates == 1 + (nB - 1) * 1, "sample B fast timing");
}

static void TestPauseExact(void)
{
    const DialogueTextCommand cmds[] = {
        DIALOGUE_CMD_TEXT("A"),
        DIALOGUE_CMD_PAUSE(6),
        DIALOGUE_CMD_TEXT("B"),
        DIALOGUE_CMD_END()
    };
    int t;

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 4, TEXT_SPEED_FAST);

    s_updateIndex = 1;
    DialogueTextPrinter_Update(0, 0); /* A, then PAUSE(6) */
    ExpectTrue(s_surface.drawCount == 1, "pause: A drawn");
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_TIMED_PAUSE,
        "pause entered");

    for (t = 2; t <= 7; t++)
    {
        s_updateIndex = t;
        DialogueTextPrinter_Update(1, 1); /* B held + A must not skip/end pause */
        ExpectTrue(s_surface.drawCount == 1, "pause: no B during wait window");
    }
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_PRINTING
            || DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_TIMED_PAUSE,
        "pause near end");

    /* T+1..T+6 are updates 2..7; resume at T+7 = update 8 */
    s_updateIndex = 8;
    DialogueTextPrinter_Update(0, 0);
    ExpectTrue(s_surface.drawCount == 2 && s_surface.chars[1] == 'B', "pause resume at T+7");
}

static void TestPause0(void)
{
    const DialogueTextCommand cmds[] = {
        DIALOGUE_CMD_TEXT("A"),
        DIALOGUE_CMD_PAUSE(0),
        DIALOGUE_CMD_TEXT("B"),
        DIALOGUE_CMD_END()
    };

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 4, TEXT_SPEED_FAST);
    s_updateIndex = 1;
    DialogueTextPrinter_Update(0, 0);
    /* PAUSE(0) adds no wait: both glyphs may complete in one update at Fast. */
    ExpectTrue(s_surface.drawCount == 2, "pause0 continues immediately");
    ExpectTrue(DialogueTextPrinter_IsComplete(), "pause0 ends same update at fast");
}

static void TestWaitAndPage(void)
{
    const DialogueTextCommand cmds[] = {
        DIALOGUE_CMD_TEXT("HI"),
        DIALOGUE_CMD_WAIT(),
        DIALOGUE_CMD_WAIT(),
        DIALOGUE_CMD_TEXT("X"),
        DIALOGUE_CMD_PAGE(),
        DIALOGUE_CMD_TEXT("Y"),
        DIALOGUE_CMD_END()
    };

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 7, TEXT_SPEED_FAST);

    s_updateIndex = 1;
    DialogueTextPrinter_Update(0, 0);
    s_updateIndex = 2;
    DialogueTextPrinter_Update(0, 1); /* A while printing second char — ignored for wait */
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_WAIT_INPUT
            || DialogueTextPrinter_IsPrinting(),
        "after HI");
    /* Finish into first WAIT without satisfying via same-update A */
    while (DialogueTextPrinter_IsPrinting())
    {
        s_updateIndex++;
        DialogueTextPrinter_Update(0, 0);
    }
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_WAIT_INPUT,
        "first WAIT");
    ExpectTrue(s_surface.cells[0][0] == 'H', "WAIT preserves text");

    s_updateIndex++;
    DialogueTextPrinter_Update(0, 1); /* resolve WAIT1, hit WAIT2 */
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_WAIT_INPUT,
        "second WAIT needs new press");

    s_updateIndex++;
    DialogueTextPrinter_Update(0, 1); /* resolve WAIT2, print X path */
    while (DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_PRINTING)
    {
        s_updateIndex++;
        DialogueTextPrinter_Update(0, 0);
    }
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_WAIT_PAGE,
        "PAGE wait");
    ExpectTrue(s_surface.cells[0][2] == 'X' || s_surface.drawCount >= 3, "X before page");

    s_updateIndex++;
    DialogueTextPrinter_Update(1, 0); /* B must not advance PAGE */
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_WAIT_PAGE,
        "B ignores PAGE");

    s_updateIndex++;
    DialogueTextPrinter_Update(0, 1); /* clear page, continue */
    ExpectTrue(s_surface.clearCount[0] >= 2, "PAGE cleared lines");
    ExpectTrue(s_surface.cells[0][0] == '\0' || DialogueTextPrinter_IsPrinting()
            || DialogueTextPrinter_IsComplete()
            || DialogueTextPrinter_ShowsMarker(),
        "page cleared or printing Y");
}

static void TestAccelStopsAtPause(void)
{
    const DialogueTextCommand cmds[] = {
        DIALOGUE_CMD_TEXT("AB"),
        DIALOGUE_CMD_PAUSE(2),
        DIALOGUE_CMD_TEXT("C"),
        DIALOGUE_CMD_END()
    };

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 4, TEXT_SPEED_SLOW);
    s_updateIndex = 1;
    DialogueTextPrinter_Update(1, 0);
    ExpectTrue(s_surface.drawCount == 2, "B reveals up to pause");
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_TIMED_PAUSE,
        "accel stops at pause");
    ExpectTrue(s_surface.sfxCount == 0, "no stray sfx");
}

static void TestSpeedColorSfx(void)
{
    const DialogueTextCommand cmds[] = {
        DIALOGUE_CMD_SPEED(TEXT_SPEED_SLOW),
        DIALOGUE_CMD_TEXT("A"),
        DIALOGUE_CMD_SPEED(TEXT_SPEED_FAST),
        DIALOGUE_CMD_TEXT("B"),
        DIALOGUE_CMD_SPEED(DIALOGUE_TEXT_SPEED_RESTORE_BASE),
        DIALOGUE_CMD_COLOR(DIALOGUE_TEXT_COLOR_EMPHASIS),
        DIALOGUE_CMD_TEXT("C"),
        DIALOGUE_CMD_COLOR(DIALOGUE_TEXT_COLOR_DEFAULT),
        DIALOGUE_CMD_SFX(DIALOGUE_TEXT_SFX_CUE_DEMO),
        DIALOGUE_CMD_TEXT("D"),
        DIALOGUE_CMD_END()
    };
    char normalCells[2][40];
    int normalSfx;

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 11, TEXT_SPEED_NORMAL);
    ExpectTrue(DialogueTextPrinter_GetBaseSpeed() == TEXT_SPEED_NORMAL, "base captured");
    s_updateIndex = 1;
    DialogueTextPrinter_Update(0, 0);
    ExpectTrue(DialogueTextPrinter_GetActiveSpeed() == TEXT_SPEED_SLOW, "slow override");
    ExpectTrue(s_surface.chars[0] == 'A', "A under slow");

    RunUntilComplete(0, 64);
    ExpectTrue(s_surface.colorRoles[2] == DIALOGUE_TEXT_COLOR_EMPHASIS, "C emphasis");
    ExpectTrue(s_surface.colorRoles[3] == DIALOGUE_TEXT_COLOR_DEFAULT, "D default");
    ExpectTrue(s_surface.sfxCount == 1, "sfx once normal");
    ExpectTrue(s_surface.sfxIds[0] == DIALOGUE_TEXT_SFX_CUE_DEMO, "sfx id");
    memcpy(normalCells, s_surface.cells, sizeof(normalCells));
    normalSfx = s_surface.sfxCount;

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 11, TEXT_SPEED_NORMAL);
    RunUntilComplete(1, 64);
    ExpectTrue(s_surface.sfxCount == normalSfx, "sfx once accelerated");
    ExpectTrue(memcmp(normalCells, s_surface.cells, sizeof(normalCells)) == 0, "accel matches");
}

static void TestValidationCommands(void)
{
    char error[64];
    const DialogueTextCommand missingEnd[] = {
        DIALOGUE_CMD_TEXT("A")
    };
    const DialogueTextCommand bad[] = {
        DIALOGUE_CMD_TEXT("A"),
        DIALOGUE_CMD_PAUSE(255),
        DIALOGUE_CMD_END()
    };
    const DialogueTextCommand ok[] = {
        DIALOGUE_CMD_TEXT("OK"),
        DIALOGUE_CMD_END()
    };

    ExpectTrue(
        !DialogueTextPrinter_ValidateCommands(missingEnd, 1, error, 64),
        "reject missing END");
    ExpectTrue(
        !DialogueTextPrinter_ValidateCommands(bad, 3, error, 64),
        "reject pause over max");
    ExpectTrue(
        DialogueTextPrinter_ValidateCommands(ok, 2, error, 64),
        "accept short script");
}

static void TestCancelStates(void)
{
    const DialogueTextCommand cmds[] = {
        DIALOGUE_CMD_TEXT("A"),
        DIALOGUE_CMD_PAUSE(3),
        DIALOGUE_CMD_WAIT(),
        DIALOGUE_CMD_PAGE(),
        DIALOGUE_CMD_END()
    };

    Fake_Reset();
    DialogueTextPrinter_Init();
    BindFake();
    DialogueTextPrinter_StartCommands(cmds, 5, TEXT_SPEED_FAST);
    DialogueTextPrinter_Update(0, 0);
    DialogueTextPrinter_Cancel();
    ExpectTrue(
        DialogueTextPrinter_GetState() == DIALOGUE_TEXT_PRINTER_INACTIVE,
        "cancel from pause/print");
}

static void TestPlanInput(void)
{
    DialogueDemoPlanInput in;
    DialogueDemoPlanResult out;

    memset(&in, 0, sizeof(in));
    in.needsUpdate = 1;
    in.isPrinting = 1;
    in.aPressed = 1;
    in.bHeld = 1;
    out = DialogueDemo_PlanUpdate(&in);
    ExpectTrue(out.action == DIALOGUE_DEMO_PLAN_TICK_PRINTER, "tick while printing");
    ExpectTrue(out.accelerate == 1, "B accelerates printing");
    ExpectTrue(out.passAPressed == 0, "A not passed while printing");

    memset(&in, 0, sizeof(in));
    in.needsUpdate = 1;
    in.isPrinting = 0;
    in.aPressed = 1;
    out = DialogueDemo_PlanUpdate(&in);
    ExpectTrue(out.passAPressed == 1, "A passed for WAIT/PAGE");

    memset(&in, 0, sizeof(in));
    in.isComplete = 1;
    in.aPressed = 1;
    out = DialogueDemo_PlanUpdate(&in);
    ExpectTrue(out.action == DIALOGUE_DEMO_PLAN_ADVANCE_SAMPLE, "A advances on complete");

    memset(&in, 0, sizeof(in));
    in.isComplete = 1;
    in.bPressed = 1;
    out = DialogueDemo_PlanUpdate(&in);
    ExpectTrue(out.action == DIALOGUE_DEMO_PLAN_ADVANCE_SAMPLE, "B advances on complete");
}

int main(void)
{
    g_failures = 0;
    TestCoreTiming();
    TestPauseExact();
    TestPause0();
    TestWaitAndPage();
    TestAccelStopsAtPause();
    TestSpeedColorSfx();
    TestValidationCommands();
    TestCancelStates();
    TestPlanInput();

    if (g_failures)
    {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("all host dialogue text printer tests passed\n");
    return 0;
}
