#include "battle/turn_order.h"

static int s_turnOrderReady;

void TurnOrder_Reset(void)
{
    s_turnOrderReady = 0;
}

int TurnOrder_IsReady(void)
{
    return s_turnOrderReady;
}
