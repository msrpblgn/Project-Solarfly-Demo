#ifndef SKILL_CARD_APPEARANCE_H
#define SKILL_CARD_APPEARANCE_H

#include "battle/battle_element.h"
#include "battle/battle_move.h"
#include "battle/battle_slot.h"
#include "battle_ui/move_presentation.h"
#include "core/gba_types.h"

/*
 * Aseprite skills_menu.aseprite card appearance (224×34).
 * Resolve move + runtime context → SkillCardViewModel, then paint.
 * Painter uses only the view model (no battle globals / move-ID branches).
 */

#define SKILL_CARD_MAX_W 232
#define SKILL_CARD_MAX_H 48
#define SKILL_CARD_W 224
#define SKILL_CARD_HEIGHT 34
#define SKILL_CARD_GAP 8

#define SKILL_CARD_FORMATION_SLOT_COUNT 5

/* Aseprite #RRGGBB → RGB15(r>>3,g>>3,b>>3) using project RGB15(). */
#define SKILL_CARD_COLOR_GREEN_BG RGB15(3, 20, 13)       /* #18A46A */
#define SKILL_CARD_COLOR_NAVY RGB15(2, 4, 7)             /* #102039 */
#define SKILL_CARD_COLOR_PALE RGB15(26, 26, 26)          /* #D5D5D5 */
#define SKILL_CARD_COLOR_ENEMY_BASE RGB15(24, 8, 8)      /* #C54141 */
#define SKILL_CARD_COLOR_FULL_FX RGB15(31, 19, 0)        /* #FF9C00 */
#define SKILL_CARD_COLOR_SECONDARY_FX RGB15(31, 25, 16)  /* #FFCF84 */
/* Temporary element card fills (presentation palette; replace later as needed). */
#define SKILL_CARD_COLOR_ELEM_NEUTRAL RGB15(20, 20, 20)  /* #A4A4A4 */
#define SKILL_CARD_COLOR_ELEM_DARK RGB15(0, 0, 0)        /* #000000 opaque fill */
#define SKILL_CARD_COLOR_ELEM_CHAOS RGB15(25, 19, 10)    /* #CD9C52 */
#define SKILL_CARD_COLOR_ELEM_FIRE RGB15(24, 3, 1)       /* #C01F0A */

typedef enum SkillCardTargetEffect {
    SKILL_CARD_TARGET_EFFECT_NONE = 0,
    SKILL_CARD_TARGET_EFFECT_SECONDARY,
    SKILL_CARD_TARGET_EFFECT_PRIMARY
} SkillCardTargetEffect;

typedef enum SkillCardDetailKind {
    SKILL_CARD_DETAIL_POWER = 0,
    SKILL_CARD_DETAIL_CATEGORY
} SkillCardDetailKind;

typedef enum SkillCardNameFit {
    SKILL_CARD_NAME_FIT_NORMAL = 0,
    SKILL_CARD_NAME_FIT_COMPACT,
    SKILL_CARD_NAME_FIT_TRUNCATED
} SkillCardNameFit;

/*
 * Runtime context for SC + target preview. Callers supply remaining SC from the
 * equipped slot (duplicates keep independent charges). Target preview is pure.
 */
typedef struct SkillCardResolveContext {
    int currentSc;
    int maxSc;
    int actorSide;
    int actorSlot;
    /* 1 = use targetSide/targetSlot; 0 = representative pattern. */
    int hasConcreteTarget;
    int targetSide;
    int targetSlot;
} SkillCardResolveContext;

typedef struct SkillCardViewModel {
    char name[MOVE_PRESENTATION_NAME_MAX];
    BattleElementId elementId;
    SkillCardDetailKind detailKind;
    char detailText[MOVE_PRESENTATION_CATEGORY_MAX];
    char scText[MOVE_PRESENTATION_SC_MAX];
    int currentSc;
    int maxSc;
    int showMax;
    SkillCardNameFit nameFit;
    SkillCardTargetEffect enemyEffects[SKILL_CARD_FORMATION_SLOT_COUNT];
    SkillCardTargetEffect allyEffects[SKILL_CARD_FORMATION_SLOT_COUNT];
} SkillCardViewModel;

typedef struct SkillCardStyle {
    u16 fill; /* fallback card fill when element has no palette entry */
    u16 border;
    u16 nameColor;
    u16 metaColor; /* category / power text */
    u16 scOkColor; /* remaining SC text */
    u16 scWarnColor;
    u16 iconColor; /* element symbol ink; independent of border/text fields */
    u16 effectColor;
    u16 enemyBaseColor;
    u16 allyBaseColor;
    u16 fullEffectColor;
    u16 secondaryEffectColor;
    int borderThickness;
} SkillCardStyle;

typedef struct SkillCardLayout {
    int width;
    int height;
    int elementX;
    int elementY;
    int elementSize;
    int nameX;
    int nameY;
    int nameMaxRight; /* exclusive */
    int detailX;
    int detailY;
    int scX;
    int scY;
    int maxX;
    int maxY;
    int maxW;
    int maxH;
} SkillCardLayout;

typedef struct SkillCardViewport {
    int x;
    int y;
    int w;
    int h;
} SkillCardViewport;

const SkillCardLayout *SkillCard_GetDefaultLayout(void);
const SkillCardStyle *SkillCard_GetDefaultStyle(void);
const SkillCardLayout *SkillCard_GetVariantLayout(void);
const SkillCardStyle *SkillCard_GetVariantStyle(void);
int SkillCard_ValidateLayout(const SkillCardLayout *layout);

/* Temporary presentation palette: Neutral/Dark/Chaos/Fire; else returns fallbackFill. */
u16 SkillCard_ResolveElementFill(BattleElementId elementId, u16 fallbackFill);

void SkillCard_ClearViewModel(SkillCardViewModel *out);
void SkillCard_ResolveViewModel(
    MoveId moveId,
    const SkillCardResolveContext *ctx,
    SkillCardViewModel *out);

/* Fingerprint of target-effect cells for cache invalidation (order-stable). */
unsigned int SkillCard_ViewModelTargetFingerprint(const SkillCardViewModel *view);

int SkillCard_PaintView(
    u16 *dest,
    int destCapacityPixels,
    const SkillCardLayout *layout,
    const SkillCardStyle *style,
    const SkillCardViewModel *view);

/*
 * Legacy string-field paint kept for older host geometry checks.
 * Builds a minimal view model (no target highlights) then paints.
 */
int SkillCard_Paint(
    u16 *dest,
    int destCapacityPixels,
    const SkillCardLayout *layout,
    const SkillCardStyle *style,
    const MovePresentationFields *fields,
    int currentSc);

void SkillCard_ResetPaintCallCount(void);
int SkillCard_GetPaintCallCount(void);

void SkillCard_BlitClipped(
    const u16 *cardPixels,
    int cardW,
    int cardH,
    int originX,
    int originY,
    const SkillCardViewport *viewport);

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
    int cardOriginY);

#endif
