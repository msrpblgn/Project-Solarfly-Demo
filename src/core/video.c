#include "core/video.h"

#ifdef VIDEO_HOST_FRAMEBUFFER
/* Host integration builds redirect Mode 3 writes to a RAM framebuffer. */
u16 g_videoHostFb[SCREEN_WIDTH * SCREEN_HEIGHT];
/* Synthetic VCOUNT for present-cost modeling (advanced by pixel writes). */
volatile u16 g_hostRegVCount;
#undef VRAM
#define VRAM ((volatile u16 *)g_videoHostFb)
#undef REG_VCOUNT
#define REG_VCOUNT (g_hostRegVCount)
u16 *Video_HostFramebuffer(void)
{
    return g_videoHostFb;
}
void Video_HostSetVCount(u16 vcount)
{
    g_hostRegVCount = vcount;
}
u16 Video_HostGetVCount(void)
{
    return g_hostRegVCount;
}
static void Video_HostAdvanceVCountPixels(int pixels)
{
    /* Crude model: one Mode-3 scanline worth of FB pixels ≈ one VCOUNT step. */
    int lines = pixels / SCREEN_WIDTH;
    if (lines <= 0 && pixels > 0)
    {
        lines = 1;
    }
    g_hostRegVCount = (u16)((g_hostRegVCount + lines) % 228);
}
#endif

#define VIDEO_FULL_VRAM_U16_COUNT 0x18000 / 2
#define VIDEO_BG_VRAM_U16_COUNT 0x10000 / 2
#define VIDEO_BG_PALETTE_U16_COUNT 256

static const u8 s_font5x7[][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* A */
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, /* B */
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, /* C */
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, /* D */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, /* E */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, /* F */
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}, /* G */
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* H */
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* I */
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, /* J */
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, /* K */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, /* L */
    {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11}, /* M */
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, /* N */
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* O */
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, /* P */
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, /* Q */
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, /* R */
    {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E}, /* S */
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* T */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* U */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, /* V */
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, /* W */
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, /* X */
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, /* Y */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, /* Z */
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, /* 0 */
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* 1 */
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F}, /* 2 */
    {0x1F, 0x01, 0x02, 0x06, 0x01, 0x11, 0x0E}, /* 3 */
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, /* 4 */
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, /* 5 */
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, /* 6 */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, /* 7 */
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, /* 8 */
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, /* 9 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, /* - */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}, /* . */
    {0x00, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x00}, /* : */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04}, /* / */
    {0x19, 0x1A, 0x04, 0x08, 0x10, 0x0B, 0x0B}, /* % */
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}, /* + */
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}, /* ( */
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}, /* ) */
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, /* ? */
};

static int Video_GetGlyphIndex(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z')
    {
        return c - 'a';
    }
    if (c >= '0' && c <= '9')
    {
        return 26 + (c - '0');
    }
    if (c == ' ')
    {
        return 36;
    }
    if (c == '-')
    {
        return 37;
    }
    if (c == '.')
    {
        return 38;
    }
    if (c == ':')
    {
        return 39;
    }
    if (c == '/')
    {
        return 40;
    }
    if (c == '%')
    {
        return 41;
    }
    if (c == '+')
    {
        return 42;
    }
    if (c == '(')
    {
        return 43;
    }
    if (c == ')')
    {
        return 44;
    }
    if (c == '?')
    {
        return 45;
    }
    return 36;
}

int Video_IsFontCharSupported(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return 1;
    }
    if (c >= 'a' && c <= 'z')
    {
        return 1;
    }
    if (c >= '0' && c <= '9')
    {
        return 1;
    }
    if (c == ' ' || c == '-' || c == '.' || c == ':' || c == '/' || c == '%'
        || c == '+' || c == '(' || c == ')' || c == '?')
    {
        return 1;
    }
    return 0;
}

const u8 *Video_GetFontGlyphRows(char c)
{
    return s_font5x7[Video_GetGlyphIndex(c)];
}

static void Video_ClearHalfwords(volatile u16 *dest, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        dest[i] = 0;
    }
}

static void Video_CopyHalfwords(volatile u16 *dest, const u16 *src, int count)
{
    int i;

    if (!dest || !src || count <= 0)
    {
        return;
    }
    for (i = 0; i < count; i++)
    {
        dest[i] = src[i];
    }
}

static void Video_CopyWords(volatile u32 *dest, const u32 *src, int count)
{
    int i;

    if (!dest || !src || count <= 0)
    {
        return;
    }
    for (i = 0; i < count; i++)
    {
        dest[i] = src[i];
    }
}

static void Video_ResetBgState(void)
{
    REG_BG0CNT = 0;
    REG_BG1CNT = 0;
    REG_BG2CNT = 0;
    REG_BG3CNT = 0;
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
    REG_BG1HOFS = 0;
    REG_BG1VOFS = 0;
    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0;
    REG_BG3HOFS = 0;
    REG_BG3VOFS = 0;
}

void Video_Init(void)
{
    Video_EnterMode3Bitmap();
}

