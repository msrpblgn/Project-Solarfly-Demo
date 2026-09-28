#ifndef GBA_TYPES_H
#define GBA_TYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

#define RGB15(r, g, b) ((u16)(((r) & 31) | (((g) & 31) << 5) | (((b) & 31) << 10)))

#define REG_DISPCNT (*(volatile u16 *)0x04000000)
#define REG_BG0CNT (*(volatile u16 *)0x04000008)
#define REG_BG1CNT (*(volatile u16 *)0x0400000A)
#define REG_BG2CNT (*(volatile u16 *)0x0400000C)
#define REG_BG3CNT (*(volatile u16 *)0x0400000E)
#define REG_BG0HOFS (*(volatile u16 *)0x04000010)
#define REG_BG0VOFS (*(volatile u16 *)0x04000012)
#define REG_BG1HOFS (*(volatile u16 *)0x04000014)
#define REG_BG1VOFS (*(volatile u16 *)0x04000016)
#define REG_BG2HOFS (*(volatile u16 *)0x04000018)
#define REG_BG2VOFS (*(volatile u16 *)0x0400001A)
#define REG_BG3HOFS (*(volatile u16 *)0x0400001C)
#define REG_BG3VOFS (*(volatile u16 *)0x0400001E)
#define REG_WIN0H (*(volatile u16 *)0x04000040)
#define REG_WIN1H (*(volatile u16 *)0x04000042)
#define REG_WIN0V (*(volatile u16 *)0x04000044)
#define REG_WIN1V (*(volatile u16 *)0x04000046)
#define REG_WININ (*(volatile u16 *)0x04000048)
#define REG_WINOUT (*(volatile u16 *)0x0400004A)
#define REG_VCOUNT (*(volatile u16 *)0x04000006)
#define REG_KEYINPUT (*(volatile u16 *)0x04000130)
#define REG_KEYCNT (*(volatile u16 *)0x04000132)

#define MODE_0 0
#define MODE_3 3
/* DISPCNT bits: mode 0-2, forced blank (bit 7), BG0-BG3 (bits 8-11) */
#define DCNT_MODE0 (MODE_0)
#define DCNT_BG0 (1 << 8)
#define DCNT_BG1 (1 << 9)
#define DCNT_MODE3 (MODE_3)
#define DCNT_BG2 (1 << 10)
#define DCNT_BG3 (1 << 11)
#define DCNT_OBJ (1 << 12)
#define DCNT_OBJ_1D (1 << 6)
#define DCNT_WIN0 (1 << 13)
#define DCNT_WIN1 (1 << 14)
#define DCNT_FORCED_BLANK (1 << 7)
/* Mode 3 framebuffer is displayed through BG2; forced blank must be off. */
#define DCNT_MODE3_BITMAP (DCNT_MODE3 | DCNT_BG2)
/* Mode 0 overworld field: BG3 ground + OBJ with 1D tile mapping. */
#define DCNT_MODE0_FIELD_BG3_OBJ (DCNT_MODE0 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D)

#define OBJ_VRAM ((volatile u16 *)0x06010000)
#define OBJ_PALETTE ((volatile u16 *)0x05000200)
#define OAM ((volatile u16 *)0x07000000)

#define BGCNT_PRIORITY(n) ((n) & 3)
#define BGCNT_CHARBASE(n) (((n) & 3) << 2)
#define BGCNT_SCREENBASE(n) (((n) & 31) << 8)
#define BGCNT_4BPP 0
#define BGCNT_SIZE_256X256 0

/* WININ/WINOUT layer enable bits (per window half). */
#define WIN_BG0 (1 << 0)
#define WIN_BG1 (1 << 1)
#define WIN_BG2 (1 << 2)
#define WIN_BG3 (1 << 3)
#define WIN_OBJ (1 << 4)
#define WIN_BLEND (1 << 5)

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#define VRAM ((volatile u16 *)0x06000000)
#define BG_PALETTE ((volatile u16 *)0x05000000)
#define CHARBLOCK_BASE(block) ((volatile u16 *)(0x06000000 + ((block) * 0x4000)))
#define SCREENBLOCK_BASE(block) ((volatile u16 *)(0x06000000 + ((block) * 0x800)))

#define KEY_A (1 << 0)
#define KEY_B (1 << 1)
#define KEY_SELECT (1 << 2)
#define KEY_START (1 << 3)
#define KEY_RIGHT (1 << 4)
#define KEY_LEFT (1 << 5)
#define KEY_UP (1 << 6)
#define KEY_DOWN (1 << 7)
#define KEY_R (1 << 8)
#define KEY_L (1 << 9)

#define KEY_MASK (KEY_A | KEY_B | KEY_SELECT | KEY_START | KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN | KEY_R | KEY_L)

#endif
