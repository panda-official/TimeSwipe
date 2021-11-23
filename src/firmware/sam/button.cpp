/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "button.hpp"

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

Sam_button::Sam_button()
{
    PORT->Group[BUTTON_PINGROUP].PINCFG[BUTTON_PINID].bit.INEN=1;
    // Enable Button LED
    PORT->Group[BUTTON_LED_PINGROUP].DIRSET.reg = (1L<<BUTTON_LED_PINID);
    PORT->Group[BUTTON_LED_PINGROUP].OUTSET.reg = (1L<<BUTTON_LED_PINID);
}
void Sam_button::TurnButtonLED(bool how)
{
    if(how)
        PORT->Group[BUTTON_LED_PINGROUP].OUTCLR.reg=(1L<<BUTTON_LED_PINID);
    else
        PORT->Group[BUTTON_LED_PINGROUP].OUTSET.reg=(1L<<BUTTON_LED_PINID);
}
bool Sam_button::IsButtonLEDon()
{
    return ( PORT->Group[BUTTON_LED_PINGROUP].OUT.reg & (1L<<BUTTON_LED_PINID) ) ? true:false;
}


bool Sam_button::impl_get_signal(void)
{
  return ( (PORT->Group[BUTTON_PINGROUP].IN.reg) & (1L<<BUTTON_PINID) ) ? false:true;
}

void Sam_button::impl_on_state_changed(const Button_state state)
{
    if(m_pSink)
    {
        m_pSink->OnButtonState(state);
    }

     if(Button_state::pressed==state || Button_state::released==state){

     m_nStateCounter++;
     nlohmann::json v = Button_state::pressed == state;
     nlohmann::json stcnt=m_nStateCounter;
     Fire_on_event("Button", v);
     Fire_on_event("ButtonStateCnt", stcnt); }
}
