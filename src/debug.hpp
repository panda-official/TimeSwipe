// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH / Dmitry Igrishin

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

#ifndef PANDA_TIMESWIPE_DEBUG_HPP
#define PANDA_TIMESWIPE_DEBUG_HPP

#include <exception> // std::terminate
#ifndef PANDA_TIMESWIPE_FIRMWARE
#include <iostream> // std::cerr
#endif

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

/**
 * @brief Checks the assertion `a`.
 *
 * @details Always active regardless of `NDEBUG`.
 *
 * @par Effects
 * Terminates the process if `!a`.
 */
#ifndef PANDA_TIMESWIPE_FIRMWARE
#define PANDA_TIMESWIPE_ASSERT(a) do {                                  \
    if (!(a)) {                                                         \
      std::cerr<<"assertion ("<<#a<<") failed at "<<__FILE__<<":"<<__LINE__<<"\n"; \
      std::terminate();                                                 \
    }                                                                   \
  } while (false)
#else
#define PANDA_TIMESWIPE_ASSERT(a) do {          \
    if (!(a)) {                                 \
      std::terminate();                         \
    }                                           \
  } while (false)
#endif

#endif  // PANDA_TIMESWIPE_DEBUG_HPP
