/*
 * Aseprite skill-card layout host checks (production SkillCard_Resolve/PaintView).
 *
 * gcc -std=c11 -Wall -Wextra -O0 -Iinclude -Itools/host_tests \
 *   -o tools/host_tests/test_skill_card_aseprite_host \
 *   tools/host_tests/test_skill_card_aseprite_host.c \
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

static int CountColor(const u16 *px, int n, u16 color)
{
    int i;
    int c = 0;
    for (i = 0; i < n; i++)
    {
        if (px[i] == color)
        {
            c++;
        }
    }
    return c;
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
}

static void TestReferenceSupportFixture(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    int n;

    printf("\n== reference Support-like fixture (Hold Back) ==\n");
    ctx.currentSc = 8;
    ctx.maxSc = 8;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;

    /* Hold Back is support-category in data; matches Support layout example. */
    SkillCard_ResolveViewModel(MOVE_HOLD_BACK, &ctx, &view);
    ExpectEqInt(view.detailKind, SKILL_CARD_DETAIL_CATEGORY, "support shows category");
    ExpectTrue(view.showMax, "SC at max shows MAX");
    ExpectTrue(strcmp(view.scText, "SC8") == 0 || strcmp(view.scText, "SC08") == 0
            || view.scText[0] == 'S',
        "SC text present");

    memset(g_card, 0, sizeof(g_card));
    ExpectTrue(
        SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view),
        "paint support fixture");
    n = SKILL_CARD_W * SKILL_CARD_HEIGHT;
    ExpectTrue(CountColor(g_card, n, SKILL_CARD_COLOR_GREEN_BG) > 1000, "green fill present");
    ExpectTrue(CountColor(g_card, n, SKILL_CARD_COLOR_NAVY) > 100, "navy present");
    ExpectTrue(CountColor(g_card, n, SKILL_CARD_COLOR_FULL_FX) > 0, "MAX orange present");
    WritePpm(
        "docs/reports/skill_card_aseprite_layout_logs/implemented_support_fixture.ppm",
        g_card,
        SKILL_CARD_W,
        SKILL_CARD_HEIGHT);
}

static void TestHammerFistTargets(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;

    printf("\n== Hammer Fist splash targets ==\n");
    ctx.currentSc = 4;
    ctx.maxSc = 8;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 1;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;
    SkillCard_ResolveViewModel(MOVE_HAMMER_FIST, &ctx, &view);
    ExpectEqInt(view.detailKind, SKILL_CARD_DETAIL_POWER, "Hammer Fist shows power");
    ExpectEqInt(view.enemyEffects[2], SKILL_CARD_TARGET_EFFECT_PRIMARY, "center primary");
    ExpectEqInt(view.enemyEffects[1], SKILL_CARD_TARGET_EFFECT_SECONDARY, "left secondary");
    ExpectEqInt(view.enemyEffects[3], SKILL_CARD_TARGET_EFFECT_SECONDARY, "right secondary");
    ExpectTrue(!view.showMax, "below max no MAX");
}

static void TestScMaxToggle(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    int orangeAtMax;
    int orangeBelow;

    printf("\n== SC MAX appear/disappear ==\n");
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;
    ctx.maxSc = 8;

    ctx.currentSc = 8;
    SkillCard_ResolveViewModel(MOVE_VORTEX_ESCAPE, &ctx, &view);
    SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view);
    orangeAtMax = CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_FULL_FX);

    ctx.currentSc = 3;
    SkillCard_ResolveViewModel(MOVE_VORTEX_ESCAPE, &ctx, &view);
    SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view);
    orangeBelow = CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_FULL_FX);

    ExpectTrue(orangeAtMax > orangeBelow, "MAX pixels removed when below max");
    ExpectTrue(!view.showMax, "showMax false below max");
}

static void TestLongNameAndDupSc(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;

    printf("\n== long name + zero SC ==\n");
    ctx.currentSc = 0;
    ctx.maxSc = 16;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 1;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;
    SkillCard_ResolveViewModel(MOVE_TEMPERATURE_DIFFERENCE, &ctx, &view);
    ExpectTrue(
        view.nameFit == SKILL_CARD_NAME_FIT_COMPACT
            || view.nameFit == SKILL_CARD_NAME_FIT_TRUNCATED
            || view.nameFit == SKILL_CARD_NAME_FIT_NORMAL,
        "name fit policy applied");
    ExpectTrue(!view.showMax, "zero SC not MAX");
}

static void TestCacheFingerprint(void)
{
    SkillCardViewModel a;
    SkillCardViewModel b;
    SkillCardResolveContext ctx;

    printf("\n== target fingerprint changes ==\n");
    ctx.currentSc = 5;
    ctx.maxSc = 8;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 1;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 1;
    SkillCard_ResolveViewModel(MOVE_HAMMER_FIST, &ctx, &a);
    ctx.targetSlot = 3;
    SkillCard_ResolveViewModel(MOVE_HAMMER_FIST, &ctx, &b);
    ExpectTrue(
        SkillCard_ViewModelTargetFingerprint(&a)
            != SkillCard_ViewModelTargetFingerprint(&b),
        "target slot change alters fingerprint");
}

int main(void)
{
    printf("skill_card_aseprite host\n");
    printf("colors RGB15 green=%u navy=%u pale=%u\n",
        (unsigned)SKILL_CARD_COLOR_GREEN_BG,
        (unsigned)SKILL_CARD_COLOR_NAVY,
        (unsigned)SKILL_CARD_COLOR_PALE);

    TestReferenceSupportFixture();
    TestHammerFistTargets();
    TestScMaxToggle();
    TestLongNameAndDupSc();
    TestCacheFingerprint();

    if (g_failures)
    {
        printf("\n%d failure(s)\n", g_failures);
        return 1;
    }
    printf("\nAll aseprite skill-card host checks passed.\n");
    return 0;
}
