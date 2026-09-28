#include "dialogue/dialogue_panel.h"

#include "assets/dialogue_frame.h"
#include "core/video.h"
#include "dialogue/dialogue_resources.h"

#ifdef DIALOGUE_HOST_TEST
#include "host_vram_shim.h"
#endif

#define DIALOGUE_PANEL_DYNAMIC_TILE_MAX 192
#define DIALOGUE_PANEL_COLOR_TRANSPARENT 0
#define DIALOGUE_PANEL_COLOR_BLACK 1
#define DIALOGUE_PANEL_COLOR_ORANGE 2
#define DIALOGUE_PANEL_COLOR_LIGHT_GOLD 3
#define DIALOGUE_PANEL_COLOR_GOLD 4

#define DIALOGUE_PANEL_SCREEN_ENTRY(tileId) \
    ((u16)(((tileId) & 0x03FF) | (DIALOGUE_FRAME_PALETTE_BANK << 12)))

typedef struct DialoguePanelState {
    int active;
    int promptVisible;
    DialogueNameplateSide nameplateSide;
    char nameplateText[24];
    char line1[40];
    char line2[40];
    char demoHint[40];
    int dynamicCount;
    u16 dynamicMapTiles[DIALOGUE_PANEL_DYNAMIC_TILE_MAX];
    u8 dynamicTileData[DIALOGUE_PANEL_DYNAMIC_TILE_MAX][32];
    u8 dirtyFlags[DIALOGUE_PANEL_DYNAMIC_TILE_MAX];
    u16 liveMap[DIALOGUE_FRAME_MAP_ENTRIES];
} DialoguePanelState;

static DialoguePanelState s_panel;

_Static_assert(
    DIALOGUE_FRAME_TILE_COUNT + DIALOGUE_PANEL_DYNAMIC_TILE_MAX
        <= DIALOGUE_MODE0_CHARBLOCK_TILE_CAPACITY,
    "Dialogue frame + dynamic tiles must fit in reserved charblock 1");
_Static_assert(
    DIALOGUE_FRAME_PALETTE_BANK
        == (DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE / 16),
    "Dialogue frame palette bank must match reserved BG palette base");

static void DialoguePanel_CopyWords(volatile u32 *dest, const u32 *src, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        dest[i] = src[i];
    }
}

static void DialoguePanel_CopyHalfwords(volatile u16 *dest, const u16 *src, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        dest[i] = src[i];
    }
}

static void DialoguePanel_ClearBytes(u8 *dest, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        dest[i] = 0;
    }
}

static volatile u32 *DialoguePanel_CharBase(void)
{
    return (volatile u32 *)CHARBLOCK_BASE(DIALOGUE_MODE0_BG_CHARBLOCK);
}

static volatile u16 *DialoguePanel_ScreenBase(void)
{
    return SCREENBLOCK_BASE(DIALOGUE_MODE0_BG_SCREENBLOCK);
}

static void DialoguePanel_UploadTile(int tileId, const u8 bytes[32])
{
    volatile u32 *dest = DialoguePanel_CharBase() + (tileId * 8);
    const u32 *src = (const u32 *)bytes;
    int i;

    for (i = 0; i < 8; i++)
    {
        dest[i] = src[i];
    }
}

static void DialoguePanel_MarkDirty(int slot)
{
    if (slot >= 0 && slot < DIALOGUE_PANEL_DYNAMIC_TILE_MAX)
    {
        s_panel.dirtyFlags[slot] = 1;
    }
}

static void DialoguePanel_ResetDynamic(void)
{
    int i;

    s_panel.dynamicCount = 0;
    for (i = 0; i < DIALOGUE_PANEL_DYNAMIC_TILE_MAX; i++)
    {
        s_panel.dynamicMapTiles[i] = 0xFFFF;
        s_panel.dirtyFlags[i] = 0;
        DialoguePanel_ClearBytes(s_panel.dynamicTileData[i], 32);
    }
}

