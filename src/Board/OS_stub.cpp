/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//this file implement only Wait funct stub.

unsigned long get_tick_mS(void);

void Wait(unsigned long time_mS)
{
	unsigned long start_time=get_tick_mS();
	while( (get_tick_mS()-start_time)<time_mS ){ asm("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;"); }
}
