#include "battle/stat_curve.h"

static u8 StatCurve_ClampLevel(u8 level)
{
    if (level < 1)
    {
        return 1;
    }
    if (level > STAT_CURVE_NATURAL_LEVEL_MAX)
    {
        return STAT_CURVE_NATURAL_LEVEL_MAX;
    }
    return level;
}

u16 StatCurve_CalculateLevel1Stat(u16 level50Stat, u16 minimum)
{
    u16 value;

    value = (u16)(((u32)level50Stat * 20U + 50U) / 100U);
    if (value < minimum)
    {
        return minimum;
    }
    return value;
}

u16 StatCurve_CalculateLevel100Stat(u16 level50Stat)
{
    return (u16)(((u32)level50Stat * 160U + 50U) / 100U);
}

u16 StatCurve_CalculateNaturalStatAtLevel(u16 level50Stat, u8 level, u16 level1Minimum)
{
    u8 clampedLevel;
    u16 level1Stat;
    u16 level100Stat;
    u32 numerator;

    clampedLevel = StatCurve_ClampLevel(level);
    level1Stat = StatCurve_CalculateLevel1Stat(level50Stat, level1Minimum);
    level100Stat = StatCurve_CalculateLevel100Stat(level50Stat);

    if (clampedLevel <= 1)
    {
        return level1Stat;
    }
    if (clampedLevel == 50)
    {
        return level50Stat;
    }
    if (clampedLevel >= STAT_CURVE_NATURAL_LEVEL_MAX)
    {
        return level100Stat;
    }

    if (clampedLevel <= 50)
    {
        /* Interpolate level1 → level50 across levels 1..50 (49 steps). */
        numerator = ((u32)(clampedLevel - 1U) * ((u32)level50Stat - (u32)level1Stat)) + 24U;
        return (u16)((u32)level1Stat + (numerator / 49U));
    }

    /* Interpolate level50 → level100 across levels 50..100 (50 steps). */
    numerator = ((u32)(clampedLevel - 50U) * ((u32)level100Stat - (u32)level50Stat)) + 25U;
    return (u16)((u32)level50Stat + (numerator / 50U));
}

void StatCurve_CalculateNaturalStatsAtLevel(
    const StatCurveStats *level50Stats,
    u8 level,
    StatCurveStats *outStats)
{
    if (!level50Stats || !outStats)
    {
        return;
    }

    outStats->hp = StatCurve_CalculateNaturalStatAtLevel(
        level50Stats->hp,
        level,
        STAT_CURVE_LEVEL1_HP_MINIMUM);
    outStats->attack = StatCurve_CalculateNaturalStatAtLevel(
        level50Stats->attack,
        level,
        STAT_CURVE_LEVEL1_COMBAT_MINIMUM);
    outStats->defence = StatCurve_CalculateNaturalStatAtLevel(
        level50Stats->defence,
        level,
        STAT_CURVE_LEVEL1_COMBAT_MINIMUM);
    outStats->speed = StatCurve_CalculateNaturalStatAtLevel(
        level50Stats->speed,
        level,
        STAT_CURVE_LEVEL1_COMBAT_MINIMUM);
}