static void DialoguePanel_RestoreBaseMap(void)
{
    int i;

    for (i = 0; i < DIALOGUE_FRAME_MAP_ENTRIES; i++)
    {
        s_panel.liveMap[i] = DIALOGUE_PANEL_SCREEN_ENTRY(gDialogueFrameMap[i]);
    }
    DialoguePanel_CopyHalfwords(
        DialoguePanel_ScreenBase(),
        s_panel.liveMap,
        DIALOGUE_FRAME_MAP_ENTRIES);
}

static int DialoguePanel_FindDynamicSlot(u16 mapIndex)
{
    int i;

    for (i = 0; i < s_panel.dynamicCount; i++)
    {
        if (s_panel.dynamicMapTiles[i] == mapIndex)
        {
            return i;
        }
    }
    return -1;
}

static void DialoguePanel_CopyFrameTileToSlot(int slot, u16 mapIndex)
{
    int baseTileId = (int)(gDialogueFrameMap[mapIndex] & 0x03FF);
    const u32 *baseWords = gDialogueFrameTiles + (baseTileId * 8);
    u8 *dest = s_panel.dynamicTileData[slot];
    int i;

    for (i = 0; i < 8; i++)
    {
        ((u32 *)dest)[i] = baseWords[i];
    }
}

static int DialoguePanel_EnsureDynamicTile(int tileX, int tileY)
{
    u16 mapIndex;
    int slot;

    if (tileX < 0 || tileX >= DIALOGUE_FRAME_VISIBLE_TILE_W
        || tileY < 0 || tileY >= DIALOGUE_FRAME_VISIBLE_TILE_H)
    {
        return -1;
    }

    mapIndex = (u16)(tileY * DIALOGUE_FRAME_MAP_W + tileX);
    slot = DialoguePanel_FindDynamicSlot(mapIndex);
    if (slot >= 0)
    {
        return slot;
    }
    if (s_panel.dynamicCount >= DIALOGUE_PANEL_DYNAMIC_TILE_MAX)
    {
        return -1;
    }

    slot = s_panel.dynamicCount++;
    s_panel.dynamicMapTiles[slot] = mapIndex;
    DialoguePanel_CopyFrameTileToSlot(slot, mapIndex);
    DialoguePanel_MarkDirty(slot);
    return slot;
}

static void DialoguePanel_SetPixel(int x, int y, int colorIndex)
{
    int tileX;
    int tileY;
    int localX;
    int localY;
    int slot;
    int byteIndex;
    u8 *tileBytes;
    u8 shift;

    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
    {
        return;
    }
    if (colorIndex < 0 || colorIndex > 15)
    {
        return;
    }

    tileX = x >> 3;
    tileY = y >> 3;
    localX = x & 7;
    localY = y & 7;
    slot = DialoguePanel_EnsureDynamicTile(tileX, tileY);
    if (slot < 0)
    {
        return;
    }

    tileBytes = s_panel.dynamicTileData[slot];
    byteIndex = localY * 4 + (localX >> 1);
    shift = (u8)((localX & 1) * 4);
    tileBytes[byteIndex] = (u8)(
        (tileBytes[byteIndex] & (u8)(~(0x0F << shift)))
        | ((colorIndex & 0x0F) << shift));
    DialoguePanel_MarkDirty(slot);
}

static void DialoguePanel_FillRect(int x, int y, int w, int h, int colorIndex)
{
    int row;
    int col;

    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            DialoguePanel_SetPixel(x + col, y + row, colorIndex);
        }
    }
}

static void DialoguePanel_DrawGlyph(int x, int y, char ch, int colorIndex)
{
    const u8 *rows = Video_GetFontGlyphRows(ch);
    int row;
    int col;

    for (row = 0; row < 7; row++)
    {
        for (col = 0; col < 5; col++)
        {
            if (rows[row] & (0x10 >> col))
            {
                DialoguePanel_SetPixel(x + col, y + row, colorIndex);
            }
        }
    }
}

