#include "battle_feel/battle_pacing.h"

static int s_waitFrames;

void BattlePacing_Reset(void)
{
    s_waitFrames = 0;
}

int BattlePacing_IsWaiting(void)
{
    return s_waitFrames > 0;
}

void BattlePacing_StartWait(int frames)
{
    s_waitFrames = frames;
}

void BattlePacing_Update(void)
{
    if (s_waitFrames > 0)
    {
        s_waitFrames--;
    }
}
