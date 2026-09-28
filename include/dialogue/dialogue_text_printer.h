#ifndef DIALOGUE_TEXT_PRINTER_H
#define DIALOGUE_TEXT_PRINTER_H

#include "battle_ui/battle_message_queue.h"
#include "core/gba_types.h"

#define DIALOGUE_TEXT_PRINTER_MAX_CHARS 96
#define DIALOGUE_TEXT_PRINTER_MAX_LINES 2
#define DIALOGUE_TEXT_PRINTER_MAX_COMMANDS 48
#define DIALOGUE_TEXT_PRINTER_MAX_PAUSE 240
#define DIALOGUE_TEXT_PRINTER_MAX_PROCESS_STEPS 256

/* Demo diagnostic cue id (no audio engine yet). */
#define DIALOGUE_TEXT_SFX_CUE_DEMO 1

typedef enum DialogueTextPrinterState {
    DIALOGUE_TEXT_PRINTER_INACTIVE = 0,
    DIALOGUE_TEXT_PRINTER_PRINTING,
    DIALOGUE_TEXT_PRINTER_TIMED_PAUSE,
    DIALOGUE_TEXT_PRINTER_WAIT_INPUT,
    DIALOGUE_TEXT_PRINTER_WAIT_PAGE,
    DIALOGUE_TEXT_PRINTER_COMPLETE
} DialogueTextPrinterState;

typedef enum DialogueTextCmdOp {
    DIALOGUE_TEXT_OP_TEXT = 1,
    DIALOGUE_TEXT_OP_NEWLINE,
    DIALOGUE_TEXT_OP_PAUSE,
    DIALOGUE_TEXT_OP_WAIT,
    DIALOGUE_TEXT_OP_PAGE,
    DIALOGUE_TEXT_OP_SPEED,
    DIALOGUE_TEXT_OP_COLOR,
    DIALOGUE_TEXT_OP_SFX,
    DIALOGUE_TEXT_OP_END
} DialogueTextCmdOp;

/* arg for SPEED: TextSpeed value, or RESTORE_BASE. */
#define DIALOGUE_TEXT_SPEED_RESTORE_BASE 0xFFu

typedef enum DialogueTextColorRole {
    DIALOGUE_TEXT_COLOR_DEFAULT = 0,
    DIALOGUE_TEXT_COLOR_EMPHASIS = 1,
    DIALOGUE_TEXT_COLOR_COUNT
} DialogueTextColorRole;

/*
 * Typed ROM command array (Milestone 4).
 * TEXT uses a pointer to a printable C string (no embedded newlines).
 * Automatic punctuation delays remain disabled.
 */
typedef struct DialogueTextCommand {
    u8 op;
    u8 arg;
    const char *text;
} DialogueTextCommand;

#define DIALOGUE_CMD_TEXT(s) \
    { DIALOGUE_TEXT_OP_TEXT, 0, (s) }
#define DIALOGUE_CMD_NEWLINE() \
    { DIALOGUE_TEXT_OP_NEWLINE, 0, 0 }
#define DIALOGUE_CMD_PAUSE(n) \
    { DIALOGUE_TEXT_OP_PAUSE, (u8)(n), 0 }
#define DIALOGUE_CMD_WAIT() \
    { DIALOGUE_TEXT_OP_WAIT, 0, 0 }
#define DIALOGUE_CMD_PAGE() \
    { DIALOGUE_TEXT_OP_PAGE, 0, 0 }
#define DIALOGUE_CMD_SPEED(v) \
    { DIALOGUE_TEXT_OP_SPEED, (u8)(v), 0 }
#define DIALOGUE_CMD_COLOR(v) \
    { DIALOGUE_TEXT_OP_COLOR, (u8)(v), 0 }
#define DIALOGUE_CMD_SFX(id) \
    { DIALOGUE_TEXT_OP_SFX, (u8)(id), 0 }
#define DIALOGUE_CMD_END() \
    { DIALOGUE_TEXT_OP_END, 0, 0 }

typedef struct DialogueTextPrinterCallbacks {
    void (*clear_line)(int lineIndex, void *ctx);
    void (*draw_char)(int x, int y, char ch, int colorRole, void *ctx);
    void (*sfx)(u16 cueId, void *ctx);
    void *ctx;
} DialogueTextPrinterCallbacks;

/*
 * Nonblocking dialogue text printer.
 * Timing uses game updates (Slow=4 / Normal=2 / Fast=1 between printables).
 * Authored PAUSE/WAIT/PAGE/SPEED/COLOR/SFX extend the same parser path.
 *
 * SPEED/COLOR/SFX are zero-time: when encountered before a printable in an
 * eligible update, the following glyph may appear in that same update.
 * They never emit an extra printable beyond the single-glyph-per-update rule
 * under normal (non-accelerated) printing. Automatic punctuation delays are off.
 */
void DialogueTextPrinter_Init(void);
void DialogueTextPrinter_SetCallbacks(const DialogueTextPrinterCallbacks *callbacks);

int DialogueTextPrinter_Validate(
    const char *text,
    char *error,
    int errorSize);
int DialogueTextPrinter_ValidateCommands(
    const DialogueTextCommand *commands,
    int commandCount,
    char *error,
    int errorSize);

/* Plain-text wrapper: builds an internal TEXT/NEWLINE/END command stream. */
int DialogueTextPrinter_Start(const char *text, TextSpeed speed);
int DialogueTextPrinter_StartCommands(
    const DialogueTextCommand *commands,
    int commandCount,
    TextSpeed baseSpeed);
void DialogueTextPrinter_Cancel(void);

/*
 * One game-update tick.
 * accelerate: B-held printable drain until pause/input/end (does not skip PAUSE).
 * aPressed: fresh A edge; resolves at most one WAIT/PAGE boundary this update.
 * Returns 1 if the printer became COMPLETE during this call.
 */
int DialogueTextPrinter_Update(int accelerate, int aPressed);

DialogueTextPrinterState DialogueTextPrinter_GetState(void);
int DialogueTextPrinter_IsPrinting(void);
int DialogueTextPrinter_IsComplete(void);
int DialogueTextPrinter_NeedsUpdate(void);
int DialogueTextPrinter_ShowsMarker(void);
TextSpeed DialogueTextPrinter_GetActiveSpeed(void);
TextSpeed DialogueTextPrinter_GetBaseSpeed(void);
DialogueTextColorRole DialogueTextPrinter_GetActiveColor(void);
int DialogueTextPrinter_GetIntervalForSpeed(TextSpeed speed);
int DialogueTextPrinter_ColorIndexForRole(DialogueTextColorRole role);

#endif
