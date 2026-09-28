#ifndef DIALOGUE_PORTRAIT_CONTROLLER_H
#define DIALOGUE_PORTRAIT_CONTROLLER_H

#include "core/gba_types.h"

typedef enum DialoguePortraitActor {
    DIALOGUE_PORTRAIT_ACTOR_NONE = 0,
    DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST,
    DIALOGUE_PORTRAIT_ACTOR_AKSIL
} DialoguePortraitActor;

typedef enum DialoguePortraitSide {
    DIALOGUE_PORTRAIT_SIDE_LEFT = 0,
    DIALOGUE_PORTRAIT_SIDE_RIGHT = 1
} DialoguePortraitSide;

typedef enum DialoguePortraitSlotState {
    DIALOGUE_PORTRAIT_HIDDEN = 0,
    DIALOGUE_PORTRAIT_ENTERING,
    DIALOGUE_PORTRAIT_INACTIVE,
    DIALOGUE_PORTRAIT_ACTIVE,
    DIALOGUE_PORTRAIT_EXITING
} DialoguePortraitSlotState;

typedef enum DialoguePortraitSceneState {
    DIALOGUE_PORTRAIT_SCENE_IDLE = 0,
    DIALOGUE_PORTRAIT_SCENE_ENTERING,
    DIALOGUE_PORTRAIT_SCENE_FOCUS,
    DIALOGUE_PORTRAIT_SCENE_EXITING,
    DIALOGUE_PORTRAIT_SCENE_TECH_TEST
} DialoguePortraitSceneState;

/* Focus frames include endpoints (5 unique positions). */
#define DIALOGUE_PORTRAIT_FOCUS_FRAMES 5
/* Enter/exit linear tracks include endpoints (10 frames). */
#define DIALOGUE_PORTRAIT_SLIDE_FRAMES 10

void DialoguePortrait_Init(void);
void DialoguePortrait_Shutdown(void);

/* Load default Protagonist(left)/Aksil(right) graphics and begin entrance. */
void DialoguePortrait_BeginEntrance(int leftActive);

/* Begin 5-frame focus exchange toward leftActive speaker. No-op if already settled. */
void DialoguePortrait_BeginFocus(int leftActive);

/* Exit both portraits offscreen (linear 10 frames). */
void DialoguePortrait_BeginExitBoth(void);

/* SELECT / scene technical test: left replace + dual exit/enter replay. */
void DialoguePortrait_BeginTechTest(void);

void DialoguePortrait_Cancel(void);
void DialoguePortrait_Update(void);
void DialoguePortrait_Commit(void);

int DialoguePortrait_IsBusy(void);
int DialoguePortrait_IsSettled(void);
DialoguePortraitSceneState DialoguePortrait_GetSceneState(void);
DialoguePortraitActor DialoguePortrait_GetActor(DialoguePortraitSide side);
int DialoguePortrait_IsLeftActive(void);
int DialoguePortrait_TakeTechTestFinished(void);

/* Presentation top-left helpers (signed). */
void DialoguePortrait_GetPresentationPos(DialoguePortraitSide side, int *x, int *y);

#endif
