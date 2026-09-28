/*
 * Visual repair host: fixtures through production PaintView + exact checks.
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
        printf("WARN: could not write %s\n", path);
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

static void PaintFixture(
    const char *name,
    BattleElementId element,
    const char *detail,
    const char *sc,
    int showMax,
    SkillCardTargetEffect e0,
    SkillCardTargetEffect e1,
    SkillCardTargetEffect e2,
    SkillCardTargetEffect e3,
    SkillCardTargetEffect e4,
    const char *ppmPath)
{
    SkillCardViewModel view;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    SkillCard_ClearViewModel(&view);
    strncpy(view.name, name, sizeof(view.name) - 1);
    view.name[sizeof(view.name) - 1] = '\0';
    view.elementId = element;
    view.detailKind = (detail[0] >= '0' && detail[0] <= '9')
        ? SKILL_CARD_DETAIL_POWER
        : SKILL_CARD_DETAIL_CATEGORY;
    strncpy(view.detailText, detail, sizeof(view.detailText) - 1);
    view.detailText[sizeof(view.detailText) - 1] = '\0';
    strncpy(view.scText, sc, sizeof(view.scText) - 1);
    view.scText[sizeof(view.scText) - 1] = '\0';
    view.showMax = showMax;
    view.nameFit = SKILL_CARD_NAME_FIT_NORMAL;
    view.enemyEffects[0] = e0;
    view.enemyEffects[1] = e1;
    view.enemyEffects[2] = e2;
    view.enemyEffects[3] = e3;
    view.enemyEffects[4] = e4;

    memset(g_card, 0, sizeof(g_card));
    ExpectTrue(
        SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view),
        ppmPath);
    WritePpm(ppmPath, g_card, SKILL_CARD_W, SKILL_CARD_HEIGHT);
}

static void TestHammerFistSupportMaxFixture(void)
{
    printf("\n== CP5 fixture: Hammer Fist / Support / SC8 / MAX ==\n");
    /* Upper center primary; adjacent secondary; remaining base. */
    PaintFixture(
        "Hammer Fist",
        BATTLE_ELEMENT_NONE,
        "Support",
        "SC8",
        1,
        SKILL_CARD_TARGET_EFFECT_NONE,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_PRIMARY,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_NONE,
        "docs/reports/skill_card_visual_repair_logs/impl_support_max.ppm");
}

static void TestSupportNoMax(void)
{
    printf("\n== CP5 fixture: Support no MAX ==\n");
    PaintFixture(
        "Hammer Fist",
        BATTLE_ELEMENT_NONE,
        "Support",
        "SC8",
        0,
        SKILL_CARD_TARGET_EFFECT_NONE,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_PRIMARY,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_NONE,
        "docs/reports/skill_card_visual_repair_logs/impl_support_nomax.ppm");
}

static void TestPower30NoMax(void)
{
    printf("\n== CP5 fixture: power 30 no MAX ==\n");
    PaintFixture(
        "Hammer Fist",
        BATTLE_ELEMENT_NONE,
        "30",
        "SC8",
        0,
        SKILL_CARD_TARGET_EFFECT_NONE,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_PRIMARY,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_NONE,
        "docs/reports/skill_card_visual_repair_logs/impl_power30_nomax.ppm");
}

