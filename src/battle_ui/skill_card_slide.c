#include "battle_ui/skill_card_slide.h"

/*
 * Decisive ease-out over 3 inclusive endpoints (0..2). Values are /256 of travel.
 * Shorter than the prior 4-frame slide; one mid sample keeps the carousel smooth.
 */
static const int s_skillCardSlideProgress[SKILL_CARD_SLIDE_FRAME_COUNT] = {
    0, 180, 256
};

void SkillCardSlide_Clear(SkillCardSlideState *slide)
{
    if (!slide)
    {
        return;
    }
    slide->active = 0;
    slide->frame = 0;
    slide->finished = 0;
    slide->direction = 0;
    slide->sourceSlot = 0;
    slide->destSlot = 0;
    slide->actorSlot = -1;
}

int SkillCardSlide_Progress(int frame)
{
    if (frame <= 0)
    {
        return 0;
    }
    if (frame >= SKILL_CARD_SLIDE_FRAME_COUNT - 1)
    {
        return SKILL_CARD_SLIDE_PROGRESS_MAX;
    }
    return s_skillCardSlideProgress[frame];
}

int SkillCardSlide_OffsetPx(int frame, int travelPx)
{
    int progress;

    if (travelPx <= 0)
    {
        return 0;
    }
    progress = SkillCardSlide_Progress(frame);
    if (progress >= SKILL_CARD_SLIDE_PROGRESS_MAX)
    {
        return travelPx;
    }
    return (progress * travelPx) / SKILL_CARD_SLIDE_PROGRESS_MAX;
}

int SkillCardSlide_Begin(
    SkillCardSlideState *slide,
    int actorSlot,
    int sourceSlot,
    int destSlot,
    int direction)
{
    if (!slide || direction == 0 || sourceSlot == destSlot)
    {
        return 0;
    }
    slide->active = 1;
    slide->frame = 0;
    slide->finished = 0;
    slide->direction = direction;
    slide->sourceSlot = sourceSlot;
    slide->destSlot = destSlot;
    slide->actorSlot = actorSlot;
    return 1;
}

int SkillCardSlide_Advance(SkillCardSlideState *slide)
{
    if (!slide || !slide->active)
    {
        return 0;
    }
    if (slide->finished)
    {
        SkillCardSlide_Clear(slide);
        return -1;
    }
    if (slide->frame < SKILL_CARD_SLIDE_FRAME_COUNT - 1)
    {
        slide->frame++;
        if (slide->frame == SKILL_CARD_SLIDE_FRAME_COUNT - 1)
        {
            slide->finished = 1;
            return 1;
        }
    }
    return 0;
}

int SkillCardSlide_CountSlot(const SkillCardSlideState *slide, int settledSlot)
{
    if (!slide || !slide->active)
    {
        return settledSlot;
    }
    if (slide->frame < SKILL_CARD_SLIDE_FRAME_COUNT - 1)
    {
        return slide->sourceSlot;
    }
    return slide->destSlot;
}
