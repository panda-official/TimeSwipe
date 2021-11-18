/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "SAMbutton.h"

#include <sam.h>

#if defined(__SAME54P20A__)
//Pin PC16
#define BUTTON_LED_PINGROUP 2	//Group C
#define BUTTON_LED_PINID 16 //ID 16
#elif defined(__SAME53N19A__)
//Pin PC19
#define BUTTON_LED_PINGROUP 2	//Group C
#define BUTTON_LED_PINID 19 //ID 19
#else
#error Unsupported SAM
#endif

//Pin PA16
#define BUTTON_PINGROUP 0	//Group A
#define BUTTON_PINID 16 //ID 16

SAMButton::SAMButton()
{
	PORT->Group[BUTTON_PINGROUP].PINCFG[BUTTON_PINID].bit.INEN=1;
    // Enable Button LED
    PORT->Group[BUTTON_LED_PINGROUP].DIRSET.reg = (1L<<BUTTON_LED_PINID);
    PORT->Group[BUTTON_LED_PINGROUP].OUTSET.reg = (1L<<BUTTON_LED_PINID);
}
void SAMButton::TurnButtonLED(bool how)
{
    if(how)
        PORT->Group[BUTTON_LED_PINGROUP].OUTCLR.reg=(1L<<BUTTON_LED_PINID);
    else
        PORT->Group[BUTTON_LED_PINGROUP].OUTSET.reg=(1L<<BUTTON_LED_PINID);
}
bool SAMButton::IsButtonLEDon()
{
    return ( PORT->Group[BUTTON_LED_PINGROUP].OUT.reg & (1L<<BUTTON_LED_PINID) ) ? true:false;
}


bool SAMButton::impl_get_signal(void)
{
  return ( (PORT->Group[BUTTON_PINGROUP].IN.reg) & (1L<<BUTTON_PINID) ) ? false:true;
}

void SAMButton::impl_on_state_changed(typeButtonState nState)
{
    if(m_pSink)
    {
        m_pSink->OnButtonState(nState);
    }

     if(typeButtonState::pressed==nState || typeButtonState::released==nState){

     m_nStateCounter++;
     nlohmann::json v=typeButtonState::pressed==nState ? true:false;
     nlohmann::json stcnt=m_nStateCounter;
     Fire_on_event("Button", v);
     Fire_on_event("ButtonStateCnt", stcnt); }
}
