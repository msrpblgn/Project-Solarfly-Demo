/*
 * Host checks: MAX x150 spacing, Vortex Esc. alias, no-wrap carousel navigation.
 *
 * gcc -std=c11 -Wall -Wextra -O0 -Iinclude -Itools/host_tests \
 *   -o tools/host_tests/test_skill_card_spacing_nav_host.exe \
 *   tools/host_tests/test_skill_card_spacing_nav_host.c \
 *   src/battle_ui/skill_card.c src/battle_ui/move_presentation.c \
 *   src/battle/battle_move.c src/core/video.c src/core/random.c \
 *   src/data/move_database.c src/data/element_database.c \
 *   src/data/race_database.c src/data/status_database.c \
 *   -DVIDEO_HOST_FRAMEBUFFER
 */
#include <stdio.h>
#include <string.h>

#include "battle/battle_move.h"
#include "battle/battle_slot.h"
#include "battle_ui/move_presentation.h"
#include "battle_ui/skill_card.h"
#include "core/video.h"

static int g_failures;
static u16 g_card[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];

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

static void ExpectEqStr(const char *got, const char *want, const char *label)
{
    if (!got || !want || strcmp(got, want) != 0)
    {
        printf("FAIL: %s (got '%s' want '%s')\n", label, got ? got : "(null)", want ? want : "(null)");
        g_failures++;
    }
    else
    {
        printf("OK:   %s\n", label);
    }
}

static void WritePpm(const char *path, const u16 *px, int w, int h)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f)
    {
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < w * h; i++)
    {
        u16 c = px[i];
        unsigned char rgb[3];
        rgb[0] = (unsigned char)(((c) & 31) << 3);
        rgb[1] = (unsigned char)(((c >> 5) & 31) << 3);
        rgb[2] = (unsigned char)(((c >> 10) & 31) << 3);
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("Wrote %s\n", path);
}

static int CountColorInRect(const u16 *px, int destW, int x, int y, int w, int h, u16 color)
{
    int row, col, n = 0;
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            if (px[(y + row) * destW + (x + col)] == color)
            {
                n++;
            }
        }
    }
    return n;
}

static void PaintScMax(int currentSc, int maxSc, int showMaxForce, const char *path)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    ctx.currentSc = currentSc;
    ctx.maxSc = maxSc;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;
    SkillCard_ResolveViewModel(MOVE_HOLD_BACK, &ctx, &view);
    if (showMaxForce >= 0)
    {
        view.showMax = showMaxForce;
    }
    /* Force SC text for two-digit case independent of resolve formatting. */
    if (currentSc == 16)
    {
        strncpy(view.scText, "SC16", sizeof(view.scText) - 1);
        view.scText[sizeof(view.scText) - 1] = '\0';
        view.showMax = 1;
        view.maxSc = 16;
        view.currentSc = 16;
    }
    memset(g_card, 0, sizeof(g_card));
    SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view);
    WritePpm(path, g_card, SKILL_CARD_W, SKILL_CARD_HEIGHT);
}

static void TestMaxSpacing(void)
{
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    int orangeAt;
    int orangeGone;
    int gapCol;
    int paleInGap;

    printf("\n== MAX spacing x150 ==\n");
    ExpectEqInt(layout->maxX, 150, "layout maxX is 150");
    ExpectEqInt(layout->maxY, 19, "layout maxY unchanged");
    ExpectEqInt(layout->maxW, 16, "layout maxW unchanged");
    ExpectEqInt(layout->maxH, 7, "layout maxH unchanged");

    PaintScMax(16, 16, 1, "docs/reports/skill_card_spacing_nav_logs/sc16_max.ppm");
    /* Gap column x149 between SC16 and MAX must not be MAX orange. */
    gapCol = 149;
    paleInGap = CountColorInRect(g_card, SKILL_CARD_W, gapCol, 19, 1, 7, SKILL_CARD_COLOR_FULL_FX);
    ExpectTrue(paleInGap == 0, "x149 gap column has no MAX primary orange");
    ExpectTrue(
        CountColorInRect(g_card, SKILL_CARD_W, 150, 19, 16, 7, SKILL_CARD_COLOR_FULL_FX) > 0,
        "MAX artwork present at x150");
    /* SC16 pale ink should exist left of the gap. */
    ExpectTrue(
        CountColorInRect(g_card, SKILL_CARD_W, 130, 20, 19, 6, SKILL_CARD_COLOR_NAVY) > 0,
        "SC16 navy ink visible");

    orangeAt = CountColorInRect(g_card, SKILL_CARD_W, 150, 19, 16, 7, SKILL_CARD_COLOR_FULL_FX);
    PaintScMax(3, 16, 0, "docs/reports/skill_card_spacing_nav_logs/sc3_nomax.ppm");
    orangeGone = CountColorInRect(g_card, SKILL_CARD_W, 150, 19, 16, 7, SKILL_CARD_COLOR_FULL_FX);
    ExpectTrue(orangeAt > 0, "MAX orange present when shown");
    ExpectEqInt(orangeGone, 0, "MAX area cleared when hidden");
}

