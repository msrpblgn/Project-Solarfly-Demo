#include "battle_ui/battle_hud.h"
#include "battle_ui/battle_slot_layout.h"
#include "battle_ui/battle_message_queue.h"
#include "battle_ui/battle_unit_draw.h"
#include "battle_ui/hud_layout.h"
#include "battle/battle_action.h"
#include "battle/battle_state.h"
#include "battle/battle_member.h"
#include "assets/aksil_front_sprite.h"
#include "assets/maren_front_sprite.h"
#include "assets/protagonist_front_sprite.h"
#include "assets/tutsil_front_sprite.h"
#include "assets/electric_armor_badger_sprite.h"
#include "assets/sprite_assets.h"
#include "core/video.h"
#include "core/gba_types.h"

#define COLOR_ALLY_TILE_EMPTY_FILL RGB15(4, 12, 16)
#define COLOR_ALLY_TILE_EMPTY_BORDER RGB15(8, 20, 24)
#define COLOR_ALLY_TILE_OCCUPIED_FILL RGB15(6, 16, 22)
#define COLOR_ALLY_TILE_OCCUPIED_BORDER RGB15(12, 28, 31)
#define COLOR_ENEMY_TILE_EMPTY_FILL RGB15(12, 4, 8)
#define COLOR_ENEMY_TILE_EMPTY_BORDER RGB15(20, 8, 16)
#define COLOR_ENEMY_TILE_OCCUPIED_FILL RGB15(16, 6, 10)
#define COLOR_ENEMY_TILE_OCCUPIED_BORDER RGB15(28, 12, 20)
#define COLOR_ALLY_UNIT RGB15(8, 16, 31)
#define COLOR_ALLY_UNIT_DIM RGB15(4, 8, 14)
#define COLOR_ENEMY_UNIT RGB15(24, 8, 8)
#define COLOR_ENEMY_UNIT_DIM RGB15(10, 4, 4)
#define COLOR_DITHER_DARK RGB15(4, 6, 3)

#define BATTLE_HP_BAR_H 4
#define BATTLE_HP_BAR_GAP 2
#define BATTLE_HP_BAR_INSET 2
#define BATTLE_TUTSIL_HP_BAR_Y_OFFSET 8

#define COLOR_ALLY_HP_FILL RGB15(8, 26, 10)
#define COLOR_ALLY_HP_EMPTY RGB15(4, 10, 6)
#define COLOR_ALLY_HP_BORDER RGB15(12, 28, 14)
#define COLOR_ALLY_HP_FILL_DIM RGB15(4, 12, 6)
#define COLOR_ENEMY_HP_FILL RGB15(28, 10, 10)
#define COLOR_ENEMY_HP_EMPTY RGB15(10, 4, 6)
#define COLOR_ENEMY_HP_BORDER RGB15(28, 12, 18)
#define COLOR_ENEMY_HP_FILL_DIM RGB15(12, 4, 4)

static u16 s_uiBorderColor = HUD_BORDER_BLUE;

static u16 BattleHud_ResolveBorderColor(BorderColor borderColor)
{
    switch (borderColor)
    {
    case BORDER_COLOR_RED:
        return HUD_BORDER_RED;
    case BORDER_COLOR_GREEN:
        return HUD_BORDER_GREEN;
    case BORDER_COLOR_GRAY:
        return HUD_BORDER_GRAY;
    case BORDER_COLOR_PURPLE:
        return HUD_BORDER_PURPLE;
    case BORDER_COLOR_GOLD:
        return HUD_BORDER_GOLD;
    case BORDER_COLOR_CYAN:
        return HUD_BORDER_CYAN;
    case BORDER_COLOR_PINK:
        return HUD_BORDER_PINK;
    case BORDER_COLOR_BLUE:
    default:
        return HUD_BORDER_BLUE;
    }
}

typedef struct BattleHudFocus {
    int enabled;
    int actorSide;
    int actorSlot;
    int targetSide;
    int targetSlot;
} BattleHudFocus;

static int BattleHud_IsSlotValidFocus(int side, int slot)
{
    if (side == BATTLE_SIDE_PLAYER)
    {
        return slot >= 0 && slot < BATTLE_PLAYER_SLOT_COUNT;
    }
    if (side == BATTLE_SIDE_ENEMY)
    {
        return slot >= 0 && slot < BATTLE_ENEMY_SLOT_COUNT;
    }
    return 0;
}

static int BattleHud_ActionHasFocusPair(const BattleAction *action)
{
    if (!action)
    {
        return 0;
    }
    return action->type == BATTLE_ACTION_ATTACK || action->type == BATTLE_ACTION_SKILL;
}

