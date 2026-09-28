#ifndef SKILL_CARD_SLIDE_H
#define SKILL_CARD_SLIDE_H

/* Whole-card Skills transition. Progress is independent of card width. */

/* Presented frames including endpoints (frame 0 .. COUNT-1). */
#define SKILL_CARD_SLIDE_FRAME_COUNT 3
#define SKILL_CARD_SLIDE_PROGRESS_MAX 256

typedef struct SkillCardSlideState {
    int active;
    int frame;
    int finished;
    int direction;
    int sourceSlot;
    int destSlot;
    int actorSlot;
} SkillCardSlideState;

void SkillCardSlide_Clear(SkillCardSlideState *slide);
int SkillCardSlide_Progress(int frame);
int SkillCardSlide_OffsetPx(int frame, int travelPx);

int SkillCardSlide_Begin(
    SkillCardSlideState *slide,
    int actorSlot,
    int sourceSlot,
    int destSlot,
    int direction);

/*
 * Advances one presented frame.
 * Returns 1 when selection should commit (arriving at final frame).
 * Returns -1 when the settled frame was already presented (clear + accept input).
 * Returns 0 while still animating.
 */
int SkillCardSlide_Advance(SkillCardSlideState *slide);

int SkillCardSlide_CountSlot(const SkillCardSlideState *slide, int settledSlot);

#endif
