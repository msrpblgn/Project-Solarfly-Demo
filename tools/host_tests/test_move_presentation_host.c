/*
 * Skills Carousel M2 host checks: move presentation fields + occupied-slot wrap.
 *
 * Build (from repo root, MSYS or similar):
 *   mkdir -p build
 *   gcc -std=c11 -Wall -Wextra -Iinclude -Itools/host_tests \
 *       -o build/host_move_presentation_test.exe \
 *       tools/host_tests/test_move_presentation_host.c \
 *       src/battle_ui/move_presentation.c \
 *       src/battle/battle_move.c \
 *       src/data/move_database.c \
 *       src/data/element_database.c \
 *       src/data/race_database.c
 */

#include <stdio.h>
#include <string.h>

#include "battle_ui/move_presentation.h"

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

static void ExpectStr(const char *got, const char *want, const char *label)
{
    if (!got || !want || strcmp(got, want) != 0)
    {
        printf("FAIL: %s (got \"%s\" want \"%s\")\n",
            label,
            got ? got : "(null)",
            want ? want : "(null)");
        g_failures++;
    }
    else
    {
        printf("OK:   %s\n", label);
    }
}

static void ExpectMaxLen(const char *text, int maxChars, const char *label)
{
    int len = text ? (int)strlen(text) : 0;
    if (len > maxChars)
    {
        printf("FAIL: %s len %d > %d (\"%s\")\n", label, len, maxChars, text);
        g_failures++;
    }
    else
    {
        printf("OK:   %s (len %d)\n", label, len);
    }
}

static void TestSlotBrowse(void)
{
    MoveId sparse[4] = { MOVE_NONE, MOVE_LONG_SHOT, MOVE_NONE, MOVE_VORTEX_ESCAPE };
    MoveId dup[4] = { MOVE_HOLD_BACK, MOVE_NONE, MOVE_HOLD_BACK, MOVE_NONE };
    MoveId one[4] = { MOVE_NONE, MOVE_NONE, MOVE_DIRECT_ATTACK, MOVE_NONE };
    MoveId none[4] = { MOVE_NONE, MOVE_NONE, MOVE_NONE, MOVE_NONE };
    int slot;

    ExpectEqInt(MovePresentation_CountOccupiedSlots(sparse, 4), 2, "sparse occupied count");
    ExpectEqInt(MovePresentation_FirstOccupiedSlot(sparse, 4), 1, "sparse first slot");
    ExpectEqInt(MovePresentation_GetOccupiedOrdinal(sparse, 4, 1), 1, "sparse ordinal slot1");
    ExpectEqInt(MovePresentation_GetOccupiedOrdinal(sparse, 4, 3), 2, "sparse ordinal slot3");

    slot = 1;
    slot = MovePresentation_StepOccupiedSlot(sparse, 4, slot, 1);
    ExpectEqInt(slot, 3, "sparse right wrap to slot3 (legacy wrap helper)");
    slot = MovePresentation_StepOccupiedSlot(sparse, 4, slot, 1);
    ExpectEqInt(slot, 1, "sparse right wrap to slot1 (legacy wrap helper)");
    slot = MovePresentation_StepOccupiedSlot(sparse, 4, slot, -1);
    ExpectEqInt(slot, 3, "sparse left wrap to slot3 (legacy wrap helper)");

    /* Battle Skills carousel uses no-wrap stepping. */
    slot = 1;
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 4, slot, 1);
    ExpectEqInt(slot, 3, "sparse no-wrap right to slot3");
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 4, slot, 1);
    ExpectEqInt(slot, 3, "sparse no-wrap right at end stays");
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 4, slot, -1);
    ExpectEqInt(slot, 1, "sparse no-wrap left to slot1");
    slot = MovePresentation_StepOccupiedSlotNoWrap(sparse, 4, slot, -1);
    ExpectEqInt(slot, 1, "sparse no-wrap left at start stays");

    ExpectEqInt(MovePresentation_CountOccupiedSlots(one, 4), 1, "one occupied count");
    slot = MovePresentation_StepOccupiedSlot(one, 4, 2, 1);
    ExpectEqInt(slot, 2, "one occupied right stays");
    slot = MovePresentation_StepOccupiedSlot(one, 4, 2, -1);
    ExpectEqInt(slot, 2, "one occupied left stays");

    ExpectEqInt(MovePresentation_CountOccupiedSlots(none, 4), 0, "zero occupied count");
    slot = MovePresentation_StepOccupiedSlot(none, 4, 0, 1);
    ExpectEqInt(slot, 0, "zero occupied step safe");

    ExpectEqInt(MovePresentation_FirstOccupiedSlot(dup, 4), 0, "dup first");
    slot = MovePresentation_StepOccupiedSlot(dup, 4, 0, 1);
    ExpectEqInt(slot, 2, "dup right to second same-id slot");
}