static void BattleHud_ComputeFocus(const BattleState *state, const BattleAction *action, BattleHudFocus *focus)
{
    focus->enabled = 0;
    focus->actorSide = -1;
    focus->actorSlot = -1;
    focus->targetSide = -1;
    focus->targetSlot = -1;

    if (!state)
    {
        return;
    }

    if (state->phase == BATTLE_PHASE_PLAYER_COMMAND || state->phase == BATTLE_PHASE_PLAYER_SKILL)
    {
        focus->enabled = 1;
        focus->actorSide = BATTLE_SIDE_PLAYER;
        focus->actorSlot = state->activePlayerSlot;
        return;
    }

    if (state->phase == BATTLE_PHASE_TARGET_SELECT)
    {
        focus->enabled = 1;
        focus->actorSide = BATTLE_SIDE_PLAYER;
        focus->actorSlot = state->activePlayerSlot;
        focus->targetSide = state->targetSide;
        focus->targetSlot = state->targetSlot;
        return;
    }

    if (state->phase == BATTLE_PHASE_LP_PROMPT && BattleHud_ActionHasFocusPair(action))
    {
        focus->enabled = 1;
        focus->actorSide = action->actorSide;
        focus->actorSlot = action->actorSlot;
        focus->targetSide = action->targetSide;
        focus->targetSlot = action->targetSlot;
        return;
    }

    if (state->phase == BATTLE_PHASE_RESOLVE_ACTION)
    {
        if (action && action->type == BATTLE_ACTION_BRACE)
        {
            focus->enabled = 1;
            focus->actorSide = action->actorSide;
            focus->actorSlot = action->actorSlot;
            return;
        }
        if (action && BattleHud_ActionHasFocusPair(action))
        {
            focus->enabled = 1;
            focus->actorSide = action->actorSide;
            focus->actorSlot = action->actorSlot;
            focus->targetSide = action->targetSide;
            focus->targetSlot = action->targetSlot;
            return;
        }
        if (BattleHud_IsSlotValidFocus(BATTLE_SIDE_PLAYER, state->activePlayerSlot))
        {
            focus->enabled = 1;
            focus->actorSide = BATTLE_SIDE_PLAYER;
            focus->actorSlot = state->activePlayerSlot;
        }
        return;
    }

    if (state->phase == BATTLE_PHASE_ENEMY_TURN)
    {
        focus->enabled = 1;
        focus->actorSide = BATTLE_SIDE_ENEMY;
        focus->actorSlot = state->activeEnemySlot;
        focus->targetSide = BATTLE_SIDE_PLAYER;
        if (BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, state->activePlayerSlot))
        {
            focus->targetSlot = state->activePlayerSlot;
        }
        else
        {
            focus->targetSlot = BattleState_FindEnemyTargetPlayerSlot(state);
        }
        return;
    }

    if (state->phase == BATTLE_PHASE_MESSAGE_QUEUE && BattleHud_ActionHasFocusPair(action))
    {
        focus->enabled = 1;
        focus->actorSide = action->actorSide;
        focus->actorSlot = action->actorSlot;
        focus->targetSide = action->targetSide;
        focus->targetSlot = action->targetSlot;
        return;
    }

    if (state->phase == BATTLE_PHASE_MESSAGE_QUEUE && action && action->type == BATTLE_ACTION_BRACE)
    {
        focus->enabled = 1;
        focus->actorSide = action->actorSide;
        focus->actorSlot = action->actorSlot;
    }
}

static BattleUnitDrawStyle BattleHud_GetUnitDrawStyle(const BattleHudFocus *focus, int side, int slot)
{
    if (!focus || !focus->enabled)
    {
        return BATTLE_UNIT_DRAW_NORMAL;
    }
    if (!BattleHud_IsSlotValidFocus(focus->actorSide, focus->actorSlot))
    {
        return BATTLE_UNIT_DRAW_NORMAL;
    }
    if (side == focus->actorSide && slot == focus->actorSlot)
    {
        return BATTLE_UNIT_DRAW_NORMAL;
    }
    if (BattleHud_IsSlotValidFocus(focus->targetSide, focus->targetSlot))
    {
        if (side == focus->targetSide && slot == focus->targetSlot)
        {
            return BATTLE_UNIT_DRAW_NORMAL;
        }
    }
    return BATTLE_UNIT_DRAW_DIMMED;
}

static void BattleHud_DrawTileOutline(int x, int y, int w, int h, u16 borderColor, u16 fillColor)
{
    Video_DrawRect(x, y, w, h, fillColor);
    Video_DrawRect(x, y, w, 2, borderColor);
    Video_DrawRect(x, y + h - 2, w, 2, borderColor);
    Video_DrawRect(x, y, 2, h, borderColor);
    Video_DrawRect(x + w - 2, y, 2, h, borderColor);
}

