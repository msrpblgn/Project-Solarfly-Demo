#include "battle_ui/battle_menu.h"
#include "battle_ui/battle_message_queue.h"
#include "battle_ui/hud_layout.h"
#include "battle_ui/move_presentation.h"
#include "battle_ui/skill_card.h"
#include "battle_ui/skill_card_slide.h"
#include "battle/battle.h"
#include "battle/battle_draw.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_state.h"
#include "data/item_database.h"
#include "core/input.h"
#include "core/video.h"
#include "core/gba_types.h"
#include "battle_ui/skill_card_present_trace.h"

static const char *const s_menuLabels[] = {
    "ATTACK",
    "SKILLS",
    "BRACE",
    "ITEM",
    "PARTY",
    "RUN"
};

#define MENU_ITEM_COUNT 6
#define CMD_ATTACK 0
#define CMD_SKILLS 1
#define CMD_BRACE 2
#define CMD_ITEM 3
#define CMD_PARTY 4
#define CMD_RUN 5
#define MOVE_SLOT_COUNT BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT
#define MENU_TEXT_MAX 48

#define BATTLE_MENU_PANEL_BG RGB15(8, 8, 16)
#define SKILLS_PANEL_BG HUD_SKILLS_PANEL_FILL
#define BATTLE_MENU_CMD_CELL_W 52
#define BATTLE_MENU_CMD_CELL_H 11
#define ACTIVE_NAME_MAX_CHARS 12
#define SKILL_PANEL_TITLE "SKILLS"
#define ITEM_PANEL_TITLE "ITEMS"

