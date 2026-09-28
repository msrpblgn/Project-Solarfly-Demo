#include "core/video.h"

#include "core/gba_types.h"

/* Full 5px ink every row — guarantees cross-tile writes for dirty tests. */
static const u8 s_hostFontSolid[7] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
static const u8 s_hostFontSpace[7] = {0, 0, 0, 0, 0, 0, 0};

static int Host_IsSupported(char c)
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
        || c == '+' || c == '(' || c == ')' || c == '?' || c == ',')
    {
        return 1;
    }
    return 0;
}

int Video_IsFontCharSupported(char c)
{
    return Host_IsSupported(c);
}

const u8 *Video_GetFontGlyphRows(char c)
{
    if (c == ' ')
    {
        return s_hostFontSpace;
    }
    return s_hostFontSolid;
}

int Video_MeasureTextWidth(const char *text)
{
    int n = 0;

    if (!text)
    {
        return 0;
    }
    while (text[n] != '\0')
    {
        n++;
    }
    return n * VIDEO_FONT_ADVANCE;
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
    (void)destX;
    (void)destY;
    (void)w;
    (void)h;
    (void)srcPixels;
    (void)srcW;
    (void)srcH;
    (void)srcX;
    (void)srcY;
}
