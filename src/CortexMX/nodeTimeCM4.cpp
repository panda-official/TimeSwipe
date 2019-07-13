/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


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
