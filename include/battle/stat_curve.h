#ifndef STAT_CURVE_H
#define STAT_CURVE_H

#include "core/gba_types.h"

#define STAT_CURVE_NATURAL_LEVEL_MAX 100
#define STAT_CURVE_LEVEL1_HP_MINIMUM 5
#define STAT_CURVE_LEVEL1_COMBAT_MINIMUM 1

typedef struct StatCurveStats {
    u16 hp;
    u16 attack;
    u16 defence;
    u16 speed;
} StatCurveStats;

u16 StatCurve_CalculateLevel1Stat(u16 level50Stat, u16 minimum);
u16 StatCurve_CalculateLevel100Stat(u16 level50Stat);
u16 StatCurve_CalculateNaturalStatAtLevel(u16 level50Stat, u8 level, u16 level1Minimum);
void StatCurve_CalculateNaturalStatsAtLevel(
    const StatCurveStats *level50Stats,
    u8 level,
    StatCurveStats *outStats);

#endif
