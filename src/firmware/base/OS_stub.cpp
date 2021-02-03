/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//this file implement only Wait funct stub.

//unsigned long get_tick_mS(void);

#include "os.h"

namespace os{
void wait(unsigned long time_mS)
{
	unsigned long start_time=get_tick_mS();
	while( (get_tick_mS()-start_time)<time_mS ){ asm("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;"); }
}

void uwait(unsigned long time_uS)
{
    for(unsigned long t=0; t<time_uS; t++)
    {
        //1us @120MHZ
        asm("nop; nop; nop; nop; nop; nop; nop; nop;" //80ns
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop;");
    }
}

}