static void BattleHud_DrawDitheredUnit(int x, int y, int w, int h, u16 baseColor)
{
    int row;
    int col;
    u16 light = baseColor;
    u16 dark = COLOR_DITHER_DARK;

    for (row = 0; row < h; row += 2)
    {
        for (col = 0; col < w; col += 2)
        {
            u16 color = (((row / 2) + (col / 2)) & 1) ? dark : light;
            int blockW = 2;
            int blockH = 2;
            if (col + blockW > w)
            {
                blockW = w - col;
            }
            if (row + blockH > h)
            {
                blockH = h - row;
            }
            Video_DrawRect(x + col, y + row, blockW, blockH, color);
        }
    }
}

static void BattleHud_DrawUnitBlock(
    int x,
    int y,
    int w,
    int h,
    u16 normalColor,
    u16 dimColor,
    BattleUnitDrawStyle style)
{
    if (style == BATTLE_UNIT_DRAW_DIMMED)
    {
        BattleHud_DrawDitheredUnit(x, y, w, h, dimColor);
        return;
    }
    Video_DrawRect(x, y, w, h, normalColor);
}

static int BattleHud_ClampHpValue(int value, int maxHp)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > maxHp)
    {
        return maxHp;
    }
    return value;
}

static void BattleHud_DrawHpBar(
    int x,
    int y,
    int width,
    int height,
    int currentHp,
    int maxHp,
    u16 fillColor,
    u16 emptyColor,
    u16 borderColor,
    BattleUnitDrawStyle style)
{
    int fillWidth;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (maxHp <= 0)
    {
        currentHp = 0;
        fillWidth = 0;
    }
    else
    {
        currentHp = BattleHud_ClampHpValue(currentHp, maxHp);
        fillWidth = (width * currentHp) / maxHp;
    }

    if (style == BATTLE_UNIT_DRAW_DIMMED)
    {
        fillColor = (fillColor == COLOR_ALLY_HP_FILL) ? COLOR_ALLY_HP_FILL_DIM : COLOR_ENEMY_HP_FILL_DIM;
    }

    Video_DrawRect(x, y, width, height, emptyColor);
    if (fillWidth > 0)
    {
        Video_DrawRect(x, y, fillWidth, height, fillColor);
    }
    Video_DrawRect(x, y, width, 1, borderColor);
    Video_DrawRect(x, y + height - 1, width, 1, borderColor);
    Video_DrawRect(x, y, 1, height, borderColor);
    Video_DrawRect(x + width - 1, y, 1, height, borderColor);
}

static int BattleHud_GetHpBarY(const BattleSlotLayout *layout, int side, const BattleMember *member)
{
    int barY;

    barY = BattleLayout_GetUnitTopY(side, member, layout)
        - BATTLE_HP_BAR_GAP
        - BATTLE_HP_BAR_H;

    if (barY < 0)
    {
        barY = 0;
    }
    barY += BATTLE_HP_BAR_OFFSET_Y;
    if (side == BATTLE_SIDE_ENEMY && member && BattleMember_IsTutsil(member))
    {
        barY += BATTLE_TUTSIL_HP_BAR_Y_OFFSET;
    }
    return barY;
}

static void BattleHud_DrawMemberHpBar(
    const BattleSlotLayout *layout,
    const BattleMember *member,
    int side,
    BattleUnitDrawStyle style)
{
    int barX;
    int barY;
    int barW;
    u16 fillColor;
    u16 emptyColor;
    u16 borderColor;

    if (!layout || !member)
    {
        return;
    }

    barW = layout->tileW - (BATTLE_HP_BAR_INSET * 2);
    barX = layout->tileX + BATTLE_HP_BAR_INSET;
    barY = BattleHud_GetHpBarY(layout, side, member);

    if (side == BATTLE_SIDE_ENEMY)
    {
        fillColor = COLOR_ENEMY_HP_FILL;
        emptyColor = COLOR_ENEMY_HP_EMPTY;
        borderColor = COLOR_ENEMY_HP_BORDER;
    }
    else
    {
        fillColor = COLOR_ALLY_HP_FILL;
        emptyColor = COLOR_ALLY_HP_EMPTY;
        borderColor = COLOR_ALLY_HP_BORDER;
    }

    /* HP bars stay fully visible for every occupied slot; never follow unit dimming. */
    (void)style;
    BattleHud_DrawHpBar(
        barX,
        barY,
        barW,
        BATTLE_HP_BAR_H,
        member->isAlive ? member->currentHp : 0,
        member->maxHp,
        fillColor,
        emptyColor,
        borderColor,
        BATTLE_UNIT_DRAW_NORMAL);
}

