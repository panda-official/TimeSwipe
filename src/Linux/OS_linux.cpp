/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#include <unistd.h>  /* UNIX standard function definitions */
#include <sys/types.h>
#include <sys/time.h>

#include <time.h>

void Wait(unsigned long time_mS)
{
	usleep(1000*time_mS);
}

unsigned long get_tick_mS(void)
{
	struct timeval te;
	
	if(0!=gettimeofday(&te, NULL))
		return 0;
	
	return (te.tv_sec*1000 + te.tv_usec/1000); 
}

void nodeTime_upd(void){}
int sys_clock_init(void){return 0;}
