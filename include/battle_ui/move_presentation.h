#ifndef MOVE_PRESENTATION_H
#define MOVE_PRESENTATION_H

#include "battle/battle_move.h"

/*
 * Shared move presentation helpers (Skills Carousel M2).
 * Converts MoveData + per-slot SC into bounded display strings.
 * Does not decide combat eligibility or compute damage.
 */

#define MOVE_PRESENTATION_NAME_MAX 40
#define MOVE_PRESENTATION_ELEMENT_MAX 16
#define MOVE_PRESENTATION_CATEGORY_MAX 8
#define MOVE_PRESENTATION_POWER_MAX 12
#define MOVE_PRESENTATION_SC_MAX 16
#define MOVE_PRESENTATION_TARGET_MAX 20
#define MOVE_PRESENTATION_EFFECT_MAX 48

/* Card content width budget (pixels / VIDEO_FONT_ADVANCE). */
#define MOVE_PRESENTATION_CARD_MAX_CHARS 36

typedef struct MovePresentationLiveSc {
    int currentSc;
    int maxSc;
} MovePresentationLiveSc;

typedef struct MovePresentationFields {
    char name[MOVE_PRESENTATION_NAME_MAX];
    char element[MOVE_PRESENTATION_ELEMENT_MAX];
    char category[MOVE_PRESENTATION_CATEGORY_MAX];
    char power[MOVE_PRESENTATION_POWER_MAX];
    char sc[MOVE_PRESENTATION_SC_MAX];
    char target[MOVE_PRESENTATION_TARGET_MAX];
    char effect[MOVE_PRESENTATION_EFFECT_MAX];
} MovePresentationFields;

/* Fills fields for an equipped/live slot. maxSc comes from MoveData.maxSc. */
void MovePresentation_FillLive(
    MoveId moveId,
    const MovePresentationLiveSc *liveSc,
    MovePresentationFields *out);

const char *MovePresentation_GetTargetLabel(TargetPattern pattern);
const char *MovePresentation_GetCategoryLabel(MoveCategory category);
int MovePresentation_IsVariablePower(const MoveData *move);

/* Occupied equipped-slot browse helpers (skip MOVE_NONE). */
int MovePresentation_CountOccupiedSlots(const MoveId *slots, int slotCount);
int MovePresentation_FirstOccupiedSlot(const MoveId *slots, int slotCount);
/* 1-based ordinal among occupied slots; 0 if fewer than n occupied. */
int MovePresentation_NthOccupiedSlot(const MoveId *slots, int slotCount, int n);
int MovePresentation_GetOccupiedOrdinal(
    const MoveId *slots,
    int slotCount,
    int slotIndex);
/* Wrap in slot order (legacy / non-battle consumers). */
int MovePresentation_StepOccupiedSlot(
    const MoveId *slots,
    int slotCount,
    int currentSlot,
    int direction);
/* No wrap: outward step at an end returns currentSlot unchanged. */
int MovePresentation_StepOccupiedSlotNoWrap(
    const MoveId *slots,
    int slotCount,
    int currentSlot,
    int direction);

#endif
