/*
 * Host stubs for Tutsil Vortex recovery tests.
 * Avoids GBA video / HUD while linking real resolve + enemy turn.
 */
#include "battle/battle_draw.h"
#include "battle_ui/battle_message_queue.h"

static unsigned int s_dirtyFlags;

void BattleDraw_MarkDirty(unsigned int flags)
{
    s_dirtyFlags |= flags;
}

void BattleDraw_MarkAllDirty(void)
{
    s_dirtyFlags = BATTLE_DRAW_DIRTY_ALL;
}

unsigned int BattleDraw_PeekDirty(void)
{
    return s_dirtyFlags;
}

unsigned int BattleDraw_ConsumeDirty(void)
{
    unsigned int flags = s_dirtyFlags;
    s_dirtyFlags = 0;
    return flags;
}

void BattleDraw_ClearDirty(void)
{
    s_dirtyFlags = 0;
}

void BattleMessageQueue_Init(void)
{
}

void BattleMessageQueue_SetTextSpeed(TextSpeed textSpeed)
{
    (void)textSpeed;
}

void BattleMessageQueue_Enqueue(const char *message)
{
    (void)message;
}

void BattleMessageQueue_EnqueuePriority(const char *message, BattleMessagePriority priority)
{
    (void)message;
    (void)priority;
}

void BattleMessageQueue_Update(void)
{
}

void BattleMessageQueue_Draw(void)
{
}

int BattleMessageQueue_IsBusy(void)
{
    return 0;
}

int BattleMessageQueue_TakeDisplayChange(void)
{
    return 0;
}
