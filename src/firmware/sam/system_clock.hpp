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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_SYSTEM_CLOCK_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_SYSTEM_CLOCK_HPP

#include <cstdint>

extern "C" {
extern std::uint32_t __isr_vector; // see startup_ARMCM4_E5x.S

#ifndef __NO_SYSTEM_INIT
void SystemInit();
#endif
} // extern "C"

/**
 * @brief Initializes the CPU main clock frequency to 120MHz.
 *
 * @returns `0` on success.
 */
int initialize_system_clock();

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_SYSTEM_CLOCK_HPP