static char s_noScMessage[MENU_TEXT_MAX];
static SkillCardSlideState s_skillCardSlide;
/* Card + viewport staging in EWRAM on GBA; plain BSS on host framebuffer builds. */
#ifdef VIDEO_HOST_FRAMEBUFFER
static u16 s_skillCardPixelsA[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
static u16 s_skillCardPixelsB[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H];
static u16 s_skillViewportPixels[HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H];
#else
static u16 s_skillCardPixelsA[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H]
    __attribute__((section(".ewram"), aligned(4)));
static u16 s_skillCardPixelsB[SKILL_CARD_MAX_W * SKILL_CARD_MAX_H]
    __attribute__((section(".ewram"), aligned(4)));
static u16 s_skillViewportPixels[HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H]
    __attribute__((section(".ewram"), aligned(4)));
#endif

typedef struct SkillCardSurfaceCache {
    int valid;
    int actorSlot;
    int moveSlot;
    MoveId moveId;
    int currentSc;
    int maxSc;
    int showMax;
    SkillCardDetailKind detailKind;
    BattleElementId elementId;
    u16 resolvedFill;
    unsigned int targetFingerprint;
    int layoutW;
    int layoutH;
    u16 styleBorder;
    u16 styleRevision;
} SkillCardSurfaceCache;

static SkillCardSurfaceCache s_cardCacheA;
static SkillCardSurfaceCache s_cardCacheB;

/* Bumped when aseprite layout/style/panel presentation changes. */
#define SKILL_CARD_STYLE_REVISION 6

static void BattleMenu_BuildResolveContext(
    const BattleState *state,
    const BattleMember *member,
    int moveSlot,
    SkillCardResolveContext *outCtx)
{
    const MoveData *move;

    if (!outCtx)
    {
        return;
    }
    outCtx->currentSc = 0;
    outCtx->maxSc = 0;
    outCtx->actorSide = BATTLE_SIDE_PLAYER;
    outCtx->actorSlot = state ? state->activePlayerSlot : BATTLE_CENTER_SLOT;
    outCtx->hasConcreteTarget = 0;
    outCtx->targetSide = BATTLE_SIDE_ENEMY;
    outCtx->targetSlot = BATTLE_CENTER_SLOT;
    if (!state || !member || moveSlot < 0 || moveSlot >= MOVE_SLOT_COUNT)
    {
        return;
    }
    outCtx->currentSc = member->moveSc[moveSlot];
    move = MoveData_Get(member->equippedMoves[moveSlot]);
    outCtx->maxSc = move ? move->maxSc : 0;
    if (state->phase == BATTLE_PHASE_TARGET_SELECT)
    {
        outCtx->hasConcreteTarget = 1;
        outCtx->targetSide = state->targetSide;
        outCtx->targetSlot = state->targetSlot;
    }
}

static void BattleMenu_InvalidateCardCaches(void)
{
    s_cardCacheA.valid = 0;
    s_cardCacheB.valid = 0;
}

static int BattleMenu_CardCacheMatches(
    const SkillCardSurfaceCache *cache,
    int actorSlot,
    int moveSlot,
    MoveId moveId,
    const SkillCardViewModel *view,
    const SkillCardLayout *layout,
    const SkillCardStyle *style,
    u16 resolvedFill)
{
    if (!cache || !cache->valid || !view || !layout || !style)
    {
        return 0;
    }
    return cache->actorSlot == actorSlot
        && cache->moveSlot == moveSlot
        && cache->moveId == moveId
        && cache->currentSc == view->currentSc
        && cache->maxSc == view->maxSc
        && cache->showMax == view->showMax
        && cache->detailKind == view->detailKind
        && cache->elementId == view->elementId
        && cache->resolvedFill == resolvedFill
        && cache->targetFingerprint == SkillCard_ViewModelTargetFingerprint(view)
        && cache->layoutW == layout->width
        && cache->layoutH == layout->height
        && cache->styleBorder == style->border
        && cache->styleRevision == SKILL_CARD_STYLE_REVISION;
}

static void BattleMenu_StoreCardCache(
    SkillCardSurfaceCache *cache,
    int actorSlot,
    int moveSlot,
    MoveId moveId,
    const SkillCardViewModel *view,
    const SkillCardLayout *layout,
    const SkillCardStyle *style,
    u16 resolvedFill)
{
    if (!cache || !view || !layout || !style)
    {
        return;
    }
    cache->valid = 1;
    cache->actorSlot = actorSlot;
    cache->moveSlot = moveSlot;
    cache->moveId = moveId;
    cache->currentSc = view->currentSc;
    cache->maxSc = view->maxSc;
    cache->showMax = view->showMax;
    cache->detailKind = view->detailKind;
    cache->elementId = view->elementId;
    cache->resolvedFill = resolvedFill;
    cache->targetFingerprint = SkillCard_ViewModelTargetFingerprint(view);
    cache->layoutW = layout->width;
    cache->layoutH = layout->height;
    cache->styleBorder = style->border;
    cache->styleRevision = SKILL_CARD_STYLE_REVISION;
}

/*
 * Paint one equipped slot into a staging surface only when presentation inputs change.
 * Duplicate move IDs still invalidate per-slot because SC is per equipped slot.
 */
static int BattleMenu_EnsureCardSurface(
    const BattleState *state,
    const BattleMember *member,
    int moveSlot,
    u16 *pixels,
    SkillCardSurfaceCache *cache,
    const SkillCardLayout *layout,
    const SkillCardStyle *style)
{
    SkillCardViewModel view;
    SkillCardResolveContext ctx;
    MoveId moveId;
    u16 resolvedFill;

    if (!state || !member || !pixels || !cache || !layout || !style)
    {
        return 0;
    }
    if (moveSlot < 0 || moveSlot >= MOVE_SLOT_COUNT)
    {
        return 0;
    }
    moveId = member->equippedMoves[moveSlot];
    BattleMenu_BuildResolveContext(state, member, moveSlot, &ctx);
    SkillCard_ResolveViewModel(moveId, &ctx, &view);
    resolvedFill = SkillCard_ResolveElementFill(view.elementId, style->fill);
    if (BattleMenu_CardCacheMatches(
            cache,
            state->activePlayerSlot,
            moveSlot,
            moveId,
            &view,
            layout,
            style,
            resolvedFill))
    {
        return 0;
    }
    if (!SkillCard_PaintView(
            pixels,
            SKILL_CARD_MAX_W * SKILL_CARD_MAX_H,
            layout,
            style,
            &view))
    {
        cache->valid = 0;
        return 0;
    }
    BattleMenu_StoreCardCache(
        cache,
        state->activePlayerSlot,
        moveSlot,
        moveId,
        &view,
        layout,
        style,
        resolvedFill);
    return 1;
}

int BattleMenu_SkillCardSlideOffsetPx(int frame, int travelPx)
{
    return SkillCardSlide_OffsetPx(frame, travelPx);
}

void BattleMenu_ClearSkillCardSlide(void)
{
    SkillCardSlide_Clear(&s_skillCardSlide);
    BattleMenu_InvalidateCardCaches();
}

int BattleMenu_IsSkillCardSlideActive(void)
{
    return s_skillCardSlide.active;
}

static int BattleMenu_SkillCardTravelPx(const SkillCardLayout *layout)
{
    if (!layout)
    {
        return SKILL_CARD_W + SKILL_CARD_GAP;
    }
    return layout->width + SKILL_CARD_GAP;
}

static void BattleMenu_FillViewportBuffer(u16 fill)
{
    int i;
    int count = HUD_SKILLS_CARD_VIEW_W * HUD_SKILLS_CARD_VIEW_H;

    for (i = 0; i < count; i++)
    {
        s_skillViewportPixels[i] = fill;
    }
}

static void BattleMenu_PresentSkillViewport(void)
{
    SkillCardPresentTrace_BeginPresent();
    Video_BlitBitmapRegion(
        HUD_SKILLS_CARD_VIEW_X,
        HUD_SKILLS_CARD_VIEW_Y,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        s_skillViewportPixels,
        HUD_SKILLS_CARD_VIEW_W,
        HUD_SKILLS_CARD_VIEW_H,
        0,
        0);
    SkillCardPresentTrace_EndPresent();
}

static void BattleMenu_DrawSkillCount(const BattleMember *member, int slotIndex);

static void BattleMenu_DrawSkillChrome(const BattleMember *member, int countSlot)
{
    SkillCardPresentTrace_BeginChrome();
    /* Clear former numeric counter strip and indicator band. */
    Video_DrawRect(
        HUD_SKILLS_INDICATOR_CX0 - (HUD_SKILLS_INDICATOR_SIZE / 2) - 1,
        HUD_SKILLS_INDICATOR_Y - 1,
        (HUD_SKILLS_INDICATOR_CX2 - HUD_SKILLS_INDICATOR_CX0) + HUD_SKILLS_INDICATOR_SIZE + 2,
        HUD_SKILLS_INDICATOR_SIZE + 2,
        SKILLS_PANEL_BG);
    Video_DrawRect(HUD_SKILLS_COUNT_X, HUD_SKILLS_TITLE_Y, 40, VIDEO_FONT_GLYPH_H, SKILLS_PANEL_BG);
    /* Clear former footer hint area so no stale A:USE / B:BACK remains. */
    Video_DrawRect(0, HUD_SKILLS_FOOTER_Y, SCREEN_WIDTH, SCREEN_HEIGHT - HUD_SKILLS_FOOTER_Y, SKILLS_PANEL_BG);
    Video_DrawText(8, HUD_SKILLS_TITLE_Y, SKILL_PANEL_TITLE, HUD_TEXT_TITLE);
    BattleMenu_DrawSkillCount(member, countSlot);
    SkillCardPresentTrace_EndChrome();
}

static int BattleMenu_BeginSkillCardSlide(
    BattleState *state,
    const BattleMember *member,
    int direction)
{
    int source;
    int dest;

    if (!state || !member || direction == 0)
    {
        return 0;
    }
    if (MovePresentation_CountOccupiedSlots(member->equippedMoves, MOVE_SLOT_COUNT) <= 1)
    {
        return 0;
    }
    source = state->skillMenuSelection;
    if (source < 0 || source >= MOVE_SLOT_COUNT || member->equippedMoves[source] == MOVE_NONE)
    {
        source = MovePresentation_FirstOccupiedSlot(member->equippedMoves, MOVE_SLOT_COUNT);
        state->skillMenuSelection = source;
    }
    dest = MovePresentation_StepOccupiedSlotNoWrap(
        member->equippedMoves, MOVE_SLOT_COUNT, source, direction);
    if (dest == source)
    {
        return 0;
    }
    if (!SkillCardSlide_Begin(
            &s_skillCardSlide,
            state->activePlayerSlot,
            source,
            dest,
            direction))
    {
        return 0;
    }
    /* Prepare both card surfaces once at transition start; draw reuses them. */
    {
        const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
        const SkillCardStyle *style = SkillCard_GetDefaultStyle();

        BattleMenu_EnsureCardSurface(
            state, member, source, s_skillCardPixelsA, &s_cardCacheA, layout, style);
        BattleMenu_EnsureCardSurface(
            state, member, dest, s_skillCardPixelsB, &s_cardCacheB, layout, style);
    }
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
    return 1;
}

static const BattleMember *BattleMenu_GetActiveMember(const BattleState *state)
{
    return &state->playerSlots[state->activePlayerSlot];
}

static int BattleMenu_FirstEquippedSlot(const BattleMember *member)
{
    return MovePresentation_FirstOccupiedSlot(member->equippedMoves, MOVE_SLOT_COUNT);
}

/*
 * Fresh Skills open from the command menu: prefer the second occupied skill
 * (counter 2/N). One skill → that skill; none → slot 0 (empty handling elsewhere).
 */
static int BattleMenu_DefaultOpenSkillSlot(const BattleMember *member)
{
    int occupied;

    if (!member)
    {
        return 0;
    }
    occupied = MovePresentation_CountOccupiedSlots(member->equippedMoves, MOVE_SLOT_COUNT);
    if (occupied <= 0)
    {
        return 0;
    }
    if (occupied == 1)
    {
        return BattleMenu_FirstEquippedSlot(member);
    }
    return MovePresentation_NthOccupiedSlot(member->equippedMoves, MOVE_SLOT_COUNT, 2);
}

static int BattleMenu_HasEquippedMove(const BattleMember *member)
{
    return MovePresentation_CountOccupiedSlots(member->equippedMoves, MOVE_SLOT_COUNT) > 0;
}

static void BattleMenu_AppendChar(char *dest, int *cursor, char c)
{
    if (*cursor < MENU_TEXT_MAX - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void BattleMenu_AppendString(char *dest, int *cursor, const char *text)
{
    while (*text)
    {
        BattleMenu_AppendChar(dest, cursor, *text);
        text++;
    }
}

static void BattleMenu_AppendNumber(char *dest, int *cursor, int value)
{
    char digits[8];
    int count = 0;
    int temp = value;

    if (temp == 0)
    {
        BattleMenu_AppendChar(dest, cursor, '0');
        return;
    }
    while (temp > 0 && count < 8)
    {
        digits[count++] = (char)('0' + (temp % 10));
        temp /= 10;
    }
    while (count > 0)
    {
        BattleMenu_AppendChar(dest, cursor, digits[--count]);
    }
}

static void BattleMenu_DrawActiveMemberHeader(const BattleState *state)
{
    const BattleMember *member;
    char line[24];

    member = BattleState_GetMemberAtSlotConst(
        state, BATTLE_SIDE_PLAYER, state->activePlayerSlot);
    if (member && BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, state->activePlayerSlot))
    {
        BattleMember_FormatNameWithLevel(member, line, (int)sizeof(line));
    }
    else
    {
        line[0] = '\0';
    }
    Video_DrawText(8, HUD_PANEL_TITLE_Y, line, HUD_TEXT_TITLE);
}

static int BattleMenu_CanProcessInput(const BattleState *state)
{
    if (!state)
    {
        return 0;
    }
    if (state->phase != BATTLE_PHASE_PLAYER_COMMAND &&
        state->phase != BATTLE_PHASE_PLAYER_SKILL &&
        state->phase != BATTLE_PHASE_PLAYER_ITEM)
    {
        return 0;
    }
    return Battle_CanActivePlayerAct(state);
}

static int BattleMenu_MoveCommandSelection(int currentSelection, int key)
{
    int row = currentSelection / 2;
    int col = currentSelection % 2;

    if (key == KEY_DOWN)
    {
        row = (row + 1) % 3;
    }
    else if (key == KEY_UP)
    {
        row = (row + 2) % 3;
    }
    else if (key == KEY_LEFT || key == KEY_RIGHT)
    {
        col = 1 - col;
    }
    else
    {
        return currentSelection;
    }
    return row * 2 + col;
}

static void BattleMenu_UpdateCommand(BattleState *state)
{
    if (!BattleMenu_CanProcessInput(state))
    {
        return;
    }
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    if (Input_JustPressed(KEY_DOWN))
    {
        state->menuSelection = BattleMenu_MoveCommandSelection(state->menuSelection, KEY_DOWN);
    }
    if (Input_JustPressed(KEY_UP))
    {
        state->menuSelection = BattleMenu_MoveCommandSelection(state->menuSelection, KEY_UP);
    }
    if (Input_JustPressed(KEY_LEFT))
    {
        state->menuSelection = BattleMenu_MoveCommandSelection(state->menuSelection, KEY_LEFT);
    }
    if (Input_JustPressed(KEY_RIGHT))
    {
        state->menuSelection = BattleMenu_MoveCommandSelection(state->menuSelection, KEY_RIGHT);
    }
    if (Input_JustPressed(KEY_A))
    {
        if (state->menuSelection == CMD_ATTACK)
        {
            Battle_BeginAttackTargetSelect(state);
        }
        else if (state->menuSelection == CMD_SKILLS)
        {
            const BattleMember *member = BattleMenu_GetActiveMember(state);
            if (BattleMenu_HasEquippedMove(member))
            {
                /* Fresh open: settle immediately on default skill (2nd when available). */
                BattleMenu_ClearSkillCardSlide();
                state->skillMenuSelection = BattleMenu_DefaultOpenSkillSlot(member);
                BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_SKILL);
            }
        }
        else if (state->menuSelection == CMD_BRACE)
        {
            Battle_BeginBraceResolve(state);
        }
        else if (state->menuSelection == CMD_ITEM)
        {
            Battle_BeginItemMenu(state);
        }
        else if (state->menuSelection == CMD_PARTY)
        {
            Battle_BeginPartyScreen(state);
        }
        else if (state->menuSelection == CMD_RUN)
        {
            Battle_AttemptRun(state);
        }
    }
}

static void BattleMenu_EnqueueNoScMessage(const char *moveName)
{
    int cursor = 0;

    BattleMenu_AppendString(s_noScMessage, &cursor, "No SC left for ");
    BattleMenu_AppendString(s_noScMessage, &cursor, moveName);
    BattleMenu_AppendChar(s_noScMessage, &cursor, '!');
    s_noScMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_noScMessage);
}

