#ifndef VIDEO_H
#define VIDEO_H

#include "core/gba_types.h"

/* Mode 0 overworld field BG3 layout (see docs/DIALOGUE_RESOURCE_MAP.md). */
#define VIDEO_MODE0_FIELD_CHARBLOCK 0
#define VIDEO_MODE0_FIELD_SCREENBLOCK 31

void Video_Init(void);
void Video_EnterMode3Bitmap(void);
void Video_BeginMode0FieldBg3(
    const u16 *palette,
    int paletteSize,
    const u32 *tileData,
    int tileWordCount);
void Video_LoadObjPaletteBank(int bank, const u16 *palette, int colorCount);
void Video_LoadObjTiles4bpp(int tileIndex, const u32 *tileData, int tileWordCount);
void Video_EndMode0FieldBg3(void);
void Video_SetMode0Bg3Scroll(int x, int y);
volatile u16 *Video_GetMode0Bg3Screenbase(void);
void Video_Clear(u16 color);
void Video_DrawRect(int x, int y, int w, int h, u16 color);
void Video_DrawText(int x, int y, const char *text, u16 color);
int Video_MeasureTextWidth(const char *text);
void Video_DrawTextClipped(
    int x,
    int y,
    const char *text,
    u16 color,
    int clipX,
    int clipW,
    int scrollOffset);
void Video_DrawBitmap(int x, int y, int w, int h, const u16 *pixels);
void Video_BlitBitmapRegion(
    int destX,
    int destY,
    int w,
    int h,
    const u16 *srcPixels,
    int srcW,
    int srcH,
    int srcX,
    int srcY);
void Video_DrawBitmapKeyColor(int x, int y, int w, int h, const u16 *pixels, u16 keyColor);
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
    u16 keyColor);

/* Engine 5x7 font metrics / accessors shared by Mode 3 and Mode 0 UI. */
#define VIDEO_FONT_GLYPH_W 5
#define VIDEO_FONT_GLYPH_H 7
#define VIDEO_FONT_ADVANCE 6
const u8 *Video_GetFontGlyphRows(char c);
/* Returns 1 if c maps to a dedicated glyph (not the unknown→space fallback). */
int Video_IsFontCharSupported(char c);

#ifdef VIDEO_HOST_FRAMEBUFFER
u16 *Video_HostFramebuffer(void);
void Video_HostSetVCount(u16 vcount);
u16 Video_HostGetVCount(void);
#endif

#endif
