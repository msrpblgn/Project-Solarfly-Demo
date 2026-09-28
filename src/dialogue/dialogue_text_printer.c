#include "dialogue/dialogue_text_printer.h"

#include "core/video.h"
#include "dialogue/dialogue_resources.h"

typedef struct DialogueTextPrinter {
    DialogueTextPrinterState state;
    DialogueTextPrinterCallbacks callbacks;
    TextSpeed baseSpeed;
    TextSpeed activeSpeed;
    DialogueTextColorRole activeColor;
    int interval;
    int countdown;
    int pauseRemaining;
    int cmdIndex;
    int cmdCount;
    int textPos;
    int cursorX;
    int cursorY;
    int lineIndex;
    int lineWidth;
    DialogueTextCommand commands[DIALOGUE_TEXT_PRINTER_MAX_COMMANDS];
    char plainStorage[DIALOGUE_TEXT_PRINTER_MAX_CHARS];
    DialogueTextCommand plainCommands[DIALOGUE_TEXT_PRINTER_MAX_COMMANDS];
} DialogueTextPrinter;

static DialogueTextPrinter s_printer;

static void DialogueTextPrinter_WriteError(char *error, int errorSize, const char *message)
{
    int i;

    if (!error || errorSize <= 0)
    {
        return;
    }
    if (!message)
    {
        error[0] = '\0';
        return;
    }
    for (i = 0; i < errorSize - 1 && message[i] != '\0'; i++)
    {
        error[i] = message[i];
    }
    error[i] = '\0';
}

static void DialogueTextPrinter_ClearLine(int lineIndex)
{
    if (s_printer.callbacks.clear_line)
    {
        s_printer.callbacks.clear_line(lineIndex, s_printer.callbacks.ctx);
    }
}

static void DialogueTextPrinter_DrawChar(int x, int y, char ch)
{
    if (s_printer.callbacks.draw_char)
    {
        s_printer.callbacks.draw_char(
            x,
            y,
            ch,
            (int)s_printer.activeColor,
            s_printer.callbacks.ctx);
    }
}

static void DialogueTextPrinter_EmitSfx(u16 cueId)
{
    if (s_printer.callbacks.sfx)
    {
        s_printer.callbacks.sfx(cueId, s_printer.callbacks.ctx);
    }
}

static void DialogueTextPrinter_ResetCursor(void)
{
    s_printer.lineIndex = 0;
    s_printer.lineWidth = 0;
    s_printer.cursorX = DIALOGUE_UI_TEXT_X;
    s_printer.cursorY = DIALOGUE_UI_TEXT_LINE1_Y;
    s_printer.textPos = 0;
}

static void DialogueTextPrinter_ResetRuntime(void)
{
    s_printer.countdown = 0;
    s_printer.pauseRemaining = 0;
    s_printer.cmdIndex = 0;
    s_printer.textPos = 0;
    s_printer.activeColor = DIALOGUE_TEXT_COLOR_DEFAULT;
    DialogueTextPrinter_ResetCursor();
}

int DialogueTextPrinter_GetIntervalForSpeed(TextSpeed speed)
{
    switch (speed)
    {
    case TEXT_SPEED_SLOW:
        return 4;
    case TEXT_SPEED_FAST:
        return 1;
    case TEXT_SPEED_NORMAL:
    default:
        return 2;
    }
}

int DialogueTextPrinter_ColorIndexForRole(DialogueTextColorRole role)
{
    switch (role)
    {
    case DIALOGUE_TEXT_COLOR_EMPHASIS:
        /* Palette index 2 = frame orange; readable on light panel fill. */
        return 2;
    case DIALOGUE_TEXT_COLOR_DEFAULT:
    default:
        return 1;
    }
}

