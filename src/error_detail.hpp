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

#ifndef PANDA_TIMESWIPE_ERROR_DETAIL_HPP
#define PANDA_TIMESWIPE_ERROR_DETAIL_HPP

#include "error.hpp"

#include "3rdparty/dmitigr/error.hpp"

namespace panda::timeswipe::detail {

/// `true` if `NDEBUG` is not set.
constexpr bool is_debug{dmitigr::is_debug};

// -----------------------------------------------------------------------------
// Generic_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * The generic exception class.
 */
class Generic_exception : public dmitigr::Basic_generic_exception<Exception> {
public:
  /// The constructor.
  explicit Generic_exception(const std::error_condition& errc,
    const std::string& what)
    : Basic_generic_exception{errc, what}
  {}

  /// @overload
  explicit Generic_exception(const std::string& what)
    : Generic_exception{Errc::generic, what}
  {}
};

// -----------------------------------------------------------------------------
// Debug_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The debug exception class.
 *
 * @details The purpose of this class is to provide the diagnostic information
 * such as the source file name and line from where the exception was thrown.
 */
class Debug_exception final : public dmitigr::Basic_debug_exception<Exception> {
public:
  /// The constructor.
  Debug_exception(const char* const file, const int line,
    const std::error_condition& errc, const std::string& what)
    : Basic_debug_exception<Exception>{file, line, errc, what}
  {}

  /// @overload
  Debug_exception(const char* const file, const int line,
    const std::string& what)
    : Debug_exception{file, line, Errc::generic, what}
  {}
};

// -----------------------------------------------------------------------------
// Sys_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception thrown on system error.
 */
class Sys_exception final : public Generic_exception {
public:
  /// The constructor.
  Sys_exception(const int ev, const std::string& what)
    : Generic_exception{std::system_category().default_error_condition(ev), what}
  {}
};

} // namespace panda::timeswipe::detail

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

/// Checks the assertion `a`.
#define PANDA_TIMESWIPE_ASSERT(a) DMITIGR_ASSERT(a)

/// Throws exception with what-string `what`.
#define PANDA_TIMESWIPE_THROW(what) DMITIGR_THROW(what)

/// Throws exception with code `errc`, what-string `what`.
#define PANDA_TIMESWIPE_THROW2(errc, what) DMITIGR_THROW2(errc, what)

/// Throws exception with what-string `what` and the debug information.
#define PANDA_TIMESWIPE_THROW_DEBUG(what) DMITIGR_THROW_DEBUG(what)

/// Throws exception with code `errc`, what-string `what` and the debug information.
#define PANDA_TIMESWIPE_THROW_DEBUG2(errc, what) DMITIGR_THROW_DEBUG2(errc, what)

#endif  // PANDA_TIMESWIPE_ERROR_DETAIL_HPP