static void BattleMenu_UpdateSkill(BattleState *state)
{
    const BattleMember *member;
    int leftPressed;
    int rightPressed;
    int navigated = 0;
    int occupied;
    int justCommittedSlide = 0;

    if (!BattleMenu_CanProcessInput(state))
    {
        return;
    }
    member = BattleMenu_GetActiveMember(state);

    if (s_skillCardSlide.active &&
        s_skillCardSlide.actorSlot != state->activePlayerSlot)
    {
        BattleMenu_ClearSkillCardSlide();
    }

    if (BattleMessageQueue_IsBusy())
    {
        return;
    }

    /* Precedence: B cancel, then directions, then A (no same-frame confirm after nav). */
    if (Input_JustPressed(KEY_B))
    {
        BattleMenu_ClearSkillCardSlide();
        BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_COMMAND);
        return;
    }

    if (s_skillCardSlide.active)
    {
        int advanceResult;

        /* Ignore directions and A while mid-slide; edge presses are not deferred. */
        advanceResult = SkillCardSlide_Advance(&s_skillCardSlide);
        if (advanceResult > 0)
        {
            /* Commit and clear so a fresh L/R on this frame can start the next
             * slide (~2x re-input vs the old commit + extra unlock tick). */
            state->skillMenuSelection = s_skillCardSlide.destSlot;
            BattleMenu_ClearSkillCardSlide();
            BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
            justCommittedSlide = 1;
        }
        else if (advanceResult < 0)
        {
            /* Settled frame already presented; force a full settled redraw ownership
             * so chrome skip ending cannot leave a wiped/missing card. */
            BattleMenu_ClearSkillCardSlide();
            BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
        }
        else
        {
            BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
            return;
        }
    }

    leftPressed = Input_JustPressed(KEY_LEFT);
    rightPressed = Input_JustPressed(KEY_RIGHT);
    occupied = MovePresentation_CountOccupiedSlots(
        member->equippedMoves, MOVE_SLOT_COUNT);

    /* Up/Down intentionally do nothing on the battle Skills card. */
    if (occupied > 1 && !(leftPressed && rightPressed))
    {
        if (leftPressed)
        {
            navigated = BattleMenu_BeginSkillCardSlide(state, member, -1);
        }
        else if (rightPressed)
        {
            navigated = BattleMenu_BeginSkillCardSlide(state, member, 1);
        }
    }

    /* No same-frame A confirm after settle or after starting a chained slide. */
    if (justCommittedSlide || navigated)
    {
        return;
    }

    if (!navigated && Input_JustPressed(KEY_A))
    {
        MoveId moveId;
        const MoveData *move;

        if (occupied <= 0)
        {
            return;
        }
        if (state->skillMenuSelection < 0
            || state->skillMenuSelection >= MOVE_SLOT_COUNT
            || member->equippedMoves[state->skillMenuSelection] == MOVE_NONE)
        {
            state->skillMenuSelection = MovePresentation_FirstOccupiedSlot(
                member->equippedMoves, MOVE_SLOT_COUNT);
        }
        moveId = member->equippedMoves[state->skillMenuSelection];
        if (moveId == MOVE_NONE)
        {
            return;
        }
        if (!BattleMember_CanUseMoveSlot(member, state->skillMenuSelection))
        {
            move = MoveData_Get(moveId);
            if (move)
            {
                BattleMenu_EnqueueNoScMessage(move->name);
            }
            return;
        }
        Battle_BeginSkillTargetSelect(state, moveId, state->skillMenuSelection);
    }
}