void DialogueTextPrinter_Init(void)
{
    s_printer.state = DIALOGUE_TEXT_PRINTER_INACTIVE;
    s_printer.callbacks.clear_line = 0;
    s_printer.callbacks.draw_char = 0;
    s_printer.callbacks.sfx = 0;
    s_printer.callbacks.ctx = 0;
    s_printer.baseSpeed = TEXT_SPEED_NORMAL;
    s_printer.activeSpeed = TEXT_SPEED_NORMAL;
    s_printer.activeColor = DIALOGUE_TEXT_COLOR_DEFAULT;
    s_printer.interval = DialogueTextPrinter_GetIntervalForSpeed(TEXT_SPEED_NORMAL);
    s_printer.cmdCount = 0;
    DialogueTextPrinter_ResetRuntime();
    s_printer.plainStorage[0] = '\0';
}

void DialogueTextPrinter_SetCallbacks(const DialogueTextPrinterCallbacks *callbacks)
{
    if (!callbacks)
    {
        s_printer.callbacks.clear_line = 0;
        s_printer.callbacks.draw_char = 0;
        s_printer.callbacks.sfx = 0;
        s_printer.callbacks.ctx = 0;
        return;
    }
    s_printer.callbacks = *callbacks;
}

static int DialogueTextPrinter_MaxLineWidth(void)
{
    return DIALOGUE_UI_TEXT_MAX_END_X - DIALOGUE_UI_TEXT_X + 1;
}

int DialogueTextPrinter_Validate(const char *text, char *error, int errorSize)
{
    int i;
    int line = 0;
    int lineWidth = 0;
    int maxWidth = DialogueTextPrinter_MaxLineWidth();

    if (!text)
    {
        DialogueTextPrinter_WriteError(error, errorSize, "text is null");
        return 0;
    }

    for (i = 0; text[i] != '\0'; i++)
    {
        if (i >= DIALOGUE_TEXT_PRINTER_MAX_CHARS - 1)
        {
            DialogueTextPrinter_WriteError(error, errorSize, "text exceeds max length");
            return 0;
        }
        if (text[i] == '\n')
        {
            if (line + 1 >= DIALOGUE_TEXT_PRINTER_MAX_LINES)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "text has more than two lines");
                return 0;
            }
            line++;
            lineWidth = 0;
            continue;
        }
        if (!Video_IsFontCharSupported(text[i]))
        {
            DialogueTextPrinter_WriteError(error, errorSize, "unsupported glyph");
            return 0;
        }
        lineWidth += VIDEO_FONT_ADVANCE;
        if (lineWidth > maxWidth)
        {
            DialogueTextPrinter_WriteError(error, errorSize, "line exceeds text safe width");
            return 0;
        }
    }

    DialogueTextPrinter_WriteError(error, errorSize, "");
    return 1;
}

