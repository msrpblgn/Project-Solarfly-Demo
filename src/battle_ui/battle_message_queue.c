#include "battle_ui/battle_message_queue.h"
#include "battle_ui/hud_layout.h"
#include "core/video.h"
#include "core/gba_types.h"

typedef struct QueuedMessage {
    char text[BATTLE_MESSAGE_MAX_LENGTH];
    int active;
    BattleMessagePriority priority;
} QueuedMessage;

static QueuedMessage s_queue[BATTLE_MESSAGE_QUEUE_CAPACITY];
static int s_queueHead;
static int s_queueTail;
static int s_queueCount;
static int s_displayTimer;
static int s_holdFrames;
static char s_currentText[BATTLE_MESSAGE_MAX_LENGTH];
static int s_displayChanged;
static int s_droppedCount;

static void BattleMessageQueue_StartNext(void);

static void BattleMessageQueue_CopyString(char *dest, const char *src)
{
    int i = 0;
    while (src[i] && i < BATTLE_MESSAGE_MAX_LENGTH - 1)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int BattleMessageQueue_GetPhysicalIndex(int offset)
{
    return (s_queueHead + offset) % BATTLE_MESSAGE_QUEUE_CAPACITY;
}

static void BattleMessageQueue_RemoveAtOffset(int offset)
{
    int i;

    if (offset < 0 || offset >= s_queueCount)
    {
        return;
    }

    for (i = offset; i < s_queueCount - 1; i++)
    {
        int destIndex = BattleMessageQueue_GetPhysicalIndex(i);
        int srcIndex = BattleMessageQueue_GetPhysicalIndex(i + 1);

        s_queue[destIndex] = s_queue[srcIndex];
    }

    s_queueCount--;
    s_queueTail = BattleMessageQueue_GetPhysicalIndex(s_queueCount);
    if (s_queueCount == 0)
    {
        s_queueHead = 0;
        s_queueTail = 0;
    }
}

static int BattleMessageQueue_RemoveOldestWithPriority(BattleMessagePriority priority)
{
    int i;

    for (i = 0; i < s_queueCount; i++)
    {
        int index = BattleMessageQueue_GetPhysicalIndex(i);

        if (s_queue[index].active && s_queue[index].priority == priority)
        {
            BattleMessageQueue_RemoveAtOffset(i);
            return 1;
        }
    }
    return 0;
}

static int BattleMessageQueue_TryMakeRoomForHigh(void)
{
    if (s_queueCount < BATTLE_MESSAGE_QUEUE_CAPACITY)
    {
        return 1;
    }
    if (BattleMessageQueue_RemoveOldestWithPriority(BATTLE_MESSAGE_PRIORITY_LOW))
    {
        return 1;
    }
    if (BattleMessageQueue_RemoveOldestWithPriority(BATTLE_MESSAGE_PRIORITY_NORMAL))
    {
        return 1;
    }
    if (BattleMessageQueue_RemoveOldestWithPriority(BATTLE_MESSAGE_PRIORITY_HIGH))
    {
        return 1;
    }
    return 0;
}

static int BattleMessageQueue_TryMakeRoomForNormal(void)
{
    if (s_queueCount < BATTLE_MESSAGE_QUEUE_CAPACITY)
    {
        return 1;
    }
    return BattleMessageQueue_RemoveOldestWithPriority(BATTLE_MESSAGE_PRIORITY_LOW);
}

static void BattleMessageQueue_InsertAtTail(const char *message, BattleMessagePriority priority)
{
    BattleMessageQueue_CopyString(s_queue[s_queueTail].text, message);
    s_queue[s_queueTail].active = 1;
    s_queue[s_queueTail].priority = priority;
    s_queueTail = (s_queueTail + 1) % BATTLE_MESSAGE_QUEUE_CAPACITY;
    s_queueCount++;

    if (s_currentText[0] == '\0' && s_displayTimer == 0)
    {
        BattleMessageQueue_StartNext();
    }
}

void BattleMessageQueue_Init(void)
{
    int i;
    s_queueHead = 0;
    s_queueTail = 0;
    s_queueCount = 0;
    s_displayTimer = 0;
    s_holdFrames = BATTLE_MESSAGE_HOLD_FRAMES_NORMAL;
    s_currentText[0] = '\0';
    s_displayChanged = 0;
    s_droppedCount = 0;
    for (i = 0; i < BATTLE_MESSAGE_QUEUE_CAPACITY; i++)
    {
        s_queue[i].active = 0;
        s_queue[i].text[0] = '\0';
        s_queue[i].priority = BATTLE_MESSAGE_PRIORITY_NORMAL;
    }
}

void BattleMessageQueue_SetTextSpeed(TextSpeed textSpeed)
{
    switch (textSpeed)
    {
    case TEXT_SPEED_SLOW:
        s_holdFrames = BATTLE_MESSAGE_HOLD_FRAMES_SLOW;
        break;
    case TEXT_SPEED_FAST:
        s_holdFrames = BATTLE_MESSAGE_HOLD_FRAMES_FAST;
        break;
    case TEXT_SPEED_NORMAL:
    default:
        s_holdFrames = BATTLE_MESSAGE_HOLD_FRAMES_NORMAL;
        break;
    }
}

void BattleMessageQueue_EnqueuePriority(const char *message, BattleMessagePriority priority)
{
    if (!message)
    {
        return;
    }

    if (priority == BATTLE_MESSAGE_PRIORITY_HIGH)
    {
        if (!BattleMessageQueue_TryMakeRoomForHigh())
        {
            s_droppedCount++;
            return;
        }
    }
    else if (priority == BATTLE_MESSAGE_PRIORITY_NORMAL)
    {
        if (!BattleMessageQueue_TryMakeRoomForNormal())
        {
            s_droppedCount++;
            return;
        }
    }
    else if (s_queueCount >= BATTLE_MESSAGE_QUEUE_CAPACITY)
    {
        s_droppedCount++;
        return;
    }

    BattleMessageQueue_InsertAtTail(message, priority);
}

void BattleMessageQueue_Enqueue(const char *message)
{
    BattleMessageQueue_EnqueuePriority(message, BATTLE_MESSAGE_PRIORITY_NORMAL);
}

static void BattleMessageQueue_StartNext(void)
{
    if (s_queueCount == 0)
    {
        s_currentText[0] = '\0';
        s_displayTimer = 0;
        return;
    }
    BattleMessageQueue_CopyString(s_currentText, s_queue[s_queueHead].text);
    s_queue[s_queueHead].active = 0;
    s_queueHead = (s_queueHead + 1) % BATTLE_MESSAGE_QUEUE_CAPACITY;
    s_queueCount--;
    s_displayTimer = s_holdFrames;
    s_displayChanged = 1;
}

void BattleMessageQueue_Update(void)
{
    char previousText[BATTLE_MESSAGE_MAX_LENGTH];
    int i;

    for (i = 0; i < BATTLE_MESSAGE_MAX_LENGTH; i++)
    {
        previousText[i] = s_currentText[i];
    }

    if (s_currentText[0] == '\0' && s_queueCount > 0)
    {
        BattleMessageQueue_StartNext();
    }
    if (s_displayTimer > 0)
    {
        s_displayTimer--;
        if (s_displayTimer == 0)
        {
            if (s_queueCount > 0)
            {
                BattleMessageQueue_StartNext();
            }
            else
            {
                s_currentText[0] = '\0';
            }
        }
    }

    for (i = 0; previousText[i] || s_currentText[i]; i++)
    {
        if (previousText[i] != s_currentText[i])
        {
            s_displayChanged = 1;
            break;
        }
    }
}

int BattleMessageQueue_TakeDisplayChange(void)
{
    int changed = s_displayChanged;
    s_displayChanged = 0;
    return changed;
}

void BattleMessageQueue_Draw(void)
{
    if (s_currentText[0] != '\0')
    {
        Video_DrawRect(
            HUD_MESSAGE_BOX_INSET_X,
            HUD_MESSAGE_BOX_INSET_Y,
            HUD_MESSAGE_BOX_W,
            HUD_MESSAGE_BOX_H,
            HUD_MESSAGE_BOX_FILL);
        /* Border Color outlines are disabled; keep fill-only message boxes. */
        Video_DrawText(HUD_MESSAGE_TEXT_X, HUD_MESSAGE_TEXT_Y, s_currentText, HUD_TEXT_LIGHT);
    }
}

int BattleMessageQueue_IsBusy(void)
{
    return s_currentText[0] != '\0' || s_queueCount > 0;
}
