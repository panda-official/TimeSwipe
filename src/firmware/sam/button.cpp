// -*- C++ -*-

// PANDA Timeswipe Project
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
#include "pin.hpp"

#include <sam.h>

namespace {
constexpr int button_led_pin_group = Sam_pin::Group::c;
#if defined(__SAME54P20A__)
constexpr int button_led_pin_number = Sam_pin::Number::p16;
#elif defined(__SAME53N19A__)
constexpr int button_led_pin_number = Sam_pin::Number::p19;
#else
#error Unsupported SAM
#endif

constexpr int button_pin_group = Sam_pin::Group::a;
constexpr int button_pin_number = Sam_pin::Number::p18;
} // namespace

Sam_button::Sam_button()
{
  PORT->Group[button_pin_group].PINCFG[button_pin_number].bit.INEN = 1;

  // Enable Button LED.
  PORT->Group[button_led_pin_group].DIRSET.reg = (1L<<button_led_pin_number);
  PORT->Group[button_led_pin_group].OUTSET.reg = (1L<<button_led_pin_number);
}

void Sam_button::enable_led(const bool on)
{
  if (on)
    PORT->Group[button_led_pin_group].OUTCLR.reg = (1L<<button_led_pin_number);
  else
    PORT->Group[button_led_pin_group].OUTSET.reg = (1L<<button_led_pin_number);
}

bool Sam_button::is_led_enabled() const noexcept
{
  return PORT->Group[button_led_pin_group].OUT.reg & (1L<<button_led_pin_number);
}

bool Sam_button::do_get_signal()
{
  return !((PORT->Group[button_pin_group].IN.reg) & (1L<<button_pin_number));
}

void Sam_button::do_on_state_changed(const Button_state state)
{
  if (extra_handler_)
    extra_handler_->handle_state(state);

  if (state == Button_state::pressed || state == Button_state::released)
    ++total_state_count_;
}
