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

#include "../os.h"

#include <sam.h>

//timer func for M4:
// FIXME: atomic
static unsigned long sys_time_mS;

extern "C" {

/**
 * @brief A CortexMX system timer interrupt handler.
 *
 * Increments a system time counter by one every millisecond.
 */
void SysTick_Handler()
{
  sys_time_mS++;
}

} // extern "C"

namespace os {

unsigned long get_tick_mS() noexcept
{
  return sys_time_mS;
}

} // namespace os
