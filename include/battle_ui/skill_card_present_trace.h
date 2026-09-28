#ifndef SKILL_CARD_PRESENT_TRACE_H
#define SKILL_CARD_PRESENT_TRACE_H

#include "core/gba_types.h"

/*
 * Compile-gated present timing samples (REG_VCOUNT).
 * Enable with -DSKILL_CARD_PRESENT_TRACE for diagnostic builds only.
 * Default ROM builds leave this off.
 */

#ifdef SKILL_CARD_PRESENT_TRACE

#define SKILL_CARD_PRESENT_TRACE_CAP 48

typedef struct SkillCardPresentSample {
    u16 vComposeBegin;
    u16 vComposeEnd;
    u16 vPresentBegin;
    u16 vPresentEnd;
    u16 vChromeBegin;
    u16 vChromeEnd;
    u8 slideActive;
    u8 slideFrame;
    u8 paintedCards;
} SkillCardPresentSample;

void SkillCardPresentTrace_Reset(void);
void SkillCardPresentTrace_BeginCompose(int slideActive, int slideFrame, int paintedCards);
void SkillCardPresentTrace_EndCompose(void);
void SkillCardPresentTrace_BeginPresent(void);
void SkillCardPresentTrace_EndPresent(void);
void SkillCardPresentTrace_BeginChrome(void);
void SkillCardPresentTrace_EndChrome(void);
int SkillCardPresentTrace_Count(void);
const SkillCardPresentSample *SkillCardPresentTrace_Get(int index);

#else

#define SkillCardPresentTrace_Reset() ((void)0)
#define SkillCardPresentTrace_BeginCompose(a, b, c) ((void)0)
#define SkillCardPresentTrace_EndCompose() ((void)0)
#define SkillCardPresentTrace_BeginPresent() ((void)0)
#define SkillCardPresentTrace_EndPresent() ((void)0)
#define SkillCardPresentTrace_BeginChrome() ((void)0)
#define SkillCardPresentTrace_EndChrome() ((void)0)

#endif

#endif