static int BattleMenu_CountUsableItems(const BattleState *state)
{
    int count = 0;
    ItemId itemId;

    if (!state)
    {
        return 0;
    }
    for (itemId = ITEM_POTION; itemId < ITEM_COUNT; itemId++)
    {
        if (BattleState_GetItemCount(state, itemId) > 0)
        {
            count++;
        }
    }
    return count;
}

static ItemId BattleMenu_GetUsableItemAtRow(const BattleState *state, int row)
{
    int currentRow = 0;
    ItemId itemId;

    if (!state || row < 0)
    {
        return ITEM_NONE;
    }
    for (itemId = ITEM_POTION; itemId < ITEM_COUNT; itemId++)
    {
        if (BattleState_GetItemCount(state, itemId) <= 0)
        {
            continue;
        }
        if (currentRow == row)
        {
            return itemId;
        }
        currentRow++;
    }
    return ITEM_NONE;
}

static int BattleMenu_StepUsableItemRow(const BattleState *state, int currentRow, int direction)
{
    int row = currentRow;
    int count = BattleMenu_CountUsableItems(state);
    int i;

    if (count <= 0)
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        row = (row + direction + count) % count;
        if (BattleMenu_GetUsableItemAtRow(state, row) != ITEM_NONE)
        {
            return row;
        }
    }
    return currentRow;
}

