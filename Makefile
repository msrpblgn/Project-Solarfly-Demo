# Project Solarfly GBA — devkitARM Makefile

ifeq ($(strip $(DEVKITARM)),)
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
endif
DEVKITPRO ?= /opt/devkitpro

TARGET := project_solarfly_gba
BUILD  := build

PREFIX  := $(DEVKITARM)/bin/arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
GBAFIX  ?= $(DEVKITPRO)/tools/bin/gbafix

INCLUDE := -Iinclude
CFLAGS  := -mthumb -mthumb-interwork -O2 -Wall -Wextra $(INCLUDE) -MMD -MP
LDFLAGS := -specs=gba.specs -Wl,-Map,$(BUILD)/$(TARGET).map
LIBS    := -lgcc

# Host Python for asset converters. Override with PYTHON=... when needed.
# Auto-detect skips the Windows Store stub under …/WindowsApps/… (exits non-zero).
ifndef PYTHON
PYTHON := $(shell (command -v python3; command -v python) 2>/dev/null | grep -v WindowsApps | head -n 1)
ifeq ($(strip $(PYTHON)),)
PYTHON := $(firstword \
	$(wildcard /mnt/c/msys64/ucrt64/bin/python3.exe) \
	$(wildcard /mnt/c/msys64/mingw64/bin/python3.exe) \
	$(wildcard /mnt/c/Users/*/AppData/Local/Programs/Python/Python*/python.exe) \
	$(wildcard /mnt/c/Users/*/anaconda3/python.exe))
endif
ifeq ($(strip $(PYTHON)),)
PYTHON := python3
endif
endif
MAP_COMPILER := tools/compile_tiled_map.py
PLAYER_SPRITE_CONVERTER := tools/convert_overworld_player_idles.py
DIALOGUE_FRAME_CONVERTER := tools/convert_dialogue_frame.py
DIALOGUE_PORTRAIT_CONVERTER := tools/convert_dialogue_portraits.py

OVERWORLD_TEST_TMX := assets_src/overworld/maps/overworld_test.tmx
OVERWORLD_TEST_TSX := assets_src/overworld/tilesets/overworld_test.tsx
OVERWORLD_TEST_PREVIEW := assets_src/overworld/tilesets/overworld_test_preview.png
OVERWORLD_COLLISION_TSX := assets_src/overworld/tilesets/overworld_collision_profiles.tsx
OVERWORLD_COLLISION_PREVIEW := assets_src/overworld/tilesets/overworld_collision_profiles.png
OVERWORLD_TEST_MAP_C := src/assets/maps/overworld_test_map.c
OVERWORLD_TEST_MAP_H := include/assets/maps/overworld_test_map.h
OVERWORLD_PLAYER_ASE_DIR := assets_src/overworld/player
OVERWORLD_PLAYER_ASE_FILES := \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_down.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_downleft.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_left.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_upleft.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_up.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_upright.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_right.aseprite \
	$(OVERWORLD_PLAYER_ASE_DIR)/idle_downright.aseprite
OVERWORLD_PLAYER_SPRITE_C := src/assets/overworld_player_sprite.c
OVERWORLD_PLAYER_SPRITE_H := include/assets/overworld_player_sprite.h
DIALOGUE_FRAME_PNG := assets_src/ui/Dialogue\ Ui.png
DIALOGUE_FRAME_C := src/assets/dialogue_frame.c
DIALOGUE_FRAME_H := include/assets/dialogue_frame.h
DIALOGUE_PORTRAITS_C := src/assets/dialogue_portraits.c
DIALOGUE_PORTRAITS_H := include/assets/dialogue_portraits.h

GENERATED_MAP_CFILES := \
	$(OVERWORLD_TEST_MAP_C)
GENERATED_PLAYER_SPRITE_CFILES := \
	$(OVERWORLD_PLAYER_SPRITE_C)
GENERATED_DIALOGUE_FRAME_CFILES := \
	$(DIALOGUE_FRAME_C)
GENERATED_DIALOGUE_PORTRAIT_CFILES := \
	$(DIALOGUE_PORTRAITS_C)

CFILES := \
	src/main.c \
	src/core/system.c \
	src/core/input.c \
	src/core/video.c \
	src/core/oam.c \
	src/core/random.c \
	src/dialogue/dialogue_resources.c \
	src/dialogue/dialogue_panel.c \
	src/dialogue/dialogue_text_printer.c \
	src/dialogue/dialogue_portrait_controller.c \
	src/dialogue/dialogue_scene.c \
	src/dialogue/dialogue_overlay.c \
	src/dialogue/dialogue_scene_runner.c \
	src/dialogue/dialogue_demo.c \
	src/overworld/overworld.c \
	src/overworld/overworld_map.c \
	src/overworld/overworld_player.c \
	src/overworld/overworld_collision.c \
	src/overworld/overworld_streamer.c \
	src/battle/battle.c \
	src/battle/battle_state.c \
	src/battle/battle_member.c \
	src/battle/stat_curve.c \
	src/battle/battle_slot.c \
	src/battle/battle_move.c \
	src/battle/battle_action.c \
	src/battle/battle_result.c \
	src/battle/damage_calc.c \
	src/battle/custom_battle_setup.c \
	src/battle/target_selection.c \
	src/battle/battle_resolve.c \
	src/battle/battle_enemy_turn.c \
	src/battle/turn_order.c \
	src/battle/test_battle.c \
	src/battle/battle_draw.c \
	src/battle/battle_skills_panel.c \
	src/battle_ui/battle_hud.c \
	src/battle_ui/enemy_status_bar.c \
	src/battle_ui/battle_slot_layout.c \
	src/battle_ui/battle_menu.c \
	src/battle_ui/move_presentation.c \
	src/battle_ui/skill_card_slide.c \
	src/battle_ui/skill_card.c \
	src/battle_ui/skill_card_present_trace.c \
	src/battle_ui/party_screen.c \
	src/battle_ui/battle_lp_prompt.c \
	src/battle_ui/battle_message_queue.c \
	src/battle_ui/target_cursor.c \
	src/battle_ui/damage_numbers.c \
	src/battle_ui/hp_lp_sc_display.c \
	src/battle_feel/battle_feel_config.c \
	src/battle_feel/battle_pacing.c \
	src/battle_feel/battle_feedback.c \
	src/data/move_database.c \
	src/data/element_database.c \
	src/data/race_database.c \
	src/data/status_database.c \
	src/data/item_database.c \
	src/data/member_database.c \
	src/data/test_battle_data.c \
	src/assets/aksil_front_sprite.c \
	src/assets/maren_front_sprite.c \
	src/assets/protagonist_front_sprite.c \
	src/assets/tutsil_front_sprite.c \
	src/assets/place_holder_enemy_sprite.c \
	src/assets/electric_armor_badger_sprite.c \
	src/assets/placeholder_sprites.c \
	src/assets/placeholder_backgrounds.c \
	src/assets/petalburg_battle_background.c \
	src/assets/placeholder_palettes.c \
	$(GENERATED_PLAYER_SPRITE_CFILES) \
	$(GENERATED_MAP_CFILES) \
	$(GENERATED_DIALOGUE_FRAME_CFILES) \
	$(GENERATED_DIALOGUE_PORTRAIT_CFILES)

OFILES := $(patsubst src/%.c,$(BUILD)/%.o,$(CFILES))
DFILES := $(OFILES:.o=.d)

.PHONY: all clean info maps player-sprite dialogue-frame dialogue-portraits

all: $(BUILD)/$(TARGET).gba

maps: $(GENERATED_MAP_CFILES)
player-sprite: $(GENERATED_PLAYER_SPRITE_CFILES)
dialogue-frame: $(GENERATED_DIALOGUE_FRAME_CFILES)
dialogue-portraits: $(GENERATED_DIALOGUE_PORTRAIT_CFILES)

# One recipe writes both outputs. If only the header is missing, regenerate both.
$(OVERWORLD_TEST_MAP_H): $(OVERWORLD_TEST_MAP_C)
	@if [ ! -f $@ ]; then \
		$(PYTHON) $(MAP_COMPILER) \
			--input $(OVERWORLD_TEST_TMX) \
			--output-c $(OVERWORLD_TEST_MAP_C) \
			--output-h $(OVERWORLD_TEST_MAP_H) \
			--symbol-prefix gOverworldTestMap; \
	fi
$(OVERWORLD_TEST_MAP_C): $(OVERWORLD_TEST_TMX) $(OVERWORLD_TEST_TSX) $(OVERWORLD_TEST_PREVIEW) $(OVERWORLD_COLLISION_TSX) $(OVERWORLD_COLLISION_PREVIEW) $(MAP_COMPILER)
	$(PYTHON) $(MAP_COMPILER) \
		--input $(OVERWORLD_TEST_TMX) \
		--output-c $(OVERWORLD_TEST_MAP_C) \
		--output-h $(OVERWORLD_TEST_MAP_H) \
		--symbol-prefix gOverworldTestMap

$(OVERWORLD_PLAYER_SPRITE_H): $(OVERWORLD_PLAYER_SPRITE_C)
	@if [ ! -f $@ ]; then \
		$(PYTHON) $(PLAYER_SPRITE_CONVERTER) \
			--aseprite-dir $(OVERWORLD_PLAYER_ASE_DIR) \
			--output-c $(OVERWORLD_PLAYER_SPRITE_C) \
			--output-h $(OVERWORLD_PLAYER_SPRITE_H); \
	fi
$(OVERWORLD_PLAYER_SPRITE_C): $(OVERWORLD_PLAYER_ASE_FILES) $(PLAYER_SPRITE_CONVERTER) tools/inspect_aseprite_idle.py
	$(PYTHON) $(PLAYER_SPRITE_CONVERTER) \
		--aseprite-dir $(OVERWORLD_PLAYER_ASE_DIR) \
		--output-c $(OVERWORLD_PLAYER_SPRITE_C) \
		--output-h $(OVERWORLD_PLAYER_SPRITE_H)

$(DIALOGUE_FRAME_H): $(DIALOGUE_FRAME_C)
	@if [ ! -f $@ ]; then \
		$(PYTHON) $(DIALOGUE_FRAME_CONVERTER) \
			--input "assets_src/ui/Dialogue Ui.png" \
			--output-c $(DIALOGUE_FRAME_C) \
			--output-h $(DIALOGUE_FRAME_H); \
	fi
$(DIALOGUE_FRAME_C): $(DIALOGUE_FRAME_PNG) $(DIALOGUE_FRAME_CONVERTER)
	$(PYTHON) $(DIALOGUE_FRAME_CONVERTER) \
		--input "assets_src/ui/Dialogue Ui.png" \
		--output-c $(DIALOGUE_FRAME_C) \
		--output-h $(DIALOGUE_FRAME_H)

$(DIALOGUE_PORTRAITS_H): $(DIALOGUE_PORTRAITS_C)
	@if [ ! -f $@ ]; then \
		$(PYTHON) $(DIALOGUE_PORTRAIT_CONVERTER); \
	fi
$(DIALOGUE_PORTRAITS_C): $(DIALOGUE_PORTRAIT_CONVERTER) \
		assets_src/ui/dialogue/aksil_front_4bpp_candidate.png \
		assets_src/ui/dialogue/protagonist_front_4bpp_candidate.png
	$(PYTHON) $(DIALOGUE_PORTRAIT_CONVERTER)

$(BUILD)/$(TARGET).gba: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -p -t"SOLARFLY" -cSOLA -mSF1
	@echo "Built ROM: $@"

$(BUILD)/$(TARGET).elf: $(OFILES)
	@mkdir -p $(BUILD)
	$(CC) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

# Ensure generated data exists before compiling sources that include it.
$(BUILD)/overworld/overworld_map.o: $(OVERWORLD_TEST_MAP_H)
$(BUILD)/assets/maps/overworld_test_map.o: $(OVERWORLD_TEST_MAP_C) $(OVERWORLD_TEST_MAP_H)
$(BUILD)/overworld/overworld.o: $(OVERWORLD_PLAYER_SPRITE_H)
$(BUILD)/overworld/overworld_player.o: $(OVERWORLD_PLAYER_SPRITE_H)
$(BUILD)/assets/overworld_player_sprite.o: $(OVERWORLD_PLAYER_SPRITE_C) $(OVERWORLD_PLAYER_SPRITE_H)
$(BUILD)/dialogue/dialogue_panel.o: $(DIALOGUE_FRAME_H)
$(BUILD)/dialogue/dialogue_demo.o: $(DIALOGUE_FRAME_H) $(DIALOGUE_PORTRAITS_H)
$(BUILD)/dialogue/dialogue_portrait_controller.o: $(DIALOGUE_PORTRAITS_H)
$(BUILD)/dialogue/dialogue_scene.o: $(DIALOGUE_FRAME_H)
$(BUILD)/dialogue/dialogue_scene_runner.o: $(DIALOGUE_FRAME_H) $(DIALOGUE_PORTRAITS_H)
$(BUILD)/assets/dialogue_frame.o: $(DIALOGUE_FRAME_C) $(DIALOGUE_FRAME_H)
$(BUILD)/assets/dialogue_portraits.o: $(DIALOGUE_PORTRAITS_C) $(DIALOGUE_PORTRAITS_H)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DFILES)

clean:
	rm -rf $(BUILD)

info:
	@echo "DEVKITARM=$(DEVKITARM)"
	@echo "CC=$(CC)"
	@echo "Sources: $(words $(CFILES)) files"
	@echo "PYTHON=$(PYTHON)"
