#include "battle_ui/skill_card.h"

#include "battle/battle_slot.h"
#include "battle_ui/hud_layout.h"
#include "battle_ui/skill_card_fonts.inc"
#include "core/video.h"
#include "data/element_database.h"

static int s_skillCardPaintCallCount;

/* Bump when painter/fonts/icons change so battle_menu discards stale caches. */
#ifndef SKILL_CARD_PAINTER_REVISION
#define SKILL_CARD_PAINTER_REVISION 2
#endif

/* Aseprite upper-row square origins (enemy formation L→R). */
static const int s_enemySquareX[SKILL_CARD_FORMATION_SLOT_COUNT] = {
    175, 184, 193, 202, 211
};
/* Aseprite lower-row square origins (ally formation L→R), staggered. */
static const int s_allySquareX[SKILL_CARD_FORMATION_SLOT_COUNT] = {
    172, 181, 190, 199, 208
};
#define SKILL_CARD_ENEMY_SQUARE_Y 8
#define SKILL_CARD_ALLY_SQUARE_Y 18
#define SKILL_CARD_SQUARE_SIZE 8
#define SKILL_CARD_SQUARE_INSET 1
#define SKILL_CARD_SQUARE_INNER 6

/*
 * MAX mark 16×7 from skills_menu.aseprite (originally x149,y19; card uses x150).
 * Values: RGB15 pixel, or 0xFFFF = transparent (show card fill).
 */
#define SKILL_CARD_MAX_TRANSPARENT 0xFFFFu
static const u16 s_maxMark[7][16] = {
    { 639, 0xFFFF, 0xFFFF, 0xFFFF, 639, 0xFFFF, 0xFFFF, 639, 639, 0xFFFF, 0xFFFF, 639, 0xFFFF, 0xFFFF, 0xFFFF, 639 },
    { 639, 639, 0xFFFF, 639, 639, 0xFFFF, 639, 17215, 17215, 639, 0xFFFF, 17215, 639, 0xFFFF, 639, 17215 },
    { 639, 17215, 639, 17215, 639, 0xFFFF, 639, 0xFFFF, 0xFFFF, 639, 0xFFFF, 0xFFFF, 17215, 639, 17215, 0xFFFF },
    { 639, 0xFFFF, 17215, 0xFFFF, 639, 0xFFFF, 639, 639, 639, 639, 0xFFFF, 0xFFFF, 0xFFFF, 639, 0xFFFF, 0xFFFF },
    { 639, 0xFFFF, 0xFFFF, 0xFFFF, 639, 0xFFFF, 639, 17215, 17215, 639, 0xFFFF, 0xFFFF, 639, 17215, 639, 0xFFFF },
    { 639, 0xFFFF, 0xFFFF, 0xFFFF, 639, 0xFFFF, 639, 0xFFFF, 0xFFFF, 639, 0xFFFF, 639, 17215, 0xFFFF, 17215, 639 },
    { 17215, 0xFFFF, 0xFFFF, 0xFFFF, 17215, 0xFFFF, 17215, 0xFFFF, 0xFFFF, 17215, 0xFFFF, 17215, 0xFFFF, 0xFFFF, 0xFFFF, 17215 }
};

/* Name + compact fonts: include/battle_ui/skill_card_fonts.inc (Aseprite-matched). */

static const SkillCardLayout s_defaultLayout = {
    SKILL_CARD_W,
    SKILL_CARD_HEIGHT,
    4,  /* elementX */
    13, /* elementY */
    12, /* elementSize */
    20, /* nameX */
    12, /* nameY */
    124, /* nameMaxRight exclusive (x20..x123) */
    130, /* detailX */
    12,  /* detailY */
    130, /* scX */
    20,  /* scY */
    150, /* maxX (1px right of Aseprite sample x149 for SC separation) */
    19,  /* maxY */
    16,  /* maxW */
    7    /* maxH */
};

static const SkillCardStyle s_defaultStyle = {
    SKILL_CARD_COLOR_GREEN_BG, /* fallback fill for unmapped elements */
    HUD_TEXT_TITLE,            /* border = SKILLS heading (1px; independent of text) */
    SKILL_CARD_COLOR_NAVY,     /* name */
    SKILL_CARD_COLOR_NAVY,     /* meta / category-power */
    SKILL_CARD_COLOR_NAVY,     /* sc text */
    SKILL_CARD_COLOR_PALE,     /* sc warn (unused in paint path) */
    SKILL_CARD_COLOR_NAVY,     /* icon = card text color; independent field */
    SKILL_CARD_COLOR_PALE,     /* effect (legacy) */
    SKILL_CARD_COLOR_ENEMY_BASE,
    SKILL_CARD_COLOR_NAVY,
    SKILL_CARD_COLOR_FULL_FX,
    SKILL_CARD_COLOR_SECONDARY_FX,
    1
};

static const SkillCardLayout s_variantLayout = {
    200, 40, 4, 14, 12, 20, 14, 110, 120, 14, 120, 24, 145, 23, 16, 7
};

