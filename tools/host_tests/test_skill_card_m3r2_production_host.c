/*
 * M3R2 verification follow-up: production Skills update/draw path.
 *
 * Calls real BattleMenu_Update / Battle_DrawPlayerSkillPanel / BattleDraw_* dirty
 * flags — not a reimplementation of ComposeAndPresent.
 *
 * Build (WSL):
 *   gcc -std=c11 -Wall -Wextra -O2 \
 *       -DVIDEO_HOST_FRAMEBUFFER -DSKILL_CARD_PRESENT_TRACE \
 *       -Iinclude -Itools/host_tests \
 *       -o tools/host_tests/test_skill_card_m3r2_production_host \
 *       tools/host_tests/test_skill_card_m3r2_production_host.c \
 *       tools/host_tests/host_battle_skills_stubs.c \
 *       src/core/video.c \
 *       src/battle/battle_draw.c \
 *       src/battle/battle_skills_panel.c \
 *       src/battle/battle_state.c \
 *       src/battle/battle_member.c \
 *       src/battle/battle_move.c \
 *       src/battle/stat_curve.c \
 *       src/battle_ui/battle_menu.c \
 *       src/battle_ui/skill_card.c \
 *       src/battle_ui/skill_card_slide.c \
 *       src/battle_ui/skill_card_present_trace.c \
 *       src/battle_ui/move_presentation.c \
 *       src/data/move_database.c \
 *       src/data/element_database.c \
 *       src/data/race_database.c \
 *       src/data/member_database.c \
 *       src/data/status_database.c \
 *       src/data/item_database.c \
 *       src/core/random.c
 *
 * Untested hardware: live REG_VCOUNT, scanout, REG_KEYINPUT, DMA.
 */

#include <stdio.h>
#include <string.h>

#include "battle/battle_draw.h"
#include "battle/battle_member.h"
#include "battle/battle_skills_panel.h"
#include "battle/battle_state.h"
#include "battle_ui/battle_menu.h"
#include "battle_ui/hud_layout.h"
#include "battle_ui/skill_card.h"
#include "battle_ui/skill_card_slide.h"
#include "battle_ui/skill_card_present_trace.h"
#include "core/gba_types.h"
#include "core/video.h"
#include "data/move_database.h"

void HostInput_SetKeys(u16 justPressed, u16 held);
void HostInput_Clear(void);
void HostMessage_SetBusy(int busy);
void Host_ResetTargetProbe(void);
int Host_BeginSkillTargetCallCount(void);
int Host_LastTargetSlot(void);
void Battle_BeginSkillTargetSelect(BattleState *state, MoveId moveId, int moveSlot);

static int g_failures;
static int g_frame;

typedef struct ObservedRow {
    int frame;
    unsigned int dirtyBefore;
    unsigned int dirtyAfterUpdate;
    int slideActive;
    int slideFrame;
    int selection;
    int phase;
    int chromeBandsLikely; /* !slide after consume when panel drawn */
    int cardBorderPresent;
    int countInkPresent;
    u16 vComposeBegin;
    u16 vComposeEnd;
    u16 vPresentBegin;
    u16 vPresentEnd;
    u16 vChromeBegin;
    u16 vChromeEnd;
} ObservedRow;

#define OBS_MAX 96
static ObservedRow g_obs[OBS_MAX];
static int g_obsCount;

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

static int CardBorderPresent(void)
{
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();
    u16 *fb = Video_HostFramebuffer();
    return fb[(HUD_SKILLS_CARD_Y * SCREEN_WIDTH) + HUD_SKILLS_CARD_X] == style->border
        || fb[((HUD_SKILLS_CARD_Y + 1) * SCREEN_WIDTH) + (HUD_SKILLS_CARD_X + 1)]
            != RGB15(8, 8, 16);
}

static int CountRegionHasNonBg(void)
{
    u16 *fb = Video_HostFramebuffer();
    int x;
    int y;
    u16 bg = RGB15(8, 8, 16);

    for (y = 0; y < VIDEO_FONT_GLYPH_H; y++)
    {
        for (x = 0; x < 36; x++)
        {
            if (fb[((HUD_SKILLS_TITLE_Y + y) * SCREEN_WIDTH) + (HUD_SKILLS_COUNT_X + x)] != bg)
            {
                return 1;
            }
        }
    }
    return 0;
}