static void BattleHud_DrawSideTiles(const BattleState *state, int side)
{
    int slotCount;
    int i;
    u16 emptyFill;
    u16 emptyBorder;
    u16 occupiedFill;
    u16 occupiedBorder;

    if (side == BATTLE_SIDE_ENEMY)
    {
        slotCount = BATTLE_ENEMY_SLOT_COUNT;
        emptyFill = COLOR_ENEMY_TILE_EMPTY_FILL;
        emptyBorder = COLOR_ENEMY_TILE_EMPTY_BORDER;
        occupiedFill = COLOR_ENEMY_TILE_OCCUPIED_FILL;
        occupiedBorder = COLOR_ENEMY_TILE_OCCUPIED_BORDER;
    }
    else
    {
        slotCount = BATTLE_PLAYER_SLOT_COUNT;
        emptyFill = COLOR_ALLY_TILE_EMPTY_FILL;
        emptyBorder = COLOR_ALLY_TILE_EMPTY_BORDER;
        occupiedFill = COLOR_ALLY_TILE_OCCUPIED_FILL;
        occupiedBorder = COLOR_ALLY_TILE_OCCUPIED_BORDER;
    }

    for (i = 0; i < slotCount; i++)
    {
        const BattleSlotLayout *layout = BattleSlotLayout_Get(side, i);
        int occupied = BattleState_IsSlotOccupied(state, side, i);
        u16 fillColor = occupied ? occupiedFill : emptyFill;
        u16 borderColor = occupied ? occupiedBorder : emptyBorder;

        if (!layout)
        {
            continue;
        }
        BattleHud_DrawTileOutline(
            layout->tileX,
            layout->tileY,
            layout->tileW,
            layout->tileH,
            borderColor,
            fillColor);
        if (BattleState_HasTileEffect(state, side, i))
        {
            const BattleTileEffect *effect = BattleState_GetTileEffect(state, side, i);
            u16 markerColor = RGB15(31, 20, 0);

            if (effect && effect->effectId == BATTLE_TILE_EFFECT_VEIL_TEST)
            {
                markerColor = RGB15(20, 10, 31);
            }
            Video_DrawRect(layout->tileX + layout->tileW - 6, layout->tileY + 2, 4, 4, markerColor);
        }
        /* Tile Veil luck spots: small cyan "V" at top-left, clear of HP bars. */
        if (BattleState_HasVeilTile(state, side, i))
        {
            Video_DrawText(
                layout->tileX + 2,
                layout->tileY + 2,
                "V",
                RGB15(10, 28, 31));
        }
    }
}

static void BattleHud_DrawMemberFrontClipped(
    const BattleMember *member,
    int side,
    int drawX,
    int drawY,
    int clipX,
    int clipY,
    int clipW,
    int clipH)
{
    const u16 *pixels = 0;
    int spriteW = 0;
    int spriteH = 0;
    u16 keyColor = RGB15(31, 0, 31);

    if (!member)
    {
        return;
    }
    if (side == BATTLE_SIDE_PLAYER && BattleMember_IsProtagonist(member))
    {
        pixels = protagonist_front_spriteBitmap;
        spriteW = PROTAGONIST_FRONT_SPRITE_WIDTH;
        spriteH = PROTAGONIST_FRONT_SPRITE_HEIGHT;
        keyColor = PROTAGONIST_FRONT_SPRITE_TRANSPARENT_COLOR;
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsAksil(member))
    {
        pixels = aksil_front_spriteBitmap;
        spriteW = AKSIL_FRONT_SPRITE_WIDTH;
        spriteH = AKSIL_FRONT_SPRITE_HEIGHT;
        keyColor = AKSIL_FRONT_SPRITE_TRANSPARENT_COLOR;
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsMaren(member))
    {
        pixels = maren_front_spriteBitmap;
        spriteW = MAREN_FRONT_SPRITE_WIDTH;
        spriteH = MAREN_FRONT_SPRITE_HEIGHT;
        keyColor = MAREN_FRONT_SPRITE_TRANSPARENT_COLOR;
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsTutsil(member))
    {
        pixels = tutsil_front_spriteBitmap;
        spriteW = TUTSIL_FRONT_SPRITE_WIDTH;
        spriteH = TUTSIL_FRONT_SPRITE_HEIGHT;
        keyColor = TUTSIL_FRONT_SPRITE_TRANSPARENT_COLOR;
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsElectricArmorBadger(member))
    {
        pixels = electric_armor_badger_spriteBitmap;
        spriteW = ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH;
        spriteH = ELECTRIC_ARMOR_BADGER_SPRITE_HEIGHT;
        keyColor = ELECTRIC_ARMOR_BADGER_SPRITE_TRANSPARENT_COLOR;
    }
    if (!pixels || spriteW <= 0 || spriteH <= 0)
    {
        return;
    }
    Video_DrawBitmapKeyColorRegion(
        drawX,
        drawY,
        clipX,
        clipY,
        clipW,
        clipH,
        spriteW,
        0,
        0,
        spriteW,
        spriteH,
        pixels,
        keyColor);
}

