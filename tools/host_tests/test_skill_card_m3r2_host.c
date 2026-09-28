/*
 * M3R2 composition-order host checks (real engine font via video.c).
 *
 * This is NOT a full Battle_Update/Battle_Draw integration test.
 * It exercises SkillCard paint/blit compose-before-present ordering and slide
 * math with a local ComposeAndPresent helper. See
 * test_skill_card_m3r2_production_host.c for the production Skills path.
 *
 * Build (from repo root, MSYS2/MinGW or similar):
 *   gcc -std=c11 -Wall -Wextra -O2 -DVIDEO_HOST_FRAMEBUFFER \
 *       -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_skill_card_m3r2_host.exe \
 *       tools/host_tests/test_skill_card_m3r2_host.c \
 *       src/core/video.c \
 *       src/battle_ui/skill_card.c \
 *       src/battle_ui/skill_card_slide.c \
 *       src/battle_ui/move_presentation.c \
 *       src/data/move_database.c
 *
 * Hardware not exercised here: REG_VCOUNT / System_WaitForVBlank timing,
 * DMA, live Mode 3 scanout, Battle dirty flags, or Battle_Draw chrome ownership.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "battle_ui/hud_layout.h"
#include "battle_ui/move_presentation.h"
#include "battle_ui/skill_card.h"
#include "battle_ui/skill_card_slide.h"
#include "core/video.h"
#include "core/gba_types.h"
#include "data/move_database.h"

#define PANEL_BG RGB15(8, 8, 16)
#define TITLE_COLOR RGB15(24, 24, 31)
#define OUTSIDE_SENTINEL RGB15(1, 2, 3)

static int g_failures;
static int g_frameCounter;

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

static u16 *Fb(void)
{
    return Video_HostFramebuffer();
}

static void FbClear(u16 color)
{
    int i;
    u16 *fb = Fb();

    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        fb[i] = color;
    }
}

static void FbFillRect(int x, int y, int w, int h, u16 color)
{
    int row;
    int col;
    u16 *fb = Fb();

    for (row = 0; row < h; row++)
    {
        int py = y + row;
        if (py < 0 || py >= SCREEN_HEIGHT)
        {
            continue;
        }
        for (col = 0; col < w; col++)
        {
            int px = x + col;
            if (px < 0 || px >= SCREEN_WIDTH)
            {
                continue;
            }
            fb[(py * SCREEN_WIDTH) + px] = color;
        }
    }
}

static int RegionHasColor(int x, int y, int w, int h, u16 color)
{
    int row;
    int col;
    u16 *fb = Fb();

    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            if (fb[((y + row) * SCREEN_WIDTH) + (x + col)] == color)
            {
                return 1;
            }
        }
    }
    return 0;
}

static int RegionOnlyColor(int x, int y, int w, int h, u16 color)
{
    int row;
    int col;
    u16 *fb = Fb();

    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            if (fb[((y + row) * SCREEN_WIDTH) + (x + col)] != color)
            {
                return 0;
            }
        }
    }
    return 1;
}

static int FieldAbovePanelUnchanged(u16 sentinel)
{
    int y;
    int x;
    u16 *fb = Fb();

    for (y = 0; y < HUD_SKILLS_PANEL_TOP_Y; y++)
    {
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
            if (fb[(y * SCREEN_WIDTH) + x] != sentinel)
            {
                return 0;
            }
        }
    }
    return 1;
}

static int CardSettledBorderPresent(const SkillCardStyle *style)
{
    /* Top-left corner of settled card must be border ink. */
    u16 *fb = Fb();
    int x = HUD_SKILLS_CARD_X;
    int y = HUD_SKILLS_CARD_Y;

    return fb[(y * SCREEN_WIDTH) + x] == style->border;
}

static int CardBearingRegionHasBorder(const SkillCardStyle *style)
{
    return RegionHasColor(
        HUD_SKILLS_CARD_VIEW_X,
        HUD_SKILLS_CARD_VIEW_Y,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        style->border);
}

/* --- Production-order composition (matches BattleMenu_DrawSkill M3R2) --- */