static const SkillCardStyle s_variantStyle = {
    SKILL_CARD_COLOR_GREEN_BG,
    HUD_TEXT_TITLE,            /* border matches default title ink */
    SKILL_CARD_COLOR_NAVY,
    SKILL_CARD_COLOR_NAVY,
    SKILL_CARD_COLOR_NAVY,
    SKILL_CARD_COLOR_PALE,
    SKILL_CARD_COLOR_NAVY,     /* icon matches card text */
    SKILL_CARD_COLOR_PALE,
    SKILL_CARD_COLOR_ENEMY_BASE,
    SKILL_CARD_COLOR_NAVY,
    SKILL_CARD_COLOR_FULL_FX,
    SKILL_CARD_COLOR_SECONDARY_FX,
    2
};

void SkillCard_ResetPaintCallCount(void)
{
    s_skillCardPaintCallCount = 0;
}

int SkillCard_GetPaintCallCount(void)
{
    return s_skillCardPaintCallCount;
}

const SkillCardLayout *SkillCard_GetDefaultLayout(void)
{
    return &s_defaultLayout;
}

const SkillCardStyle *SkillCard_GetDefaultStyle(void)
{
    return &s_defaultStyle;
}

const SkillCardLayout *SkillCard_GetVariantLayout(void)
{
    return &s_variantLayout;
}

const SkillCardStyle *SkillCard_GetVariantStyle(void)
{
    return &s_variantStyle;
}

int SkillCard_ValidateLayout(const SkillCardLayout *layout)
{
    if (!layout)
    {
        return 0;
    }
    if (layout->width <= 0 || layout->height <= 0)
    {
        return 0;
    }
    if (layout->width > SKILL_CARD_MAX_W || layout->height > SKILL_CARD_MAX_H)
    {
        return 0;
    }
    return 1;
}

/*
 * Temporary element → card fill palette. Explicit element IDs only.
 * Unlisted elements keep fallbackFill (default green card fill).
 */
u16 SkillCard_ResolveElementFill(BattleElementId elementId, u16 fallbackFill)
{
    switch (elementId)
    {
    case BATTLE_ELEMENT_NONE:
        return SKILL_CARD_COLOR_ELEM_NEUTRAL;
    case BATTLE_ELEMENT_DARK:
        return SKILL_CARD_COLOR_ELEM_DARK;
    case BATTLE_ELEMENT_CHAOS:
        return SKILL_CARD_COLOR_ELEM_CHAOS;
    case BATTLE_ELEMENT_FIRE:
        return SKILL_CARD_COLOR_ELEM_FIRE;
    default:
        return fallbackFill;
    }
}

static void SkillCard_FillRect(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    int w,
    int h,
    u16 color)
{
    int row;
    int col;

    for (row = 0; row < h; row++)
    {
        int py = y + row;
        if (py < 0 || py >= destH)
        {
            continue;
        }
        for (col = 0; col < w; col++)
        {
            int px = x + col;
            if (px < 0 || px >= destW)
            {
                continue;
            }
            dest[(py * destW) + px] = color;
        }
    }
}