static void BattleHud_DrawLivingMemberSprite(
    const BattleMember *member,
    int side,
    const BattleSlotLayout *layout,
    u16 unitColor,
    u16 unitDimColor,
    BattleUnitDrawStyle style)
{
    if (!member || !layout)
    {
        return;
    }
    if (side == BATTLE_SIDE_PLAYER && BattleMember_IsProtagonist(member))
    {
        int drawX = layout->tileX + (layout->tileW - PROTAGONIST_FRONT_SPRITE_WIDTH) / 2;
        int drawY = BattleLayout_GetUnitDrawY(side, member, layout);

        SpriteAssets_DrawProtagonistFront(drawX, drawY);
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsAksil(member))
    {
        int drawX = layout->tileX + (layout->tileW - AKSIL_FRONT_SPRITE_WIDTH) / 2;
        int drawY = BattleLayout_GetUnitDrawY(side, member, layout);

        SpriteAssets_DrawAksilFront(drawX, drawY);
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsMaren(member))
    {
        int drawX = layout->tileX + (layout->tileW - MAREN_FRONT_SPRITE_WIDTH) / 2;
        int drawY = BattleLayout_GetUnitDrawY(side, member, layout);

        SpriteAssets_DrawMarenFront(drawX, drawY);
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsTutsil(member))
    {
        int drawX = layout->tileX + (layout->tileW - TUTSIL_FRONT_SPRITE_WIDTH) / 2;
        int drawY = BattleLayout_GetUnitDrawY(side, member, layout);

        SpriteAssets_DrawTutsilFront(drawX, drawY);
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsElectricArmorBadger(member))
    {
        int drawX = layout->tileX
            + (layout->tileW - ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH) / 2;
        int drawY = BattleLayout_GetUnitDrawY(side, member, layout);

        SpriteAssets_DrawElectricArmorBadgerFront(drawX, drawY);
    }
    else
    {
        BattleHud_DrawUnitBlock(
            layout->spriteX,
            layout->spriteY,
            layout->spriteW,
            layout->spriteH,
            unitColor,
            unitDimColor,
            style);
    }
}

static void BattleHud_DrawKoBurrowMember(
    const BattleMember *member,
    int side,
    const BattleSlotLayout *layout)
{
    int baseY;
    int drawX;
    int drawY;
    int spriteW;
    int groundY;

    if (!member || !layout || !BattleMember_IsKoBurrowing(member))
    {
        return;
    }

    baseY = BattleLayout_GetUnitDrawY(side, member, layout);
    drawY = baseY + (int)member->koBurrowYOffset;
    groundY = layout->tileY + layout->tileH;

    if (side == BATTLE_SIDE_PLAYER && BattleMember_IsProtagonist(member))
    {
        spriteW = PROTAGONIST_FRONT_SPRITE_WIDTH;
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsAksil(member))
    {
        spriteW = AKSIL_FRONT_SPRITE_WIDTH;
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsMaren(member))
    {
        spriteW = MAREN_FRONT_SPRITE_WIDTH;
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsTutsil(member))
    {
        spriteW = TUTSIL_FRONT_SPRITE_WIDTH;
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsElectricArmorBadger(member))
    {
        spriteW = ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH;
    }
    else
    {
        /* Placeholder block: sink + stop when below tile. */
        int blockY = layout->spriteY + (int)member->koBurrowYOffset;
        int visibleH = layout->spriteH - (int)member->koBurrowYOffset;

        if (visibleH <= 0)
        {
            return;
        }
        Video_DrawRect(
            layout->spriteX,
            blockY,
            layout->spriteW,
            visibleH,
            side == BATTLE_SIDE_ENEMY ? COLOR_ENEMY_UNIT_DIM : COLOR_ALLY_UNIT_DIM);
        return;
    }

    drawX = layout->tileX + (layout->tileW - spriteW) / 2;
    /* Clip to the tile: bottom of sprite sinks below the ground line. */
    BattleHud_DrawMemberFrontClipped(
        member,
        side,
        drawX,
        drawY,
        layout->tileX,
        layout->tileY,
        layout->tileW,
        groundY - layout->tileY);
}