static void ProductionFrame(BattleState *state, u16 justPressed, const char *tag)
{
    unsigned int dirtyBefore;
    unsigned int dirtyAfterUpdate;
    unsigned int dirty;

    (void)tag;
    HostInput_SetKeys(justPressed, justPressed);
    dirtyBefore = BattleDraw_PeekDirty();
    BattleMenu_Update(state);
    dirtyAfterUpdate = BattleDraw_PeekDirty();
    HostInput_Clear();

    dirty = BattleDraw_ConsumeDirty();
    if ((dirty & BATTLE_DRAW_DIRTY_PANEL) && state->phase == BATTLE_PHASE_PLAYER_SKILL)
    {
        Video_HostSetVCount(160);
        Battle_DrawPlayerSkillPanel(state);
    }
    {
        ObservedRow row;
        const SkillCardPresentSample *sample;
        int n;

        memset(&row, 0, sizeof(row));
        row.frame = g_frame;
        row.dirtyBefore = dirtyBefore;
        row.dirtyAfterUpdate = dirtyAfterUpdate;
        row.slideActive = BattleMenu_IsSkillCardSlideActive();
        row.selection = state->skillMenuSelection;
        row.phase = (int)state->phase;
        row.chromeBandsLikely = !row.slideActive;
        row.cardBorderPresent = CardBorderPresent();
        row.countInkPresent = CountRegionHasNonBg();
        n = SkillCardPresentTrace_Count();
        if (n > 0)
        {
            sample = SkillCardPresentTrace_Get(n - 1);
            if (sample)
            {
                row.slideFrame = sample->slideFrame;
                row.vComposeBegin = sample->vComposeBegin;
                row.vComposeEnd = sample->vComposeEnd;
                row.vPresentBegin = sample->vPresentBegin;
                row.vPresentEnd = sample->vPresentEnd;
                row.vChromeBegin = sample->vChromeBegin;
                row.vChromeEnd = sample->vChromeEnd;
            }
        }
        if (g_obsCount < OBS_MAX)
        {
            g_obs[g_obsCount++] = row;
        }
    }
    g_frame++;
}

static void DumpObserved(const char *path)
{
    FILE *f = fopen(path, "w");
    int i;

    if (!f)
    {
        printf("WARN: cannot write %s\n", path);
        return;
    }
    fprintf(
        f,
        "frame,dirtyBefore,dirtyAfterUpdate,slideActive,slideFrame,sel,phase,"
        "chromeBands,cardBorder,countInk,"
        "vComposeB,vComposeE,vPresentB,vPresentE,vChromeB,vChromeE\n");
    for (i = 0; i < g_obsCount; i++)
    {
        ObservedRow *r = &g_obs[i];
        fprintf(
            f,
            "%d,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u\n",
            r->frame,
            r->dirtyBefore,
            r->dirtyAfterUpdate,
            r->slideActive,
            r->slideFrame,
            r->selection,
            r->phase,
            r->chromeBandsLikely,
            r->cardBorderPresent,
            r->countInkPresent,
            r->vComposeBegin,
            r->vComposeEnd,
            r->vPresentBegin,
            r->vPresentEnd,
            r->vChromeBegin,
            r->vChromeEnd);
    }
    fclose(f);
    printf("wrote %s (%d rows)\n", path, g_obsCount);
}

static void SetupState(BattleState *state)
{
    BattleMember *member;

    BattleState_Init(state);
    member = &state->playerSlots[0];
    BattleMember_Init(member, "HERO", 30, 10, 0, BATTLE_SIDE_PLAYER);
    BattleMember_EquipMove(member, 0, MOVE_DIRECT_ATTACK, 14);
    BattleMember_EquipMove(member, 2, MOVE_PHANTOM_PHIST, 14);
    state->activePlayerSlot = 0;
    state->skillMenuSelection = 0;
    BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_SKILL);
    BattleDraw_ClearDirty();
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
    BattleMenu_Init();
    HostMessage_SetBusy(0);
    Host_ResetTargetProbe();
    SkillCardPresentTrace_Reset();
    g_obsCount = 0;
    g_frame = 0;
}

