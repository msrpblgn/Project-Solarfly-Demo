#include "core/system.h"
#include "core/gba_types.h"

void System_Init(void)
{
}

void System_WaitForVBlank(void)
{
    while (REG_VCOUNT >= 160)
    {
    }
    while (REG_VCOUNT < 160)
    {
    }
}
