#include "core/random.h"

static u32 s_seed;

void Random_Init(u32 seed)
{
    s_seed = seed ? seed : 1;
}

u32 Random_Next(void)
{
    s_seed = s_seed * 1664525u + 1013904223u;
    return s_seed;
}

int Random_Range(int min, int max)
{
    u32 range;
    if (max <= min)
    {
        return min;
    }
    range = (u32)(max - min);
    return min + (int)(Random_Next() % range);
}

int Random_Percent(int chancePercent)
{
    if (chancePercent <= 0)
    {
        return 0;
    }
    if (chancePercent >= 100)
    {
        return 1;
    }
    return (int)(Random_Next() % 100) < chancePercent;
}

int Random_Permille(int chancePermille)
{
    if (chancePermille <= 0)
    {
        return 0;
    }
    if (chancePermille >= 1000)
    {
        return 1;
    }
    return (int)(Random_Next() % 1000) < chancePermille;
}