static void TestLiveResolve(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;

    printf("\n== CP6: live resolve checks ==\n");
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;

    ctx.currentSc = 8;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_HOLD_BACK, &ctx, &view);
    ExpectEqStr(view.detailText, "Support", "Hold Back category Support");
    ExpectTrue(view.showMax, "Hold Back SC==max shows MAX");
    ExpectEqStr(view.scText, "SC8", "Hold Back SC8");

    ctx.currentSc = 9;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_HOLD_BACK, &ctx, &view);
    ExpectTrue(!view.showMax, "over-max does not show MAX");

    ctx.currentSc = 4;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_HAMMER_FIST, &ctx, &view);
    ExpectEqStr(view.name, "Hammer Fist", "Hammer Fist name");
    ExpectTrue(!view.showMax, "Hammer Fist below max");
    ExpectEqInt(view.enemyEffects[2], SKILL_CARD_TARGET_EFFECT_PRIMARY, "HF primary");
    ExpectEqInt(view.enemyEffects[1], SKILL_CARD_TARGET_EFFECT_SECONDARY, "HF left splash");
    ExpectEqInt(view.enemyEffects[3], SKILL_CARD_TARGET_EFFECT_SECONDARY, "HF right splash");

    ctx.currentSc = 0;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_INFLUENCE, &ctx, &view);
    ExpectEqStr(view.name, "Influence", "Influence name");
    ExpectTrue(!view.showMax, "Influence SC0 not MAX");
    ExpectEqStr(view.scText, "SC0", "Influence SC0 text");

    ctx.currentSc = 8;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_TEMPERATURE_DIFFERENCE, &ctx, &view);
    ExpectTrue(view.name[0] != '\0', "long name present");
    ExpectTrue(
        view.nameFit == SKILL_CARD_NAME_FIT_NORMAL
            || view.nameFit == SKILL_CARD_NAME_FIT_COMPACT
            || view.nameFit == SKILL_CARD_NAME_FIT_TRUNCATED,
        "long name fit policy");

    /* Aksil learnset samples: Jab / Vortex Escape / Fiery Strike */
    ctx.currentSc = 3;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_JAB, &ctx, &view);
    ExpectEqStr(view.name, "Jab", "Aksil Jab name");
    SkillCard_ResolveViewModel(MOVE_VORTEX_ESCAPE, &ctx, &view);
    ExpectEqStr(view.name, "Vortex Escape", "Aksil Vortex Escape name");
    SkillCard_ResolveViewModel(MOVE_FIERY_STRIKE, &ctx, &view);
    ExpectEqStr(view.name, "Fiery Strike", "Aksil Fiery Strike name");

    /* Duplicate move IDs in different slots keep independent SC via context. */
    ctx.currentSc = 2;
    ctx.maxSc = 8;
    SkillCard_ResolveViewModel(MOVE_HAMMER_FIST, &ctx, &view);
    ExpectEqStr(view.scText, "SC2", "dup slot A SC2");
    ExpectTrue(!view.showMax, "dup slot A not max");
    ctx.currentSc = 8;
    SkillCard_ResolveViewModel(MOVE_HAMMER_FIST, &ctx, &view);
    ExpectEqStr(view.scText, "SC8", "dup slot B SC8");
    ExpectTrue(view.showMax, "dup slot B at max");
}

static void TestTargetBorder(void)
{
    int x = 193;
    int y = 8;
    printf("\n== CP4: target square border ==\n");
    PaintFixture(
        "Hammer Fist",
        BATTLE_ELEMENT_NONE,
        "Support",
        "SC8",
        1,
        SKILL_CARD_TARGET_EFFECT_NONE,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_PRIMARY,
        SKILL_CARD_TARGET_EFFECT_SECONDARY,
        SKILL_CARD_TARGET_EFFECT_NONE,
        "docs/reports/skill_card_visual_repair_logs/impl_support_max.ppm");
    ExpectEqInt(g_card[y * SKILL_CARD_W + x], SKILL_CARD_COLOR_ENEMY_BASE, "primary corner is base red");
    ExpectEqInt(
        g_card[(y + 1) * SKILL_CARD_W + (x + 1)],
        SKILL_CARD_COLOR_FULL_FX,
        "primary inset is orange");
}

int main(void)
{
    printf("skill_card_visual_repair_host (CP3-CP6)\n");
    TestHammerFistSupportMaxFixture();
    TestSupportNoMax();
    TestPower30NoMax();
    TestTargetBorder();
    TestLiveResolve();
    printf("\nfailures=%d\n", g_failures);
    return g_failures ? 1 : 0;
}
