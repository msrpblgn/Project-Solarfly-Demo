#include "host_vram_shim.h"

#include <string.h>

u8 g_hostBgVram[0x10000];
u16 g_hostBgPalette[256];
u16 g_hostRegBg0Cnt;
u16 g_hostRegBg0Hofs;
u16 g_hostRegBg0Vofs;

void HostVram_Reset(void)
{
    memset(g_hostBgVram, 0, sizeof(g_hostBgVram));
    memset(g_hostBgPalette, 0, sizeof(g_hostBgPalette));
    g_hostRegBg0Cnt = 0;
    g_hostRegBg0Hofs = 0;
    g_hostRegBg0Vofs = 0;
}
