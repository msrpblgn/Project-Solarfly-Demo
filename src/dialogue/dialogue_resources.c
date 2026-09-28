#include "dialogue/dialogue_resources.h"

#define DIALOGUE_FIELD_CHAR_START \
    BG_VRAM_CHARBLOCK_BYTE_START(VIDEO_MODE0_FIELD_CHARBLOCK)
#define DIALOGUE_FIELD_CHAR_END \
    BG_VRAM_CHARBLOCK_BYTE_END(VIDEO_MODE0_FIELD_CHARBLOCK)
#define DIALOGUE_FIELD_SCREEN_START \
    BG_VRAM_SCREENBLOCK_BYTE_START(VIDEO_MODE0_FIELD_SCREENBLOCK)
#define DIALOGUE_FIELD_SCREEN_END \
    BG_VRAM_SCREENBLOCK_BYTE_END(VIDEO_MODE0_FIELD_SCREENBLOCK)
#define DIALOGUE_CHAR_START \
    BG_VRAM_CHARBLOCK_BYTE_START(DIALOGUE_MODE0_BG_CHARBLOCK)
#define DIALOGUE_CHAR_END \
    BG_VRAM_CHARBLOCK_BYTE_END(DIALOGUE_MODE0_BG_CHARBLOCK)
#define DIALOGUE_SCREEN_START \
    BG_VRAM_SCREENBLOCK_BYTE_START(DIALOGUE_MODE0_BG_SCREENBLOCK)
#define DIALOGUE_SCREEN_END \
    BG_VRAM_SCREENBLOCK_BYTE_END(DIALOGUE_MODE0_BG_SCREENBLOCK)

/* Compile-time reservation validation (Milestone 0). */

_Static_assert(
    VIDEO_MODE0_FIELD_CHARBLOCK < BG_VRAM_CHARBLOCK_COUNT,
    "Field charblock index must be within GBA limits");
_Static_assert(
    VIDEO_MODE0_FIELD_SCREENBLOCK < BG_VRAM_SCREENBLOCK_COUNT,
    "Field screenblock index must be within GBA limits");
_Static_assert(
    DIALOGUE_MODE0_BG_CHARBLOCK < BG_VRAM_CHARBLOCK_COUNT,
    "Dialogue charblock index must be within GBA limits");
_Static_assert(
    DIALOGUE_MODE0_BG_SCREENBLOCK < BG_VRAM_SCREENBLOCK_COUNT,
    "Dialogue screenblock index must be within GBA limits");

_Static_assert(
    (BG_VRAM_CHARBLOCK_BYTE_SIZE % BG_VRAM_SCREENBLOCK_BYTE_SIZE) == 0,
    "Character block size must be screenblock-aligned");
_Static_assert(
    BG_VRAM_CHARBLOCK_BYTE_SIZE
        == (BG_VRAM_CHARBLOCK_SCREENBLOCK_SPAN * BG_VRAM_SCREENBLOCK_BYTE_SIZE),
    "Each character block must span eight screenblocks");

_Static_assert(
    !BG_VRAM_INTERVALS_OVERLAP(
        DIALOGUE_FIELD_CHAR_START,
        DIALOGUE_FIELD_CHAR_END,
        DIALOGUE_CHAR_START,
        DIALOGUE_CHAR_END),
    "Field and dialogue character blocks must not overlap in BG VRAM");
_Static_assert(
    !BG_VRAM_INTERVALS_OVERLAP(
        DIALOGUE_FIELD_CHAR_START,
        DIALOGUE_FIELD_CHAR_END,
        DIALOGUE_SCREEN_START,
        DIALOGUE_SCREEN_END),
    "Field character block must not overlap dialogue screenblock");
_Static_assert(
    !BG_VRAM_INTERVALS_OVERLAP(
        DIALOGUE_FIELD_SCREEN_START,
        DIALOGUE_FIELD_SCREEN_END,
        DIALOGUE_CHAR_START,
        DIALOGUE_CHAR_END),
    "Field screenblock must not overlap dialogue character block");
_Static_assert(
    !BG_VRAM_INTERVALS_OVERLAP(
        DIALOGUE_FIELD_SCREEN_START,
        DIALOGUE_FIELD_SCREEN_END,
        DIALOGUE_SCREEN_START,
        DIALOGUE_SCREEN_END),
    "Field and dialogue screenblocks must not overlap in BG VRAM");