static void BattleMenu_DrawQtyText(int x, int y, int quantity, u16 color)
{
    char line[12];
    int cursor = 0;

    BattleMenu_AppendChar(line, &cursor, 'x');
    BattleMenu_AppendNumber(line, &cursor, quantity);
    line[cursor] = '\0';
    Video_DrawText(x, y, line, color);
}

static void BattleMenu_UpdateItem(BattleState *state)
{
    ItemId itemId;
    const ItemData *item;

    if (!BattleMenu_CanProcessInput(state))
    {
        return;
    }
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    if (BattleMenu_CountUsableItems(state) <= 0)
    {
        BattleMessageQueue_Enqueue("No items available.");
        BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_COMMAND);
        return;
    }
    if (Input_JustPressed(KEY_B))
    {
        BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_COMMAND);
        return;
    }
    if (Input_JustPressed(KEY_DOWN))
    {
        state->itemMenuSelection =
            BattleMenu_StepUsableItemRow(state, state->itemMenuSelection, 1);
    }
    if (Input_JustPressed(KEY_UP))
    {
        state->itemMenuSelection =
            BattleMenu_StepUsableItemRow(state, state->itemMenuSelection, -1);
    }
    if (Input_JustPressed(KEY_A))
    {
        itemId = BattleMenu_GetUsableItemAtRow(state, state->itemMenuSelection);
        item = ItemData_Get(itemId);
        if (!item)
        {
            return;
        }
        if (itemId == ITEM_POTION)
        {
            if (BattleState_GetItemCount(state, ITEM_POTION) <= 0)
            {
                BattleMessageQueue_Enqueue("No Potion left.");
                return;
            }
            Battle_BeginItemTargetSelect(state, ITEM_POTION);
            return;
        }
        if (itemId == ITEM_CURE)
        {
            if (BattleState_GetItemCount(state, ITEM_CURE) <= 0)
            {
                BattleMessageQueue_Enqueue("No Cure left.");
                return;
            }
            Battle_BeginItemTargetSelect(state, ITEM_CURE);
        }
    }
}