static void ComposeAndPresent(
    const u16 *cardA,
    const u16 *cardB,
    int cardW,
    int cardH,
    int originAX,
    int originBX,
    int originY,
    u16 *viewport)
{
    int i;
    int count = HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H;

    for (i = 0; i < count; i++)
    {
        viewport[i] = PANEL_BG;
    }
    SkillCard_BlitIntoBuffer(
        viewport,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        HUD_SKILLS_CARD_VIEW_X,
        HUD_SKILLS_CARD_VIEW_Y,
        cardA,
        cardW,
        cardH,
        originAX,
        originY);
    if (cardB)
    {
        SkillCard_BlitIntoBuffer(
            viewport,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            HUD_SKILLS_CARD_VIEW_X,
            HUD_SKILLS_CARD_VIEW_Y,
            cardB,
            cardW,
            cardH,
            originBX,
            originY);
    }
    /* Assert card present in RAM before any VRAM write. */
    {
        int hasCardInRam = 0;
        int idx;

        for (idx = 0; idx < count; idx++)
        {
            if (viewport[idx] != PANEL_BG)
            {
                hasCardInRam = 1;
                break;
            }
        }
        ExpectTrue(hasCardInRam, "RAM viewport has card pixels before present");
    }
    Video_BlitBitmapRegion(
        HUD_SKILLS_CARD_VIEW_X,
        HUD_SKILLS_CARD_VIEW_Y,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        viewport,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        0,
        0);
}

/* Deliberate intermediate-clear probe: shows a blank card region is possible
 * after a VRAM clear. This does NOT prove that state was scanout in the failing
 * ROM or that it explains Sarp's screenshots. */
static void LegacyClearThenPaintDetect(
    const SkillCardStyle *style,
    u16 *viewport,
    const u16 *card,
    int cardW,
    int cardH)
{
    int detectedBlank;

    FbFillRect(
        HUD_SKILLS_CARD_VIEW_X,
        HUD_SKILLS_CARD_VIEW_Y,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        PANEL_BG);
    detectedBlank = !CardBearingRegionHasBorder(style);
    ExpectTrue(
        detectedBlank,
        "deliberate clear creates blank card region (intermediate-state probe only)");
    /* Complete the old path after the blank window. */
    {
        int i;
        for (i = 0; i < HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H; i++)
        {
            viewport[i] = PANEL_BG;
        }
        SkillCard_BlitIntoBuffer(
            viewport,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            HUD_SKILLS_CARD_VIEW_X,
            HUD_SKILLS_CARD_VIEW_Y,
            card,
            cardW,
            cardH,
            HUD_SKILLS_CARD_X,
            HUD_SKILLS_CARD_Y);
        Video_BlitBitmapRegion(
            HUD_SKILLS_CARD_VIEW_X,
            HUD_SKILLS_CARD_VIEW_Y,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            viewport,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            0,
            0);
    }
    ExpectTrue(CardSettledBorderPresent(style), "legacy path eventually paints card");
}

static void DrawChromeCount(int ordinal, int occupied)
{
    char line[12];

    FbFillRect(HUD_SKILLS_COUNT_X, HUD_SKILLS_TITLE_Y, 36, VIDEO_FONT_GLYPH_H, PANEL_BG);
    Video_DrawText(8, HUD_SKILLS_TITLE_Y, "SKILLS", TITLE_COLOR);
    snprintf(line, sizeof(line), "%d/%d", ordinal, occupied);
    Video_DrawText(HUD_SKILLS_COUNT_X, HUD_SKILLS_TITLE_Y, line, HUD_TEXT_MUTED);
    Video_DrawText(8, HUD_SKILLS_FOOTER_Y, "A:USE  B:BACK  <>", HUD_TEXT_MUTED);
}

