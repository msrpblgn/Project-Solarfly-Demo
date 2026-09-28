#ifndef DIALOGUE_RESOURCES_H
#define DIALOGUE_RESOURCES_H

#include "assets/overworld_player_sprite.h"
#include "core/bg_vram_layout.h"
#include "core/gba_types.h"
#include "core/oam.h"
#include "core/video.h"

/*
 * Named dialogue resource reservations for Milestone 0.
 * Mode 0 (overworld field) only — no Mode 3 or OBJ-tile ranges are reserved yet.
 * See docs/DIALOGUE_RESOURCE_MAP.md for ownership evidence and lifetime rules.
 */

/* Visual reference geometry from assets_src/ui/Dialogue Ui.png (not runtime VRAM). */
#define DIALOGUE_UI_REF_WIDTH SCREEN_WIDTH
#define DIALOGUE_UI_REF_HEIGHT SCREEN_HEIGHT
#define DIALOGUE_UI_PANEL_X 0
#define DIALOGUE_UI_PANEL_Y 108
#define DIALOGUE_UI_PANEL_W SCREEN_WIDTH
#define DIALOGUE_UI_PANEL_H 51

/* Approved mockup text metrics: y values are glyph-top rows, not baselines. */
#define DIALOGUE_UI_TEXT_X 12
#define DIALOGUE_UI_TEXT_LINE1_Y 132
#define DIALOGUE_UI_TEXT_LINE2_Y 144
#define DIALOGUE_UI_TEXT_MAX_END_X 221
#define DIALOGUE_UI_NAMEPLATE_Y 106
#define DIALOGUE_UI_NAMEPLATE_LEFT_X 8
#define DIALOGUE_UI_NAMEPLATE_PAD_X 4
#define DIALOGUE_UI_NAMEPLATE_PAD_Y 2
#define DIALOGUE_UI_NAMEPLATE_HEIGHT 11
#define DIALOGUE_UI_PROMPT_X 228
#define DIALOGUE_UI_PROMPT_Y 149
#define DIALOGUE_UI_PROMPT_W 6
#define DIALOGUE_UI_PROMPT_H 6
#define DIALOGUE_UI_DEMO_HINT_Y 8

/* Future portrait presentation targets (Milestone 1 approved). Not rendered yet. */
#define DIALOGUE_UI_PORTRAIT_PRESENTATION_W 128
#define DIALOGUE_UI_PORTRAIT_PRESENTATION_H 128
#define DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_X (-8)
#define DIALOGUE_UI_PORTRAIT_ACTIVE_LEFT_Y 36
#define DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_X (-16)
#define DIALOGUE_UI_PORTRAIT_INACTIVE_LEFT_Y 44
#define DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_X 116
#define DIALOGUE_UI_PORTRAIT_ACTIVE_RIGHT_Y 36
#define DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_X 124
#define DIALOGUE_UI_PORTRAIT_INACTIVE_RIGHT_Y 44

/* Chest cutoff: OBJ window exclusive bottom (matches mockup solid panel row). */
#define DIALOGUE_UI_PORTRAIT_CLIP_Y 116
#define DIALOGUE_UI_PORTRAIT_OFFSCREEN_LEFT_X (-128)
#define DIALOGUE_UI_PORTRAIT_OFFSCREEN_RIGHT_X 240

/* Mode 0 BG0: dialogue panel and field UI (mutually exclusive with Mode 3). */
#define DIALOGUE_MODE0_BG_NUMBER 0
#define DIALOGUE_MODE0_BG_CHARBLOCK 1
#define DIALOGUE_MODE0_BG_SCREENBLOCK 30
#define DIALOGUE_MODE0_BG_PRIORITY 0
#define DIALOGUE_MODE0_BG_TILE_FORMAT_4BPP 1
#define DIALOGUE_MODE0_BG_MAP_TILE_W 32
#define DIALOGUE_MODE0_BG_MAP_TILE_H 32

/* BG palette color indices (shared 256-entry BG palette in Mode 0). */
#define DIALOGUE_MODE0_BG_PALETTE_COLOR_BASE 16
#define DIALOGUE_MODE0_BG_PALETTE_COLOR_COUNT 16

/* Portrait OAM slots (OBJ tile VRAM ranges deferred until 4bpp vs 8bpp decision). */
#define DIALOGUE_OAM_INDEX_LEFT_PORTRAIT 1
#define DIALOGUE_OAM_INDEX_RIGHT_PORTRAIT 2
#define DIALOGUE_OBJ_PALETTE_BANK_LEFT 1
#define DIALOGUE_OBJ_PALETTE_BANK_RIGHT 2
/* Shared 2x scale matrix; occupies fill of OAM entries 12..15 (not portrait objs). */
#define DIALOGUE_OBJ_AFFINE_MATRIX_PORTRAIT 3
#define DIALOGUE_OBJ_PRIORITY_PORTRAIT 1

#define DIALOGUE_MODE0_CHARBLOCK_TILE_CAPACITY 512
#define DIALOGUE_MODE0_PANEL_TILE_W ((DIALOGUE_UI_PANEL_W + 7) / 8)
#define DIALOGUE_MODE0_PANEL_TILE_H ((DIALOGUE_UI_PANEL_H + 7) / 8)
#define DIALOGUE_MODE0_PANEL_TILE_COUNT \
    (DIALOGUE_MODE0_PANEL_TILE_W * DIALOGUE_MODE0_PANEL_TILE_H)

#endif