static void TestPresentationFields(void)
{
    MovePresentationFields fields;
    MovePresentationLiveSc liveA;
    MovePresentationLiveSc liveB;
    const MoveData *longShot;
    const MoveData *vortex;
    const MoveData *tempDiff;
    const MoveData *holdBack;

    liveA.currentSc = 3;
    liveA.maxSc = 12;
    liveB.currentSc = 0;
    liveB.maxSc = 12;

    MovePresentation_FillLive(MOVE_LONG_SHOT, &liveA, &fields);
    ExpectStr(fields.name, "Long Shot", "long shot name");
    ExpectStr(fields.power, "PWR VAR", "long shot variable power");
    ExpectStr(fields.sc, "SC 3/12", "long shot live SC slot A");
    ExpectStr(fields.target, "1 ENEMY", "long shot target");
    ExpectMaxLen(fields.effect, MOVE_PRESENTATION_CARD_MAX_CHARS, "long shot effect width");

    MovePresentation_FillLive(MOVE_LONG_SHOT, &liveB, &fields);
    ExpectStr(fields.sc, "SC 0/12", "duplicate id different SC");

    MovePresentation_FillLive(MOVE_VORTEX_ESCAPE, &liveA, &fields);
    ExpectStr(fields.name, "Vortex Escape", "vortex name");
    ExpectStr(fields.power, "--", "support power not zero attack");
    ExpectStr(fields.category, "SUPP", "vortex category");
    ExpectStr(fields.target, "EMPTY ALLY SLOT", "vortex empty-ally target label");
    ExpectMaxLen(fields.effect, MOVE_PRESENTATION_CARD_MAX_CHARS, "vortex effect width");
    ExpectTrue(strstr(fields.effect, "LP") != 0, "vortex effect keeps LP cost");

    MovePresentation_FillLive(MOVE_TEMPERATURE_DIFFERENCE, &liveA, &fields);
    ExpectStr(fields.name, "Temperature Difference", "temp diff name");
    ExpectMaxLen(fields.name, MOVE_PRESENTATION_CARD_MAX_CHARS, "temp diff name width");
    ExpectStr(fields.power, "--", "temp diff support power");
    ExpectMaxLen(fields.effect, MOVE_PRESENTATION_CARD_MAX_CHARS, "temp diff effect width");

    MovePresentation_FillLive(MOVE_HOLD_BACK, &liveA, &fields);
    ExpectMaxLen(fields.effect, MOVE_PRESENTATION_CARD_MAX_CHARS, "hold back effect width");
    ExpectTrue(strstr(fields.effect, "25%") != 0, "hold back keeps reduction");

    MovePresentation_FillLive(MOVE_NONE, 0, &fields);
    ExpectStr(fields.name, "EMPTY", "empty slot name");

    longShot = MoveData_Get(MOVE_LONG_SHOT);
    vortex = MoveData_Get(MOVE_VORTEX_ESCAPE);
    tempDiff = MoveData_Get(MOVE_TEMPERATURE_DIFFERENCE);
    holdBack = MoveData_Get(MOVE_HOLD_BACK);
    ExpectTrue(longShot != 0 && vortex != 0 && tempDiff != 0 && holdBack != 0, "move db lookups");
    ExpectTrue(MovePresentation_IsVariablePower(longShot), "long shot variable flag");
    ExpectTrue(!MovePresentation_IsVariablePower(vortex), "vortex not variable power");

    ExpectEqInt(
        (int)strlen(MovePresentation_GetTargetLabel(MOVE_TARGET_ALLY_SLOT)),
        15,
        "longest common target label length");
    ExpectMaxLen(
        MovePresentation_GetTargetLabel(MOVE_TARGET_OUTER_ENEMIES),
        MOVE_PRESENTATION_TARGET_MAX - 1,
        "outer enemies target buffer");
}

int main(void)
{
    g_failures = 0;
    TestSlotBrowse();
    TestPresentationFields();
    if (g_failures)
    {
        printf("\n%d failure(s)\n", g_failures);
        return 1;
    }
    printf("\nAll move presentation host checks passed.\n");
    return 0;
}
