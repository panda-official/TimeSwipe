// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
  PORT->Group[BUTTON_PINGROUP].PINCFG[BUTTON_PINID].bit.INEN = 1;
  // Enable Button LED
  PORT->Group[BUTTON_LED_PINGROUP].DIRSET.reg = (1L<<BUTTON_LED_PINID);
  PORT->Group[BUTTON_LED_PINGROUP].OUTSET.reg = (1L<<BUTTON_LED_PINID);
}

void Sam_button::enable_led(const bool on)
{
  if (on)
    PORT->Group[BUTTON_LED_PINGROUP].OUTCLR.reg = (1L<<BUTTON_LED_PINID);
  else
    PORT->Group[BUTTON_LED_PINGROUP].OUTSET.reg = (1L<<BUTTON_LED_PINID);
}

bool Sam_button::is_led_enabled() const noexcept
{
  return PORT->Group[BUTTON_LED_PINGROUP].OUT.reg & (1L<<BUTTON_LED_PINID);
}

bool Sam_button::impl_get_signal()
{
  return !((PORT->Group[BUTTON_PINGROUP].IN.reg) & (1L<<BUTTON_PINID));
}

void Sam_button::impl_on_state_changed(const Button_state state)
{
  if (extra_handler_)
    extra_handler_->OnButtonState(state);

  if (state == Button_state::pressed || state == Button_state::released) {
    ++total_state_count_;
    nlohmann::json v{Button_state::pressed == state};
    nlohmann::json stcnt{total_state_count_};
    Fire_on_event("Button", v);
    Fire_on_event("ButtonStateCnt", stcnt);
  }
}
