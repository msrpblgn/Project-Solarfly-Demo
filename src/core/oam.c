#include "core/oam.h"

#define OAM_HARDWARE ((volatile u32 *)0x07000000)

static OamEntry s_shadowOam[OAM_ENTRY_COUNT];

_Static_assert(sizeof(s_shadowOam) == 1024, "Shadow OAM RAM must be 1024 bytes");

void Oam_InitHidden(void)
{
    int i;

    for (i = 0; i < OAM_ENTRY_COUNT; i++)
    {
        s_shadowOam[i].attr0 = OAM_ATTR0_OBJ_DISABLE;
        s_shadowOam[i].attr1 = 0;
        s_shadowOam[i].attr2 = 0;
        s_shadowOam[i].fill = 0;
    }
}

void Oam_Hide(int index)
{
    if (index < 0 || index >= OAM_ENTRY_COUNT)
    {
        return;
    }
    /* Preserve fill so affine matrices in shared OAM memory stay intact. */
    s_shadowOam[index].attr0 = OAM_ATTR0_OBJ_DISABLE;
    s_shadowOam[index].attr1 = 0;
    s_shadowOam[index].attr2 = 0;
}

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
    int vFlip)
{
    u16 attr0;
    u16 attr1;
    u16 attr2;

    if (index < 0 || index >= OAM_ENTRY_COUNT)
    {
        return;
    }

    attr0 = (u16)((screenY & OAM_ATTR0_Y_MASK) | OAM_ATTR0_COLOR_16 | shapeAttr0);
    attr1 = (u16)((screenX & OAM_ATTR1_X_MASK)
        | ((sizeCode & 3) << OAM_ATTR1_SIZE_SHIFT));
    if (hFlip)
    {
        attr1 |= OAM_ATTR1_HFLIP;
    }
    if (vFlip)
    {
        attr1 |= OAM_ATTR1_VFLIP;
    }
    attr2 = (u16)((tileIndex & OAM_ATTR2_TILE_MASK)
        | ((priority & 3) << OAM_ATTR2_PRIORITY_SHIFT)
        | ((paletteBank & 15) << OAM_ATTR2_PALBANK_SHIFT));

    s_shadowOam[index].attr0 = attr0;
    s_shadowOam[index].attr1 = attr1;
    s_shadowOam[index].attr2 = attr2;
    /* Regular objects: clear fill (not part of an active affine matrix write). */
    s_shadowOam[index].fill = 0;
}

void Oam_SetAffine(
    int index,
    int screenX,
    int screenY,
    int tileIndex,
    int paletteBank,
    int priority,
    int affineId,
    int sizeCode,
    int useDoubleSize)
{
    u16 attr0;
    u16 attr1;
    u16 attr2;
    u16 mode;

    if (index < 0 || index >= OAM_ENTRY_COUNT)
    {
        return;
    }
    if (affineId < 0 || affineId >= OAM_AFFINE_COUNT)
    {
        return;
    }

    mode = useDoubleSize ? OAM_ATTR0_MODE_AFFINE_DBL : OAM_ATTR0_MODE_AFFINE;
    attr0 = (u16)((screenY & OAM_ATTR0_Y_MASK) | mode | OAM_ATTR0_COLOR_16
        | OAM_ATTR0_SHAPE_SQUARE);
    attr1 = (u16)((screenX & OAM_ATTR1_X_MASK)
        | ((affineId & 31) << OAM_ATTR1_AFFINE_ID_SHIFT)
        | ((sizeCode & 3) << OAM_ATTR1_SIZE_SHIFT));
    attr2 = (u16)((tileIndex & OAM_ATTR2_TILE_MASK)
        | ((priority & 3) << OAM_ATTR2_PRIORITY_SHIFT)
        | ((paletteBank & 15) << OAM_ATTR2_PALBANK_SHIFT));

    s_shadowOam[index].attr0 = attr0;
    s_shadowOam[index].attr1 = attr1;
    s_shadowOam[index].attr2 = attr2;
    /* fill left untouched — matrix coefficients. */
}

void Oam_SetAffineMatrix(int affineId, s16 pa, s16 pb, s16 pc, s16 pd)
{
    int base;

    if (affineId < 0 || affineId >= OAM_AFFINE_COUNT)
    {
        return;
    }
    /* Matrix N occupies the fill fields of objects 4N .. 4N+3. */
    base = affineId * 4;
    s_shadowOam[base + 0].fill = (u16)pa;
    s_shadowOam[base + 1].fill = (u16)pb;
    s_shadowOam[base + 2].fill = (u16)pc;
    s_shadowOam[base + 3].fill = (u16)pd;
}

void Oam_Commit(void)
{
    volatile u32 *dest = OAM_HARDWARE;
    const u32 *src = (const u32 *)s_shadowOam;
    int wordCount = OAM_ENTRY_COUNT * 2;
    int i;

    for (i = 0; i < wordCount; i++)
    {
        dest[i] = src[i];
    }
}
