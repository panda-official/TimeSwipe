// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_ASSERT_HPP
#define PANDA_TIMESWIPE_ASSERT_HPP

#include "error.hpp"

#include "../3rdparty/dmitigr/assert.hpp"

namespace panda::timeswipe::detail {

/// `true` if `NDEBUG` is not set.
constexpr bool is_debug{dmitigr::is_debug};

// -----------------------------------------------------------------------------
// Exception_with_info
// -----------------------------------------------------------------------------

/// Exception with source info.
template<class StdError>
class Exception_with_info :
    public dmitigr::Exception_with_info<Basic_exception<StdError>> {
  using Super = dmitigr::Exception_with_info<Basic_exception<StdError>>;
  using Super::Super;
};

// -----------------------------------------------------------------------------
// CHECK macros are for logic errors debugging
// -----------------------------------------------------------------------------

#define PANDA_TIMESWIPE_CHECK_GENERIC(a, Base)                          \
  DMITIGR_CHECK_GENERIC(a, panda::timeswipe::detail::Exception_with_info<Base>)
#define PANDA_TIMESWIPE_CHECK(a)                        \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::logic_error)
#define PANDA_TIMESWIPE_CHECK_ARG(a)                        \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::invalid_argument)
#define PANDA_TIMESWIPE_CHECK_DOMAIN(a)                 \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::domain_error)
#define PANDA_TIMESWIPE_CHECK_LENGTH(a)                 \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::length_error)
#define PANDA_TIMESWIPE_CHECK_RANGE(a)                  \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::out_of_range)

// -----------------------------------------------------------------------------
// THROW macros are for exception with extra information.
// -----------------------------------------------------------------------------

#define PANDA_TIMESWIPE_THROW(errc)                                     \
  do {                                                                  \
    using E = panda::timeswipe::detail::Exception_with_info<std::runtime_error>; \
    throw E{__FILE__, __LINE__, errc};                                  \
  } while(true)

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_ASSERT_HPP
