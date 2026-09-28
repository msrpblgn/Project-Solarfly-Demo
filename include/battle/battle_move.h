#ifndef BATTLE_MOVE_H
#define BATTLE_MOVE_H

#include "battle/battle_element.h"
#include "battle/battle_status.h"

typedef enum MoveId {
    MOVE_NONE = 0,
    MOVE_PRESSURE_CANNON,
    MOVE_LUCKY_SWING_TEST,
    MOVE_STEP_TEST,
    MOVE_LONG_SHOT_TEST,
    MOVE_BURN_TEST,
    MOVE_POISON_TEST,
    MOVE_CURSE_TEST,
    MOVE_MATTER_STATUS_TEST,
    MOVE_INSTINCT_STATUS_TEST,
    MOVE_BALANCE_STATUS_TEST,
    MOVE_CLEAR_STATUS_TEST,
    MOVE_DARK_TEST,
    MOVE_LIGHT_TEST,
    MOVE_CHAOS_TEST,
    MOVE_SWING,
    MOVE_JAB,
    MOVE_DIRECT_ATTACK,
    MOVE_FAST_PUNCH,
    MOVE_KICK,
    MOVE_HEADBUTT,
    MOVE_PUNT,
    MOVE_SLAP,
    MOVE_STAB,
    MOVE_PUNCH,
    MOVE_SHOOT,
    MOVE_FLASH,
    MOVE_SPOUT,
    MOVE_LONG_SHOT,
    MOVE_EQUALIZER,
    MOVE_YOUNG_BLOOD,
    MOVE_TIDE,
    MOVE_TIPPING_THE_SCALES,
    MOVE_PEP_TALK,
    MOVE_FAKIE,
    MOVE_SNIPE_DOWN,
    MOVE_HAMMER_FIST,
    MOVE_PHANTOM_PHIST,
    MOVE_VORTEX_ESCAPE,
    MOVE_HOLD_BACK,
    MOVE_INFLUENCE,
    MOVE_FLAME_CHARGE,
    MOVE_TEMPERATURE_DIFFERENCE,
    MOVE_UPPERCUT,
    MOVE_ELECTRO_THERAPY,
    MOVE_CHARGE,
    MOVE_BUBBLE,
    MOVE_WIDE_THROW,
    MOVE_VEIL,
    MOVE_VERTIGO,
    /* 41A metadata-only stubs; not selectable until gameplay milestones. */
    MOVE_HEALING_ROLLOUT,
    MOVE_LAST_CHANCE,
    MOVE_FIERY_BLOOD,
    /* 41B-A Petalburg kit alignment */
    MOVE_FIERY_STRIKE,
    MOVE_ALLY_SWAP,
    MOVE_RAM,
    MOVE_HAIL_MARY,
    MOVE_TEAM_FOCUS,
    MOVE_IMMENSE_SUPPORT,
    MOVE_STATIC_LOCK
} MoveId;

typedef enum MoveWeaponKind {
    MOVE_WEAPON_KIND_NONE = 0,
    MOVE_WEAPON_KIND_SLINGSHOT,
    MOVE_WEAPON_KIND_SWORD
} MoveWeaponKind;

typedef enum MoveCategory {
    MOVE_CATEGORY_PHYSICAL = 0,
    MOVE_CATEGORY_SPECIAL,
    MOVE_CATEGORY_SUPPORT,
    MOVE_CATEGORY_STATUS,
    MOVE_CATEGORY_DISRUPT
} MoveCategory;

/*
 * Production target patterns. Values 0-3 match legacy TargetPattern for compatibility.
 */
typedef enum MoveTargetPattern {
    MOVE_TARGET_SINGLE_ENEMY = 0,
    MOVE_TARGET_SELF,
    MOVE_TARGET_ALL_ENEMIES,
    MOVE_TARGET_OUTER_ENEMIES,
    MOVE_TARGET_ALLY_SLOT,
    MOVE_TARGET_SINGLE_ALLY,
    MOVE_TARGET_ADJACENT_ALLIES,
    MOVE_TARGET_ALL_ALLIES,
    MOVE_TARGET_USER_AND_ENEMY
} MoveTargetPattern;

#define TARGET_PATTERN_SINGLE_ENEMY MOVE_TARGET_SINGLE_ENEMY
#define TARGET_PATTERN_SELF MOVE_TARGET_SELF
#define TARGET_PATTERN_ALL_ENEMIES MOVE_TARGET_ALL_ENEMIES
#define TARGET_PATTERN_ALLY_SLOT MOVE_TARGET_ALLY_SLOT
#define TARGET_PATTERN_SINGLE_ALLY MOVE_TARGET_SINGLE_ALLY
#define TARGET_PATTERN_ADJACENT_ALLIES MOVE_TARGET_ADJACENT_ALLIES