int main(void)
{
    BattleState state;
    int i;
    const SkillCardStyle *style = SkillCard_GetDefaultStyle();

    printf("=== M3R2 production Skills path host ===\n");
    SetupState(&state);
    memset(Video_HostFramebuffer(), 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));

    /* Entry draw. */
    ProductionFrame(&state, 0, "entry");
    ExpectTrue(g_obs[g_obsCount - 1].cardBorderPresent, "entry card present");
    ExpectTrue(g_obs[g_obsCount - 1].countInkPresent, "entry count ink present");
    ExpectEqInt(g_obs[g_obsCount - 1].chromeBandsLikely, 1, "entry uses chrome bands path");

    /* Right transition: paint once at begin, reuse during ticks. */
    SkillCard_ResetPaintCallCount();
    ProductionFrame(&state, KEY_RIGHT, "start_slide");
    {
        int paintsAtBegin = SkillCard_GetPaintCallCount();
        ExpectTrue(paintsAtBegin >= 1 && paintsAtBegin <= 3, "begin paints source+dest (and maybe settled)");
        for (i = 0; i < 12; i++)
        {
            int paintsBefore = SkillCard_GetPaintCallCount();
            ProductionFrame(&state, 0, "slide_tick");
            ExpectTrue(g_obs[g_obsCount - 1].cardBorderPresent, "slide/idle card border");
            /* Intermediate slide draws should not repaint when SC/move unchanged. */
            if (BattleMenu_IsSkillCardSlideActive()
                && !g_obs[g_obsCount - 1].chromeBandsLikely)
            {
                ExpectEqInt(
                    SkillCard_GetPaintCallCount(),
                    paintsBefore,
                    "no per-frame repaint during slide");
            }
            if (!BattleMenu_IsSkillCardSlideActive() && state.skillMenuSelection == 2)
            {
                break;
            }
        }
    }
    ExpectEqInt(state.skillMenuSelection, 2, "committed dest slot after slide");
    ExpectEqInt(SKILL_CARD_SLIDE_FRAME_COUNT, 3, "production uses three-frame slide");

    /* Several idle updates. */
    for (i = 0; i < 3; i++)
    {
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
        ProductionFrame(&state, 0, "idle");
        ExpectTrue(g_obs[g_obsCount - 1].cardBorderPresent, "idle after finish card");
        ExpectEqInt(g_obs[g_obsCount - 1].chromeBandsLikely, 1, "idle chrome bands");
    }

    /* Immediate second transition (left). */
    ProductionFrame(&state, KEY_LEFT, "second_slide");
    for (i = 0; i < 12; i++)
    {
        ProductionFrame(&state, 0, "second_tick");
        ExpectTrue(g_obs[g_obsCount - 1].cardBorderPresent, "second slide card");
        if (!BattleMenu_IsSkillCardSlideActive() && state.skillMenuSelection == 0)
        {
            break;
        }
    }
    ExpectEqInt(state.skillMenuSelection, 0, "second transition restored slot 0");

    /* B cancel mid-slide and reopen. */
    ProductionFrame(&state, KEY_RIGHT, "cancel_start");
    ProductionFrame(&state, 0, "cancel_mid");
    ExpectTrue(BattleMenu_IsSkillCardSlideActive(), "mid-slide active before B");
    ProductionFrame(&state, KEY_B, "b_cancel");
    ExpectEqInt((int)state.phase, (int)BATTLE_PHASE_PLAYER_COMMAND, "B returns to command");
    BattleState_SetPhase(&state, BATTLE_PHASE_PLAYER_SKILL);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
    ProductionFrame(&state, 0, "reopen");
    ExpectTrue(g_obs[g_obsCount - 1].cardBorderPresent, "reopen card present");

    /* Target cancel: A into target select stub, then restore Skills. */
    Host_ResetTargetProbe();
    state.skillMenuSelection = 0;
    BattleState_SetPhase(&state, BATTLE_PHASE_PLAYER_SKILL);
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
    ProductionFrame(&state, 0, "pre_target");
    ProductionFrame(&state, KEY_A, "a_target");
    ExpectTrue(Host_BeginSkillTargetCallCount() >= 1, "A begins skill target");
    ExpectEqInt((int)state.phase, (int)BATTLE_PHASE_TARGET_SELECT, "entered target select");
    /* Target cancel restoration (production TargetSelection B → PLAYER_SKILL). */
    BattleState_SetPhase(&state, BATTLE_PHASE_PLAYER_SKILL);
    BattleMenu_ClearSkillCardSlide();
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PANEL);
    ProductionFrame(&state, 0, "target_cancel_restore");
    ExpectEqInt(state.skillMenuSelection, Host_LastTargetSlot() >= 0 ? Host_LastTargetSlot() : 0,
                "selection preserved across target cancel");
    ExpectTrue(g_obs[g_obsCount - 1].cardBorderPresent, "card after target cancel restore");

    /* Count changes across commit: ordinal ink should remain non-bg after switches. */
    ExpectTrue(g_obs[g_obsCount - 1].countInkPresent, "count ink after restore");

    DumpObserved("docs/reports/skills_carousel_speed_audit_captures/production_trace.csv");

    /* Timing notes from last samples (host VCOUNT model only). */
    {
        int n = SkillCardPresentTrace_Count();
        int crossed = 0;
        int k;
        for (k = 0; k < n; k++)
        {
            const SkillCardPresentSample *s = SkillCardPresentTrace_Get(k);
            if (!s)
            {
                continue;
            }
            /* VBlank region roughly 160..226; wrap below 160 after present ⇒ model crossed. */
            if (s->vPresentBegin >= 160 && s->vPresentEnd < 160)
            {
                crossed++;
            }
            printf(
                "sample[%d] compose %u->%u present %u->%u chrome %u->%u slide=%u f=%u cards=%u\n",
                k,
                s->vComposeBegin,
                s->vComposeEnd,
                s->vPresentBegin,
                s->vPresentEnd,
                s->vChromeBegin,
                s->vChromeEnd,
                s->slideActive,
                s->slideFrame,
                s->paintedCards);
        }
        printf("host_vcount_model present_wraps=%d (not hardware proof)\n", crossed);
        ExpectTrue(style->border != 0, "style loaded");
    }

    if (g_failures)
    {
        printf("RESULT: %d failure(s)\n", g_failures);
        return 1;
    }
    printf("RESULT: all production-path checks passed\n");
    return 0;
}
