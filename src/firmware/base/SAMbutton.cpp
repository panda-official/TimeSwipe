/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "SAMbutton.h"

#include <sam.h>

namespace {
constexpr int button_led_pin_group = 2; // Group C
#if defined(__SAME54P20A__)
// Pin PC16
constexpr int button_led_pin_number = 16; // Number 16
#elif defined(__SAME53N19A__)
// Pin PC19
constexpr int button_led_pin_number = 19; // Number 19
#else
#error Unsupported SAM
#endif

constexpr int button_pin_group = 0; // Group A
constexpr int button_pin_number = 18; // Number 18
} // namespace


SAMButton::SAMButton()
{
    PORT->Group[button_pin_group].PINCFG[button_pin_number].bit.INEN=1;
    // Enable Button LED
    PORT->Group[button_led_pin_group].DIRSET.reg = (1L<<button_led_pin_number);
    PORT->Group[button_led_pin_group].OUTSET.reg = (1L<<button_led_pin_number);
}
void SAMButton::TurnButtonLED(bool how)
{
    if(how)
        PORT->Group[button_led_pin_group].OUTCLR.reg=(1L<<button_led_pin_number);
    else
        PORT->Group[button_led_pin_group].OUTSET.reg=(1L<<button_led_pin_number);
}
bool SAMButton::IsButtonLEDon()
{
    return ( PORT->Group[button_led_pin_group].OUT.reg & (1L<<button_led_pin_number) ) ? true:false;
}


bool SAMButton::impl_get_signal(void)
{
  return ( (PORT->Group[button_pin_group].IN.reg) & (1L<<button_pin_number) ) ? false:true;
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
