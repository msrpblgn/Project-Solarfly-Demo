#ifndef OVERWORLD_H
#define OVERWORLD_H

#define OVERWORLD_UPDATE_NONE 0
#define OVERWORLD_UPDATE_EXIT_MENU 1
#define OVERWORLD_UPDATE_ENTER_BATTLE 2
#define OVERWORLD_UPDATE_START_DIALOGUE 3

/* Red battle walk-on AABB (world pixels, half-open). Metatile (10,8). */
#define OVERWORLD_TRIGGER_LEFT 160
#define OVERWORLD_TRIGGER_RIGHT 176
#define OVERWORLD_TRIGGER_TOP 128
#define OVERWORLD_TRIGGER_BOTTOM 144

/*
 * Blue dialogue marker AABB (world pixels, half-open). Metatile (9,6).
 * Reachable east of spawn (~120,96). Press A while foot is inside.
 * Distinct from the red battle trigger; does not auto-fire on tile entry.
 */
#define OVERWORLD_DIALOGUE_TRIGGER_LEFT 144
#define OVERWORLD_DIALOGUE_TRIGGER_RIGHT 160
#define OVERWORLD_DIALOGUE_TRIGGER_TOP 96
#define OVERWORLD_DIALOGUE_TRIGGER_BOTTOM 112

void Overworld_Init(void);
void Overworld_ResumeAfterBattle(void);
int Overworld_Update(void);
void Overworld_Shutdown(void);

/* Dialogue host lock: suspend movement/triggers; restore presentation on unlock. */
void Overworld_LockForDialogue(void);
void Overworld_UnlockAfterDialogue(void);
int Overworld_IsDialogueLocked(void);

#endif
