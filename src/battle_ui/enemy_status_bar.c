#include "battle_ui/enemy_status_bar.h"
#include "battle_ui/battle_slot_layout.h"
#include "battle_ui/hud_layout.h"
#include "battle/battle_member.h"
#include "battle/battle_slot.h"
#include "battle/battle_state.h"
#include "data/status_database.h"
#include "core/video.h"
#include "core/gba_types.h"

/* Alive: three status icons only. Down: compact DN strip. */
#define MEMBER_STATUS_STRIP_H 9
#define MEMBER_STATUS_STRIP_GAP_ABOVE_TILE 2
#define MEMBER_STATUS_STRIP_GAP_ABOVE_HP 2
#define MEMBER_STATUS_ICON_SIZE 5
#define MEMBER_STATUS_ICON_GAP 2
#define MEMBER_STATUS_ALIVE_CLUSTER_W \
    ((BATTLE_MEMBER_STATUS_SLOT_COUNT * MEMBER_STATUS_ICON_SIZE) \
    + ((BATTLE_MEMBER_STATUS_SLOT_COUNT - 1) * MEMBER_STATUS_ICON_GAP))
#define MEMBER_STATUS_DOWN_STRIP_W 22
/* Legacy clear width covers prior wider OK/reaction bar to avoid stale pixels. */
#define MEMBER_STATUS_LEGACY_CLEAR_W 46

#define BATTLE_HP_BAR_H 4
#define BATTLE_HP_BAR_GAP 2

/*
 * Status slots are visual scaffolding only. Future status application logic can
 * populate BattleMember.statuses and this bar will display them.
 *
 * Effectiveness reactions are stored in BattleState for future feedback, but the
 * field HUD currently hides HE/CI indicators to keep the overhead bar clean.
 */
#define COLOR_ENEMY_STATUS_FILL RGB15(8, 6, 10)
#define COLOR_ENEMY_STATUS_BORDER RGB15(22, 12, 18)
#define COLOR_ENEMY_STATUS_DOWN RGB15(20, 10, 10)

static void MemberStatusBar_GetEnemyAliveClusterPosition(
    const BattleSlotLayout *layout,
    const BattleMember *member,
    int *outX,
    int *outY)
{
    int unitTopY;
    int hpBarY;
    int x;
    int y;

    unitTopY = BattleLayout_GetUnitTopY(BATTLE_SIDE_ENEMY, member, layout);
    hpBarY = unitTopY - BATTLE_HP_BAR_GAP - BATTLE_HP_BAR_H + BATTLE_HP_BAR_OFFSET_Y;
    if (hpBarY < 0)
    {
        hpBarY = 0;
    }
    y = hpBarY - MEMBER_STATUS_STRIP_GAP_ABOVE_HP - MEMBER_STATUS_STRIP_H;
    if (y < 0)
    {
        y = 0;
    }
    x = layout->tileX + ((layout->tileW - MEMBER_STATUS_ALIVE_CLUSTER_W) / 2);
    *outX = x;
    *outY = y;
}

static void MemberStatusBar_GetAllyAliveClusterPosition(
    const BattleSlotLayout *layout,
    const BattleMember *member,
    int *outX,
    int *outY)
{
    int unitTopY;
    int hpBarY;
    int x;
    int y;

    unitTopY = BattleLayout_GetUnitTopY(BATTLE_SIDE_PLAYER, member, layout);
    hpBarY = unitTopY - BATTLE_HP_BAR_GAP - BATTLE_HP_BAR_H + BATTLE_HP_BAR_OFFSET_Y;
    if (hpBarY < 0)
    {
        hpBarY = 0;
    }
    y = hpBarY - MEMBER_STATUS_STRIP_GAP_ABOVE_HP - MEMBER_STATUS_STRIP_H;
    if (y < 0)
    {
        y = 0;
    }
    x = layout->tileX + ((layout->tileW - MEMBER_STATUS_ALIVE_CLUSTER_W) / 2);
    *outX = x;
    *outY = y;
}

static void MemberStatusBar_GetDownStripPosition(
    const BattleSlotLayout *layout,
    int *outX,
    int *outY)
{
    int x;
    int y;

    x = layout->tileX + ((layout->tileW - MEMBER_STATUS_DOWN_STRIP_W) / 2);
    y = layout->tileY - MEMBER_STATUS_STRIP_H - MEMBER_STATUS_STRIP_GAP_ABOVE_TILE;
    if (y < 0)
    {
        y = 0;
    }
    *outX = x;
    *outY = y;
}

static void MemberStatusBar_ClearLegacyArea(int centerX, int tileW, int barY)
{
    int clearX;

    clearX = centerX + ((tileW - MEMBER_STATUS_LEGACY_CLEAR_W) / 2);
    if (clearX < 0)
    {
        clearX = 0;
    }
    Video_DrawRect(clearX, barY, MEMBER_STATUS_LEGACY_CLEAR_W, MEMBER_STATUS_STRIP_H, COLOR_ENEMY_STATUS_FILL);
}