static void DialoguePanel_DrawString(int x, int y, const char *text, int colorIndex)
{
    int cursor = x;

    if (!text)
    {
        return;
    }
    while (*text)
    {
        DialoguePanel_DrawGlyph(cursor, y, *text, colorIndex);
        cursor += VIDEO_FONT_ADVANCE;
        text++;
    }
}

void DialoguePanel_FlushDirty(void)
{
    int i;
    int tileId;
    u16 mapIndex;

    if (!s_panel.active)
    {
        return;
    }

    for (i = 0; i < s_panel.dynamicCount; i++)
    {
        if (!s_panel.dirtyFlags[i])
        {
            continue;
        }
        tileId = DIALOGUE_FRAME_TILE_COUNT + i;
        mapIndex = s_panel.dynamicMapTiles[i];
        DialoguePanel_UploadTile(tileId, s_panel.dynamicTileData[i]);
        s_panel.liveMap[mapIndex] = DIALOGUE_PANEL_SCREEN_ENTRY(tileId);
        DialoguePanel_ScreenBase()[mapIndex] = s_panel.liveMap[mapIndex];
        s_panel.dirtyFlags[i] = 0;
    }
}

static void DialoguePanel_DrawNameplatePixels(void)
{
    int width;
    int height = DIALOGUE_UI_NAMEPLATE_HEIGHT;
    int x;
    int y = DIALOGUE_UI_NAMEPLATE_Y;
    int fill;

    if (s_panel.nameplateSide == DIALOGUE_NAMEPLATE_NONE || s_panel.nameplateText[0] == '\0')
    {
        return;
    }

    width = Video_MeasureTextWidth(s_panel.nameplateText)
        + DIALOGUE_UI_NAMEPLATE_PAD_X * 2;
    if (s_panel.nameplateSide == DIALOGUE_NAMEPLATE_LEFT)
    {
        x = DIALOGUE_UI_NAMEPLATE_LEFT_X;
        fill = DIALOGUE_PANEL_COLOR_LIGHT_GOLD;
    }
    else
    {
        x = SCREEN_WIDTH - DIALOGUE_UI_NAMEPLATE_LEFT_X - width;
        fill = DIALOGUE_PANEL_COLOR_GOLD;
    }

    DialoguePanel_FillRect(x, y, width, height, fill);
    DialoguePanel_FillRect(x, y, width, 1, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_FillRect(x, y + height - 1, width, 1, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_FillRect(x, y, 1, height, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_FillRect(x + width - 1, y, 1, height, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_FillRect(x + 1, y + 1, width - 2, 1, DIALOGUE_PANEL_COLOR_ORANGE);
    DialoguePanel_FillRect(x + 1, y + height - 2, width - 2, 1, DIALOGUE_PANEL_COLOR_ORANGE);
    DialoguePanel_FillRect(x + 1, y + 1, 1, height - 2, DIALOGUE_PANEL_COLOR_ORANGE);
    DialoguePanel_FillRect(x + width - 2, y + 1, 1, height - 2, DIALOGUE_PANEL_COLOR_ORANGE);
    DialoguePanel_DrawString(
        x + DIALOGUE_UI_NAMEPLATE_PAD_X,
        y + 2,
        s_panel.nameplateText,
        DIALOGUE_PANEL_COLOR_BLACK);
}

static void DialoguePanel_DrawPromptPixels(void)
{
    int x0 = DIALOGUE_UI_PROMPT_X;
    int y0 = DIALOGUE_UI_PROMPT_Y;

    if (!s_panel.promptVisible)
    {
        return;
    }

    DialoguePanel_SetPixel(x0 + 1, y0 + 1, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 2, y0 + 1, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 3, y0 + 1, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 4, y0 + 1, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 2, y0 + 2, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 3, y0 + 2, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 2, y0 + 3, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 2, y0 + 4, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 4, y0 + 4, DIALOGUE_PANEL_COLOR_BLACK);
    DialoguePanel_SetPixel(x0 + 5, y0 + 5, DIALOGUE_PANEL_COLOR_BLACK);
}

static void DialoguePanel_CopyCString(char *dest, int destSize, const char *src)
{
    int i;

    if (destSize <= 0)
    {
        return;
    }
    if (!src)
    {
        dest[0] = '\0';
        return;
    }
    for (i = 0; i < destSize - 1 && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static void DialoguePanel_RestoreRectToFrame(int x0, int y0, int x1, int y1)
{
    int tileX0 = x0 >> 3;
    int tileY0 = y0 >> 3;
    int tileX1 = (x1 - 1) >> 3;
    int tileY1 = (y1 - 1) >> 3;
    int ty;
    int tx;

    for (ty = tileY0; ty <= tileY1; ty++)
    {
        for (tx = tileX0; tx <= tileX1; tx++)
        {
            int slot = DialoguePanel_EnsureDynamicTile(tx, ty);
            u16 mapIndex;

            if (slot < 0)
            {
                continue;
            }
            mapIndex = s_panel.dynamicMapTiles[slot];
            DialoguePanel_CopyFrameTileToSlot(slot, mapIndex);
            DialoguePanel_MarkDirty(slot);
        }
    }
}

static void DialoguePanel_RebuildChrome(void)
{
    DialoguePanel_ResetDynamic();
    DialoguePanel_RestoreBaseMap();

    if (s_panel.demoHint[0] != '\0')
    {
        DialoguePanel_DrawString(
            8,
            DIALOGUE_UI_DEMO_HINT_Y,
            s_panel.demoHint,
            DIALOGUE_PANEL_COLOR_BLACK);
    }
    DialoguePanel_DrawNameplatePixels();
    DialoguePanel_DrawPromptPixels();
    DialoguePanel_FlushDirty();
}

void DialoguePanel_ClearTextLine(int lineIndex)
{
    int y;

    if (!s_panel.active)
    {
        return;
    }
    if (lineIndex == 0)
    {
        y = DIALOGUE_UI_TEXT_LINE1_Y;
    }
    else if (lineIndex == 1)
    {
        y = DIALOGUE_UI_TEXT_LINE2_Y;
    }
    else
    {
        return;
    }

    DialoguePanel_RestoreRectToFrame(
        DIALOGUE_UI_TEXT_X,
        y,
        DIALOGUE_UI_TEXT_MAX_END_X + 1,
        y + VIDEO_FONT_GLYPH_H);

    /* Prompt shares nearby tiles with line 2; redraw if visible. */
    if (lineIndex == 1 && s_panel.promptVisible)
    {
        DialoguePanel_DrawPromptPixels();
    }
}

void DialoguePanel_DrawTextChar(int x, int y, char ch)
{
    DialoguePanel_DrawTextCharColor(x, y, ch, DIALOGUE_PANEL_COLOR_BLACK);
}

void DialoguePanel_DrawTextCharColor(int x, int y, char ch, int colorIndex)
{
    if (!s_panel.active)
    {
        return;
    }
    if (ch == ' ')
    {
        return;
    }
    if (colorIndex < 0 || colorIndex > 15)
    {
        colorIndex = DIALOGUE_PANEL_COLOR_BLACK;
    }
    /* Never overwrite transparent index 0 as ink. */
    if (colorIndex == DIALOGUE_PANEL_COLOR_TRANSPARENT)
    {
        colorIndex = DIALOGUE_PANEL_COLOR_BLACK;
    }
    DialoguePanel_DrawGlyph(x, y, ch, colorIndex);
}

int DialoguePanel_CountDirty(void)
{
    int i;
    int count = 0;

    for (i = 0; i < s_panel.dynamicCount; i++)
    {
        if (s_panel.dirtyFlags[i])
        {
            count++;
        }
    }
    return count;
}

int DialoguePanel_GetDynamicSlotTile(int slot, int *tileX, int *tileY)
{
    u16 mapIndex;

    if (slot < 0 || slot >= s_panel.dynamicCount)
    {
        return 0;
    }
    mapIndex = s_panel.dynamicMapTiles[slot];
    if (tileX)
    {
        *tileX = (int)(mapIndex % DIALOGUE_FRAME_MAP_W);
    }
    if (tileY)
    {
        *tileY = (int)(mapIndex / DIALOGUE_FRAME_MAP_W);
    }
    return s_panel.dirtyFlags[slot] ? 1 : 0;
}

int DialoguePanel_IsDynamicSlotDirty(int slot)
{
    if (slot < 0 || slot >= s_panel.dynamicCount)
    {
        return 0;
    }
    return s_panel.dirtyFlags[slot] ? 1 : 0;
}

void DialoguePanel_Init(void)
{
    volatile u16 *palette = BG_PALETTE + DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE;

    DialoguePanel_CopyHalfwords(palette, gDialogueFramePalette, DIALOGUE_FRAME_PALETTE_COUNT);
    DialoguePanel_CopyWords(
        DialoguePanel_CharBase(),
        gDialogueFrameTiles,
        DIALOGUE_FRAME_TILE_WORDS);

    REG_BG0CNT = BGCNT_PRIORITY(DIALOGUE_MODE0_BG_PRIORITY)
        | BGCNT_CHARBASE(DIALOGUE_MODE0_BG_CHARBLOCK)
        | BGCNT_4BPP
        | BGCNT_SCREENBASE(DIALOGUE_MODE0_BG_SCREENBLOCK)
        | BGCNT_SIZE_256X256;
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    s_panel.active = 1;
    s_panel.promptVisible = 0;
    s_panel.nameplateSide = DIALOGUE_NAMEPLATE_NONE;
    s_panel.nameplateText[0] = '\0';
    s_panel.line1[0] = '\0';
    s_panel.line2[0] = '\0';
    s_panel.demoHint[0] = '\0';
    DialoguePanel_ResetDynamic();
    DialoguePanel_RestoreBaseMap();
}

void DialoguePanel_ShowFrame(void)
{
    if (!s_panel.active)
    {
        return;
    }
    DialoguePanel_RebuildChrome();
}

void DialoguePanel_BeginSample(
    DialogueNameplateSide side,
    const char *name,
    const char *demoHint)
{
    s_panel.nameplateSide = side;
    DialoguePanel_CopyCString(
        s_panel.nameplateText,
        (int)sizeof(s_panel.nameplateText),
        name);
    DialoguePanel_CopyCString(
        s_panel.demoHint,
        (int)sizeof(s_panel.demoHint),
        demoHint);
    s_panel.line1[0] = '\0';
    s_panel.line2[0] = '\0';
    s_panel.promptVisible = 0;
    if (s_panel.active)
    {
        DialoguePanel_RebuildChrome();
    }
}

void DialoguePanel_SetNameplate(DialogueNameplateSide side, const char *name)
{
    s_panel.nameplateSide = side;
    DialoguePanel_CopyCString(
        s_panel.nameplateText,
        (int)sizeof(s_panel.nameplateText),
        name);
    if (s_panel.active)
    {
        DialoguePanel_RebuildChrome();
        if (s_panel.line1[0] || s_panel.line2[0])
        {
            DialoguePanel_DrawString(
                DIALOGUE_UI_TEXT_X,
                DIALOGUE_UI_TEXT_LINE1_Y,
                s_panel.line1,
                DIALOGUE_PANEL_COLOR_BLACK);
            if (s_panel.line2[0])
            {
                DialoguePanel_DrawString(
                    DIALOGUE_UI_TEXT_X,
                    DIALOGUE_UI_TEXT_LINE2_Y,
                    s_panel.line2,
                    DIALOGUE_PANEL_COLOR_BLACK);
            }
            DialoguePanel_FlushDirty();
        }
    }
}

void DialoguePanel_SetDemoHint(const char *text)
{
    DialoguePanel_CopyCString(
        s_panel.demoHint,
        (int)sizeof(s_panel.demoHint),
        text);
    if (!s_panel.active)
    {
        return;
    }

    /* Hint lives in the transparent upper field; restore then redraw locally. */
    DialoguePanel_RestoreRectToFrame(0, 0, SCREEN_WIDTH, 24);
    if (s_panel.demoHint[0] != '\0')
    {
        DialoguePanel_DrawString(
            8,
            DIALOGUE_UI_DEMO_HINT_Y,
            s_panel.demoHint,
            DIALOGUE_PANEL_COLOR_BLACK);
    }
    DialoguePanel_FlushDirty();
}

void DialoguePanel_SetPromptVisible(int visible)
{
    int wasVisible = s_panel.promptVisible;

    s_panel.promptVisible = visible ? 1 : 0;
    if (!s_panel.active)
    {
        return;
    }

    if (wasVisible && !s_panel.promptVisible)
    {
        DialoguePanel_RestoreRectToFrame(
            DIALOGUE_UI_PROMPT_X,
            DIALOGUE_UI_PROMPT_Y,
            DIALOGUE_UI_PROMPT_X + DIALOGUE_UI_PROMPT_W,
            DIALOGUE_UI_PROMPT_Y + DIALOGUE_UI_PROMPT_H);
    }
    else if (s_panel.promptVisible)
    {
        DialoguePanel_DrawPromptPixels();
    }
    DialoguePanel_FlushDirty();
}

void DialoguePanel_SetText(const char *line1, const char *line2)
{
    DialoguePanel_CopyCString(s_panel.line1, (int)sizeof(s_panel.line1), line1);
    DialoguePanel_CopyCString(s_panel.line2, (int)sizeof(s_panel.line2), line2);
    if (!s_panel.active)
    {
        return;
    }
    DialoguePanel_ClearTextLine(0);
    DialoguePanel_ClearTextLine(1);
    DialoguePanel_DrawString(
        DIALOGUE_UI_TEXT_X,
        DIALOGUE_UI_TEXT_LINE1_Y,
        s_panel.line1,
        DIALOGUE_PANEL_COLOR_BLACK);
    if (s_panel.line2[0])
    {
        DialoguePanel_DrawString(
            DIALOGUE_UI_TEXT_X,
            DIALOGUE_UI_TEXT_LINE2_Y,
            s_panel.line2,
            DIALOGUE_PANEL_COLOR_BLACK);
    }
    DialoguePanel_FlushDirty();
}

void DialoguePanel_ClearContent(void)
{
    s_panel.nameplateSide = DIALOGUE_NAMEPLATE_NONE;
    s_panel.nameplateText[0] = '\0';
    s_panel.line1[0] = '\0';
    s_panel.line2[0] = '\0';
    s_panel.promptVisible = 0;
    if (s_panel.active)
    {
        DialoguePanel_RebuildChrome();
    }
}

void DialoguePanel_Apply(
    DialogueNameplateSide side,
    const char *name,
    const char *line1,
    const char *line2,
    int promptVisible,
    const char *demoHint)
{
    DialoguePanel_BeginSample(side, name, demoHint);
    DialoguePanel_SetText(line1, line2);
    DialoguePanel_SetPromptVisible(promptVisible);
}

void DialoguePanel_Close(void)
{
    REG_BG0CNT = 0;
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
    s_panel.active = 0;
    s_panel.promptVisible = 0;
    s_panel.nameplateSide = DIALOGUE_NAMEPLATE_NONE;
    s_panel.nameplateText[0] = '\0';
    s_panel.line1[0] = '\0';
    s_panel.line2[0] = '\0';
    s_panel.demoHint[0] = '\0';
    DialoguePanel_ResetDynamic();
}
