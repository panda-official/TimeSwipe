// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

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
class Debug_exception final :
    public dmitigr::Exception_with_info<Basic_exception<std::runtime_error>> {
  using Super = dmitigr::Exception_with_info<Basic_exception<std::runtime_error>>;
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
  } while(true)

#endif  // PANDA_TIMESWIPE_DEBUG_HPP