static void MemberStatusBar_DrawAliveStatusSlots(int clusterX, int clusterY, const BattleMember *member)
{
    int i;
    BattleStatusId balanceStatus;
    u16 balanceFill;
    u16 balanceBorder;

    if (!member)
    {
        return;
    }

    balanceStatus = BattleMember_GetBalanceStatus(member);
    balanceFill = StatusDatabase_GetIconFillColor(balanceStatus);
    balanceBorder = StatusDatabase_GetIconBorderColor(balanceStatus);

    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        BattleStatusId status;
        u16 fillColor;
        u16 borderColor;
        int iconX;
        int iconY;

        status = BattleMember_GetStatusAt(member, i);
        if (balanceStatus != BATTLE_STATUS_NONE)
        {
            fillColor = balanceFill;
            borderColor = balanceBorder;
        }
        else
        {
            fillColor = StatusDatabase_GetIconFillColor(status);
            borderColor = StatusDatabase_GetIconBorderColor(status);
        }
        iconX = clusterX + (i * (MEMBER_STATUS_ICON_SIZE + MEMBER_STATUS_ICON_GAP));
        iconY = clusterY + ((MEMBER_STATUS_STRIP_H - MEMBER_STATUS_ICON_SIZE) / 2);

        Video_DrawRect(iconX, iconY, MEMBER_STATUS_ICON_SIZE, MEMBER_STATUS_ICON_SIZE, fillColor);
        Video_DrawRect(iconX, iconY, MEMBER_STATUS_ICON_SIZE, 1, borderColor);
        Video_DrawRect(iconX, iconY + MEMBER_STATUS_ICON_SIZE - 1, MEMBER_STATUS_ICON_SIZE, 1, borderColor);
        Video_DrawRect(iconX, iconY, 1, MEMBER_STATUS_ICON_SIZE, borderColor);
        Video_DrawRect(iconX + MEMBER_STATUS_ICON_SIZE - 1, iconY, 1, MEMBER_STATUS_ICON_SIZE, borderColor);
    }
}

static void MemberStatusBar_DrawDownStrip(int barX, int barY)
{
    Video_DrawRect(barX, barY, MEMBER_STATUS_DOWN_STRIP_W, MEMBER_STATUS_STRIP_H, COLOR_ENEMY_STATUS_FILL);
    Video_DrawRect(barX, barY, MEMBER_STATUS_DOWN_STRIP_W, 1, COLOR_ENEMY_STATUS_BORDER);
    Video_DrawRect(barX, barY + MEMBER_STATUS_STRIP_H - 1, MEMBER_STATUS_DOWN_STRIP_W, 1, COLOR_ENEMY_STATUS_BORDER);
    Video_DrawRect(barX, barY, 1, MEMBER_STATUS_STRIP_H, COLOR_ENEMY_STATUS_BORDER);
    Video_DrawRect(barX + MEMBER_STATUS_DOWN_STRIP_W - 1, barY, 1, MEMBER_STATUS_STRIP_H, COLOR_ENEMY_STATUS_BORDER);
    Video_DrawText(barX + 5, barY + 1, "DN", COLOR_ENEMY_STATUS_DOWN);
}

static void MemberStatusBar_DrawEnemyForSlot(const BattleState *state, int enemySlot)
{
    const BattleSlotLayout *layout;
    const BattleMember *member;
    int barX;
    int barY;
    int isAlive;

    if (!state || enemySlot < 0 || enemySlot >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return;
    }
    if (!BattleState_IsSlotOccupied(state, BATTLE_SIDE_ENEMY, enemySlot))
    {
        return;
    }

    layout = BattleSlotLayout_Get(BATTLE_SIDE_ENEMY, enemySlot);
    member = BattleState_GetMemberAtSlotConst(state, BATTLE_SIDE_ENEMY, enemySlot);
    if (!layout || !member)
    {
        return;
    }

    isAlive = BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, enemySlot);
    if (isAlive)
    {
        MemberStatusBar_GetEnemyAliveClusterPosition(layout, member, &barX, &barY);
        MemberStatusBar_DrawAliveStatusSlots(barX, barY, member);
    }
    else
    {
        MemberStatusBar_GetDownStripPosition(layout, &barX, &barY);
        MemberStatusBar_ClearLegacyArea(layout->tileX, layout->tileW, barY);
        MemberStatusBar_DrawDownStrip(barX, barY);
    }
}

static void MemberStatusBar_DrawAllyForSlot(const BattleState *state, int allySlot)
{
    const BattleSlotLayout *layout;
    const BattleMember *member;
    int barX;
    int barY;

    if (!state || allySlot < 0 || allySlot >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return;
    }
    if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, allySlot))
    {
        return;
    }

    layout = BattleSlotLayout_Get(BATTLE_SIDE_PLAYER, allySlot);
    member = BattleState_GetMemberAtSlotConst(state, BATTLE_SIDE_PLAYER, allySlot);
    if (!layout || !member)
    {
        return;
    }
    if (!BattleMember_HasAnyStatus(member))
    {
        return;
    }

    MemberStatusBar_GetAllyAliveClusterPosition(layout, member, &barX, &barY);
    MemberStatusBar_DrawAliveStatusSlots(barX, barY, member);
}

void EnemyStatusBar_DrawForSlot(const BattleState *state, int enemySlot)
{
    MemberStatusBar_DrawEnemyForSlot(state, enemySlot);
}

void EnemyStatusBar_DrawAll(const BattleState *state)
{
    int slot;

    if (!state)
    {
        return;
    }

    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        MemberStatusBar_DrawEnemyForSlot(state, slot);
    }
    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        MemberStatusBar_DrawAllyForSlot(state, slot);
    }
}
