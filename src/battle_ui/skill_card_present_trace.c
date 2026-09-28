#include "battle_ui/skill_card_present_trace.h"

#ifdef SKILL_CARD_PRESENT_TRACE

#include "core/gba_types.h"

#ifdef VIDEO_HOST_FRAMEBUFFER
extern volatile u16 g_hostRegVCount;
#undef REG_VCOUNT
#define REG_VCOUNT (g_hostRegVCount)
#endif

static SkillCardPresentSample s_samples[SKILL_CARD_PRESENT_TRACE_CAP];
static int s_count;
static SkillCardPresentSample s_current;

void SkillCardPresentTrace_Reset(void)
{
    s_count = 0;
}

void SkillCardPresentTrace_BeginCompose(int slideActive, int slideFrame, int paintedCards)
{
    s_current.vComposeBegin = (u16)REG_VCOUNT;
    s_current.vComposeEnd = 0;
    s_current.vPresentBegin = 0;
    s_current.vPresentEnd = 0;
    s_current.vChromeBegin = 0;
    s_current.vChromeEnd = 0;
    s_current.slideActive = (u8)(slideActive ? 1 : 0);
    s_current.slideFrame = (u8)slideFrame;
    s_current.paintedCards = (u8)paintedCards;
}

void SkillCardPresentTrace_EndCompose(void)
{
    s_current.vComposeEnd = (u16)REG_VCOUNT;
}

void SkillCardPresentTrace_BeginPresent(void)
{
    s_current.vPresentBegin = (u16)REG_VCOUNT;
}

void SkillCardPresentTrace_EndPresent(void)
{
    s_current.vPresentEnd = (u16)REG_VCOUNT;
}

void SkillCardPresentTrace_BeginChrome(void)
{
    s_current.vChromeBegin = (u16)REG_VCOUNT;
}

void SkillCardPresentTrace_EndChrome(void)
{
    s_current.vChromeEnd = (u16)REG_VCOUNT;
    if (s_count < SKILL_CARD_PRESENT_TRACE_CAP)
    {
        s_samples[s_count++] = s_current;
    }
}

int SkillCardPresentTrace_Count(void)
{
    return s_count;
}

const SkillCardPresentSample *SkillCardPresentTrace_Get(int index)
{
    if (index < 0 || index >= s_count)
    {
        return 0;
    }
    return &s_samples[index];
}

#else

void SkillCardPresentTrace_Anchor(void)
{
}

#endif