void Video_EnterMode3Bitmap(void)
{
    REG_DISPCNT = DCNT_FORCED_BLANK;
    Video_ResetBgState();
    Video_ClearHalfwords(BG_PALETTE, VIDEO_BG_PALETTE_U16_COUNT);
    Video_ClearHalfwords(VRAM, VIDEO_FULL_VRAM_U16_COUNT);
    /* Mode 3: 240x160 16-bit bitmap via BG2. Do not use BG0 for this framebuffer. */
    REG_DISPCNT = DCNT_MODE3_BITMAP;
}

void Video_BeginMode0FieldBg3(
    const u16 *palette,
    int paletteSize,
    const u32 *tileData,
    int tileWordCount)
{
    volatile u32 *charBase = (volatile u32 *)CHARBLOCK_BASE(VIDEO_MODE0_FIELD_CHARBLOCK);

    REG_DISPCNT = DCNT_FORCED_BLANK;
    Video_ResetBgState();
    Video_ClearHalfwords(BG_PALETTE, VIDEO_BG_PALETTE_U16_COUNT);
    Video_ClearHalfwords(VRAM, VIDEO_BG_VRAM_U16_COUNT);
    Video_CopyHalfwords(BG_PALETTE, palette, paletteSize);
    Video_CopyWords(charBase, tileData, tileWordCount);
    REG_BG3CNT = BGCNT_PRIORITY(3)
        | BGCNT_CHARBASE(VIDEO_MODE0_FIELD_CHARBLOCK)
        | BGCNT_4BPP
        | BGCNT_SCREENBASE(VIDEO_MODE0_FIELD_SCREENBLOCK)
        | BGCNT_SIZE_256X256;
    REG_BG3HOFS = 0;
    REG_BG3VOFS = 0;
}

void Video_LoadObjPaletteBank(int bank, const u16 *palette, int colorCount)
{
    if (!palette || bank < 0 || bank > 15 || colorCount <= 0)
    {
        return;
    }
    if (colorCount > 16)
    {
        colorCount = 16;
    }
    Video_CopyHalfwords(OBJ_PALETTE + (bank * 16), palette, colorCount);
}

void Video_LoadObjTiles4bpp(int tileIndex, const u32 *tileData, int tileWordCount)
{
    volatile u32 *dest;

    if (!tileData || tileIndex < 0 || tileWordCount <= 0)
    {
        return;
    }
    /* 4bpp OBJ tiles are 32 bytes (8 words) each, starting at OBJ VRAM. */
    dest = (volatile u32 *)(OBJ_VRAM + (tileIndex * 16));
    Video_CopyWords(dest, tileData, tileWordCount);
}

void Video_EndMode0FieldBg3(void)
{
    REG_DISPCNT = DCNT_MODE0_FIELD_BG3_OBJ;
}

void Video_SetMode0Bg3Scroll(int x, int y)
{
    REG_BG3HOFS = (u16)x;
    REG_BG3VOFS = (u16)y;
}

volatile u16 *Video_GetMode0Bg3Screenbase(void)
{
    return SCREENBLOCK_BASE(VIDEO_MODE0_FIELD_SCREENBLOCK);
}

void Video_Clear(u16 color)
{
    int i;
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        VRAM[i] = color;
    }
}

void Video_DrawRect(int x, int y, int w, int h, u16 color)
{
    int row;
    int col;
    int px;
    int py;
#ifdef VIDEO_HOST_FRAMEBUFFER
    int written = 0;
#endif

    for (row = 0; row < h; row++)
    {
        py = y + row;
        if (py < 0 || py >= SCREEN_HEIGHT)
        {
            continue;
        }
        for (col = 0; col < w; col++)
        {
            px = x + col;
            if (px < 0 || px >= SCREEN_WIDTH)
            {
                continue;
            }
            VRAM[py * SCREEN_WIDTH + px] = color;
#ifdef VIDEO_HOST_FRAMEBUFFER
            written++;
#endif
        }
    }
#ifdef VIDEO_HOST_FRAMEBUFFER
    Video_HostAdvanceVCountPixels(written);
#endif
}

static void Video_DrawGlyph(int x, int y, int glyphIndex, u16 color)
{
    int row;
    int col;
    const u8 *glyph = s_font5x7[glyphIndex];

    for (row = 0; row < 7; row++)
    {
        for (col = 0; col < 5; col++)
        {
            if (glyph[row] & (1 << (4 - col)))
            {
                Video_DrawRect(x + col, y + row, 1, 1, color);
            }
        }
    }
}

static void Video_DrawGlyphClipped(
    int x,
    int y,
    int glyphIndex,
    u16 color,
    int clipX,
    int clipRight)
{
    int row;
    int col;
    const u8 *glyph = s_font5x7[glyphIndex];

    for (row = 0; row < 7; row++)
    {
        for (col = 0; col < 5; col++)
        {
            if (glyph[row] & (1 << (4 - col)))
            {
                int px = x + col;
                int py = y + row;

                if (px >= clipX && px < clipRight && px >= 0 && px < SCREEN_WIDTH &&
                    py >= 0 && py < SCREEN_HEIGHT)
                {
                    Video_DrawRect(px, py, 1, 1, color);
                }
            }
        }
    }
}