static void WritePpm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int y;
    int x;
    u16 *fb = Fb();

    if (!f)
    {
        printf("WARN: could not write %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    for (y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (x = 0; x < SCREEN_WIDTH; x++)
        {
            u16 c = fb[(y * SCREEN_WIDTH) + x];
            unsigned char rgb[3];
            rgb[0] = (unsigned char)(((c >> 0) & 31) * 255 / 31);
            rgb[1] = (unsigned char)(((c >> 5) & 31) * 255 / 31);
            rgb[2] = (unsigned char)(((c >> 10) & 31) * 255 / 31);
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
    printf("wrote %s\n", path);
}

typedef struct TraceRow {
    int frame;
    int slideActive;
    int slideFrame;
    int sourceSlot;
    int destSlot;
    int selection;
    int chromeClearedCard;
    int viewportClearedRam;
    int cardsPainted;
    int presented;
    int cardBorderPresent;
} TraceRow;

#define TRACE_MAX 64
static TraceRow g_trace[TRACE_MAX];
static int g_traceCount;

static void TracePush(const TraceRow *row)
{
    if (g_traceCount < TRACE_MAX)
    {
        g_trace[g_traceCount++] = *row;
    }
}

static void DumpTrace(const char *path)
{
    FILE *f = fopen(path, "w");
    int i;

    if (!f)
    {
        return;
    }
    fprintf(
        f,
        "frame,slideActive,slideFrame,src,dst,sel,chromeClearedCard,viewRam,painted,presented,border\n");
    for (i = 0; i < g_traceCount; i++)
    {
        TraceRow *r = &g_trace[i];
        fprintf(
            f,
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            r->frame,
            r->slideActive,
            r->slideFrame,
            r->sourceSlot,
            r->destSlot,
            r->selection,
            r->chromeClearedCard,
            r->viewportClearedRam,
            r->cardsPainted,
            r->presented,
            r->cardBorderPresent);
    }
    fclose(f);
    printf("wrote %s\n", path);
}

static void TestProductionPathSequence(void)
{
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    MovePresentationFields fieldsA;
    MovePresentationFields fieldsB;
    MovePresentationLiveSc live;
    u16 cardA[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    u16 cardB[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    u16 viewport[HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H];
    SkillCardSlideState slide;
    int selection = 0;
    int travel;
    int i;
    int result;
    MoveId moves[4] = {
        MOVE_DIRECT_ATTACK,
        MOVE_NONE,
        MOVE_PHANTOM_PHIST,
        MOVE_NONE
    };

    live.currentSc = 14;
    live.maxSc = 14;
    MovePresentation_FillLive(moves[0], &live, &fieldsA);
    MovePresentation_FillLive(moves[2], &live, &fieldsB);
    ExpectTrue(
        SkillCard_Paint(cardA, SKILL_CARD_MAX_W * SKILL_CARD_MAX_H, layout, style, &fieldsA, 14),
        "paint source with real font");
    ExpectTrue(
        SkillCard_Paint(cardB, SKILL_CARD_MAX_W * SKILL_CARD_MAX_H, layout, style, &fieldsB, 14),
        "paint dest with real font");
    ExpectTrue(cardA[0] == style->border, "painted card uses real border pixel");

    travel = layout->width + SKILL_CARD_GAP;
    g_traceCount = 0;
    g_frameCounter = 0;

    /* Settled initial frame: chrome bands only (not card), then compose-present. */
    FbClear(OUTSIDE_SENTINEL);
    FbFillRect(0, HUD_SKILLS_PANEL_TOP_Y, SCREEN_WIDTH,
               HUD_SKILLS_CARD_VIEW_Y - HUD_SKILLS_PANEL_TOP_Y, PANEL_BG);
    FbFillRect(0, HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H, SCREEN_WIDTH,
               SCREEN_HEIGHT - (HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H), PANEL_BG);
    ComposeAndPresent(
        cardA, NULL, layout->width, layout->height,
        HUD_SKILLS_CARD_X, 0, HUD_SKILLS_CARD_Y, viewport);
    DrawChromeCount(1, 2);
    ExpectTrue(CardSettledBorderPresent(style), "settled card present");
    ExpectTrue(
        RegionHasColor(8, HUD_SKILLS_TITLE_Y, 40, VIDEO_FONT_GLYPH_H, TITLE_COLOR)
            || !RegionOnlyColor(8, HUD_SKILLS_TITLE_Y, 30, VIDEO_FONT_GLYPH_H, PANEL_BG),
        "title glyphs present");
    WritePpm("docs/reports/skills_carousel_tutsil_m3r2_captures/01_settled.ppm");
    {
        TraceRow row = {g_frameCounter++, 0, 0, 0, 0, selection, 0, 1, 1, 1,
                        CardSettledBorderPresent(style)};
        TracePush(&row);
    }

    /* Detect original failure mode. */
    LegacyClearThenPaintDetect(style, viewport, cardA, layout->width, layout->height);

    /* New path: mid-clear never hits FB before compose. */
    FbClear(OUTSIDE_SENTINEL);
    FbFillRect(0, HUD_SKILLS_PANEL_TOP_Y, SCREEN_WIDTH,
               HUD_SKILLS_CARD_VIEW_Y - HUD_SKILLS_PANEL_TOP_Y, PANEL_BG);
    FbFillRect(0, HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H, SCREEN_WIDTH,
               SCREEN_HEIGHT - (HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H), PANEL_BG);
    ComposeAndPresent(
        cardA, NULL, layout->width, layout->height,
        HUD_SKILLS_CARD_X, 0, HUD_SKILLS_CARD_Y, viewport);
    ExpectTrue(CardSettledBorderPresent(style), "compose-first never blanks FB mid-step");

    SkillCardSlide_Begin(&slide, 0, 0, 2, 1);
    selection = 0;

    for (i = 0; i < SKILL_CARD_SLIDE_FRAME_COUNT; i++)
    {
        int offset = SkillCardSlide_OffsetPx(slide.frame, travel);
        int oxA = HUD_SKILLS_CARD_X - offset;
        int oxB = HUD_SKILLS_CARD_X + travel - offset;
        int countSlot = SkillCardSlide_CountSlot(&slide, selection);
        int ordinal = (countSlot == 0) ? 1 : 2;

        /* Chrome skipped while sliding — do not clear card on FB. */
        ComposeAndPresent(
            cardA, cardB, layout->width, layout->height,
            oxA, oxB, HUD_SKILLS_CARD_Y, viewport);
        DrawChromeCount(ordinal, 2);
        ExpectTrue(
            CardBearingRegionHasBorder(style),
            "slide frame has card-bearing border pixels");
        ExpectTrue(FieldAbovePanelUnchanged(OUTSIDE_SENTINEL),
                   "field pixels above skills panel unchanged");

        {
            TraceRow row = {
                g_frameCounter++,
                1,
                slide.frame,
                slide.sourceSlot,
                slide.destSlot,
                selection,
                0,
                1,
                2,
                1,
                CardBearingRegionHasBorder(style)};
            TracePush(&row);
        }

        if (i == 2)
        {
            char name[128];
            snprintf(name, sizeof(name),
                     "docs/reports/skills_carousel_tutsil_m3r2_captures/02_slide_f%d.ppm",
                     slide.frame);
            WritePpm(name);
        }

        result = SkillCardSlide_Advance(&slide);
        if (result > 0)
        {
            selection = slide.destSlot;
            ExpectEqInt(selection, 2, "commit selects dest slot");
        }
        if (result < 0)
        {
            ExpectEqInt(result, -1, "finish unlocks input");
            break;
        }
    }

    SkillCardSlide_Clear(&slide);
    ComposeAndPresent(
        cardB, NULL, layout->width, layout->height,
        HUD_SKILLS_CARD_X, 0, HUD_SKILLS_CARD_Y, viewport);
    DrawChromeCount(2, 2);
    ExpectTrue(CardSettledBorderPresent(style), "idle after finish has card");
    WritePpm("docs/reports/skills_carousel_tutsil_m3r2_captures/03_idle_after_finish.ppm");
    {
        TraceRow row = {g_frameCounter++, 0, 0, 0, 0, selection, 0, 1, 1, 1,
                        CardSettledBorderPresent(style)};
        TracePush(&row);
    }

    /* Idle frames after finish. */
    for (i = 0; i < 3; i++)
    {
        ComposeAndPresent(
            cardB, NULL, layout->width, layout->height,
            HUD_SKILLS_CARD_X, 0, HUD_SKILLS_CARD_Y, viewport);
        DrawChromeCount(2, 2);
        ExpectTrue(CardSettledBorderPresent(style), "post-finish idle keeps card");
    }

    /* Back-to-back switch reverse. */
    SkillCardSlide_Begin(&slide, 0, 2, 0, -1);
    for (i = 0; i < SKILL_CARD_SLIDE_FRAME_COUNT; i++)
    {
        int offset = SkillCardSlide_OffsetPx(slide.frame, travel);
        int oxA = HUD_SKILLS_CARD_X + offset;
        int oxB = HUD_SKILLS_CARD_X - travel + offset;

        ComposeAndPresent(
            cardB, cardA, layout->width, layout->height,
            oxA, oxB, HUD_SKILLS_CARD_Y, viewport);
        ExpectTrue(CardBearingRegionHasBorder(style), "back-to-back slide has card");
        result = SkillCardSlide_Advance(&slide);
        if (result > 0)
        {
            selection = slide.destSlot;
        }
    }
    SkillCardSlide_Advance(&slide);
    ExpectEqInt(selection, 0, "back-to-back commit restored source");

    /* Cancellation mid-slide: clear slide, restore settled selection (source). */
    SkillCardSlide_Begin(&slide, 0, 0, 2, 1);
    SkillCardSlide_Advance(&slide);
    SkillCardSlide_Advance(&slide);
    SkillCardSlide_Clear(&slide);
    selection = 0;
    ComposeAndPresent(
        cardA, NULL, layout->width, layout->height,
        HUD_SKILLS_CARD_X, 0, HUD_SKILLS_CARD_Y, viewport);
    DrawChromeCount(1, 2);
    ExpectTrue(CardSettledBorderPresent(style), "B-cancel reopen settled source card");
    ExpectEqInt(selection, 0, "cancel keeps source selection");
    WritePpm("docs/reports/skills_carousel_tutsil_m3r2_captures/04_cancel_reopen.ppm");

    DumpTrace("docs/reports/skills_carousel_tutsil_m3r2_captures/trace.csv");
}

static void TestChromeSkipsCardRegion(void)
{
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    u16 viewport[HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H];
    u16 card[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    MovePresentationFields fields;
    MovePresentationLiveSc live;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    int i;

    live.currentSc = 10;
    live.maxSc = 10;
    MovePresentation_FillLive(MOVE_DIRECT_ATTACK, &live, &fields);
    SkillCard_Paint(card, SKILL_CARD_MAX_W * SKILL_CARD_MAX_H, layout, style, &fields, 10);

    FbClear(PANEL_BG);
    ComposeAndPresent(
        card, NULL, layout->width, layout->height,
        HUD_SKILLS_CARD_X, 0, HUD_SKILLS_CARD_Y, viewport);
    ExpectTrue(CardSettledBorderPresent(style), "pre-chrome card present");

    /* Skills chrome bands only — must not wipe card viewport. */
    FbFillRect(
        0,
        HUD_SKILLS_PANEL_TOP_Y,
        SCREEN_WIDTH,
        HUD_SKILLS_CARD_VIEW_Y - HUD_SKILLS_PANEL_TOP_Y,
        PANEL_BG);
    FbFillRect(
        0,
        HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H,
        SCREEN_WIDTH,
        SCREEN_HEIGHT - (HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H),
        PANEL_BG);
    ExpectTrue(CardSettledBorderPresent(style), "skills chrome bands leave card intact");

    /* Contrast: full-panel wipe would blank the card (old chrome). */
    FbFillRect(0, HUD_SKILLS_PANEL_TOP_Y, SCREEN_WIDTH, HUD_SKILLS_PANEL_HEIGHT, PANEL_BG);
    ExpectTrue(!CardSettledBorderPresent(style), "full panel wipe blanks card (old behavior)");
    (void)i;
}

int main(void)
{
    printf("=== M3R2 skill card production-path host ===\n");
    TestChromeSkipsCardRegion();
    TestProductionPathSequence();
    if (g_failures)
    {
        printf("RESULT: %d failure(s)\n", g_failures);
        return 1;
    }
    printf("RESULT: all checks passed\n");
    return 0;
}
