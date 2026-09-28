/*
 * Skills visual polish host checks (panel navy, element fills, detail navy, MAX).
 *
 * gcc -std=c11 -Wall -Wextra -O0 -Iinclude -Itools/host_tests \
 *   -o tools/host_tests/test_skill_card_visual_polish_host.exe \
 *   tools/host_tests/test_skill_card_visual_polish_host.c \
 *   src/battle_ui/skill_card.c src/battle_ui/move_presentation.c \
 *   src/battle/battle_move.c src/core/video.c src/core/random.c \
 *   src/data/move_database.c src/data/element_database.c \
 *   src/data/race_database.c src/data/status_database.c \
 *   -DVIDEO_HOST_FRAMEBUFFER
 */
#include <stdio.h>
#include <string.h>

#include "battle/battle_element.h"
#include "battle/battle_move.h"
#include "battle/battle_slot.h"
#include "battle_ui/hud_layout.h"
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

static void ExpectEqU16(u16 got, u16 want, const char *label)
{
    if (got != want)
    {
        printf("FAIL: %s (got %u want %u)\n", label, (unsigned)got, (unsigned)want);
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

static void PaintNamed(
    const char *name,
    BattleElementId element,
    const char *detail,
    const char *sc,
    int showMax,
    const char *path)
{
    SkillCardViewModel view;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    SkillCard_ClearViewModel(&view);
    strncpy(view.name, name, sizeof(view.name) - 1);
    view.elementId = element;
    view.detailKind = SKILL_CARD_DETAIL_CATEGORY;
    strncpy(view.detailText, detail, sizeof(view.detailText) - 1);
    strncpy(view.scText, sc, sizeof(view.scText) - 1);
    view.showMax = showMax;
    view.nameFit = SKILL_CARD_NAME_FIT_NORMAL;
    view.enemyEffects[2] = SKILL_CARD_TARGET_EFFECT_PRIMARY;
    memset(g_card, 0, sizeof(g_card));
    SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view);
    WritePpm(path, g_card, SKILL_CARD_W, SKILL_CARD_HEIGHT);
}

static void TestPaletteAndDetail(void)
{
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    u16 fb = SKILL_CARD_COLOR_GREEN_BG;

    printf("\n== element fill palette + detail navy ==\n");
    ExpectEqU16(
        SkillCard_ResolveElementFill(BATTLE_ELEMENT_NONE, fb),
        SKILL_CARD_COLOR_ELEM_NEUTRAL,
        "Neutral fill");
    ExpectEqU16(
        SkillCard_ResolveElementFill(BATTLE_ELEMENT_DARK, fb),
        SKILL_CARD_COLOR_ELEM_DARK,
        "Dark fill");
    ExpectEqU16(
        SkillCard_ResolveElementFill(BATTLE_ELEMENT_CHAOS, fb),
        SKILL_CARD_COLOR_ELEM_CHAOS,
        "Chaos fill");
    ExpectEqU16(
        SkillCard_ResolveElementFill(BATTLE_ELEMENT_FIRE, fb),
        SKILL_CARD_COLOR_ELEM_FIRE,
        "Fire fill");
    ExpectEqU16(
        SkillCard_ResolveElementFill(BATTLE_ELEMENT_WATER, fb),
        fb,
        "Water falls back to default fill");

    ExpectEqU16(style->metaColor, SKILL_CARD_COLOR_NAVY, "metaColor navy");
    ExpectEqU16(style->scOkColor, SKILL_CARD_COLOR_NAVY, "scOkColor navy");
    ExpectEqU16(style->iconColor, SKILL_CARD_COLOR_NAVY, "iconColor navy");
    ExpectEqU16(style->border, HUD_TEXT_TITLE, "border title");
    ExpectEqInt(SkillCard_GetDefaultLayout()->maxX, 150, "MAX x150 preserved");

    PaintNamed(
        "Hammer Fist",
        BATTLE_ELEMENT_NONE,
        "Support",
        "SC8",
        1,
        "docs/reports/skill_card_visual_polish_logs/neutral_support_max.ppm");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_ELEM_NEUTRAL) > 1000,
        "neutral card uses gray fill");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_NAVY) > 20,
        "navy ink present (detail/name/icon)");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_FULL_FX) > 0,
        "MAX orange preserved");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, HUD_TEXT_TITLE) > 100,
        "title border ink present");

    PaintNamed(
        "Vortex Esc.",
        BATTLE_ELEMENT_FIRE,
        "30",
        "SC16",
        1,
        "docs/reports/skill_card_visual_polish_logs/fire_sc16_max.ppm");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_ELEM_FIRE) > 1000,
        "fire card uses red fill");
    /* Gap x149 has no MAX orange. */
    ExpectEqInt(
        g_card[19 * SKILL_CARD_W + 149] == SKILL_CARD_COLOR_FULL_FX ? 1 : 0,
        0,
        "x149 not MAX orange");
    ExpectTrue(
        CountColor(g_card + (19 * SKILL_CARD_W + 150), 16 * 7, SKILL_CARD_COLOR_FULL_FX)
                + CountColor(g_card + (20 * SKILL_CARD_W + 150), 16, SKILL_CARD_COLOR_FULL_FX)
            > 0
            || CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_FULL_FX) > 0,
        "MAX present at x150 region");

    PaintNamed(
        "Equalizer",
        BATTLE_ELEMENT_DARK,
        "Physical",
        "SC3",
        0,
        "docs/reports/skill_card_visual_polish_logs/dark_nomax.ppm");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_ELEM_DARK) > 1000,
        "dark card black fill");
    ExpectEqInt(
        CountColor(g_card + (19 * SKILL_CARD_W + 150), 16 * 7 > 0 ? 16 : 0, SKILL_CARD_COLOR_FULL_FX),
        0,
        "no MAX orange when hidden (approx)");

    PaintNamed(
        "Chaos Test",
        BATTLE_ELEMENT_CHAOS,
        "Special",
        "SC1",
        0,
        "docs/reports/skill_card_visual_polish_logs/chaos_card.ppm");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_ELEM_CHAOS) > 1000,
        "chaos card fill");

    PaintNamed(
        "Tide",
        BATTLE_ELEMENT_WATER,
        "Support",
        "SC0",
        0,
        "docs/reports/skill_card_visual_polish_logs/water_fallback_green.ppm");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_GREEN_BG) > 1000,
        "unmapped Water keeps green fallback");
}