_Static_assert(
    !BG_VRAM_INTERVALS_OVERLAP(
        DIALOGUE_CHAR_START,
        DIALOGUE_CHAR_END,
        DIALOGUE_SCREEN_START,
        DIALOGUE_SCREEN_END),
    "Dialogue character block must not overlap dialogue screenblock");

_Static_assert(
    BG_VRAM_SCREENBLOCK_OUTSIDE_CHARBLOCK(
        DIALOGUE_MODE0_BG_SCREENBLOCK,
        VIDEO_MODE0_FIELD_CHARBLOCK),
    "Dialogue screenblock must lie outside field character block span");
_Static_assert(
    BG_VRAM_SCREENBLOCK_OUTSIDE_CHARBLOCK(
        DIALOGUE_MODE0_BG_SCREENBLOCK,
        DIALOGUE_MODE0_BG_CHARBLOCK),
    "Dialogue screenblock must lie outside dialogue character block span");
_Static_assert(
    BG_VRAM_SCREENBLOCK_OUTSIDE_CHARBLOCK(
        VIDEO_MODE0_FIELD_SCREENBLOCK,
        VIDEO_MODE0_FIELD_CHARBLOCK),
    "Field screenblock must lie outside field character block span");
_Static_assert(
    BG_VRAM_SCREENBLOCK_OUTSIDE_CHARBLOCK(
        VIDEO_MODE0_FIELD_SCREENBLOCK,
        DIALOGUE_MODE0_BG_CHARBLOCK),
    "Field screenblock must lie outside dialogue character block span");

_Static_assert(
    DIALOGUE_MODE0_PANEL_TILE_COUNT <= DIALOGUE_MODE0_CHARBLOCK_TILE_CAPACITY,
    "Dialogue panel tile count must fit in one 4bpp charblock");
_Static_assert(
    (DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE + DIALOGUE_MODE0_BG_PALETTE_COLOR_COUNT)
        <= 256,
    "Dialogue BG palette reservation must fit in BG palette RAM");
_Static_assert(
    DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE >= 16,
    "Dialogue BG palette reservation must not overlap overworld indices 0-15");

_Static_assert(
    DIALOGUE_OAM_INDEX_LEFT_PORTRAIT != OAM_INDEX_OVERWORLD_PLAYER
        && DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT != OAM_INDEX_OVERWORLD_PLAYER
        && DIALOGUE_OAM_INDEX_LEFT_PORTRAIT != DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT,
    "Dialogue portrait OAM indices must not overlap overworld player or each other");
_Static_assert(
    DIALOGUE_OAM_INDEX_LEFT_PORTRAIT >= 0
        && DIALOGUE_OAM_INDEX_LEFT_PORTRAIT < OAM_ENTRY_COUNT
        && DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT >= 0
        && DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT < OAM_ENTRY_COUNT,
    "Dialogue portrait OAM indices must be within hardware OAM range");

_Static_assert(
    DIALOGUE_OBJ_PALETTE_BANK_LEFT != OVERWORLD_PLAYER_SPRITE_PALETTE_BANK
        && DIALOGUE_OBJ_PALETTE_BANK_RIGHT != OVERWORLD_PLAYER_SPRITE_PALETTE_BANK
        && DIALOGUE_OBJ_PALETTE_BANK_LEFT != DIALOGUE_OBJ_PALETTE_BANK_RIGHT,
    "Dialogue OBJ palette banks must not overlap overworld player or each other");
_Static_assert(
    DIALOGUE_OBJ_PALETTE_BANK_LEFT >= 0 && DIALOGUE_OBJ_PALETTE_BANK_LEFT <= 15
        && DIALOGUE_OBJ_PALETTE_BANK_RIGHT >= 0
        && DIALOGUE_OBJ_PALETTE_BANK_RIGHT <= 15,
    "Dialogue OBJ palette banks must be valid hardware banks");

/* Milestone 5: affine matrix must not alias portrait object indices 1–2. */
_Static_assert(
    (DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT * 4) > DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT,
    "Portrait affine matrix fill range must not overlap portrait OAM entries");
_Static_assert(
    DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT >= 0
        && DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT < 32,
    "Portrait affine matrix id must be valid");
