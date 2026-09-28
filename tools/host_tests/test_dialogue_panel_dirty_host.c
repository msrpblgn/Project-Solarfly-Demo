/*
 * Host test: actual dialogue_panel rasterizer four-tile dirty marking.
 * Build with -DDIALOGUE_HOST_TEST (see test_dialogue_text_printer.py).
 */

#include <stdio.h>
#include <string.h>

#include "host_vram_shim.h"
#include "dialogue/dialogue_panel.h"

/* Solid 5x7 ink so every pixel of the glyph bbox writes. */
const unsigned char *Video_GetFontGlyphRows(char c);
int Video_IsFontCharSupported(char c);
int Video_MeasureTextWidth(const char *text);

static int g_failures;

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

static int SlotMatches(int slot, int expectX, int expectY)
{
    int tx = -1;
    int ty = -1;
    int dirty = DialoguePanel_GetDynamicSlotTile(slot, &tx, &ty);
    return dirty && tx == expectX && ty == expectY;
}

int main(void)
{
    int i;
    int found[4];
    /* Glyph at (30,132): cols 3-4, rows 16-17 */
    const int expectX[4] = {3, 4, 3, 4};
    const int expectY[4] = {16, 16, 17, 17};

    g_failures = 0;
    HostVram_Reset();
    DialoguePanel_Init();
    DialoguePanel_ShowFrame();
    DialoguePanel_FlushDirty();

    /* Clear dirty from chrome build. */
    DialoguePanel_FlushDirty();
    ExpectTrue(DialoguePanel_CountDirty() == 0, "chrome flushed clean");

    DialoguePanel_DrawTextCharColor(30, 132, 'H', 1);
    ExpectTrue(DialoguePanel_CountDirty() >= 4, "at least four dirty slots");

    memset(found, 0, sizeof(found));
    for (i = 0; i < 192; i++)
    {
        int t;
        for (t = 0; t < 4; t++)
        {
            if (!found[t] && SlotMatches(i, expectX[t], expectY[t]))
            {
                found[t] = 1;
            }
        }
    }
    ExpectTrue(found[0] && found[1] && found[2] && found[3], "four boundary tiles dirty");

    DialoguePanel_FlushDirty();
    ExpectTrue(DialoguePanel_CountDirty() == 0, "flush clears dirty flags");

    /* Clearing a line marks restored tiles dirty. */
    DialoguePanel_DrawTextCharColor(12, 132, 'A', 1);
    DialoguePanel_FlushDirty();
    DialoguePanel_ClearTextLine(0);
    ExpectTrue(DialoguePanel_CountDirty() > 0, "clear marks dirty");

    if (g_failures)
    {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("all host dialogue panel dirty tests passed\n");
    return 0;
}
