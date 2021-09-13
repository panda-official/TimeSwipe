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

#include "error.hpp"

#include "3rdparty/dmitigr/assert.hpp"

namespace panda::timeswipe::detail {

/// `true` if `NDEBUG` is not set.
constexpr bool is_debug{dmitigr::is_debug};

// -----------------------------------------------------------------------------
// Debug_exception
// -----------------------------------------------------------------------------

/// Exception with source info.
class Debug_exception final : public dmitigr::Exception_with_info<Exception> {
public:
  using Super = dmitigr::Exception_with_info<Exception>;
  using Super::Super;
};

} // namespace panda::timeswipe::detail

/// Checks the assertion `a`.
#define PANDA_TIMESWIPE_ASSERT(a) DMITIGR_ASSERT(a)

/// Throws exception with code `errc` and the debug information.
#define PANDA_TIMESWIPE_THROW(errc)                                     \
  do {                                                                  \
    using E = panda::timeswipe::detail::Debug_exception;                \
    throw E{__FILE__, __LINE__, errc};                                  \
  } while (false)

#endif  // PANDA_TIMESWIPE_DEBUG_HPP