int DialogueTextPrinter_ValidateCommands(
    const DialogueTextCommand *commands,
    int commandCount,
    char *error,
    int errorSize)
{
    int i;
    int line = 0;
    int lineWidth = 0;
    int maxWidth = DialogueTextPrinter_MaxLineWidth();
    int sawEnd = 0;

    if (!commands || commandCount <= 0)
    {
        DialogueTextPrinter_WriteError(error, errorSize, "commands empty");
        return 0;
    }
    if (commandCount > DIALOGUE_TEXT_PRINTER_MAX_COMMANDS)
    {
        DialogueTextPrinter_WriteError(error, errorSize, "too many commands");
        return 0;
    }

    for (i = 0; i < commandCount; i++)
    {
        const DialogueTextCommand *cmd = &commands[i];

        if (sawEnd)
        {
            DialogueTextPrinter_WriteError(error, errorSize, "commands after END");
            return 0;
        }

        switch (cmd->op)
        {
        case DIALOGUE_TEXT_OP_TEXT:
        {
            int t;

            if (!cmd->text)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "TEXT null");
                return 0;
            }
            for (t = 0; cmd->text[t] != '\0'; t++)
            {
                if (cmd->text[t] == '\n')
                {
                    DialogueTextPrinter_WriteError(error, errorSize, "TEXT embeds newline");
                    return 0;
                }
                if (!Video_IsFontCharSupported(cmd->text[t]))
                {
                    DialogueTextPrinter_WriteError(error, errorSize, "unsupported glyph");
                    return 0;
                }
                lineWidth += VIDEO_FONT_ADVANCE;
                if (lineWidth > maxWidth)
                {
                    DialogueTextPrinter_WriteError(error, errorSize, "line exceeds text safe width");
                    return 0;
                }
            }
            break;
        }
        case DIALOGUE_TEXT_OP_NEWLINE:
            if (line + 1 >= DIALOGUE_TEXT_PRINTER_MAX_LINES)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "third line before PAGE");
                return 0;
            }
            line++;
            lineWidth = 0;
            break;
        case DIALOGUE_TEXT_OP_PAUSE:
            if ((int)cmd->arg > DIALOGUE_TEXT_PRINTER_MAX_PAUSE)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "pause out of range");
                return 0;
            }
            break;
        case DIALOGUE_TEXT_OP_WAIT:
        case DIALOGUE_TEXT_OP_PAGE:
            break;
        case DIALOGUE_TEXT_OP_SPEED:
            if (cmd->arg != DIALOGUE_TEXT_SPEED_RESTORE_BASE
                && cmd->arg != (u8)TEXT_SPEED_SLOW
                && cmd->arg != (u8)TEXT_SPEED_NORMAL
                && cmd->arg != (u8)TEXT_SPEED_FAST)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "invalid speed");
                return 0;
            }
            break;
        case DIALOGUE_TEXT_OP_COLOR:
            if (cmd->arg >= (u8)DIALOGUE_TEXT_COLOR_COUNT)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "invalid color");
                return 0;
            }
            break;
        case DIALOGUE_TEXT_OP_SFX:
            if (cmd->arg == 0)
            {
                DialogueTextPrinter_WriteError(error, errorSize, "invalid sfx id");
                return 0;
            }
            break;
        case DIALOGUE_TEXT_OP_END:
            sawEnd = 1;
            break;
        default:
            DialogueTextPrinter_WriteError(error, errorSize, "unknown opcode");
            return 0;
        }

        if (cmd->op == DIALOGUE_TEXT_OP_PAGE)
        {
            line = 0;
            lineWidth = 0;
        }
    }

    if (!sawEnd)
    {
        DialogueTextPrinter_WriteError(error, errorSize, "missing END");
        return 0;
    }

    DialogueTextPrinter_WriteError(error, errorSize, "");
    return 1;
}

static int DialogueTextPrinter_BuildPlainCommands(const char *text, int *outCount)
{
    int i = 0;
    int cmd = 0;
    int start;
    int plainLen = 0;

    s_printer.plainStorage[0] = '\0';

    while (text[i] != '\0')
    {
        if (cmd >= DIALOGUE_TEXT_PRINTER_MAX_COMMANDS - 1)
        {
            return 0;
        }
        if (text[i] == '\n')
        {
            s_printer.plainCommands[cmd].op = DIALOGUE_TEXT_OP_NEWLINE;
            s_printer.plainCommands[cmd].arg = 0;
            s_printer.plainCommands[cmd].text = 0;
            cmd++;
            i++;
            continue;
        }

        start = plainLen;
        while (text[i] != '\0' && text[i] != '\n')
        {
            if (plainLen >= DIALOGUE_TEXT_PRINTER_MAX_CHARS - 1)
            {
                return 0;
            }
            s_printer.plainStorage[plainLen++] = text[i++];
        }
        s_printer.plainStorage[plainLen++] = '\0';
        s_printer.plainCommands[cmd].op = DIALOGUE_TEXT_OP_TEXT;
        s_printer.plainCommands[cmd].arg = 0;
        s_printer.plainCommands[cmd].text = &s_printer.plainStorage[start];
        cmd++;
    }

    if (cmd >= DIALOGUE_TEXT_PRINTER_MAX_COMMANDS)
    {
        return 0;
    }
    s_printer.plainCommands[cmd].op = DIALOGUE_TEXT_OP_END;
    s_printer.plainCommands[cmd].arg = 0;
    s_printer.plainCommands[cmd].text = 0;
    cmd++;
    *outCount = cmd;
    return 1;
}

static void DialogueTextPrinter_ApplySpeedArg(u8 arg)
{
    if (arg == DIALOGUE_TEXT_SPEED_RESTORE_BASE)
    {
        s_printer.activeSpeed = s_printer.baseSpeed;
    }
    else
    {
        s_printer.activeSpeed = (TextSpeed)arg;
    }
    s_printer.interval = DialogueTextPrinter_GetIntervalForSpeed(s_printer.activeSpeed);
}

