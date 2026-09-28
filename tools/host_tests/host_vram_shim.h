#ifndef HOST_VRAM_SHIM_H
#define HOST_VRAM_SHIM_H

/*
 * Host-side stand-ins for GBA VRAM/register macros used by dialogue_panel.c.
 * Include after core/gba_types.h and before using CHARBLOCK_BASE / REG_*.
 */
#include "core/gba_types.h"

extern u8 g_hostBgVram[0x10000];
extern u16 g_hostBgPalette[256];
extern u16 g_hostRegBg0Cnt;
extern u16 g_hostRegBg0Hofs;
extern u16 g_hostRegBg0Vofs;

#undef REG_BG0CNT
#undef REG_BG0HOFS
#undef REG_BG0VOFS
#undef BG_PALETTE
#undef CHARBLOCK_BASE
#undef SCREENBLOCK_BASE

#define REG_BG0CNT g_hostRegBg0Cnt
#define REG_BG0HOFS g_hostRegBg0Hofs
#define REG_BG0VOFS g_hostRegBg0Vofs
#define BG_PALETTE g_hostBgPalette
#define CHARBLOCK_BASE(block) \
    ((volatile u16 *)(g_hostBgVram + ((unsigned)(block) * 0x4000u)))
#define SCREENBLOCK_BASE(block) \
    ((volatile u16 *)(g_hostBgVram + ((unsigned)(block) * 0x800u)))

void HostVram_Reset(void);

#endif