static void BattleMenu_DrawItemSelectionBox(int x, int y)
{
    Video_DrawRect(x - 2, y - 1, HUD_ITEM_BOX_W, 1, HUD_TEXT_HIGHLIGHT);
    Video_DrawRect(x - 2, y + HUD_ITEM_BOX_H - 2, HUD_ITEM_BOX_W, 1, HUD_TEXT_HIGHLIGHT);
    Video_DrawRect(x - 2, y - 1, 1, HUD_ITEM_BOX_H, HUD_TEXT_HIGHLIGHT);
    Video_DrawRect(x + HUD_ITEM_BOX_W - 3, y - 1, 1, HUD_ITEM_BOX_H, HUD_TEXT_HIGHLIGHT);
}

static void BattleMenu_GetItemRowPosition(int row, int *outX, int *outY)
{
    *outX = HUD_ITEM_COL_LEFT_X;
    *outY = HUD_ITEM_ROW1_Y + (row * HUD_ITEM_ROW_GAP);
}

static void BattleMenu_DrawItem(const BattleState *state)
{
    int menuRow;
    int x;
    int y;
    u16 color;
    ItemId itemId;
    const ItemData *item;
    int itemCount = BattleMenu_CountUsableItems(state);

    Video_DrawText(8, HUD_PANEL_TITLE_Y, ITEM_PANEL_TITLE, HUD_TEXT_TITLE);
    Video_DrawRect(
        8,
        HUD_ITEM_ROW1_Y - 2,
        224,
        (HUD_ITEM_ROW2_Y - HUD_ITEM_ROW1_Y) + HUD_ITEM_BOX_H + 4,
        BATTLE_MENU_PANEL_BG);

    for (menuRow = 0; menuRow < itemCount; menuRow++)
    {
        itemId = BattleMenu_GetUsableItemAtRow(state, menuRow);
        item = ItemData_Get(itemId);
        if (!item)
        {
            continue;
        }

        BattleMenu_GetItemRowPosition(menuRow, &x, &y);
        color = (menuRow == state->itemMenuSelection) ? HUD_TEXT_HIGHLIGHT : HUD_TEXT_MUTED;

        if (menuRow == state->itemMenuSelection)
        {
            BattleMenu_DrawItemSelectionBox(x, y);
        }

        Video_DrawText(x + HUD_ITEM_NAME_X_PAD, y, item->name, color);
        BattleMenu_DrawQtyText(
            HUD_ITEM_COUNT_X,
            y,
            BattleState_GetItemCount(state, itemId),
            color);
    }
}

static void BattleMenu_GetCommandPosition(int index, int *outX, int *outY)
{
    switch (index)
    {
    case CMD_ATTACK:
        *outX = HUD_CMD_COL_LEFT_X;
        *outY = HUD_CMD_ROW1_Y;
        break;
    case CMD_SKILLS:
        *outX = HUD_CMD_COL_RIGHT_X;
        *outY = HUD_CMD_ROW1_Y;
        break;
    case CMD_BRACE:
        *outX = HUD_CMD_COL_LEFT_X;
        *outY = HUD_CMD_ROW2_Y;
        break;
    case CMD_ITEM:
        *outX = HUD_CMD_COL_RIGHT_X;
        *outY = HUD_CMD_ROW2_Y;
        break;
    case CMD_PARTY:
        *outX = HUD_CMD_COL_LEFT_X;
        *outY = HUD_CMD_ROW3_Y;
        break;
    case CMD_RUN:
        *outX = HUD_CMD_COL_RIGHT_X;
        *outY = HUD_CMD_ROW3_Y;
        break;
    default:
        *outX = HUD_CMD_COL_LEFT_X;
        *outY = HUD_CMD_ROW1_Y;
        break;
    }
}

static int BattleMenu_LabelWidth(const char *label)
{
    int width = 0;
    while (label[width])
    {
        width++;
    }
    return width * 6;
}