static int DialogueTextPrinter_Begin(const DialogueTextCommand *commands, int commandCount, TextSpeed baseSpeed)
{
    char error[48];
    int i;

    DialogueTextPrinter_Cancel();

    if (!DialogueTextPrinter_ValidateCommands(commands, commandCount, error, (int)sizeof(error)))
    {
        return 0;
    }

    for (i = 0; i < commandCount; i++)
    {
        s_printer.commands[i] = commands[i];
    }
    s_printer.cmdCount = commandCount;
    if (baseSpeed >= TEXT_SPEED_COUNT)
    {
        baseSpeed = TEXT_SPEED_NORMAL;
    }
    s_printer.baseSpeed = baseSpeed;
    s_printer.activeSpeed = baseSpeed;
    s_printer.interval = DialogueTextPrinter_GetIntervalForSpeed(baseSpeed);
    s_printer.activeColor = DIALOGUE_TEXT_COLOR_DEFAULT;
    DialogueTextPrinter_ResetRuntime();
    DialogueTextPrinter_ClearLine(0);
    DialogueTextPrinter_ClearLine(1);
    s_printer.state = DIALOGUE_TEXT_PRINTER_PRINTING;
    return 1;
}

int DialogueTextPrinter_Start(const char *text, TextSpeed speed)
{
    char error[48];
    int count = 0;

    if (!DialogueTextPrinter_Validate(text, error, (int)sizeof(error)))
    {
        DialogueTextPrinter_Cancel();
        return 0;
    }
    if (!DialogueTextPrinter_BuildPlainCommands(text, &count))
    {
        DialogueTextPrinter_Cancel();
        return 0;
    }
    return DialogueTextPrinter_Begin(s_printer.plainCommands, count, speed);
}

int DialogueTextPrinter_StartCommands(
    const DialogueTextCommand *commands,
    int commandCount,
    TextSpeed baseSpeed)
{
    return DialogueTextPrinter_Begin(commands, commandCount, baseSpeed);
}

void DialogueTextPrinter_Cancel(void)
{
    s_printer.state = DIALOGUE_TEXT_PRINTER_INACTIVE;
    s_printer.cmdCount = 0;
    s_printer.cmdIndex = 0;
    DialogueTextPrinter_ResetRuntime();
    s_printer.activeColor = DIALOGUE_TEXT_COLOR_DEFAULT;
}

static void DialogueTextPrinter_EnterNewline(void)
{
    if (s_printer.lineIndex + 1 < DIALOGUE_TEXT_PRINTER_MAX_LINES)
    {
        s_printer.lineIndex++;
        s_printer.lineWidth = 0;
        s_printer.cursorX = DIALOGUE_UI_TEXT_X;
        s_printer.cursorY = DIALOGUE_UI_TEXT_LINE2_Y;
    }
}

static int DialogueTextPrinter_MorePrintablePending(void)
{
    int i = s_printer.cmdIndex;
    int pos = s_printer.textPos;

    while (i < s_printer.cmdCount)
    {
        const DialogueTextCommand *cmd = &s_printer.commands[i];

        switch (cmd->op)
        {
        case DIALOGUE_TEXT_OP_TEXT:
            if (cmd->text && cmd->text[pos] != '\0')
            {
                return 1;
            }
            i++;
            pos = 0;
            break;
        case DIALOGUE_TEXT_OP_NEWLINE:
        case DIALOGUE_TEXT_OP_SPEED:
        case DIALOGUE_TEXT_OP_COLOR:
        case DIALOGUE_TEXT_OP_SFX:
            i++;
            pos = 0;
            break;
        default:
            return 0;
        }
    }
    return 0;
}

/*
 * Process zero-time controls and at most one printable (unless accelerating).
 * Returns:
 *   0 = yielded (printed a glyph / entered wait/pause / need another update)
 *   1 = became complete
 */