void Video_DrawText(int x, int y, const char *text, u16 color)
{
    int cursorX = x;
    while (*text)
    {
        Video_DrawGlyph(cursorX, y, Video_GetGlyphIndex(*text), color);
        cursorX += 6;
        text++;
    }
}

int Video_MeasureTextWidth(const char *text)
{
    int count = 0;

    while (text && text[count])
    {
        count++;
    }
    return count * 6;
}

void Video_DrawTextClipped(
    int x,
    int y,
    const char *text,
    u16 color,
    int clipX,
    int clipW,
    int scrollOffset)
{
    int cursorX = x - scrollOffset;
    int clipRight = clipX + clipW;

    while (*text)
    {
        Video_DrawGlyphClipped(
            cursorX,
            y,
            Video_GetGlyphIndex(*text),
            color,
            clipX,
            clipRight);
        cursorX += 6;
        text++;
    }
}

void Video_DrawBitmap(int x, int y, int w, int h, const u16 *pixels)
{
    int row;
    int col;

    if (!pixels || w <= 0 || h <= 0)
    {
        return;
    }

    for (row = 0; row < h; row++)
    {
        int py = y + row;
        if (py < 0 || py >= SCREEN_HEIGHT)
        {
            continue;
        }
        for (col = 0; col < w; col++)
        {
            int px = x + col;
            if (px < 0 || px >= SCREEN_WIDTH)
            {
                continue;
            }
            VRAM[py * SCREEN_WIDTH + px] = pixels[(row * w) + col];
        }
    }
}

void Video_BlitBitmapRegion(
    int destX,
    int destY,
    int w,
    int h,
    const u16 *srcPixels,
    int srcW,
    int srcH,
    int srcX,
    int srcY)
{
    int row;
    int col;
#ifdef VIDEO_HOST_FRAMEBUFFER
    int written = 0;
#endif

    if (!srcPixels || w <= 0 || h <= 0 || srcW <= 0 || srcH <= 0)
    {
        return;
    }

    for (row = 0; row < h; row++)
    {
        int py = destY + row;
        int srcRow = srcY + row;

        if (py < 0 || py >= SCREEN_HEIGHT || srcRow < 0 || srcRow >= srcH)
        {
            continue;
        }
        for (col = 0; col < w; col++)
        {
            int px = destX + col;
            int srcCol = srcX + col;

            if (px < 0 || px >= SCREEN_WIDTH || srcCol < 0 || srcCol >= srcW)
            {
                continue;
            }
            VRAM[py * SCREEN_WIDTH + px] = srcPixels[(srcRow * srcW) + srcCol];
#ifdef VIDEO_HOST_FRAMEBUFFER
            written++;
#endif
        }
    }
#ifdef VIDEO_HOST_FRAMEBUFFER
    Video_HostAdvanceVCountPixels(written);
#endif
}

void Video_DrawBitmapKeyColor(int x, int y, int w, int h, const u16 *pixels, u16 keyColor)
{
    int row;
    int col;

    if (!pixels || w <= 0 || h <= 0)
    {
        return;
    }

    for (row = 0; row < h; row++)
    {
        int py = y + row;
        if (py < 0 || py >= SCREEN_HEIGHT)
        {
            continue;
        }
        for (col = 0; col < w; col++)
        {
            u16 color = pixels[(row * w) + col];
            int px = x + col;

            if (color == keyColor)
            {
                continue;
            }
            if (px < 0 || px >= SCREEN_WIDTH)
            {
                continue;
            }
            VRAM[py * SCREEN_WIDTH + px] = color;
        }
    }
}

void Video_DrawBitmapKeyColorRegion(
    int destX,
    int destY,
    int clipX,
    int clipY,
    int clipW,
    int clipH,
    int srcBitmapW,
    int srcCropX,
    int srcCropY,
    int cropW,
    int cropH,
    const u16 *pixels,
    u16 keyColor)
{
    int row;
    int col;

    if (!pixels || cropW <= 0 || cropH <= 0 || srcBitmapW <= 0 || clipW <= 0 || clipH <= 0)
    {
        return;
    }

    for (row = 0; row < cropH; row++)
    {
        int py = destY + row;
        int srcRow = srcCropY + row;

        if (py < clipY || py >= clipY + clipH || py < 0 || py >= SCREEN_HEIGHT)
        {
            continue;
        }
        if (srcRow < 0)
        {
            continue;
        }

        for (col = 0; col < cropW; col++)
        {
            int px = destX + col;
            int srcCol = srcCropX + col;
            u16 color;

            if (px < clipX || px >= clipX + clipW || px < 0 || px >= SCREEN_WIDTH)
            {
                continue;
            }
            if (srcCol < 0 || srcCol >= srcBitmapW)
            {
                continue;
            }

            color = pixels[(srcRow * srcBitmapW) + srcCol];
            if (color == keyColor)
            {
                continue;
            }
            VRAM[py * SCREEN_WIDTH + px] = color;
        }
    }
}