static void BattleHud_DrawSideMembers(const BattleState *state, int side, const BattleHudFocus *focus)
{
    int slotCount;
    int i;
    u16 unitColor;
    u16 unitDimColor;

    if (side == BATTLE_SIDE_ENEMY)
    {
        slotCount = BATTLE_ENEMY_SLOT_COUNT;
        unitColor = COLOR_ENEMY_UNIT;
        unitDimColor = COLOR_ENEMY_UNIT_DIM;
    }
    else
    {
        slotCount = BATTLE_PLAYER_SLOT_COUNT;
        unitColor = COLOR_ALLY_UNIT;
        unitDimColor = COLOR_ALLY_UNIT_DIM;
    }

    for (i = 0; i < slotCount; i++)
    {
        const BattleSlotLayout *layout;
        const BattleMember *member;
        BattleUnitDrawStyle style;

        member = BattleState_GetMemberAtSlotConst(state, side, i);
        layout = BattleSlotLayout_Get(side, i);
        if (!member || !layout || member->maxHp <= 0)
        {
            continue;
        }

        style = BattleHud_GetUnitDrawStyle(focus, side, i);

        if (BattleState_IsAliveOccupiedSlot(state, side, i))
        {
            BattleHud_DrawLivingMemberSprite(
                member,
                side,
                layout,
                unitColor,
                unitDimColor,
                style);
            BattleHud_DrawMemberHpBar(layout, member, side, style);
        }
        else if (BattleMember_IsKoBurrowing(member))
        {
            BattleHud_DrawKoBurrowMember(member, side, layout);
        }
        /* Fully removed / burrow finished: draw nothing; tile looks empty. */
    }
}

void BattleHud_Init(void)
{
    s_uiBorderColor = HUD_BORDER_BLUE;
}

void BattleHud_SetBorderColor(BorderColor borderColor)
{
    s_uiBorderColor = BattleHud_ResolveBorderColor(borderColor);
}

u16 BattleHud_GetBorderColorRgb(void)
{
    return s_uiBorderColor;
}

void BattleHud_DrawCombatants(const BattleState *state, const BattleAction *action)
{
    BattleHudFocus focus;

    BattleHud_ComputeFocus(state, action, &focus);
    BattleHud_DrawSideTiles(state, BATTLE_SIDE_ENEMY);
    BattleHud_DrawSideTiles(state, BATTLE_SIDE_PLAYER);
    BattleHud_DrawSideMembers(state, BATTLE_SIDE_ENEMY, &focus);
    BattleHud_DrawSideMembers(state, BATTLE_SIDE_PLAYER, &focus);
}

