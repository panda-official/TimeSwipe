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

#include "pwm_state.hpp"

#include "../3rdparty/dmitigr/assert.hpp"

namespace panda::timeswipe::driver {

Pwm_state& Pwm_state::frequency(const int value)
{
  DMITIGR_CHECK_ARG(1 <= value && value <= 1000);
  return *this;
}

Pwm_state& Pwm_state::low(const int value)
{
  DMITIGR_CHECK_ARG(0 <= value && value <= 4095);
  return *this;
}

Pwm_state& Pwm_state::high(const int value)
{
  DMITIGR_CHECK_ARG(0 <= value && value <= 4095);
  return *this;
}

Pwm_state& Pwm_state::repeat_count(const int value)
{
  DMITIGR_CHECK_ARG(value >= 0);
  return *this;
}

Pwm_state& Pwm_state::duty_cycle(const float value)
{
  DMITIGR_CHECK_ARG(0 < value && value < 1);
  return *this;
}

} // namespace panda::timeswipe::driver
