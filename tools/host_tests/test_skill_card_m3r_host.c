/*
 * M3R host checks: whole-card paint, 6-frame slide, variant geometry, captures.
 */

#include <stdio.h>
#include <string.h>

#include "battle_ui/move_presentation.h"
#include "battle_ui/skill_card.h"
#include "battle_ui/skill_card_slide.h"
#include "core/video.h"

static int g_failures;

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

static void TestSlideTiming(void)
{
    SkillCardSlideState slide;
    int selection = 1;
    int i;
    int result;

    ExpectEqInt(SKILL_CARD_SLIDE_FRAME_COUNT, 3, "three presented frames");
    ExpectEqInt(SkillCardSlide_OffsetPx(0, 232), 0, "frame0 offset");
    ExpectEqInt(SkillCardSlide_OffsetPx(SKILL_CARD_SLIDE_FRAME_COUNT - 1, 232), 232, "final frame exact travel");
    ExpectEqInt(SkillCardSlide_OffsetPx(1, 232), (180 * 232) / 256, "mid ease progress");

    SkillCardSlide_Begin(&slide, 0, 1, 3, 1);
    for (i = 0; i < SKILL_CARD_SLIDE_FRAME_COUNT - 2; i++)
    {
        result = SkillCardSlide_Advance(&slide);
        ExpectEqInt(result, 0, "no commit mid");
        ExpectEqInt(selection, 1, "still pending");
    }
    result = SkillCardSlide_Advance(&slide);
    ExpectEqInt(result, 1, "commit on final frame");
    selection = slide.destSlot;
    ExpectEqInt(selection, 3, "dest committed");
    result = SkillCardSlide_Advance(&slide);
    ExpectEqInt(result, -1, "finished signals input unlock");
}

static void TestPaintAndVariant(void)
{
    u16 buf[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    MovePresentationFields fields;
    MovePresentationLiveSc live;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    const SkillCardLayout *variant = SkillCard_GetVariantLayout();

    live.currentSc = 14;
    live.maxSc = 14;
    MovePresentation_FillLive(MOVE_DIRECT_ATTACK, &live, &fields);
    ExpectTrue(SkillCard_ValidateLayout(layout), "default layout valid");
    ExpectTrue(
        SkillCard_Paint(buf, SKILL_CARD_MAX_W * SKILL_CARD_MAX_H, layout, style, &fields, 14),
        "paint default card");
    ExpectTrue(buf[0] == style->border, "top-left is border");
    ExpectTrue(SkillCard_ValidateLayout(variant), "variant layout valid");
    ExpectTrue(variant->width != layout->width, "variant width differs");
    ExpectTrue(variant->height != layout->height, "variant height differs");
    ExpectTrue(
        SkillCard_Paint(
            buf,
            SKILL_CARD_MAX_W * SKILL_CARD_MAX_H,
            variant,
            SkillCard_GetVariantStyle(),
            &fields,
            14),
        "paint variant without transition edits");
}

static void TestDuplicateSc(void)
{
    MovePresentationFields a;
    MovePresentationFields b;
    MovePresentationLiveSc liveA;
    MovePresentationLiveSc liveB;

    liveA.currentSc = 8;
    liveA.maxSc = 16;
    liveB.currentSc = 1;
    liveB.maxSc = 16;
    MovePresentation_FillLive(MOVE_HOLD_BACK, &liveA, &a);
    MovePresentation_FillLive(MOVE_HOLD_BACK, &liveB, &b);
    ExpectTrue(strcmp(a.sc, "SC 8/16") == 0, "source SC");
    ExpectTrue(strcmp(b.sc, "SC 1/16") == 0, "dest SC distinct");
}

static void WritePpm(const char *path, const u16 *buf, int w, int h)
{
    FILE *fp = fopen(path, "wb");
    int i;

    if (!fp)
    {
        printf("FAIL: cannot write %s\n", path);
        g_failures++;
        return;
    }
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < w * h; i++)
    {
        u16 c = buf[i];
        unsigned char r = (unsigned char)(((c & 31) * 255) / 31);
        unsigned char g = (unsigned char)((((c >> 5) & 31) * 255) / 31);
        unsigned char b = (unsigned char)((((c >> 10) & 31) * 255) / 31);
        fputc(r, fp);
        fputc(g, fp);
        fputc(b, fp);
    }
    fclose(fp);
    printf("OK:   wrote %s\n", path);
}

