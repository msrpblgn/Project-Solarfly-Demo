#ifndef DIALOGUE_DEMO_PLAN_H
#define DIALOGUE_DEMO_PLAN_H

/*
 * Pure input planner for Dialogue Demo.
 * Decisions use printer state captured at update entry.
 */

typedef enum DialogueDemoPlanAction {
    DIALOGUE_DEMO_PLAN_NONE = 0,
    DIALOGUE_DEMO_PLAN_EXIT,
    DIALOGUE_DEMO_PLAN_SPEED_PREV,
    DIALOGUE_DEMO_PLAN_SPEED_NEXT,
    DIALOGUE_DEMO_PLAN_TICK_PRINTER,
    DIALOGUE_DEMO_PLAN_ADVANCE_SAMPLE
} DialogueDemoPlanAction;

typedef struct DialogueDemoPlanInput {
    int entryLatch;
    int needsUpdate;
    int isPrinting;
    int isComplete;
    int aPressed;
    int bHeld;
    int bPressed;
    int startPressed;
    int lPressed;
    int rPressed;
} DialogueDemoPlanInput;

typedef struct DialogueDemoPlanResult {
    DialogueDemoPlanAction action;
    int accelerate;
    int passAPressed;
    int alsoCycleSpeedPrev;
    int alsoCycleSpeedNext;
} DialogueDemoPlanResult;

static inline DialogueDemoPlanResult DialogueDemo_PlanUpdate(const DialogueDemoPlanInput *in)
{
    DialogueDemoPlanResult out;

    out.action = DIALOGUE_DEMO_PLAN_NONE;
    out.accelerate = 0;
    out.passAPressed = 0;
    out.alsoCycleSpeedPrev = 0;
    out.alsoCycleSpeedNext = 0;

    if (!in || in->entryLatch)
    {
        return out;
    }

    if (in->startPressed)
    {
        out.action = DIALOGUE_DEMO_PLAN_EXIT;
        return out;
    }

    if (in->lPressed)
    {
        out.alsoCycleSpeedPrev = 1;
    }
    else if (in->rPressed)
    {
        out.alsoCycleSpeedNext = 1;
    }

    if (in->needsUpdate)
    {
        out.action = DIALOGUE_DEMO_PLAN_TICK_PRINTER;
        /* B accelerates printables only; never shortens PAUSE or resolves WAIT/PAGE. */
        out.accelerate = (in->isPrinting && in->bHeld) ? 1 : 0;
        /*
         * Pass A only when not printing/pausing at entry. An A that arrives while
         * printing cannot satisfy a WAIT/PAGE reached later in the same update
         * (printer resolves WAIT/PAGE only at update start).
         */
        out.passAPressed = (!in->isPrinting && in->aPressed) ? 1 : 0;
        return out;
    }

    if (in->isComplete && (in->aPressed || in->bPressed))
    {
        out.action = DIALOGUE_DEMO_PLAN_ADVANCE_SAMPLE;
        return out;
    }

    return out;
}

#endif
