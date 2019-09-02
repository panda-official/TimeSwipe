/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "sam.h"

//timer func for M4:
static unsigned long sys_time_mS;

extern "C"{		//01.09.2019
void SysTick_Handler(void)
{
	sys_time_mS++;
}}


unsigned long get_tick_mS(void)
{
	return sys_time_mS;
}
