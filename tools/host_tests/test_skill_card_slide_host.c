/*
 * Skills Carousel M3 host checks: slide commit timing + cancel + offset table.
 *
 *   gcc -std=c11 -Wall -Wextra -Iinclude \
 *       -o build/host_skill_card_slide_test.exe \
 *       tools/host_tests/test_skill_card_slide_host.c \
 *       src/battle_ui/skill_card_slide.c \
 *       src/battle_ui/move_presentation.c \
 *       src/battle/battle_move.c \
 *       src/data/move_database.c \
 *       src/data/element_database.c \
 *       src/data/race_database.c
 */

#include <stdio.h>
#include <string.h>

#include "battle_ui/move_presentation.h"
#include "battle_ui/skill_card_slide.h"

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

static void TestOffsetTable(void)
{
    ExpectEqInt(SkillCardSlide_OffsetPx(0, 220), 0, "frame0 offset 0");
    ExpectEqInt(SkillCardSlide_OffsetPx(SKILL_CARD_SLIDE_FRAME_COUNT - 1, 220), 220, "final frame exact travel");
    ExpectEqInt(SkillCardSlide_OffsetPx(1, 220), (180 * 220) / 256, "mid ease progress");
    ExpectEqInt(SKILL_CARD_SLIDE_FRAME_COUNT, 3, "three presented frames");
}

static void TestPendingUntilCommit(void)
{
    SkillCardSlideState slide;
    int selection = 1;
    int i;
    int result;
    MoveId sparse[4] = { MOVE_NONE, MOVE_LONG_SHOT, MOVE_NONE, MOVE_VORTEX_ESCAPE };

    SkillCardSlide_Clear(&slide);
    ExpectEqInt(
        MovePresentation_StepOccupiedSlot(sparse, 4, 1, 1),
        3,
        "sparse wrap destination");
    ExpectTrue(
        SkillCardSlide_Begin(&slide, 0, 1, 3, 1),
        "begin slide");
    ExpectEqInt(slide.frame, 0, "start at frame 0");
    ExpectEqInt(selection, 1, "selection pending at start");
    ExpectEqInt(SkillCardSlide_CountSlot(&slide, selection), 1, "count uses source");

    for (i = 0; i < SKILL_CARD_SLIDE_FRAME_COUNT - 2; i++)
    {
        result = SkillCardSlide_Advance(&slide);
        ExpectEqInt(result, 0, "no commit before final frame");
        ExpectEqInt(selection, 1, "selection still pending mid-slide");
    }
    result = SkillCardSlide_Advance(&slide);
    ExpectEqInt(result, 1, "commit signal on final frame");
    ExpectEqInt(slide.frame, SKILL_CARD_SLIDE_FRAME_COUNT - 1, "frame is final");
    selection = slide.destSlot;
    ExpectEqInt(selection, 3, "selection commits to dest");
    ExpectEqInt(SkillCardSlide_CountSlot(&slide, selection), 3, "count uses dest at settle");

    result = SkillCardSlide_Advance(&slide);
    ExpectEqInt(result, -1, "end after settled frame presented");
    ExpectEqInt(slide.active, 0, "slide cleared");
}

static void TestCancelMidSlide(void)
{
    SkillCardSlideState slide;
    int selection = 0;

    SkillCardSlide_Begin(&slide, 2, 0, 2, 1);
    SkillCardSlide_Advance(&slide);
    ExpectEqInt(slide.frame, 1, "intermediate frame");
    ExpectEqInt(slide.active, 1, "still active mid-slide");
    ExpectEqInt(selection, 0, "selection unchanged before cancel");
    SkillCardSlide_Clear(&slide);
    ExpectEqInt(slide.active, 0, "cleared on B cancel");
    ExpectEqInt(selection, 0, "cancel does not commit dest");
}

static void TestDuplicateScPresentations(void)
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
    ExpectTrue(strcmp(a.name, b.name) == 0, "dup id same name");
    ExpectTrue(strcmp(a.sc, "SC 8/16") == 0, "slot A SC");
    ExpectTrue(strcmp(b.sc, "SC 1/16") == 0, "slot B SC distinct");
}

static void TestReachableSummaries(void)
{
    static const MoveId reachable[] = {
        MOVE_DIRECT_ATTACK,
        MOVE_PHANTOM_PHIST,
        MOVE_HOLD_BACK,
        MOVE_RAM,
        MOVE_VORTEX_ESCAPE,
        MOVE_FIERY_STRIKE,
        MOVE_FAKIE,
        MOVE_ALLY_SWAP,
        MOVE_YOUNG_BLOOD,
        MOVE_TEMPERATURE_DIFFERENCE,
        MOVE_LONG_SHOT,
        MOVE_HEADBUTT,
        MOVE_ELECTRO_THERAPY,
        MOVE_CHARGE,
        MOVE_STATIC_LOCK,
        MOVE_INFLUENCE,
        MOVE_FLAME_CHARGE
    };
    MovePresentationFields fields;
    MovePresentationLiveSc live;
    unsigned int i;

    live.currentSc = 4;
    live.maxSc = 8;
    for (i = 0; i < sizeof(reachable) / sizeof(reachable[0]); i++)
    {
        MovePresentation_FillLive(reachable[i], &live, &fields);
        ExpectTrue(
            (int)strlen(fields.name) <= MOVE_PRESENTATION_CARD_MAX_CHARS,
            "reachable name fits");
        ExpectTrue(
            (int)strlen(fields.effect) <= MOVE_PRESENTATION_CARD_MAX_CHARS,
            "reachable effect fits");
        ExpectTrue(
            (int)strlen(fields.target) < MOVE_PRESENTATION_TARGET_MAX,
            "reachable target fits buffer");
        ExpectTrue(
            (int)strlen(fields.sc) < MOVE_PRESENTATION_SC_MAX,
            "reachable sc fits buffer");
    }
    MovePresentation_FillLive(MOVE_LONG_SHOT, &live, &fields);
    ExpectTrue(strcmp(fields.power, "PWR VAR") == 0, "long shot still VAR");
    MovePresentation_FillLive(MOVE_FIERY_STRIKE, &live, &fields);
    ExpectTrue(strstr(fields.effect, "closest") != 0, "fiery strike keeps closest");
    MovePresentation_FillLive(MOVE_FAKIE, &live, &fields);
    ExpectTrue(strstr(fields.effect, "crit") != 0, "fakie keeps crit note");
}

int main(void)
{
    g_failures = 0;
    TestOffsetTable();
    TestPendingUntilCommit();
    TestCancelMidSlide();
    TestDuplicateScPresentations();
    TestReachableSummaries();
    if (g_failures)
    {
        printf("\n%d failure(s)\n", g_failures);
        return 1;
    }
    printf("\nAll skill card slide host checks passed.\n");
    return 0;
}