static void BattleMenu_DrawCommandCell(int index, int selected)
{
    int x;
    int y;

    if (index < 0 || index >= MENU_ITEM_COUNT)
    {
        return;
    }
    BattleMenu_GetCommandPosition(index, &x, &y);
    Video_DrawRect(x - 4, y, BATTLE_MENU_CMD_CELL_W, BATTLE_MENU_CMD_CELL_H, BATTLE_MENU_PANEL_BG);
    Video_DrawText(
        x,
        y,
        s_menuLabels[index],
        selected ? HUD_TEXT_HIGHLIGHT : HUD_TEXT_MUTED);
    if (selected)
    {
        Video_DrawRect(
            x - 4,
            y + 8,
            BattleMenu_LabelWidth(s_menuLabels[index]) + 4,
            2,
            HUD_TEXT_HIGHLIGHT);
    }
}

static void BattleMenu_DrawCommand(const BattleState *state)
{
    int i;
    int x;
    int y;
    u16 color;

    BattleMenu_DrawActiveMemberHeader(state);

    for (i = 0; i < MENU_ITEM_COUNT; i++)
    {
        BattleMenu_GetCommandPosition(i, &x, &y);
        color = (i == state->menuSelection) ? HUD_TEXT_HIGHLIGHT : HUD_TEXT_MUTED;
        Video_DrawText(x, y, s_menuLabels[i], color);
        if (i == state->menuSelection)
        {
            Video_DrawRect(x - 4, y + 8, BattleMenu_LabelWidth(s_menuLabels[i]) + 4, 2, HUD_TEXT_HIGHLIGHT);
        }
    }
}

static void BattleMenu_DrawIndicatorCircle(int centerX, int centerY, int selected)
{
    /*
     * Shared 7×7 circle masks (bit0 = leftmost). Outline = 1px ring;
     * fill = interior only. Selected interior uses full-effect orange.
     */
    static const u8 s_outline[7] = { 0x1C, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1C };
    static const u8 s_fill[7] = { 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00 };
    int row;
    int col;
    int originX = centerX - (HUD_SKILLS_INDICATOR_SIZE / 2);
    int originY = centerY - (HUD_SKILLS_INDICATOR_SIZE / 2);
    u16 outline = HUD_TEXT_TITLE;
    u16 fill = selected ? HUD_SKILLS_INDICATOR_SELECTED : SKILLS_PANEL_BG;

    for (row = 0; row < HUD_SKILLS_INDICATOR_SIZE; row++)
    {
        for (col = 0; col < HUD_SKILLS_INDICATOR_SIZE; col++)
        {
            int bit = 1 << col;
            u16 color;
            if (s_outline[row] & bit)
            {
                color = outline;
            }
            else if (s_fill[row] & bit)
            {
                color = fill;
            }
            else
            {
                continue;
            }
            Video_DrawRect(originX + col, originY + row, 1, 1, color);
        }
    }
}

static void BattleMenu_DrawSkillCount(const BattleMember *member, int slotIndex)
{
    static const int s_centers[3] = {
        HUD_SKILLS_INDICATOR_CX0,
        HUD_SKILLS_INDICATOR_CX1,
        HUD_SKILLS_INDICATOR_CX2
    };
    int occupied;
    int ordinal;
    int i;
    int cy = HUD_SKILLS_INDICATOR_Y + (HUD_SKILLS_INDICATOR_SIZE / 2);

    occupied = MovePresentation_CountOccupiedSlots(member->equippedMoves, MOVE_SLOT_COUNT);
    ordinal = MovePresentation_GetOccupiedOrdinal(
        member->equippedMoves, MOVE_SLOT_COUNT, slotIndex);
    if (occupied <= 0)
    {
        ordinal = 0;
    }
    else if (ordinal <= 0)
    {
        ordinal = 1;
    }

    for (i = 0; i < 3; i++)
    {
        int selected = 0;
        /* Map occupied skills left-to-right onto the three indicators. */
        if (occupied > 0 && (i + 1) <= occupied && ordinal == (i + 1))
        {
            selected = 1;
        }
        BattleMenu_DrawIndicatorCircle(s_centers[i], cy, selected);
    }
}