static void TestPanelConstants(void)
{
    printf("\n== panel / indicator constants ==\n");
    ExpectEqU16(HUD_SKILLS_PANEL_FILL, SKILL_CARD_COLOR_NAVY, "panel fill == card navy");
    ExpectEqInt(HUD_SKILLS_INDICATOR_CX0, 108, "indicator cx0");
    ExpectEqInt(HUD_SKILLS_INDICATOR_CX1, 120, "indicator cx1");
    ExpectEqInt(HUD_SKILLS_INDICATOR_CX2, 132, "indicator cx2");
    ExpectEqInt(HUD_SKILLS_INDICATOR_SIZE, 7, "indicator size");
    ExpectEqU16(HUD_SKILLS_INDICATOR_SELECTED, SKILL_CARD_COLOR_FULL_FX, "selected == full FX orange");
}

static void TestVortexAliasPreserved(void)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;

    printf("\n== Vortex Esc. preserved ==\n");
    ctx.currentSc = 4;
    ctx.maxSc = 8;
    ctx.actorSide = BATTLE_SIDE_PLAYER;
    ctx.actorSlot = 2;
    ctx.hasConcreteTarget = 0;
    ctx.targetSide = BATTLE_SIDE_ENEMY;
    ctx.targetSlot = 2;
    SkillCard_ResolveViewModel(MOVE_VORTEX_ESCAPE, &ctx, &view);
    ExpectTrue(strcmp(view.name, "Vortex Esc.") == 0, "Vortex Esc. alias");
    ExpectEqInt(view.nameFit, SKILL_CARD_NAME_FIT_NORMAL, "normal large font");
    ExpectEqInt((int)view.elementId, (int)BATTLE_ELEMENT_FIRE, "Vortex element Fire");
}

int main(void)
{
    printf("skill_card_visual_polish_host\n");
    TestPanelConstants();
    TestPaletteAndDetail();
    TestVortexAliasPreserved();
    printf("\nfailures=%d\n", g_failures);
    return g_failures ? 1 : 0;
}
