#ifndef DIALOGUE_OVERLAY_H
#define DIALOGUE_OVERLAY_H

#include "dialogue/dialogue_host.h"

/*
 * Reusable Mode 0 dialogue overlay (BG0 panel + portraits + WIN0).
 * DEMO mode also installs the green BG3 inspect backdrop.
 * FIELD mode keeps the current BG3 map tiles and only adds the overlay.
 */
void DialogueOverlay_Init(DialogueOverlayMode mode);
void DialogueOverlay_Shutdown(void);
void DialogueOverlay_Commit(void);
int DialogueOverlay_IsActive(void);

#endif