static void BattleMenu_DrawSkill(const BattleState *state)
{
    const BattleMember *member = BattleMenu_GetActiveMember(state);
    const SkillCardLayout *layout = SkillCard_GetDefaultLayout();
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    int slot;
    int countSlot;
    int occupied;
    int travel;
    int offset;
    int settledX = HUD_SKILLS_CARD_X;
    int settledY = HUD_SKILLS_CARD_Y;
    int sourceOriginX;
    int destOriginX;

    occupied = MovePresentation_CountOccupiedSlots(member->equippedMoves, MOVE_SLOT_COUNT);
    slot = state->skillMenuSelection;
    if (occupied > 0
        && (slot < 0 || slot >= MOVE_SLOT_COUNT || member->equippedMoves[slot] == MOVE_NONE))
    {
        slot = MovePresentation_FirstOccupiedSlot(member->equippedMoves, MOVE_SLOT_COUNT);
    }

    countSlot = SkillCardSlide_CountSlot(&s_skillCardSlide, slot);
    travel = BattleMenu_SkillCardTravelPx(layout);

    /*
     * M3R2: never clear live VRAM card pixels before composition is ready.
     * Paint cards into EWRAM, assemble the full viewport buffer, then present
     * that buffer in one blit. Title/count/footer stay outside the viewport.
     */
    SkillCardPresentTrace_BeginCompose(
        s_skillCardSlide.active,
        s_skillCardSlide.frame,
        s_skillCardSlide.active ? 2 : 1);
    BattleMenu_FillViewportBuffer(SKILLS_PANEL_BG);

    if (occupied <= 0)
    {
        SkillCardPresentTrace_EndCompose();
        BattleMenu_PresentSkillViewport();
        BattleMenu_DrawSkillChrome(member, countSlot);
        Video_DrawText(
            HUD_SKILLS_CARD_X + 2,
            HUD_SKILLS_CARD_Y + 2,
            "NO SKILLS",
            HUD_TEXT_MUTED);
        Video_DrawText(
            HUD_SKILLS_CARD_X + 2,
            HUD_SKILLS_CARD_Y + 20,
            "No equipped moves.",
            HUD_TEXT_MUTED);
        return;
    }

    if (s_skillCardSlide.active)
    {
        /* Reuse surfaces from Begin unless actor/slot/move/SC/layout/style changed. */
        BattleMenu_EnsureCardSurface(
            state,
            member,
            s_skillCardSlide.sourceSlot,
            s_skillCardPixelsA,
            &s_cardCacheA,
            layout,
            style);
        BattleMenu_EnsureCardSurface(
            state,
            member,
            s_skillCardSlide.destSlot,
            s_skillCardPixelsB,
            &s_cardCacheB,
            layout,
            style);
        offset = SkillCardSlide_OffsetPx(s_skillCardSlide.frame, travel);
        if (s_skillCardSlide.direction > 0)
        {
            sourceOriginX = settledX - offset;
            destOriginX = settledX + travel - offset;
        }
        else
        {
            sourceOriginX = settledX + offset;
            destOriginX = settledX - travel + offset;
        }
        SkillCard_BlitIntoBuffer(
            s_skillViewportPixels,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            HUD_SKILLS_CARD_VIEW_X,
            HUD_SKILLS_CARD_VIEW_Y,
            s_skillCardPixelsA,
            layout->width,
            layout->height,
            sourceOriginX,
            settledY);
        SkillCard_BlitIntoBuffer(
            s_skillViewportPixels,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            HUD_SKILLS_CARD_VIEW_X,
            HUD_SKILLS_CARD_VIEW_Y,
            s_skillCardPixelsB,
            layout->width,
            layout->height,
            destOriginX,
            settledY);
    }
    else
    {
        BattleMenu_EnsureCardSurface(
            state, member, slot, s_skillCardPixelsA, &s_cardCacheA, layout, style);
        SkillCard_BlitIntoBuffer(
            s_skillViewportPixels,
            HUD_SKILLS_CARD_VIEW_W,
            HUD_SKILLS_CARD_VIEW_H,
            HUD_SKILLS_CARD_VIEW_X,
            HUD_SKILLS_CARD_VIEW_Y,
            s_skillCardPixelsA,
            layout->width,
            layout->height,
            settledX,
            settledY);
    }

    SkillCardPresentTrace_EndCompose();
    BattleMenu_PresentSkillViewport();
    BattleMenu_DrawSkillChrome(member, countSlot);
}

void BattleMenu_DrawCommandSelectionChange(int oldIndex, int newIndex)
{
    if (oldIndex == newIndex)
    {
        return;
    }
    if (oldIndex >= 0 && oldIndex < MENU_ITEM_COUNT)
    {
        BattleMenu_DrawCommandCell(oldIndex, 0);
    }
    if (newIndex >= 0 && newIndex < MENU_ITEM_COUNT)
    {
        BattleMenu_DrawCommandCell(newIndex, 1);
    }
}

void BattleMenu_Init(void)
{
    s_noScMessage[0] = '\0';
    BattleMenu_ClearSkillCardSlide();
}

void BattleMenu_Update(BattleState *state)
{
    if (state->phase == BATTLE_PHASE_PLAYER_SKILL)
    {
        BattleMenu_UpdateSkill(state);
    }
    else if (state->phase == BATTLE_PHASE_PLAYER_ITEM)
    {
        BattleMenu_UpdateItem(state);
    }
    else if (state->phase == BATTLE_PHASE_PLAYER_COMMAND)
    {
        BattleMenu_UpdateCommand(state);
    }
}

void BattleMenu_Draw(const BattleState *state)
{
    if (state->phase == BATTLE_PHASE_PLAYER_SKILL)
    {
        BattleMenu_DrawSkill(state);
    }
    else if (state->phase == BATTLE_PHASE_PLAYER_ITEM)
    {
        BattleMenu_DrawItem(state);
    }
    else if (state->phase == BATTLE_PHASE_PLAYER_COMMAND)
    {
        BattleMenu_DrawCommand(state);
    }
}
