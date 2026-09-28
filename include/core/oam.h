#ifndef OAM_H
#define OAM_H

#include "core/gba_types.h"

#define OAM_ENTRY_COUNT 128
#define OAM_AFFINE_COUNT 32
#define OAM_INDEX_OVERWORLD_PLAYER 0

#define OAM_ATTR0_Y_MASK 0x00FF
/* ATTR0 bits 8-9: 00=reg, 01=affine, 10=disabled, 11=affine+double-size */
#define OAM_ATTR0_MODE_AFFINE (1 << 8)
#define OAM_ATTR0_MODE_DISABLED (1 << 9)
#define OAM_ATTR0_MODE_AFFINE_DBL ((1 << 8) | (1 << 9))
#define OAM_ATTR0_OBJ_DISABLE OAM_ATTR0_MODE_DISABLED
#define OAM_ATTR0_COLOR_16 0
#define OAM_ATTR0_SHAPE_SQUARE 0
#define OAM_ATTR0_SHAPE_WIDE (1 << 14)
#define OAM_ATTR0_SHAPE_TALL (2 << 14)

#define OAM_ATTR1_X_MASK 0x01FF
#define OAM_ATTR1_AFFINE_ID_SHIFT 9
#define OAM_ATTR1_HFLIP (1 << 12)
#define OAM_ATTR1_VFLIP (1 << 13)
#define OAM_ATTR1_SIZE_SHIFT 14

#define OAM_ATTR2_TILE_MASK 0x03FF
#define OAM_ATTR2_PRIORITY_SHIFT 10
#define OAM_ATTR2_PALBANK_SHIFT 12

/* Tall size 2 = 16x32 pixels. Square size 3 = 64x64. */
#define OAM_SIZE_TALL_16X32 2
#define OAM_SIZE_SQUARE_64X64 3

/* 8.8 fixed: 256 = 1.0 identity; 128 = 0.5 inverse => 2x on-screen scale. */
#define OAM_AFFINE_IDENTITY 256
#define OAM_AFFINE_SCALE_2X 128

typedef struct OamEntry {
    u16 attr0;
    u16 attr1;
    u16 attr2;
    u16 fill;
} OamEntry;

_Static_assert(sizeof(OamEntry) == 8, "OAM entry must be 8 bytes");
_Static_assert(
    sizeof(OamEntry) * OAM_ENTRY_COUNT == 1024,
    "Shadow OAM must be 1024 bytes");

void Oam_InitHidden(void);
void Oam_Hide(int index);
void Oam_SetRegular(
    int index,
    int screenX,
    int screenY,
    int tileIndex,
    int paletteBank,
    int priority,
    u16 shapeAttr0,
    int sizeCode,
    int hFlip,
    int vFlip);

/*
 * Affine object. screenX/screenY are signed presentation top-left of the
 * double-size rendering box when useDoubleSize is nonzero.
 * Does not modify the fill field (affine matrix coefficients live there).
 */
void Oam_SetAffine(
    int index,
    int screenX,
    int screenY,
    int tileIndex,
    int paletteBank,
    int priority,
    int affineId,
    int sizeCode,
    int useDoubleSize);

/* Writes pa/pb/pc/pd into matrix affineId without touching object attrs. */
void Oam_SetAffineMatrix(int affineId, s16 pa, s16 pb, s16 pc, s16 pd);

void Oam_Commit(void);

#endif
