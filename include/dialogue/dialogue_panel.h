#ifndef DIALOGUE_PANEL_H
#define DIALOGUE_PANEL_H

#include "core/gba_types.h"

typedef enum DialogueNameplateSide {
    DIALOGUE_NAMEPLATE_NONE = 0,
    DIALOGUE_NAMEPLATE_LEFT,
    DIALOGUE_NAMEPLATE_RIGHT
} DialogueNameplateSide;

/*
 * Mode 0 dialogue panel.
 * Frame: BG0 charblock 1 / screenblock 30 / priority 0 / palette bank 1.
 * Chrome (nameplate/hint/prompt) and text share the dynamic tile pool.
 * Text glyphs mark dirty tiles; FlushDirty uploads only those tiles.
 */
void DialoguePanel_Init(void);
void DialoguePanel_ShowFrame(void);
void DialoguePanel_Close(void);

/* Rebuild chrome (frame + nameplate + demo hint). Clears text and hides prompt. */
void DialoguePanel_BeginSample(
    DialogueNameplateSide side,
    const char *name,
    const char *demoHint);

void DialoguePanel_SetNameplate(DialogueNameplateSide side, const char *name);
void DialoguePanel_SetDemoHint(const char *text);
void DialoguePanel_SetPromptVisible(int visible);

/* Text surface used by the incremental printer. */
void DialoguePanel_ClearTextLine(int lineIndex);
void DialoguePanel_DrawTextChar(int x, int y, char ch);
/* colorIndex is a BG palette bank local index (1=black, 2=orange emphasis, ...). */
void DialoguePanel_DrawTextCharColor(int x, int y, char ch, int colorIndex);
void DialoguePanel_FlushDirty(void);

/* Dirty-slot diagnostics (host tests / debug). */
int DialoguePanel_CountDirty(void);
int DialoguePanel_GetDynamicSlotTile(int slot, int *tileX, int *tileY);
int DialoguePanel_IsDynamicSlotDirty(int slot);

/* Legacy full-content helpers (complete reraster). Prefer BeginSample + printer. */
void DialoguePanel_SetText(const char *line1, const char *line2);
void DialoguePanel_ClearContent(void);
void DialoguePanel_Apply(
    DialogueNameplateSide side,
    const char *name,
    const char *line1,
    const char *line2,
    int promptVisible,
    const char *demoHint);

#endif
