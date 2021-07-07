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
#include "error.hpp"

namespace panda::timeswipe {

/// `true` if `NDEBUG` is not set.
constexpr bool kIsDebug{dmitigr::is_debug};

/// Basic exception.
template<class StdError>
class BasicException :
    public dmitigr::Sourced_exception<BasicExceptionBase<StdError>> {
  using Super = dmitigr::Sourced_exception<BasicExceptionBase<StdError>>;
  using Super::Super;
};

} // namespace panda::timeswipe

#define PANDA_TIMESWIPE_ASSERT(a) DMITIGR_ASSERT(a)

#define PANDA_TIMESWIPE_CHECK_GENERIC(a, Base) \
  DMITIGR_CHECK_GENERIC(a, panda::timeswipe::BasicException<Base>)
#define PANDA_TIMESWIPE_CHECK(a) \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::logic_error)
#define PANDA_TIMESWIPE_CHECK_ARG(a) \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::invalid_argument)
#define PANDA_TIMESWIPE_CHECK_DOMAIN(a) \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::domain_error)
#define PANDA_TIMESWIPE_CHECK_LENGTH(a) \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::length_error)
#define PANDA_TIMESWIPE_CHECK_RANGE(a) \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::out_of_range)

#define PANDA_TIMESWIPE_THROW(errc)                                     \
  do {                                                                  \
    using RuntimeError = panda::timeswipe::BasicException<std::runtime_error>; \
    throw RuntimeError{__FILE__, __LINE__, errc};                       \
  } while(true)

#endif  // PANDA_TIMESWIPE_COMMON_ASSERT_HPP
