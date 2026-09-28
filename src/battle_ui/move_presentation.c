#include "battle_ui/move_presentation.h"

#include "data/element_database.h"

static void MovePresentation_Clear(char *dest, int size)
{
    if (!dest || size <= 0)
    {
        return;
    }
    dest[0] = '\0';
}

static void MovePresentation_Copy(char *dest, int size, const char *src)
{
    int i;

    if (!dest || size <= 0)
    {
        return;
    }
    if (!src)
    {
        dest[0] = '\0';
        return;
    }
    for (i = 0; i < size - 1 && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int MovePresentation_StrLen(const char *text)
{
    int len = 0;

    if (!text)
    {
        return 0;
    }
    while (text[len] != '\0')
    {
        len++;
    }
    return len;
}

static void MovePresentation_AppendChar(char *dest, int size, int *cursor, char c)
{
    if (!dest || !cursor || size <= 0)
    {
        return;
    }
    if (*cursor < size - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
        dest[*cursor] = '\0';
    }
}

static void MovePresentation_AppendString(char *dest, int size, int *cursor, const char *src)
{
    if (!src)
    {
        return;
    }
    while (*src)
    {
        MovePresentation_AppendChar(dest, size, cursor, *src);
        src++;
    }
}

static void MovePresentation_AppendNumber(char *dest, int size, int *cursor, int value)
{
    char digits[12];
    int count = 0;
    int n;

    if (value < 0)
    {
        MovePresentation_AppendChar(dest, size, cursor, '-');
        value = -value;
    }
    if (value == 0)
    {
        MovePresentation_AppendChar(dest, size, cursor, '0');
        return;
    }
    n = value;
    while (n > 0 && count < (int)sizeof(digits))
    {
        digits[count++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (count > 0)
    {
        MovePresentation_AppendChar(dest, size, cursor, digits[--count]);
    }
}

int MovePresentation_IsVariablePower(const MoveData *move)
{
    if (!move)
    {
        return 0;
    }
    if (move->id == MOVE_LONG_SHOT || move->id == MOVE_LONG_SHOT_TEST)
    {
        return 1;
    }
    return MoveData_HasDistanceScale(move);
}

const char *MovePresentation_GetCategoryLabel(MoveCategory category)
{
    switch (category)
    {
    case MOVE_CATEGORY_PHYSICAL:
        return "PHYS";
    case MOVE_CATEGORY_SPECIAL:
        return "SPEC";
    case MOVE_CATEGORY_SUPPORT:
        return "SUPP";
    case MOVE_CATEGORY_STATUS:
        return "STAT";
    case MOVE_CATEGORY_DISRUPT:
        return "DISR";
    default:
        return "????";
    }
}

const char *MovePresentation_GetTargetLabel(TargetPattern pattern)
{
    switch (pattern)
    {
    case MOVE_TARGET_SINGLE_ENEMY:
        return "1 ENEMY";
    case MOVE_TARGET_SELF:
        return "SELF";
    case MOVE_TARGET_ALL_ENEMIES:
        return "ALL ENEMIES";
    case MOVE_TARGET_OUTER_ENEMIES:
        return "OUTER ENEMIES";
    case MOVE_TARGET_ALLY_SLOT:
        return "EMPTY ALLY SLOT";
    case MOVE_TARGET_SINGLE_ALLY:
        return "1 ALLY";
    case MOVE_TARGET_ADJACENT_ALLIES:
        return "ADJ. ALLIES";
    case MOVE_TARGET_ALL_ALLIES:
        return "ALL ALLIES";
    case MOVE_TARGET_USER_AND_ENEMY:
        return "SELF+ENEMY";
    default:
        return "TARGET?";
    }
}

static void MovePresentation_FillPower(const MoveData *move, char *dest, int size)
{
    int cursor = 0;
    int power;

    MovePresentation_Clear(dest, size);
    if (!move)
    {
        MovePresentation_Copy(dest, size, "--");
        return;
    }
    if (MovePresentation_IsVariablePower(move))
    {
        MovePresentation_Copy(dest, size, "PWR VAR");
        return;
    }
    power = MoveData_GetDamagePower(move);
    if (move->category == MOVE_CATEGORY_SUPPORT
        || move->category == MOVE_CATEGORY_STATUS
        || move->category == MOVE_CATEGORY_DISRUPT)
    {
        if (power <= 0)
        {
            MovePresentation_Copy(dest, size, "--");
            return;
        }
    }
    if (power <= 0)
    {
        MovePresentation_Copy(dest, size, "--");
        return;
    }
    MovePresentation_AppendString(dest, size, &cursor, "PWR ");
    MovePresentation_AppendNumber(dest, size, &cursor, power);
}

static void MovePresentation_FillSc(
    const MoveData *move,
    const MovePresentationLiveSc *liveSc,
    char *dest,
    int size)
{
    int cursor = 0;
    int current;
    int maxSc;

    MovePresentation_Clear(dest, size);
    if (!move || !liveSc)
    {
        MovePresentation_Copy(dest, size, "SC -/-");
        return;
    }
    current = liveSc->currentSc;
    maxSc = liveSc->maxSc;
    if (maxSc < 0)
    {
        maxSc = move->maxSc;
    }
    if (current < 0)
    {
        current = 0;
    }
    MovePresentation_AppendString(dest, size, &cursor, "SC ");
    MovePresentation_AppendNumber(dest, size, &cursor, current);
    MovePresentation_AppendChar(dest, size, &cursor, '/');
    MovePresentation_AppendNumber(dest, size, &cursor, maxSc);
}

/*
 * Concise effect line for the settled card. Prefer mechanics needed to choose
 * the move; omit flavor. Must fit MOVE_PRESENTATION_CARD_MAX_CHARS when possible.
 */
static void MovePresentation_FillEffect(const MoveData *move, char *dest, int size)
{
    const char *summary = 0;

    MovePresentation_Clear(dest, size);
    if (!move)
    {
        return;
    }

    switch (move->id)
    {
    case MOVE_LONG_SHOT:
    case MOVE_LONG_SHOT_TEST:
        summary = "Power rises with range (10-34).";
        break;
    case MOVE_VORTEX_ESCAPE:
        /* Shared card text; Tutsil next-turn skip recovery is host-specific. */
        summary = "Empty ally tile; 1 LP to act again.";
        break;
    case MOVE_HOLD_BACK:
        summary = "-25% dmg 1t; next atk hits all.";
        break;
    case MOVE_TEMPERATURE_DIFFERENCE:
        summary = "Next move becomes Wind type.";
        break;
    case MOVE_INFLUENCE:
        summary = "Encore 1t; 1 LP extends to 2t.";
        break;
    case MOVE_YOUNG_BLOOD:
        summary = "Raises user attack by 15%.";
        break;
    case MOVE_PHANTOM_PHIST:
        summary = "Crit Curse; side splash damage.";
        break;
    case MOVE_FLAME_CHARGE:
        summary = "Next Slingshot attack becomes Fire.";
        break;
    case MOVE_FIERY_STRIKE:
        summary = "Only hits the closest enemy.";
        break;
    case MOVE_FAKIE:
        summary = "Two hits; 2nd has raised crit.";
        break;
    case MOVE_ELECTRO_THERAPY:
        summary = "Clears status from one ally.";
        break;
    default:
        break;
    }

    if (summary)
    {
        MovePresentation_Copy(dest, size, summary);
        return;
    }
    if (move->description && move->description[0] != '\0')
    {
        if (MovePresentation_StrLen(move->description) <= MOVE_PRESENTATION_CARD_MAX_CHARS)
        {
            MovePresentation_Copy(dest, size, move->description);
            return;
        }
        /* Keep a bounded prefix only when no curated summary exists. */
        MovePresentation_Copy(dest, size, move->description);
        if (MovePresentation_StrLen(dest) > MOVE_PRESENTATION_CARD_MAX_CHARS
            && MOVE_PRESENTATION_CARD_MAX_CHARS >= 3
            && size > MOVE_PRESENTATION_CARD_MAX_CHARS)
        {
            dest[MOVE_PRESENTATION_CARD_MAX_CHARS - 1] = '.';
            dest[MOVE_PRESENTATION_CARD_MAX_CHARS - 2] = '.';
            dest[MOVE_PRESENTATION_CARD_MAX_CHARS - 3] = '.';
            dest[MOVE_PRESENTATION_CARD_MAX_CHARS] = '\0';
        }
        return;
    }
    MovePresentation_Copy(dest, size, "");
}

void MovePresentation_FillLive(
    MoveId moveId,
    const MovePresentationLiveSc *liveSc,
    MovePresentationFields *out)
{
    const MoveData *move;
    const char *elementName;
    MovePresentationLiveSc localSc;

    if (!out)
    {
        return;
    }
    MovePresentation_Clear(out->name, (int)sizeof(out->name));
    MovePresentation_Clear(out->element, (int)sizeof(out->element));
    MovePresentation_Clear(out->category, (int)sizeof(out->category));
    MovePresentation_Clear(out->power, (int)sizeof(out->power));
    MovePresentation_Clear(out->sc, (int)sizeof(out->sc));
    MovePresentation_Clear(out->target, (int)sizeof(out->target));
    MovePresentation_Clear(out->effect, (int)sizeof(out->effect));

    if (moveId == MOVE_NONE)
    {
        MovePresentation_Copy(out->name, (int)sizeof(out->name), "EMPTY");
        MovePresentation_Copy(out->element, (int)sizeof(out->element), "--");
        MovePresentation_Copy(out->category, (int)sizeof(out->category), "--");
        MovePresentation_Copy(out->power, (int)sizeof(out->power), "--");
        MovePresentation_Copy(out->sc, (int)sizeof(out->sc), "SC -/-");
        MovePresentation_Copy(out->target, (int)sizeof(out->target), "--");
        MovePresentation_Copy(out->effect, (int)sizeof(out->effect), "No move in this slot.");
        return;
    }

    move = MoveData_Get(moveId);
    if (!move)
    {
        MovePresentation_Copy(out->name, (int)sizeof(out->name), "UNKNOWN");
        MovePresentation_Copy(out->effect, (int)sizeof(out->effect), "Missing move data.");
        return;
    }

    MovePresentation_Copy(out->name, (int)sizeof(out->name), move->name ? move->name : "MOVE");
    elementName = ElementDatabase_GetShortName(move->element);
    if (!elementName || elementName[0] == '\0')
    {
        elementName = ElementDatabase_GetName(move->element);
    }
    MovePresentation_Copy(
        out->element,
        (int)sizeof(out->element),
        elementName ? elementName : "---");
    MovePresentation_Copy(
        out->category,
        (int)sizeof(out->category),
        MovePresentation_GetCategoryLabel(move->category));
    MovePresentation_FillPower(move, out->power, (int)sizeof(out->power));

    if (liveSc)
    {
        localSc = *liveSc;
    }
    else
    {
        localSc.currentSc = 0;
        localSc.maxSc = move->maxSc;
    }
    if (localSc.maxSc <= 0)
    {
        localSc.maxSc = move->maxSc;
    }
    MovePresentation_FillSc(move, &localSc, out->sc, (int)sizeof(out->sc));
    MovePresentation_Copy(
        out->target,
        (int)sizeof(out->target),
        MovePresentation_GetTargetLabel(move->targetPattern));
    MovePresentation_FillEffect(move, out->effect, (int)sizeof(out->effect));
}

int MovePresentation_CountOccupiedSlots(const MoveId *slots, int slotCount)
{
    int i;
    int count = 0;

    if (!slots || slotCount <= 0)
    {
        return 0;
    }
    for (i = 0; i < slotCount; i++)
    {
        if (slots[i] != MOVE_NONE)
        {
            count++;
        }
    }
    return count;
}

int MovePresentation_FirstOccupiedSlot(const MoveId *slots, int slotCount)
{
    int i;

    if (!slots || slotCount <= 0)
    {
        return 0;
    }
    for (i = 0; i < slotCount; i++)
    {
        if (slots[i] != MOVE_NONE)
        {
            return i;
        }
    }
    return 0;
}

int MovePresentation_NthOccupiedSlot(const MoveId *slots, int slotCount, int n)
{
    int i;
    int seen = 0;

    if (!slots || slotCount <= 0 || n <= 0)
    {
        return 0;
    }
    for (i = 0; i < slotCount; i++)
    {
        if (slots[i] != MOVE_NONE)
        {
            seen++;
            if (seen == n)
            {
                return i;
            }
        }
    }
    return 0;
}

int MovePresentation_GetOccupiedOrdinal(
    const MoveId *slots,
    int slotCount,
    int slotIndex)
{
    int i;
    int ordinal = 0;

    if (!slots || slotCount <= 0 || slotIndex < 0 || slotIndex >= slotCount)
    {
        return 0;
    }
    if (slots[slotIndex] == MOVE_NONE)
    {
        return 0;
    }
    for (i = 0; i <= slotIndex; i++)
    {
        if (slots[i] != MOVE_NONE)
        {
            ordinal++;
        }
    }
    return ordinal;
}

int MovePresentation_StepOccupiedSlot(
    const MoveId *slots,
    int slotCount,
    int currentSlot,
    int direction)
{
    int i;
    int start;
    int step;
    int lastOccupied;

    if (!slots || slotCount <= 0)
    {
        return 0;
    }
    if (MovePresentation_CountOccupiedSlots(slots, slotCount) <= 0)
    {
        return currentSlot;
    }
    if (direction == 0)
    {
        return currentSlot;
    }

    if (currentSlot < 0 || currentSlot >= slotCount || slots[currentSlot] == MOVE_NONE)
    {
        if (direction > 0)
        {
            return MovePresentation_FirstOccupiedSlot(slots, slotCount);
        }
        lastOccupied = MovePresentation_FirstOccupiedSlot(slots, slotCount);
        for (i = 0; i < slotCount; i++)
        {
            if (slots[i] != MOVE_NONE)
            {
                lastOccupied = i;
            }
        }
        return lastOccupied;
    }

    step = (direction > 0) ? 1 : -1;
    start = currentSlot;
    for (i = 0; i < slotCount; i++)
    {
        start += step;
        if (start < 0)
        {
            start = slotCount - 1;
        }
        else if (start >= slotCount)
        {
            start = 0;
        }
        if (slots[start] != MOVE_NONE)
        {
            return start;
        }
    }
    return currentSlot;
}

int MovePresentation_StepOccupiedSlotNoWrap(
    const MoveId *slots,
    int slotCount,
    int currentSlot,
    int direction)
{
    int i;
    int start;
    int step;

    if (!slots || slotCount <= 0)
    {
        return 0;
    }
    if (MovePresentation_CountOccupiedSlots(slots, slotCount) <= 0)
    {
        return currentSlot;
    }
    if (direction == 0)
    {
        return currentSlot;
    }

    if (currentSlot < 0 || currentSlot >= slotCount || slots[currentSlot] == MOVE_NONE)
    {
        /* Invalid cursor: land on first (right) or last (left) without wrapping past ends. */
        if (direction > 0)
        {
            return MovePresentation_FirstOccupiedSlot(slots, slotCount);
        }
        start = MovePresentation_FirstOccupiedSlot(slots, slotCount);
        for (i = 0; i < slotCount; i++)
        {
            if (slots[i] != MOVE_NONE)
            {
                start = i;
            }
        }
        return start;
    }

    step = (direction > 0) ? 1 : -1;
    start = currentSlot;
    for (i = 0; i < slotCount; i++)
    {
        start += step;
        if (start < 0 || start >= slotCount)
        {
            /* Hit an end: no wrap — keep current selection. */
            return currentSlot;
        }
        if (slots[start] != MOVE_NONE)
        {
            return start;
        }
    }
    return currentSlot;
}