static void TestVortexAlias(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    printf("\n== Vortex Esc. alias ==\n");
    ctx.currentSc = 4;
    ctx.maxSc = 8;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;
    SkillCard_ResolveViewModel(MOVE_VORTEX_ESCAPE, &ctx, &view);
    ExpectEqStr(view.name, "Vortex Esc.", "display name Vortex Esc.");
    ExpectEqInt(view.nameFit, SKILL_CARD_NAME_FIT_NORMAL, "Vortex Esc. uses normal large font");
    ExpectTrue(strchr(view.name, '.') != 0, "period preserved in alias");

    memset(g_card, 0, sizeof(g_card));
    SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view);
    WritePpm("docs/reports/skill_card_spacing_nav_logs/vortex_esc.ppm", g_card, SKILL_CARD_W, SKILL_CARD_HEIGHT);
    ExpectTrue(
        CountColorInRect(g_card, SKILL_CARD_W, 20, 12, 104, 12, SKILL_CARD_COLOR_NAVY) > 0,
        "name band has navy ink");
}

static void TestNoWrapNav(void)
{
    MoveId three[3] = { MOVE_HOLD_BACK, MOVE_HAMMER_FIST, MOVE_INFLUENCE };
    MoveId two[3] = { MOVE_HOLD_BACK, MOVE_HAMMER_FIST, MOVE_NONE };
    MoveId one[3] = { MOVE_NONE, MOVE_DIRECT_ATTACK, MOVE_NONE };
    MoveId sparse[3] = { MOVE_HOLD_BACK, MOVE_NONE, MOVE_INFLUENCE };
    MoveId none[3] = { MOVE_NONE, MOVE_NONE, MOVE_NONE };
    int slot;

    printf("\n== no-wrap navigation ==\n");
    ExpectEqInt(MovePresentation_NthOccupiedSlot(three, 3, 2), 1, "3 skills: 2nd occupied is index 1");
    ExpectEqInt(MovePresentation_NthOccupiedSlot(two, 3, 2), 1, "2 skills: 2nd is index 1");
    ExpectEqInt(MovePresentation_NthOccupiedSlot(one, 3, 2), 0, "1 skill: Nth(2) returns 0 (caller uses first)");
    ExpectEqInt(MovePresentation_NthOccupiedSlot(sparse, 3, 2), 2, "sparse 2nd occupied is index 2");

    slot = 1;
    slot = MovePresentation_StepOccupiedSlotNoWrap(three, 3, slot, -1);
    ExpectEqInt(slot, 0, "from 2 left -> 1");
    slot = MovePresentation_StepOccupiedSlotNoWrap(three, 3, slot, -1);
    ExpectEqInt(slot, 0, "from 1 left stays (no wrap)");
    slot = MovePresentation_StepOccupiedSlotNoWrap(three, 3, 0, 1);
    ExpectEqInt(slot, 1, "from 1 right -> 2");
    slot = MovePresentation_StepOccupiedSlotNoWrap(three, 3, 1, 1);
    ExpectEqInt(slot, 2, "from 2 right -> 3");
    slot = MovePresentation_StepOccupiedSlotNoWrap(three, 3, 2, 1);
    ExpectEqInt(slot, 2, "from 3 right stays (no wrap)");
    slot = MovePresentation_StepOccupiedSlotNoWrap(three, 3, 2, -1);
    ExpectEqInt(slot, 1, "from 3 left -> 2");

    slot = MovePresentation_StepOccupiedSlotNoWrap(one, 3, 1, 1);
    ExpectEqInt(slot, 1, "one skill right stays");
    slot = MovePresentation_StepOccupiedSlotNoWrap(one, 3, 1, -1);
    ExpectEqInt(slot, 1, "one skill left stays");

    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 3, 0, 1);
    ExpectEqInt(slot, 2, "sparse right skips empty to last");
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 3, 2, 1);
    ExpectEqInt(slot, 2, "sparse at end right stays");
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 3, 2, -1);
    ExpectEqInt(slot, 0, "sparse left skips empty to first");
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 3, 0, -1);
    ExpectEqInt(slot, 0, "sparse at start left stays");

    slot = MovePresentation_StepOccupiedSlotNoWrap(none, 3, 0, 1);
    ExpectEqInt(slot, 0, "empty step safe");

    /* Legacy wrap helper still wraps (unchanged for other consumers). */
    slot = 2;
    slot = MovePresentation_StepOccupiedSlot(three, 3, slot, 1);
    ExpectEqInt(slot, 0, "legacy wrap helper still wraps");
}

int main(void)
{
    printf("skill_card_spacing_nav_host\n");
    TestMaxSpacing();
    TestVortexAlias();
    TestNoWrapNav();
    printf("\nfailures=%d\n", g_failures);
    return g_failures ? 1 : 0;
}
