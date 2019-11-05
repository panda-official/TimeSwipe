/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "SAMbutton.h"
#include "sam.h"

SAMButton::SAMButton(CButtonEvent &sink) : m_sink(sink)
{
 #ifdef TIME_SWIPE_BRD_V0
    PORT->Group[0].PINCFG[16].bit.INEN=1;
 #else
    PORT->Group[0].PINCFG[18].bit.INEN=1;
 #endif
}
bool SAMButton::get_signal(void){

#ifdef EMU
        return is_key_pressed();
#else
    #ifdef TIME_SWIPE_BRD_V0
         return ( (PORT->Group[0].IN.reg) & (1L<<16) ) ? false:true; //this is the right one for the PandaBoard! 24.04.2019
    #else
         return ( (PORT->Group[0].IN.reg) & (1L<<18) ) ? false:true;
    #endif
#endif

}

void SAMButton::send_event(typeButtonState nState)
{

     m_nStateCounter++;
     m_sink.OnButtonState(nState);

     //14.06.2019:
     nlohmann::json v=typeButtonState::pressed==nState ? true:false;
     nlohmann::json stcnt=m_nStateCounter;
     Fire_on_event("Button", v);
     Fire_on_event("ButtonStateCnt", stcnt);
}
