#ifndef DIALOGUE_DEMO_H
#define DIALOGUE_DEMO_H

#include "battle_ui/battle_message_queue.h"

/* Dedicated Mode 0 Dialogue Demo scene. */
void DialogueDemo_Enter(TextSpeed initialSpeed);
/* Returns 1 when the demo requests return to the startup menu. */
int DialogueDemo_Update(void);
void DialogueDemo_Exit(void);

#endif