static void SoftBlitCard(
    u16 *screen,
    int screenW,
    int screenH,
    const u16 *card,
    int cardW,
    int cardH,
    int originX,
    int originY)
{
    int row;
    int col;

    for (row = 0; row < cardH; row++)
    {
        for (col = 0; col < cardW; col++)
        {
            int dx = originX + col;
            int dy = originY + row;
            if (dx >= 0 && dx < screenW && dy >= 0 && dy < screenH)
            {
                screen[(dy * screenW) + dx] = card[(row * cardW) + col];
            }
        }
    }
}

static void CaptureSlideSequence(const char *dir, int direction)
{
    enum { VW = 240, VH = 40 };
    u16 screen[VW * VH];
    u16 cardA[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    u16 cardB[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    MovePresentationFields fieldsA;
    MovePresentationFields fieldsB;
    MovePresentationLiveSc live;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    int frame;
    int travel = layout->width + SKILL_CARD_GAP;
    int settledX = 8;
    char path[256];
    const u16 sentinel = RGB15(31, 0, 31);
    int blankFrames = 0;

    live.currentSc = 14;
    live.maxSc = 14;
    MovePresentation_FillLive(MOVE_DIRECT_ATTACK, &live, &fieldsA);
    live.currentSc = 6;
    live.maxSc = 6;
    MovePresentation_FillLive(MOVE_PHANTOM_PHIST, &live, &fieldsB);
    SkillCard_Paint(cardA, SKILL_CARD_MAX_W * SKILL_CARD_MAX_H, layout, style, &fieldsA, 14);
    SkillCard_Paint(cardB, SKILL_CARD_MAX_W * SKILL_CARD_MAX_H, layout, style, &fieldsB, 6);

    for (frame = 0; frame < SKILL_CARD_SLIDE_FRAME_COUNT; frame++)
    {
        int offset = SkillCardSlide_OffsetPx(frame, travel);
        int oxA;
        int oxB;
        int y;
        int x;
        int painted = 0;

        for (y = 0; y < VH; y++)
        {
            for (x = 0; x < VW; x++)
            {
                if (x == 0 || y == 0 || x == VW - 1 || y == VH - 1)
                {
                    screen[(y * VW) + x] = sentinel;
                }
                else
                {
                    screen[(y * VW) + x] = RGB15(8, 8, 16);
                }
            }
        }

        if (direction > 0)
        {
            oxA = settledX - offset;
            oxB = settledX + travel - offset;
        }
        else
        {
            oxA = settledX + offset;
            oxB = settledX - travel + offset;
        }

        SoftBlitCard(screen, VW, VH, cardA, layout->width, layout->height, oxA, 3);
        SoftBlitCard(screen, VW, VH, cardB, layout->width, layout->height, oxB, 3);

        for (y = 1; y < VH - 1; y++)
        {
            for (x = 1; x < VW - 1; x++)
            {
                if (screen[(y * VW) + x] != RGB15(8, 8, 16))
                {
                    painted++;
                }
            }
        }
        if (painted == 0)
        {
            blankFrames++;
        }

        /* Sentinel corners must remain. */
        ExpectTrue(screen[0] == sentinel, "sentinel top-left");
        ExpectTrue(screen[VW - 1] == sentinel, "sentinel top-right");

        snprintf(
            path,
            sizeof(path),
            "%s/%s_frame%d.ppm",
            dir,
            direction > 0 ? "right" : "left",
            frame);
        WritePpm(path, screen, VW, VH);
    }
    ExpectEqInt(blankFrames, 0, direction > 0 ? "right no blank frame" : "left no blank frame");
}

int main(int argc, char **argv)
{
    const char *outDir = "docs/reports/skills_carousel_tutsil_m3r_captures";
    char path[256];
    u16 card[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
    MovePresentationFields fields;
    MovePresentationLiveSc live;

    g_failures = 0;
    if (argc > 1)
    {
        outDir = argv[1];
    }

    TestSlideTiming();
    TestPaintAndVariant();
    TestDuplicateSc();
    CaptureSlideSequence(outDir, 1);
    CaptureSlideSequence(outDir, -1);

    live.currentSc = 14;
    live.maxSc = 14;
    MovePresentation_FillLive(MOVE_DIRECT_ATTACK, &live, &fields);
    SkillCard_Paint(
        card,
        SKILL_CARD_MAX_W * SKILL_CARD_MAX_H,
        SkillCard_GetDefaultLayout(),
        SkillCard_GetDefaultStyle(),
        &fields,
        14);
    snprintf(path, sizeof(path), "%s/settled_direct_attack.ppm", outDir);
    WritePpm(path, card, SkillCard_GetDefaultLayout()->width, SkillCard_GetDefaultLayout()->height);

    if (g_failures)
    {
        printf("\n%d failure(s)\n", g_failures);
        return 1;
    }
    printf("\nAll M3R skill card host checks passed.\n");
    return 0;
}