static int BattleHud_RectsOverlap(
    int ax,
    int ay,
    int aw,
    int ah,
    int bx,
    int by,
    int bw,
    int bh)
{
    if (aw <= 0 || ah <= 0 || bw <= 0 || bh <= 0)
    {
        return 0;
    }
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static int BattleHud_TryGetMemberSpriteBounds(
    const BattleMember *member,
    int side,
    const BattleSlotLayout *layout,
    int *outX,
    int *outY,
    int *outW,
    int *outH)
{
    int spriteW = 0;
    int spriteH = 0;

    if (!member || !layout || !outX || !outY || !outW || !outH)
    {
        return 0;
    }

    if (side == BATTLE_SIDE_PLAYER && BattleMember_IsProtagonist(member))
    {
        spriteW = PROTAGONIST_FRONT_SPRITE_WIDTH;
        spriteH = PROTAGONIST_FRONT_SPRITE_HEIGHT;
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsAksil(member))
    {
        spriteW = AKSIL_FRONT_SPRITE_WIDTH;
        spriteH = AKSIL_FRONT_SPRITE_HEIGHT;
    }
    else if (side == BATTLE_SIDE_PLAYER && BattleMember_IsMaren(member))
    {
        spriteW = MAREN_FRONT_SPRITE_WIDTH;
        spriteH = MAREN_FRONT_SPRITE_HEIGHT;
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsTutsil(member))
    {
        spriteW = TUTSIL_FRONT_SPRITE_WIDTH;
        spriteH = TUTSIL_FRONT_SPRITE_HEIGHT;
    }
    else if (side == BATTLE_SIDE_ENEMY && BattleMember_IsElectricArmorBadger(member))
    {
        spriteW = ELECTRIC_ARMOR_BADGER_SPRITE_WIDTH;
        spriteH = ELECTRIC_ARMOR_BADGER_SPRITE_HEIGHT;
    }
    else
    {
        *outX = layout->spriteX;
        *outY = layout->spriteY;
        *outW = layout->spriteW;
        *outH = layout->spriteH;
        return 1;
    }

    *outX = layout->tileX + (layout->tileW - spriteW) / 2;
    *outY = BattleLayout_GetUnitDrawY(side, member, layout);
    *outW = spriteW;
    *outH = spriteH;
    return 1;
}

static void BattleHud_DrawOneTileChrome(const BattleState *state, int side, int slot)
{
    const BattleSlotLayout *layout;
    int occupied;
    u16 fillColor;
    u16 borderColor;

    layout = BattleSlotLayout_Get(side, slot);
    if (!state || !layout)
    {
        return;
    }
    occupied = BattleState_IsSlotOccupied(state, side, slot);
    if (side == BATTLE_SIDE_ENEMY)
    {
        fillColor = occupied ? COLOR_ENEMY_TILE_OCCUPIED_FILL : COLOR_ENEMY_TILE_EMPTY_FILL;
        borderColor = occupied ? COLOR_ENEMY_TILE_OCCUPIED_BORDER : COLOR_ENEMY_TILE_EMPTY_BORDER;
    }
    else
    {
        fillColor = occupied ? COLOR_ALLY_TILE_OCCUPIED_FILL : COLOR_ALLY_TILE_EMPTY_FILL;
        borderColor = occupied ? COLOR_ALLY_TILE_OCCUPIED_BORDER : COLOR_ALLY_TILE_EMPTY_BORDER;
    }
    BattleHud_DrawTileOutline(
        layout->tileX,
        layout->tileY,
        layout->tileW,
        layout->tileH,
        borderColor,
        fillColor);
    if (BattleState_HasTileEffect(state, side, slot))
    {
        const BattleTileEffect *effect = BattleState_GetTileEffect(state, side, slot);
        u16 markerColor = RGB15(31, 20, 0);

        if (effect && effect->effectId == BATTLE_TILE_EFFECT_VEIL_TEST)
        {
            markerColor = RGB15(20, 10, 31);
        }
        Video_DrawRect(layout->tileX + layout->tileW - 6, layout->tileY + 2, 4, 4, markerColor);
    }
    if (BattleState_HasVeilTile(state, side, slot))
    {
        Video_DrawText(
            layout->tileX + 2,
            layout->tileY + 2,
            "V",
            RGB15(10, 28, 31));
    }
}

void BattleHud_DrawKoBurrowDirtyRects(
    const BattleState *state,
    const BattleAction *action,
    void (*restoreRect)(int x, int y, int w, int h))
{
    BattleHudFocus focus;
    unsigned int playerMask;
    unsigned int enemyMask;
    int minX;
    int minY;
    int maxX;
    int maxY;
    int any;
    int sidePass;
    int i;
    int slotCount;

    if (!state || !restoreRect)
    {
        return;
    }

    playerMask = BattleState_GetKoBurrowDirtyPlayerMask(state);
    enemyMask = BattleState_GetKoBurrowDirtyEnemyMask(state);
    if (playerMask == 0 && enemyMask == 0)
    {
        return;
    }

    minX = SCREEN_WIDTH;
    minY = SCREEN_HEIGHT;
    maxX = 0;
    maxY = 0;
    any = 0;

    for (i = 0; i < BATTLE_PLAYER_SLOT_COUNT; i++)
    {
        const BattleSlotLayout *layout;

        if ((playerMask & (1u << i)) == 0)
        {
            continue;
        }
        layout = BattleSlotLayout_Get(BATTLE_SIDE_PLAYER, i);
        if (!layout)
        {
            continue;
        }
        if (layout->tileX < minX)
        {
            minX = layout->tileX;
        }
        if (layout->tileY < minY)
        {
            minY = layout->tileY;
        }
        if (layout->tileX + layout->tileW > maxX)
        {
            maxX = layout->tileX + layout->tileW;
        }
        if (layout->tileY + layout->tileH > maxY)
        {
            maxY = layout->tileY + layout->tileH;
        }
        any = 1;
    }
    for (i = 0; i < BATTLE_ENEMY_SLOT_COUNT; i++)
    {
        const BattleSlotLayout *layout;

        if ((enemyMask & (1u << i)) == 0)
        {
            continue;
        }
        layout = BattleSlotLayout_Get(BATTLE_SIDE_ENEMY, i);
        if (!layout)
        {
            continue;
        }
        if (layout->tileX < minX)
        {
            minX = layout->tileX;
        }
        if (layout->tileY < minY)
        {
            minY = layout->tileY;
        }
        if (layout->tileX + layout->tileW > maxX)
        {
            maxX = layout->tileX + layout->tileW;
        }
        if (layout->tileY + layout->tileH > maxY)
        {
            maxY = layout->tileY + layout->tileH;
        }
        any = 1;
    }

    if (!any || maxX <= minX || maxY <= minY)
    {
        return;
    }

    /* Restore stable field pixels only inside the KO dirty union (no full-field clear). */
    restoreRect(minX, minY, maxX - minX, maxY - minY);

    BattleHud_ComputeFocus(state, action, &focus);

    for (sidePass = 0; sidePass < 2; sidePass++)
    {
        int drawSide = (sidePass == 0) ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;

        slotCount = (drawSide == BATTLE_SIDE_ENEMY)
            ? BATTLE_ENEMY_SLOT_COUNT
            : BATTLE_PLAYER_SLOT_COUNT;
        for (i = 0; i < slotCount; i++)
        {
            const BattleSlotLayout *layout = BattleSlotLayout_Get(drawSide, i);

            if (!layout)
            {
                continue;
            }
            if (!BattleHud_RectsOverlap(
                    layout->tileX,
                    layout->tileY,
                    layout->tileW,
                    layout->tileH,
                    minX,
                    minY,
                    maxX - minX,
                    maxY - minY))
            {
                continue;
            }
            BattleHud_DrawOneTileChrome(state, drawSide, i);
        }
    }

    for (sidePass = 0; sidePass < 2; sidePass++)
    {
        int drawSide = (sidePass == 0) ? BATTLE_SIDE_ENEMY : BATTLE_SIDE_PLAYER;
        u16 unitColor;
        u16 unitDimColor;

        slotCount = (drawSide == BATTLE_SIDE_ENEMY)
            ? BATTLE_ENEMY_SLOT_COUNT
            : BATTLE_PLAYER_SLOT_COUNT;
        if (drawSide == BATTLE_SIDE_ENEMY)
        {
            unitColor = COLOR_ENEMY_UNIT;
            unitDimColor = COLOR_ENEMY_UNIT_DIM;
        }
        else
        {
            unitColor = COLOR_ALLY_UNIT;
            unitDimColor = COLOR_ALLY_UNIT_DIM;
        }

        for (i = 0; i < slotCount; i++)
        {
            const BattleSlotLayout *layout;
            const BattleMember *member;
            BattleUnitDrawStyle style;
            int sx;
            int sy;
            int sw;
            int sh;

            member = BattleState_GetMemberAtSlotConst(state, drawSide, i);
            layout = BattleSlotLayout_Get(drawSide, i);
            if (!member || !layout || member->maxHp <= 0)
            {
                continue;
            }
            if (!BattleHud_TryGetMemberSpriteBounds(member, drawSide, layout, &sx, &sy, &sw, &sh))
            {
                continue;
            }
            /* KO clip lives in the tile; living sprites may overhang into neighboring tiles. */
            if (!BattleHud_RectsOverlap(sx, sy, sw, sh, minX, minY, maxX - minX, maxY - minY) &&
                !BattleHud_RectsOverlap(
                    layout->tileX,
                    layout->tileY,
                    layout->tileW,
                    layout->tileH,
                    minX,
                    minY,
                    maxX - minX,
                    maxY - minY))
            {
                continue;
            }

            style = BattleHud_GetUnitDrawStyle(&focus, drawSide, i);
            if (BattleState_IsAliveOccupiedSlot(state, drawSide, i))
            {
                BattleHud_DrawLivingMemberSprite(
                    member,
                    drawSide,
                    layout,
                    unitColor,
                    unitDimColor,
                    style);
                BattleHud_DrawMemberHpBar(layout, member, drawSide, style);
            }
            else if (BattleMember_IsKoBurrowing(member))
            {
                BattleHud_DrawKoBurrowMember(member, drawSide, layout);
            }
        }
    }
}

void BattleHud_DrawPanelChrome(const BattleState *state)
{
    int panelTop = HUD_COMMAND_PANEL_TOP_Y;
    int panelHeight = HUD_COMMAND_PANEL_HEIGHT;
    u16 panelFill = HUD_COMMAND_PANEL_FILL;

    /* Skills phase panel ownership lives in Battle_DrawPlayerSkillPanel.
     * Other phases keep y=112 so the y=[108,112) strip can be restored as field
     * when leaving Skills. Expanded Skills clear is not performed here. */
    if (state && state->phase == BATTLE_PHASE_PLAYER_SKILL)
    {
        panelTop = HUD_SKILLS_PANEL_TOP_Y;
        panelHeight = HUD_SKILLS_PANEL_HEIGHT;
        panelFill = HUD_SKILLS_PANEL_FILL;
    }
    Video_DrawRect(
        0,
        panelTop,
        SCREEN_WIDTH,
        panelHeight,
        panelFill);
    /* Border Color outlines are disabled; fill-only panel chrome. */
    if (state->phase == BATTLE_PHASE_VICTORY)
    {
        if (!BattleMessageQueue_IsBusy())
        {
            Video_DrawText(8, HUD_PANEL_BODY_Y, "VICTORY", HUD_TEXT_HIGHLIGHT);
        }
        return;
    }
    if (state->phase == BATTLE_PHASE_DEFEAT)
    {
        if (!BattleMessageQueue_IsBusy())
        {
            Video_DrawText(8, HUD_PANEL_BODY_Y, "DEFEAT", RGB15(31, 8, 8));
        }
        return;
    }
    if (state->phase == BATTLE_PHASE_ESCAPE)
    {
        if (!BattleMessageQueue_IsBusy())
        {
            Video_DrawText(8, HUD_PANEL_BODY_Y, "ESCAPED", RGB15(20, 28, 31));
        }
        return;
    }
}