typedef MoveTargetPattern TargetPattern;

typedef enum MoveEffectKind {
    MOVE_EFFECT_KIND_NONE = 0,
    MOVE_EFFECT_KIND_DAMAGE,
    MOVE_EFFECT_KIND_INFLICT_STATUS,
    MOVE_EFFECT_KIND_HEAL_HP,
    MOVE_EFFECT_KIND_BUBBLE_PROTECTION,
    MOVE_EFFECT_KIND_VEIL_LUCK_BUFF,
    MOVE_EFFECT_KIND_CLEAR_STATUS,
    MOVE_EFFECT_KIND_MOVE_TO_SLOT,
    MOVE_EFFECT_KIND_MOVEMENT_LOCK,
    MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE,
    MOVE_EFFECT_KIND_BONUS_ACTION,
    MOVE_EFFECT_KIND_INCOMING_DAMAGE_REDUCTION,
    MOVE_EFFECT_KIND_PENDING_ATTACK_ALL_ENEMIES,
    MOVE_EFFECT_KIND_PENDING_WEAPON_ATTACK_ELEMENT,
    MOVE_EFFECT_KIND_PENDING_NEXT_MOVE_ELEMENT,
    MOVE_EFFECT_KIND_EXTRA_ACTION_NEXT_TURN,
    MOVE_EFFECT_KIND_ENCORE,
    MOVE_EFFECT_KIND_STAT_MODIFIER,
    MOVE_EFFECT_KIND_SC_DELTA,
    MOVE_EFFECT_KIND_LP_DELTA,
    MOVE_EFFECT_KIND_DISTANCE_SCALE,
    MOVE_EFFECT_KIND_VERTIGO_WAVE_DAMAGE,
    MOVE_EFFECT_KIND_SWAP_WITH_ALLY,
    MOVE_EFFECT_KIND_TEAM_FOCUS,
    MOVE_EFFECT_KIND_LAST_CHANCE,
    MOVE_EFFECT_KIND_ON_DAMAGE_SELF_STAT_MOD,
    MOVE_EFFECT_KIND_PENDING_NEXT_DAMAGE_DOUBLE,
    MOVE_EFFECT_KIND_AUTO_PUSH_ON_DAMAGE,
    MOVE_EFFECT_KIND_DEBUG_STATUS_CYCLE,
    MOVE_EFFECT_KIND_DEBUG_CLEAR_STATUS
} MoveEffectKind;

#define MOVE_EFFECT_PARAM_NONE 0
#define MOVE_EFFECT_BONUS_ACTION_REQUIRES_LP 1
#define MOVE_EFFECT_EXTRA_ACTION_NEXT_TURN_DEFAULT 1
/*
 * TODO(source): exact Charge speed boost percent is design-pending; 15% matches other modifiers.
 */
#define MOVE_CHARGE_SPEED_BOOST_PERCENT 15
#define MOVE_MAX_EFFECTS 3

/*
 * DAMAGE effect: paramA = hit power; paramB = optional per-hit crit permille override.
 * Use MOVE_EFFECT_DAMAGE_CRIT_DEFAULT in paramB for normal move-level crit behavior.
 */
#define MOVE_EFFECT_DAMAGE_CRIT_DEFAULT MOVE_EFFECT_PARAM_NONE
#define MOVE_EFFECT_DAMAGE_CRIT_OVERRIDE(permille) (permille)

typedef struct MoveEffectData {
    MoveEffectKind kind;
    BattleStatusId status;
    int paramA;
    int paramB;
    int paramC;
} MoveEffectData;

