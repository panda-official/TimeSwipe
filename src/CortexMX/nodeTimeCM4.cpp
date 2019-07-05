#include "sam.h"

//timer func for M4:
static unsigned long sys_time_mS;

void SysTick_Handler(void)
{
	sys_time_mS++;
}
unsigned long get_tick_mS(void)
{
	return sys_time_mS;
}