static int DialogueTextPrinter_ProcessStream(int accelerate, int *aAvailable, int *steps)
{
    (void)aAvailable;

    while (*steps < DIALOGUE_TEXT_PRINTER_MAX_PROCESS_STEPS
        && s_printer.state == DIALOGUE_TEXT_PRINTER_PRINTING)
    {
        DialogueTextCommand *cmd;

        (*steps)++;
        if (s_printer.cmdIndex >= s_printer.cmdCount)
        {
            s_printer.state = DIALOGUE_TEXT_PRINTER_COMPLETE;
            return 1;
        }

        cmd = &s_printer.commands[s_printer.cmdIndex];

        switch (cmd->op)
        {
        case DIALOGUE_TEXT_OP_TEXT:
        {
            char ch;

            if (!cmd->text)
            {
                s_printer.cmdIndex++;
                s_printer.textPos = 0;
                continue;
            }
            ch = cmd->text[s_printer.textPos];
            if (ch == '\0')
            {
                s_printer.cmdIndex++;
                s_printer.textPos = 0;
                continue;
            }

            DialogueTextPrinter_DrawChar(s_printer.cursorX, s_printer.cursorY, ch);
            s_printer.cursorX += VIDEO_FONT_ADVANCE;
            s_printer.lineWidth += VIDEO_FONT_ADVANCE;
            s_printer.textPos++;

            if (cmd->text[s_printer.textPos] == '\0')
            {
                s_printer.cmdIndex++;
                s_printer.textPos = 0;
            }

            if (accelerate)
            {
                continue;
            }
            /*
             * Inter-character delay only when another printable remains before
             * the next pause/input/end boundary. Final glyph shares this update
             * with END (Milestone 3 timing) or an authored PAUSE/WAIT/PAGE.
             */
            if (DialogueTextPrinter_MorePrintablePending())
            {
                s_printer.countdown = s_printer.interval;
                return 0;
            }
            continue;
        }
        case DIALOGUE_TEXT_OP_NEWLINE:
            DialogueTextPrinter_EnterNewline();
            s_printer.cmdIndex++;
            continue;
        case DIALOGUE_TEXT_OP_PAUSE:
            s_printer.cmdIndex++;
            if (cmd->arg == 0)
            {
                continue;
            }
            s_printer.pauseRemaining = (int)cmd->arg;
            s_printer.state = DIALOGUE_TEXT_PRINTER_TIMED_PAUSE;
            s_printer.countdown = 0;
            return 0;
        case DIALOGUE_TEXT_OP_WAIT:
            s_printer.cmdIndex++;
            s_printer.state = DIALOGUE_TEXT_PRINTER_WAIT_INPUT;
            s_printer.countdown = 0;
            return 0;
        case DIALOGUE_TEXT_OP_PAGE:
            s_printer.cmdIndex++;
            s_printer.state = DIALOGUE_TEXT_PRINTER_WAIT_PAGE;
            s_printer.countdown = 0;
            return 0;
        case DIALOGUE_TEXT_OP_SPEED:
            DialogueTextPrinter_ApplySpeedArg(cmd->arg);
            s_printer.cmdIndex++;
            continue;
        case DIALOGUE_TEXT_OP_COLOR:
            s_printer.activeColor = (DialogueTextColorRole)cmd->arg;
            s_printer.cmdIndex++;
            continue;
        case DIALOGUE_TEXT_OP_SFX:
            DialogueTextPrinter_EmitSfx((u16)cmd->arg);
            s_printer.cmdIndex++;
            continue;
        case DIALOGUE_TEXT_OP_END:
            s_printer.cmdIndex++;
            s_printer.state = DIALOGUE_TEXT_PRINTER_COMPLETE;
            return 1;
        default:
            DialogueTextPrinter_Cancel();
            return 0;
        }
    }

    if (*steps >= DIALOGUE_TEXT_PRINTER_MAX_PROCESS_STEPS
        && s_printer.state == DIALOGUE_TEXT_PRINTER_PRINTING)
    {
        DialogueTextPrinter_Cancel();
    }
    return 0;
}

