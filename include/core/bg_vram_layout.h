#ifndef BG_VRAM_LAYOUT_H
#define BG_VRAM_LAYOUT_H

/*
 * GBA Mode 0 text-background VRAM is one 64 KiB address space.
 * Character blocks (16 KiB) and screenblocks (2 KiB) alias the same bytes.
 * Each character block spans eight consecutive screenblocks.
 */

#define BG_VRAM_BYTE_SIZE 0x10000u
#define BG_VRAM_CHARBLOCK_BYTE_SIZE 0x4000u
#define BG_VRAM_SCREENBLOCK_BYTE_SIZE 0x800u
#define BG_VRAM_CHARBLOCK_COUNT 4u
#define BG_VRAM_SCREENBLOCK_COUNT 32u
#define BG_VRAM_CHARBLOCK_SCREENBLOCK_SPAN 8u

#define BG_VRAM_CHARBLOCK_BYTE_START(block) \
    ((unsigned)(block) * BG_VRAM_CHARBLOCK_BYTE_SIZE)
#define BG_VRAM_CHARBLOCK_BYTE_END(block) \
    (BG_VRAM_CHARBLOCK_BYTE_START(block) + BG_VRAM_CHARBLOCK_BYTE_SIZE)

#define BG_VRAM_SCREENBLOCK_BYTE_START(block) \
    ((unsigned)(block) * BG_VRAM_SCREENBLOCK_BYTE_SIZE)
#define BG_VRAM_SCREENBLOCK_BYTE_END(block) \
    (BG_VRAM_SCREENBLOCK_BYTE_START(block) + BG_VRAM_SCREENBLOCK_BYTE_SIZE)

#define BG_VRAM_CHARBLOCK_FIRST_SCREENBLOCK(block) \
    ((unsigned)(block) * BG_VRAM_CHARBLOCK_SCREENBLOCK_SPAN)
#define BG_VRAM_CHARBLOCK_LAST_SCREENBLOCK(block) \
    (BG_VRAM_CHARBLOCK_FIRST_SCREENBLOCK(block) \
        + BG_VRAM_CHARBLOCK_SCREENBLOCK_SPAN - 1u)

/* Half-open interval overlap: [a_start, a_end) vs [b_start, b_end). */
#define BG_VRAM_INTERVALS_OVERLAP(a_start, a_end, b_start, b_end) \
    (((a_start) < (b_end)) && ((b_start) < (a_end)))

#define BG_VRAM_SCREENBLOCK_OUTSIDE_CHARBLOCK(screenblock, charblock) \
    ((BG_VRAM_SCREENBLOCK_BYTE_END(screenblock) \
         <= BG_VRAM_CHARBLOCK_BYTE_START(charblock)) \
        || (BG_VRAM_SCREENBLOCK_BYTE_START(screenblock) \
            >= BG_VRAM_CHARBLOCK_BYTE_END(charblock)))

#endif
