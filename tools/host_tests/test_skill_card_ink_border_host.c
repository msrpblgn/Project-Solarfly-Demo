/*
 * Skill card ink/border polish: navy element symbols + HUD_TEXT_TITLE outline.
 * Verifies via production SkillCard_PaintView renderer.
 *
 * gcc -std=c11 -Wall -Wextra -O0 -Iinclude -Itools/host_tests \
 *   -o tools/host_tests/test_skill_card_ink_border_host.exe \
 *   tools/host_tests/test_skill_card_ink_border_host.c \
 *   src/battle_ui/skill_card.c src/battle_ui/move_presentation.c \
 *   src/battle/battle_move.c src/core/video.c src/core/random.c \
 *   src/data/move_database.c src/data/element_database.c \
 *   src/data/race_database.c src/data/status_database.c \
 *   -DVIDEO_HOST_FRAMEBUFFER
 */
#include <stdio.h>
#include <string.h>

#include "battle/battle_element.h"
#include "battle_ui/hud_layout.h"
#include "battle_ui/skill_card.h"

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

static int CountColorInRect(
    const u16 *px,
    int destW,
    int x,
    int y,
    int w,
    int h,
    u16 color)
{
    int row;
    int col;
    int n = 0;

    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            if (px[((y + row) * destW) + (x + col)] == color)
            {
                n++;
            }
        }
    }
    return n;
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

static void PaintNeutralSupport(void)
{
    SkillCardViewModel view;
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    SkillCard_ClearViewModel(&view);
    strncpy(view.name, "Hammer Fist", sizeof(view.name) - 1);
    view.elementId = BATTLE_ELEMENT_NONE;
    view.detailKind = SKILL_CARD_DETAIL_CATEGORY;
    strncpy(view.detailText, "Support", sizeof(view.detailText) - 1);
    strncpy(view.scText, "SC8", sizeof(view.scText) - 1);
    view.showMax = 1;
    view.nameFit = SKILL_CARD_NAME_FIT_NORMAL;
    memset(g_card, 0, sizeof(g_card));
    SkillCard_PaintView(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, layout, style, &view);
}

static void TestStyleDefaults(void)
{
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    printf("\n== style defaults (independent fields) ==\n");
    ExpectEqU16(style->nameColor, SKILL_CARD_COLOR_NAVY, "nameColor navy #102039");
    ExpectEqU16(style->metaColor, SKILL_CARD_COLOR_NAVY, "metaColor navy");
    ExpectEqU16(style->scOkColor, SKILL_CARD_COLOR_NAVY, "scOkColor navy");
    ExpectEqU16(style->iconColor, SKILL_CARD_COLOR_NAVY, "iconColor navy (text match)");
    ExpectEqU16(style->iconColor, style->nameColor, "iconColor == nameColor value");
    ExpectEqU16(style->border, HUD_TEXT_TITLE, "border == HUD_TEXT_TITLE");
    ExpectTrue(style->border != style->iconColor, "border field distinct from iconColor");
    ExpectTrue(style->border != style->nameColor, "border field distinct from nameColor");
    ExpectEqInt(style->borderThickness, 1, "border thickness 1px");
    ExpectEqU16(HUD_SKILLS_PANEL_FILL, SKILL_CARD_COLOR_NAVY, "Skills panel still navy");
}

static void TestProductionPaintColors(void)
{
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    int iconInk;
    int iconHoles;
    int paleInIcon;
    int borderTop;
    int borderLeft;
    int innerNotBorder;

    printf("\n== production PaintView colors ==\n");
    PaintNeutralSupport();

    ExpectEqU16(g_card[0], HUD_TEXT_TITLE, "top-left corner is title border");
    ExpectEqU16(
        g_card[SKILL_CARD_W - 1],
        HUD_TEXT_TITLE,
        "top-right corner is title border");
    ExpectEqU16(
        g_card[(SKILL_CARD_HEIGHT - 1) * SKILL_CARD_W],
        HUD_TEXT_TITLE,
        "bottom-left corner is title border");

    borderTop = CountColorInRect(g_card, SKILL_CARD_W, 0, 0, SKILL_CARD_W, 1, HUD_TEXT_TITLE);
    borderLeft = CountColorInRect(g_card, SKILL_CARD_W, 0, 0, 1, SKILL_CARD_HEIGHT, HUD_TEXT_TITLE);
    ExpectEqInt(borderTop, SKILL_CARD_W, "full top edge is 1px title border");
    ExpectEqInt(borderLeft, SKILL_CARD_HEIGHT, "full left edge is 1px title border");

    /* Immediately inside the border must be fill, not a thicker outline. */
    innerNotBorder = g_card[1 * SKILL_CARD_W + 1] != HUD_TEXT_TITLE ? 1 : 0;
    ExpectEqInt(innerNotBorder, 1, "pixel (1,1) is not border (1px thickness)");
    ExpectEqU16(
        g_card[1 * SKILL_CARD_W + 1],
        SKILL_CARD_COLOR_ELEM_NEUTRAL,
        "interior uses neutral fill");

    iconInk = CountColorInRect(
        g_card,
        SKILL_CARD_W,
        layout->elementX,
        layout->elementY,
        12,
        12,
        style->iconColor);
    iconHoles = CountColorInRect(
        g_card,
        SKILL_CARD_W,
        layout->elementX,
        layout->elementY,
        12,
        12,
        SKILL_CARD_COLOR_ELEM_NEUTRAL);
    paleInIcon = CountColorInRect(
        g_card,
        SKILL_CARD_W,
        layout->elementX,
        layout->elementY,
        12,
        12,
        SKILL_CARD_COLOR_PALE);

    ExpectTrue(iconInk > 20, "element symbol has navy ink pixels");
    ExpectTrue(iconHoles > 20, "element symbol preserves transparent holes (fill)");
    ExpectEqInt(paleInIcon, 0, "element symbol has no pale ink");
    ExpectTrue(
        CountColor(g_card, SKILL_CARD_W * SKILL_CARD_HEIGHT, SKILL_CARD_COLOR_FULL_FX) > 0,
        "MAX artwork preserved");
}

int main(void)
{
    printf("skill_card_ink_border_host\n");
    TestStyleDefaults();
    TestProductionPaintColors();
    printf("\nfailures=%d\n", g_failures);
    return g_failures ? 1 : 0;
}
