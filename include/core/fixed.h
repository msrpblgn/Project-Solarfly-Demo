#ifndef FIXED_H
#define FIXED_H

#include "core/gba_types.h"

/* Signed Q24.8 fixed-point: 24 integer bits, 8 fractional bits. */
typedef s32 Fixed24_8;

#define FIXED24_8_SHIFT 8
#define FIXED24_8_ONE   (1 << FIXED24_8_SHIFT)

_Static_assert(sizeof(Fixed24_8) == 4, "Fixed24_8 must be 32-bit");

/* Convert a signed integer pixel/world unit into Q24.8. */
static inline Fixed24_8 Fixed24_8_FromInt(int value)
{
    return (Fixed24_8)value << FIXED24_8_SHIFT;
}

/*
 * Floor a nonnegative Q24.8 value to an integer.
 * Only valid after clamping to a nonnegative range; does not shift negatives.
 */
static inline int Fixed24_8_FloorToIntNonNeg(Fixed24_8 value)
{
    if (value < 0)
    {
        return 0;
    }
    return (int)((u32)value >> FIXED24_8_SHIFT);
}

static inline Fixed24_8 Fixed24_8_Clamp(Fixed24_8 value, Fixed24_8 minValue, Fixed24_8 maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

#endif
