#include "battle/battle_draw.h"

static unsigned int s_drawDirty;

void BattleDraw_MarkDirty(unsigned int flags)
{
    s_drawDirty |= flags;
}

void BattleDraw_MarkAllDirty(void)
{
    s_drawDirty = BATTLE_DRAW_DIRTY_ALL;
}

unsigned int BattleDraw_PeekDirty(void)
{
    return s_drawDirty;
}

unsigned int BattleDraw_ConsumeDirty(void)
{
    unsigned int dirty = s_drawDirty;
    s_drawDirty = 0;
    return dirty;
}

void BattleDraw_ClearDirty(void)
{
    s_drawDirty = 0;
}