static void SkillCard_CopyText(char *dest, int size, const char *src)
{
    int i;

    if (!dest || size <= 0)
    {
        return;
    }
    if (!src)
    {
        dest[0] = '\0';
        return;
    }
    for (i = 0; i < size - 1 && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int SkillCard_TextLen(const char *text)
{
    int len = 0;

    if (!text)
    {
        return 0;
    }
    while (text[len] != '\0')
    {
        len++;
    }
    return len;
}

static void SkillCard_FormatSc(char *dest, int size, int currentSc)
{
    int cursor = 0;
    int n;
    char digits[12];
    int count = 0;

    if (!dest || size <= 0)
    {
        return;
    }
    dest[0] = '\0';
    if (size < 3)
    {
        return;
    }
    dest[cursor++] = 'S';
    dest[cursor++] = 'C';
    n = currentSc;
    if (n < 0)
    {
        n = 0;
    }
    if (n == 0)
    {
        digits[count++] = '0';
    }
    else
    {
        while (n > 0 && count < (int)sizeof(digits))
        {
            digits[count++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }
    while (count > 0 && cursor < size - 1)
    {
        dest[cursor++] = digits[--count];
    }
    dest[cursor] = '\0';
}

static void SkillCard_FormatPower(char *dest, int size, int power)
{
    int cursor = 0;
    int n = power;
    char digits[12];
    int count = 0;

    if (!dest || size <= 0)
    {
        return;
    }
    dest[0] = '\0';
    if (n < 0)
    {
        n = 0;
    }
    if (n == 0)
    {
        digits[count++] = '0';
    }
    else
    {
        while (n > 0 && count < (int)sizeof(digits))
        {
            digits[count++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }
    while (count > 0 && cursor < size - 1)
    {
        dest[cursor++] = digits[--count];
    }
    dest[cursor] = '\0';
}

static void SkillCard_RaiseEffect(SkillCardTargetEffect *cell, SkillCardTargetEffect level)
{
    if (!cell)
    {
        return;
    }
    if ((int)level > (int)(*cell))
    {
        *cell = level;
    }
}

static void SkillCard_ClearRow(SkillCardTargetEffect *row)
{
    int i;

    for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
    {
        row[i] = SKILL_CARD_TARGET_EFFECT_NONE;
    }
}

static int SkillCard_ClampFormationSlot(int slot)
{
    if (slot < 0)
    {
        return 0;
    }
    if (slot >= SKILL_CARD_FORMATION_SLOT_COUNT)
    {
        return SKILL_CARD_FORMATION_SLOT_COUNT - 1;
    }
    return slot;
}

/*
 * Representative preview before concrete target selection:
 * use formation center slot (BATTLE_CENTER_SLOT / 2) for single-target patterns.
 */
static int SkillCard_RepresentativeSlot(void)
{
    return BATTLE_CENTER_SLOT;
}

static int SkillCard_MoveHasEffect(const MoveData *move, MoveEffectKind kind)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == kind)
        {
            return 1;
        }
    }
    return 0;
}

static void SkillCard_FillTargetEffects(
    const MoveData *move,
    const SkillCardResolveContext *ctx,
    SkillCardViewModel *out)
{
    int focusSlot;
    int i;
    SkillCardTargetEffect *enemy;
    SkillCardTargetEffect *ally;

    enemy = out->enemyEffects;
    ally = out->allyEffects;
    SkillCard_ClearRow(enemy);
    SkillCard_ClearRow(ally);
    if (!move || !ctx)
    {
        return;
    }

    focusSlot = ctx->hasConcreteTarget
        ? SkillCard_ClampFormationSlot(ctx->targetSlot)
        : SkillCard_RepresentativeSlot();

    switch (move->targetPattern)
    {
    case MOVE_TARGET_SINGLE_ENEMY:
        SkillCard_RaiseEffect(&enemy[focusSlot], SKILL_CARD_TARGET_EFFECT_PRIMARY);
        if (SkillCard_MoveHasEffect(move, MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE))
        {
            if (focusSlot > 0)
            {
                SkillCard_RaiseEffect(
                    &enemy[focusSlot - 1], SKILL_CARD_TARGET_EFFECT_SECONDARY);
            }
            if (focusSlot + 1 < SKILL_CARD_FORMATION_SLOT_COUNT)
            {
                SkillCard_RaiseEffect(
                    &enemy[focusSlot + 1], SKILL_CARD_TARGET_EFFECT_SECONDARY);
            }
        }
        if (SkillCard_MoveHasEffect(move, MOVE_EFFECT_KIND_VERTIGO_WAVE_DAMAGE))
        {
            /* Approximate wave: primary on focus, secondary on other enemies. */
            for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
            {
                if (i != focusSlot)
                {
                    SkillCard_RaiseEffect(&enemy[i], SKILL_CARD_TARGET_EFFECT_SECONDARY);
                }
            }
        }
        break;
    case MOVE_TARGET_ALL_ENEMIES:
    case MOVE_TARGET_OUTER_ENEMIES:
        for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
        {
            if (move->targetPattern == MOVE_TARGET_OUTER_ENEMIES
                && i == BATTLE_CENTER_SLOT)
            {
                continue;
            }
            SkillCard_RaiseEffect(&enemy[i], SKILL_CARD_TARGET_EFFECT_PRIMARY);
        }
        break;
    case MOVE_TARGET_SELF:
        SkillCard_RaiseEffect(
            &ally[SkillCard_ClampFormationSlot(ctx->actorSlot)],
            SKILL_CARD_TARGET_EFFECT_PRIMARY);
        if (SkillCard_MoveHasEffect(move, MOVE_EFFECT_KIND_MOVE_TO_SLOT))
        {
            /* Destination tile: concrete targetSlot, else leave actor-only. */
            if (ctx->hasConcreteTarget)
            {
                SkillCard_RaiseEffect(
                    &ally[SkillCard_ClampFormationSlot(ctx->targetSlot)],
                    SKILL_CARD_TARGET_EFFECT_PRIMARY);
            }
        }
        break;
    case MOVE_TARGET_ALLY_SLOT:
    case MOVE_TARGET_SINGLE_ALLY:
        SkillCard_RaiseEffect(&ally[focusSlot], SKILL_CARD_TARGET_EFFECT_PRIMARY);
        if (SkillCard_MoveHasEffect(move, MOVE_EFFECT_KIND_SWAP_WITH_ALLY))
        {
            SkillCard_RaiseEffect(
                &ally[SkillCard_ClampFormationSlot(ctx->actorSlot)],
                SKILL_CARD_TARGET_EFFECT_PRIMARY);
        }
        break;
    case MOVE_TARGET_ADJACENT_ALLIES:
        SkillCard_RaiseEffect(
            &ally[SkillCard_ClampFormationSlot(ctx->actorSlot)],
            SKILL_CARD_TARGET_EFFECT_PRIMARY);
        {
            int actor = SkillCard_ClampFormationSlot(ctx->actorSlot);
            if (actor > 0)
            {
                SkillCard_RaiseEffect(&ally[actor - 1], SKILL_CARD_TARGET_EFFECT_PRIMARY);
            }
            if (actor + 1 < SKILL_CARD_FORMATION_SLOT_COUNT)
            {
                SkillCard_RaiseEffect(&ally[actor + 1], SKILL_CARD_TARGET_EFFECT_PRIMARY);
            }
        }
        break;
    case MOVE_TARGET_ALL_ALLIES:
        for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
        {
            SkillCard_RaiseEffect(&ally[i], SKILL_CARD_TARGET_EFFECT_PRIMARY);
        }
        break;
    case MOVE_TARGET_USER_AND_ENEMY:
        SkillCard_RaiseEffect(
            &ally[SkillCard_ClampFormationSlot(ctx->actorSlot)],
            SKILL_CARD_TARGET_EFFECT_PRIMARY);
        SkillCard_RaiseEffect(&enemy[focusSlot], SKILL_CARD_TARGET_EFFECT_PRIMARY);
        break;
    default:
        break;
    }
}

static int SkillCard_NameGlyphAdvance(char c)
{
    unsigned char uc = (unsigned char)c;
    if (uc < 128 && s_nameFont[uc].advance > 0)
    {
        return s_nameFont[uc].advance * SKILL_CARD_NAME_SCALE;
    }
    /* Missing glyph: reserve a visible placeholder width so chars do not vanish. */
    return 4 * SKILL_CARD_NAME_SCALE;
}

static int SkillCard_CompactGlyphAdvance(char c)
{
    unsigned char uc = (unsigned char)c;
    if (uc < 128 && s_compactFont[uc].advance > 0)
    {
        return s_compactFont[uc].advance;
    }
    return 4;
}

static int SkillCard_MeasureNameRight(const char *text)
{
    int cursor = 0;
    int right = 0;
    if (!text)
    {
        return 0;
    }
    while (*text)
    {
        unsigned char uc = (unsigned char)*text;
        int w = 0;
        int adv = SkillCard_NameGlyphAdvance(*text);
        if (uc < 128 && s_nameFont[uc].width > 0)
        {
            w = s_nameFont[uc].width * SKILL_CARD_NAME_SCALE;
        }
        else if (uc < 128 && s_nameFont[uc].advance > 0)
        {
            w = 0; /* space */
        }
        else
        {
            w = 3 * SKILL_CARD_NAME_SCALE; /* placeholder block */
        }
        right = cursor + w;
        cursor += adv;
        text++;
    }
    return right;
}

static void SkillCard_ApplyNameOverflow(SkillCardViewModel *out, int maxPixels)
{
    int right;
    int i;
    int len;

    if (!out)
    {
        return;
    }
    right = SkillCard_MeasureNameRight(out->name);
    if (right <= maxPixels)
    {
        out->nameFit = SKILL_CARD_NAME_FIT_NORMAL;
        return;
    }

    /*
     * Compact fallback: same reference name font at scale 1 (half size).
     * Measured by temporarily treating scale as 1 via right/2 approximation only when
     * every glyph used the ×2 font; recompute with half advances.
     */
    {
        int cursor = 0;
        int compactRight = 0;
        const char *p = out->name;
        while (*p)
        {
            unsigned char uc = (unsigned char)*p;
            int w = 0;
            int adv = 4;
            if (uc < 128 && s_nameFont[uc].advance > 0)
            {
                adv = s_nameFont[uc].advance; /* logical, no ×2 */
                w = s_nameFont[uc].width;
            }
            compactRight = cursor + w;
            cursor += adv;
            p++;
        }
        if (compactRight <= maxPixels)
        {
            out->nameFit = SKILL_CARD_NAME_FIT_COMPACT;
            return;
        }
    }

    out->nameFit = SKILL_CARD_NAME_FIT_TRUNCATED;
    len = SkillCard_TextLen(out->name);
    /* Trim until measured compact right fits with room for "...". */
    {
        char work[MOVE_PRESENTATION_NAME_MAX];
        int n = len;
        if (n > MOVE_PRESENTATION_NAME_MAX - 1)
        {
            n = MOVE_PRESENTATION_NAME_MAX - 1;
        }
        for (; n >= 1; n--)
        {
            int j;
            int cursor = 0;
            int cr = 0;
            for (j = 0; j < n - 3 && j < n; j++)
            {
                unsigned char uc = (unsigned char)out->name[j];
                int w = 0;
                int adv = 4;
                if (uc < 128 && s_nameFont[uc].advance > 0)
                {
                    adv = s_nameFont[uc].advance;
                    w = s_nameFont[uc].width;
                }
                cr = cursor + w;
                cursor += adv;
            }
            /* ellipsis three dots: '.' advance 2 each ≈ 6 */
            cr = cursor + 5;
            if (cr <= maxPixels || n <= 4)
            {
                for (i = 0; i < n - 3 && i < n; i++)
                {
                    work[i] = out->name[i];
                }
                if (n >= 3)
                {
                    work[i++] = '.';
                    work[i++] = '.';
                    work[i++] = '.';
                }
                work[i] = '\0';
                SkillCard_CopyText(out->name, sizeof(out->name), work);
                return;
            }
        }
    }
}

static const char *SkillCard_CategoryLabel(MoveCategory category)
{
    switch (category)
    {
    case MOVE_CATEGORY_PHYSICAL:
        return "Physical";
    case MOVE_CATEGORY_SPECIAL:
        return "Special";
    case MOVE_CATEGORY_SUPPORT:
        return "Support";
    case MOVE_CATEGORY_STATUS:
        return "Status";
    case MOVE_CATEGORY_DISRUPT:
        return "Disrupt";
    default:
        return "????";
    }
}

void SkillCard_ClearViewModel(SkillCardViewModel *out)
{
    int i;

    if (!out)
    {
        return;
    }
    out->name[0] = '\0';
    out->elementId = BATTLE_ELEMENT_NONE;
    out->detailKind = SKILL_CARD_DETAIL_CATEGORY;
    out->detailText[0] = '\0';
    out->scText[0] = '\0';
    out->currentSc = 0;
    out->maxSc = 0;
    out->showMax = 0;
    out->nameFit = SKILL_CARD_NAME_FIT_NORMAL;
    for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
    {
        out->enemyEffects[i] = SKILL_CARD_TARGET_EFFECT_NONE;
        out->allyEffects[i] = SKILL_CARD_TARGET_EFFECT_NONE;
    }
}

void SkillCard_ResolveViewModel(
    MoveId moveId,
    const SkillCardResolveContext *ctx,
    SkillCardViewModel *out)
{
    const MoveData *move;
    int power;
    SkillCardResolveContext localCtx;

    SkillCard_ClearViewModel(out);
    if (!out)
    {
        return;
    }
    if (!ctx)
    {
        localCtx.currentSc = 0;
        localCtx.maxSc = 0;
        localCtx.actorSide = BATTLE_SIDE_PLAYER;
        localCtx.actorSlot = BATTLE_CENTER_SLOT;
        localCtx.hasConcreteTarget = 0;
        localCtx.targetSide = BATTLE_SIDE_ENEMY;
        localCtx.targetSlot = BATTLE_CENTER_SLOT;
        ctx = &localCtx;
    }

    move = MoveData_Get(moveId);
    if (!move)
    {
        SkillCard_CopyText(out->name, sizeof(out->name), "Empty");
        SkillCard_CopyText(out->detailText, sizeof(out->detailText), "--");
        SkillCard_FormatSc(out->scText, sizeof(out->scText), 0);
        return;
    }

    SkillCard_CopyText(out->name, sizeof(out->name), move->name ? move->name : "");
    /* Presentation aliases applied before overflow measurement. */
    if (moveId == MOVE_VORTEX_ESCAPE)
    {
        SkillCard_CopyText(out->name, sizeof(out->name), "Vortex Esc.");
    }
    out->elementId = move->element;
    out->currentSc = ctx->currentSc;
    out->maxSc = ctx->maxSc > 0 ? ctx->maxSc : move->maxSc;
    if (out->maxSc < 0)
    {
        out->maxSc = 0;
    }
    out->showMax = 0;
    if (out->maxSc > 0)
    {
        if (out->currentSc == out->maxSc)
        {
            out->showMax = 1;
        }
        else if (out->currentSc > out->maxSc)
        {
            /* Invalid over-maximum: do not treat as normal full SC / MAX. */
            out->showMax = 0;
        }
    }
    SkillCard_FormatSc(out->scText, sizeof(out->scText), out->currentSc);

    /*
     * Power vs category: damaging moves with a resolvable power number show power;
     * support/status/variable-power show category label (Support layout).
     */
    power = MoveData_GetDamagePower(move);
    if (MovePresentation_IsVariablePower(move))
    {
        out->detailKind = SKILL_CARD_DETAIL_CATEGORY;
        SkillCard_CopyText(
            out->detailText,
            sizeof(out->detailText),
            SkillCard_CategoryLabel(move->category));
    }
    else if (power > 0
        && (move->category == MOVE_CATEGORY_PHYSICAL
            || move->category == MOVE_CATEGORY_SPECIAL
            || move->category == MOVE_CATEGORY_DISRUPT))
    {
        out->detailKind = SKILL_CARD_DETAIL_POWER;
        SkillCard_FormatPower(out->detailText, sizeof(out->detailText), power);
    }
    else
    {
        out->detailKind = SKILL_CARD_DETAIL_CATEGORY;
        SkillCard_CopyText(
            out->detailText,
            sizeof(out->detailText),
            SkillCard_CategoryLabel(move->category));
    }

    SkillCard_ApplyNameOverflow(out, 124 - 20);
    SkillCard_FillTargetEffects(move, ctx, out);
}

unsigned int SkillCard_ViewModelTargetFingerprint(const SkillCardViewModel *view)
{
    unsigned int hash = 2166136261u;
    int i;

    if (!view)
    {
        return 0;
    }
    for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
    {
        hash ^= (unsigned int)view->enemyEffects[i];
        hash *= 16777619u;
        hash ^= (unsigned int)view->allyEffects[i];
        hash *= 16777619u;
    }
    return hash;
}

static void SkillCard_DrawVarGlyph(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    const SkillCardVarGlyph *glyph,
    u16 color,
    int scale)
{
    int row;
    int col;
    int sy;
    int sx;

    if (!glyph || (glyph->advance == 0 && glyph->width == 0))
    {
        return;
    }
    if (scale < 1)
    {
        scale = 1;
    }
    for (row = 0; row < (int)glyph->height && row < 7; row++)
    {
        for (col = 0; col < (int)glyph->width; col++)
        {
            if (glyph->rows[row] & (1u << col))
            {
                for (sy = 0; sy < scale; sy++)
                {
                    for (sx = 0; sx < scale; sx++)
                    {
                        int px = x + col * scale + sx;
                        int py = y + row * scale + sy;
                        if (px >= 0 && px < destW && py >= 0 && py < destH)
                        {
                            dest[(py * destW) + px] = color;
                        }
                    }
                }
            }
        }
    }
}

static void SkillCard_DrawMissingPlaceholder(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    int w,
    int h,
    u16 color)
{
    /* Hollow box so unsupported characters are visible, not silent. */
    SkillCard_FillRect(dest, destW, destH, x, y, w, 1, color);
    SkillCard_FillRect(dest, destW, destH, x, y + h - 1, w, 1, color);
    SkillCard_FillRect(dest, destW, destH, x, y, 1, h, color);
    SkillCard_FillRect(dest, destW, destH, x + w - 1, y, 1, h, color);
}

static void SkillCard_DrawNameText(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    const char *text,
    u16 color,
    int compact)
{
    int cursor = x;
    int scale = compact ? 1 : SKILL_CARD_NAME_SCALE;

    if (!text)
    {
        return;
    }
    while (*text)
    {
        unsigned char uc = (unsigned char)*text;
        if (uc < 128 && s_nameFont[uc].advance > 0)
        {
            SkillCard_DrawVarGlyph(
                dest,
                destW,
                destH,
                cursor,
                y,
                &s_nameFont[uc],
                color,
                scale);
            cursor += s_nameFont[uc].advance * scale;
        }
        else
        {
            SkillCard_DrawMissingPlaceholder(dest, destW, destH, cursor, y, 3 * scale, 6 * scale, color);
            cursor += 4 * scale;
        }
        text++;
    }
}

static void SkillCard_DrawCompactText(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    const char *text,
    u16 color)
{
    int cursor = x;

    if (!text)
    {
        return;
    }
    while (*text)
    {
        unsigned char uc = (unsigned char)*text;
        if (uc < 128 && s_compactFont[uc].advance > 0)
        {
            SkillCard_DrawVarGlyph(
                dest,
                destW,
                destH,
                cursor,
                y,
                &s_compactFont[uc],
                color,
                1);
            cursor += SkillCard_CompactGlyphAdvance(*text);
        }
        else
        {
            SkillCard_DrawMissingPlaceholder(dest, destW, destH, cursor, y, 3, 6, color);
            cursor += SkillCard_CompactGlyphAdvance(*text);
        }
        text++;
    }
}

static void SkillCard_DrawElementIcon(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    int size,
    BattleElementId element,
    u16 ink)
{
    int row;
    int col;

    (void)size;
    if (element == BATTLE_ELEMENT_NONE || !ElementDatabase_IsValid(element))
    {
        /* Exact 12×12 pale mask from Aseprite; transparent bits leave green fill. */
        for (row = 0; row < 12; row++)
        {
            for (col = 0; col < 12; col++)
            {
                if (s_neutralElementMask[row] & (1u << col))
                {
                    int px = x + col;
                    int py = y + row;
                    if (px >= 0 && px < destW && py >= 0 && py < destH)
                    {
                        dest[(py * destW) + px] = ink;
                    }
                }
            }
        }
        return;
    }

    /*
     * No authoritative per-element symbols are supplied in-repo yet.
     * Explicit temporary fallback: letter initial in a hollow square (not an invented icon).
     */
    {
        static const char *kInitials = "NCMFWNDLWP?"; /* indexed loosely; see report */
        char label = '?';
        SkillCard_FillRect(dest, destW, destH, x, y, 12, 1, ink);
        SkillCard_FillRect(dest, destW, destH, x, y + 11, 12, 1, ink);
        SkillCard_FillRect(dest, destW, destH, x, y, 1, 12, ink);
        SkillCard_FillRect(dest, destW, destH, x + 11, y, 1, 12, ink);
        if ((int)element > 0 && (int)element < (int)BATTLE_ELEMENT_COUNT)
        {
            /* Distinct letter per element id for temporary distinguishability. */
            static const char s_elemLetter[BATTLE_ELEMENT_COUNT] = {
                '?', /* NONE */
                'C', /* CURSE */
                'M', /* MIGHT */
                'X', /* CHAOS */
                'F', /* FIRE */
                'W', /* WATER */
                'N', /* NATURE */
                'I', /* ICE */
                'D', /* DARK */
                'L', /* LIGHT */
                'V', /* WIND */
                'P', /* POISON */
                'T'  /* MENTAL */
            };
            label = s_elemLetter[element];
        }
        (void)kInitials;
        {
            char buf[2];
            buf[0] = label;
            buf[1] = '\0';
            SkillCard_DrawCompactText(dest, destW, destH, x + 4, y + 3, buf, ink);
        }
    }
}

static u16 SkillCard_EffectColor(
    SkillCardTargetEffect effect,
    u16 base,
    const SkillCardStyle *style)
{
    if (effect == SKILL_CARD_TARGET_EFFECT_PRIMARY)
    {
        return style->fullEffectColor;
    }
    if (effect == SKILL_CARD_TARGET_EFFECT_SECONDARY)
    {
        return style->secondaryEffectColor;
    }
    return base;
}

static void SkillCard_DrawTargetSquare(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y,
    SkillCardTargetEffect effect,
    u16 base,
    const SkillCardStyle *style)
{
    /* 1) Full 8×8 base color. 2) Optional 6×6 inset effect; keep 1px base border. */
    SkillCard_FillRect(dest, destW, destH, x, y, SKILL_CARD_SQUARE_SIZE, SKILL_CARD_SQUARE_SIZE, base);
    if (effect != SKILL_CARD_TARGET_EFFECT_NONE)
    {
        u16 fill = SkillCard_EffectColor(effect, base, style);
        SkillCard_FillRect(
            dest,
            destW,
            destH,
            x + SKILL_CARD_SQUARE_INSET,
            y + SKILL_CARD_SQUARE_INSET,
            SKILL_CARD_SQUARE_INNER,
            SKILL_CARD_SQUARE_INNER,
            fill);
    }
}

static void SkillCard_DrawMaxMark(
    u16 *dest,
    int destW,
    int destH,
    int x,
    int y)
{
    int row;
    int col;

    for (row = 0; row < 7; row++)
    {
        for (col = 0; col < 16; col++)
        {
            u16 c = s_maxMark[row][col];
            int px;
            int py;
            if (c == SKILL_CARD_MAX_TRANSPARENT)
            {
                continue;
            }
            px = x + col;
            py = y + row;
            if (px >= 0 && px < destW && py >= 0 && py < destH)
            {
                dest[(py * destW) + px] = c;
            }
        }
    }
}

int SkillCard_PaintView(
    u16 *dest,
    int destCapacityPixels,
    const SkillCardLayout *layout,
    const SkillCardStyle *style,
    const SkillCardViewModel *view)
{
    int w;
    int h;
    int thickness;
    int i;
    int nameCompact;
    u16 scColor;
    u16 fill;

    if (!dest || !layout || !style || !view)
    {
        return 0;
    }
    if (!SkillCard_ValidateLayout(layout))
    {
        return 0;
    }
    w = layout->width;
    h = layout->height;
    if (destCapacityPixels < w * h)
    {
        return 0;
    }

    s_skillCardPaintCallCount++;
    fill = SkillCard_ResolveElementFill(view->elementId, style->fill);
    SkillCard_FillRect(dest, w, h, 0, 0, w, h, fill);
    thickness = style->borderThickness < 1 ? 1 : style->borderThickness;
    SkillCard_FillRect(dest, w, h, 0, 0, w, thickness, style->border);
    SkillCard_FillRect(dest, w, h, 0, h - thickness, w, thickness, style->border);
    SkillCard_FillRect(dest, w, h, 0, 0, thickness, h, style->border);
    SkillCard_FillRect(dest, w, h, w - thickness, 0, thickness, h, style->border);

    SkillCard_DrawElementIcon(
        dest,
        w,
        h,
        layout->elementX,
        layout->elementY,
        layout->elementSize,
        view->elementId,
        style->iconColor);

    nameCompact = 0;
    if (view->nameFit == SKILL_CARD_NAME_FIT_COMPACT
        || view->nameFit == SKILL_CARD_NAME_FIT_TRUNCATED)
    {
        nameCompact = 1;
    }
    SkillCard_DrawNameText(
        dest,
        w,
        h,
        layout->nameX,
        layout->nameY,
        view->name,
        style->nameColor,
        nameCompact);

    SkillCard_DrawCompactText(
        dest,
        w,
        h,
        layout->detailX,
        layout->detailY,
        view->detailText,
        style->metaColor);

    scColor = style->scOkColor;
    SkillCard_DrawCompactText(
        dest,
        w,
        h,
        layout->scX,
        layout->scY,
        view->scText,
        scColor);

    /* Always clear MAX area with resolved card fill so disappearing MAX leaves no remnants. */
    SkillCard_FillRect(
        dest,
        w,
        h,
        layout->maxX,
        layout->maxY,
        layout->maxW,
        layout->maxH,
        fill);
    if (view->showMax)
    {
        SkillCard_DrawMaxMark(dest, w, h, layout->maxX, layout->maxY);
    }

    for (i = 0; i < SKILL_CARD_FORMATION_SLOT_COUNT; i++)
    {
        SkillCard_DrawTargetSquare(
            dest,
            w,
            h,
            s_enemySquareX[i],
            SKILL_CARD_ENEMY_SQUARE_Y,
            view->enemyEffects[i],
            style->enemyBaseColor,
            style);
        SkillCard_DrawTargetSquare(
            dest,
            w,
            h,
            s_allySquareX[i],
            SKILL_CARD_ALLY_SQUARE_Y,
            view->allyEffects[i],
            style->allyBaseColor,
            style);
    }
    return 1;
}

int SkillCard_Paint(
    u16 *dest,
    int destCapacityPixels,
    const SkillCardLayout *layout,
    const SkillCardStyle *style,
    const MovePresentationFields *fields,
    int currentSc)
{
    SkillCardViewModel view;

    SkillCard_ClearViewModel(&view);
    if (fields)
    {
        SkillCard_CopyText(view.name, sizeof(view.name), fields->name);
        if (fields->power[0] != '\0' && fields->power[0] != '-')
        {
            view.detailKind = SKILL_CARD_DETAIL_POWER;
            SkillCard_CopyText(view.detailText, sizeof(view.detailText), fields->power);
        }
        else
        {
            view.detailKind = SKILL_CARD_DETAIL_CATEGORY;
            SkillCard_CopyText(view.detailText, sizeof(view.detailText), fields->category);
        }
    }
    view.currentSc = currentSc;
    SkillCard_FormatSc(view.scText, sizeof(view.scText), currentSc);
    SkillCard_ApplyNameOverflow(&view, 124 - 20);
    return SkillCard_PaintView(dest, destCapacityPixels, layout, style, &view);
}

void SkillCard_BlitClipped(
    const u16 *cardPixels,
    int cardW,
    int cardH,
    int originX,
    int originY,
    const SkillCardViewport *viewport)
{
    int viewLeft;
    int viewTop;
    int viewRight;
    int viewBottom;
    int cardLeft;
    int cardTop;
    int cardRight;
    int cardBottom;
    int left;
    int top;
    int right;
    int bottom;
    int srcX;
    int srcY;
    int copyW;
    int copyH;

    if (!cardPixels || cardW <= 0 || cardH <= 0 || !viewport)
    {
        return;
    }

    viewLeft = viewport->x;
    viewTop = viewport->y;
    viewRight = viewport->x + viewport->w;
    viewBottom = viewport->y + viewport->h;
    cardLeft = originX;
    cardTop = originY;
    cardRight = originX + cardW;
    cardBottom = originY + cardH;

    left = cardLeft > viewLeft ? cardLeft : viewLeft;
    top = cardTop > viewTop ? cardTop : viewTop;
    right = cardRight < viewRight ? cardRight : viewRight;
    bottom = cardBottom < viewBottom ? cardBottom : viewBottom;
    if (left >= right || top >= bottom)
    {
        return;
    }

    srcX = left - originX;
    srcY = top - originY;
    copyW = right - left;
    copyH = bottom - top;
    Video_BlitBitmapRegion(
        left,
        top,
        copyW,
        copyH,
        cardPixels,
        cardW,
        cardH,
        srcX,
        srcY);
}

void SkillCard_BlitIntoBuffer(
    u16 *destPixels,
    int destW,
    int destH,
    int destOriginX,
    int destOriginY,
    const u16 *cardPixels,
    int cardW,
    int cardH,
    int cardOriginX,
    int cardOriginY)
{
    int row;
    int col;
    int srcX0;
    int srcY0;
    int copyW;
    int copyH;
    int left;
    int top;
    int right;
    int bottom;

    if (!destPixels || !cardPixels || destW <= 0 || destH <= 0 || cardW <= 0 || cardH <= 0)
    {
        return;
    }

    left = cardOriginX;
    top = cardOriginY;
    right = cardOriginX + cardW;
    bottom = cardOriginY + cardH;
    if (left < destOriginX)
    {
        left = destOriginX;
    }
    if (top < destOriginY)
    {
        top = destOriginY;
    }
    if (right > destOriginX + destW)
    {
        right = destOriginX + destW;
    }
    if (bottom > destOriginY + destH)
    {
        bottom = destOriginY + destH;
    }
    if (left >= right || top >= bottom)
    {
        return;
    }

    srcX0 = left - cardOriginX;
    srcY0 = top - cardOriginY;
    copyW = right - left;
    copyH = bottom - top;
    for (row = 0; row < copyH; row++)
    {
        for (col = 0; col < copyW; col++)
        {
            int dx = (left - destOriginX) + col;
            int dy = (top - destOriginY) + row;
            destPixels[(dy * destW) + dx] =
                cardPixels[((srcY0 + row) * cardW) + (srcX0 + col)];
        }
    }
}