int DialogueTextPrinter_Update(int accelerate, int aPressed)
{
    int becameComplete = 0;
    int aAvailable = aPressed ? 1 : 0;
    int steps = 0;

    if (s_printer.state == DIALOGUE_TEXT_PRINTER_INACTIVE
        || s_printer.state == DIALOGUE_TEXT_PRINTER_COMPLETE)
    {
        return 0;
    }

    if (s_printer.state == DIALOGUE_TEXT_PRINTER_WAIT_INPUT)
    {
        if (!aAvailable)
        {
            return 0;
        }
        aAvailable = 0;
        s_printer.state = DIALOGUE_TEXT_PRINTER_PRINTING;
        /* Continue into stream processing with A already consumed. */
    }
    else if (s_printer.state == DIALOGUE_TEXT_PRINTER_WAIT_PAGE)
    {
        if (!aAvailable)
        {
            return 0;
        }
        aAvailable = 0;
        DialogueTextPrinter_ClearLine(0);
        DialogueTextPrinter_ClearLine(1);
        DialogueTextPrinter_ResetCursor();
        s_printer.state = DIALOGUE_TEXT_PRINTER_PRINTING;
    }
    else if (s_printer.state == DIALOGUE_TEXT_PRINTER_TIMED_PAUSE)
    {
        if (s_printer.pauseRemaining > 0)
        {
            s_printer.pauseRemaining--;
        }
        if (s_printer.pauseRemaining > 0)
        {
            return 0;
        }
        /* Pause fully elapsed this update; resume on the next update. */
        s_printer.state = DIALOGUE_TEXT_PRINTER_PRINTING;
        return 0;
    }

    if (s_printer.state != DIALOGUE_TEXT_PRINTER_PRINTING)
    {
        return 0;
    }

    if (!accelerate)
    {
        if (s_printer.countdown > 0)
        {
            s_printer.countdown--;
            if (s_printer.countdown > 0)
            {
                return 0;
            }
        }
        if (DialogueTextPrinter_ProcessStream(0, &aAvailable, &steps))
        {
            becameComplete = 1;
        }
        return becameComplete;
    }

    /* Accelerated printable drain; stop at pause/input/end. */
    s_printer.countdown = 0;
    while (s_printer.state == DIALOGUE_TEXT_PRINTER_PRINTING
        && steps < DIALOGUE_TEXT_PRINTER_MAX_PROCESS_STEPS)
    {
        if (DialogueTextPrinter_ProcessStream(1, &aAvailable, &steps))
        {
            becameComplete = 1;
            break;
        }
        if (s_printer.state != DIALOGUE_TEXT_PRINTER_PRINTING)
        {
            break;
        }
    }
    return becameComplete;
}

DialogueTextPrinterState DialogueTextPrinter_GetState(void)
{
    return s_printer.state;
}

int DialogueTextPrinter_IsPrinting(void)
{
    return s_printer.state == DIALOGUE_TEXT_PRINTER_PRINTING;
}

int DialogueTextPrinter_IsComplete(void)
{
    return s_printer.state == DIALOGUE_TEXT_PRINTER_COMPLETE;
}

int DialogueTextPrinter_NeedsUpdate(void)
{
    return s_printer.state == DIALOGUE_TEXT_PRINTER_PRINTING
        || s_printer.state == DIALOGUE_TEXT_PRINTER_TIMED_PAUSE
        || s_printer.state == DIALOGUE_TEXT_PRINTER_WAIT_INPUT
        || s_printer.state == DIALOGUE_TEXT_PRINTER_WAIT_PAGE;
}

int DialogueTextPrinter_ShowsMarker(void)
{
    return s_printer.state == DIALOGUE_TEXT_PRINTER_WAIT_INPUT
        || s_printer.state == DIALOGUE_TEXT_PRINTER_WAIT_PAGE
        || s_printer.state == DIALOGUE_TEXT_PRINTER_COMPLETE;
}

TextSpeed DialogueTextPrinter_GetActiveSpeed(void)
{
    return s_printer.activeSpeed;
}

TextSpeed DialogueTextPrinter_GetBaseSpeed(void)
{
    return s_printer.baseSpeed;
}

DialogueTextColorRole DialogueTextPrinter_GetActiveColor(void)
{
    return s_printer.activeColor;
}