/*
 * MOVEMENT_LOCK: paramA = duration turns; paramB/paramC unused.
 *
 * SIDE_SPLASH_DAMAGE: paramA = splash damage percent of main hit power;
 * paramB/paramC unused. Hits adjacent occupied enemies on the target's side.
 *
 * VERTIGO_WAVE_DAMAGE: paramA/paramB/paramC unused.
 * Single-enemy target sets wave direction. Hits living opponents in a side wave.
 * Normal power 35/30/25/20/15; with optional LP (1) uses 50/40/30/20/10.
 * Last two enemies actually hit cannot crit.
 *
 * BUBBLE_PROTECTION: paramA/paramB/paramC unused.
 * Single-ally support. Grants Bubble until the ally's next turn begins.
 * Incoming combat damage is blocked and half is converted to healing.
 *
 * VEIL_LUCK_BUFF: paramA/paramB/paramC unused.
 * Self-target support. Places 3 random battlefield Veil tiles for 3 rounds.
 * Attacker/target tiles each add +1 crit stage (+100 permille); both = +2.
 *
 * SWAP_WITH_ALLY: paramA/paramB/paramC unused.
 * Single-ally support. Actor swaps formation slots with the selected living ally.
 *
 * TEAM_FOCUS: paramA/paramB/paramC unused.
 * Self-target support. Next ally attack/skill accuracy check cannot miss once.
 *
 * LAST_CHANCE: paramA/paramB/paramC unused.
 * Self-target support. Set HP to 5% of max (min 1), grant one bonus action.
 * Equipped SC is maxed after SC spend in skill resolve.
 *
 * ON_DAMAGE_SELF_STAT_MOD: same STAT_MODIFIER packing (paramA/B/C).
 * Applies to the actor only when the preceding damage effect dealt damage > 0
 * to an opponent. Used by Fiery Blood.
 *
 * PENDING_NEXT_DAMAGE_DOUBLE: paramA/paramB/paramC unused.
 * Single-ally support. Target's next damaging move/attack deals 2x damage on
 * all hits of that attempt, then the flag is consumed.
 *
 * AUTO_PUSH_ON_DAMAGE: paramA/paramB/paramC unused.
 * After damage > 0 to a living opponent, auto-push 1 enemy slot away from the
 * actor if the destination is empty and in-bounds. Used by Ram v1.
 *
 * BONUS_ACTION: grants the actor one immediate extra command after resolve.
 * paramA = MOVE_EFFECT_BONUS_ACTION_REQUIRES_LP (1) to require LP bonus;
 * MOVE_EFFECT_PARAM_NONE grants unconditionally. Use after MOVE_TO_SLOT.
 *
 * INCOMING_DAMAGE_REDUCTION: paramA = reduction percent; paramB = duration turns.
 * Self-target support only. Reduces damage taken, not defence.
 *
 * PENDING_ATTACK_ALL_ENEMIES: paramA/paramB/paramC unused.
 * Self-target support only. Next qualifying attack hits all enemies.
 *
 * PENDING_WEAPON_ATTACK_ELEMENT: paramA = BattleElementId.
 * Self-target support only. Next qualifying slingshot attack uses paramA.
 *
 * PENDING_NEXT_MOVE_ELEMENT: paramA = BattleElementId.
 * Self-target support only. Next qualifying damaging move uses paramA.
 *
 * EXTRA_ACTION_NEXT_TURN: paramA = extra actions on the actor's next turn;
 * MOVE_EFFECT_EXTRA_ACTION_NEXT_TURN_DEFAULT (1) if paramA is none.
 *
 * ENCORE: paramA = base duration turns; paramB/paramC unused.
 * LP bonus adds optionalLpCost turns when useLpBonus is set.
 * Single-enemy disrupt. Forces target to repeat last qualifying action.
 *
 * STAT_MODIFIER: paramA = BattleStatId, paramB = signed percent,
 * paramC = MOVE_EFFECT_STAT_PACK_META(durationTurns, maxStacks).
 */
#define MOVE_EFFECT_STAT_PACK_META(durationTurns, maxStacks) \
    ((((maxStacks) & 0xFF) << 8) | ((durationTurns) & 0xFF))

#define MOVE_EFFECT_DATA_NONE \
    { \
        MOVE_EFFECT_KIND_NONE, \
        BATTLE_STATUS_NONE, \
        MOVE_EFFECT_PARAM_NONE, \
        MOVE_EFFECT_PARAM_NONE, \
        MOVE_EFFECT_PARAM_NONE \
    }

#define MOVE_EFFECTS_NONE \
    { MOVE_EFFECT_DATA_NONE, MOVE_EFFECT_DATA_NONE, MOVE_EFFECT_DATA_NONE }

typedef struct MoveData {
    MoveId id;
    const char *name;
    BattleElementId element;
    int maxSc;
    int accuracyPercent;
    int contact;
    int optionalLpCost;
    int lpBonusDamageMultiplierPercent;
    int critChanceOverridePermille;
    int critChanceBonusPermille;
    TargetPattern targetPattern;
    const char *description;
    MoveCategory category;
    int defaultPower;
    MoveWeaponKind weaponKind;
    MoveEffectData effects[MOVE_MAX_EFFECTS];
} MoveData;

const MoveData *MoveData_Get(MoveId id);
int MoveData_HasDistanceScale(const MoveData *move);
int MoveData_GetDamagePower(const MoveData *move);
int MoveData_QualifiesForLastActionRecording(const MoveData *move);
int MoveData_QualifiesForEncoreReplay(const MoveData *move);

#endif
