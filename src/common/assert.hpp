// -*- C++ -*-

// PANDA TimeSwipe Project
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

#ifndef PANDA_TIMESWIPE_COMMON_ASSERT_HPP
#define PANDA_TIMESWIPE_COMMON_ASSERT_HPP

#include "../3rdparty/dmitigr/assert.hpp"

namespace panda::timeswipe {
constexpr bool kIsDebug{dmitigr::is_debug};
} // namespace panda::timeswipe

#define PANDA_TIMESWIPE_ASSERT(a) DMITIGR_ASSERT(a)
#define PANDA_TIMESWIPE_CHECK(a) DMITIGR_CHECK(a)
#define PANDA_TIMESWIPE_CHECK_ARG(a) DMITIGR_CHECK_ARG(a)
#define PANDA_TIMESWIPE_CHECK_DOMAIN(a) DMITIGR_CHECK_DOMAIN(a)
#define PANDA_TIMESWIPE_CHECK_LENGTH(a) DMITIGR_CHECK_LENGTH(a)
#define PANDA_TIMESWIPE_CHECK_RANGE(a) DMITIGR_CHECK_RANGE(a)

#endif  // PANDA_TIMESWIPE_COMMON_ASSERT_HPP
