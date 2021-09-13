// -*- C++ -*-
// Copyright (C) 2021 Dmitry Igrishin
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Dmitry Igrishin
// dmitigr@gmail.com

#ifndef DMITIGR_ERROR_EXCEPTION_WITH_DEBUG_INFO_HPP
#define DMITIGR_ERROR_EXCEPTION_WITH_DEBUG_INFO_HPP

#include "exception.hpp"

#include <type_traits>

namespace dmitigr {

/**
 * The base exception class which provides the diagnostic information about
 * the exception.
 *
 * @tparam Base The base exception class derived from `std::exception`.
 */
template<class Base>
class Exception_with_debug_info : public Base {
public:
  static_assert(std::is_base_of_v<std::exception, Base>);

  /**
   * The constructor.
   *
   * @param file The name of file from where the exception thrown.
   * @param line The line of file from where the exception thrown.
   * @param desc The error description (what string) or error code/condition.
   */
  template<typename T>
  Exception_with_debug_info(const char* const file, const int line, const T desc)
    : Base{desc}
    , file_{file}
    , line_{line}
  {
    static_assert(std::is_same_v<T, const char*> ||
      std::is_error_code_enum_v<T> || std::is_error_condition_enum_v<T>);
  }

  /// @returns The name of file from where the exception thrown.
  const char* file() const noexcept
  {
    return file_;
  }

  /// @returns The line of file from where the exception thrown.
  int line() const noexcept
  {
    return line_;
  }

private:
  const char* file_{};
  int line_{};
};

using Debug_exception = Exception_with_debug_info<Exception>;

} // namespace dmitigr

/// Throws exception with code `errc` and the debug information.
#define DMITIGR_THROW_DEBUG(errc)                               \
  do {                                                          \
    throw dmitigr::Debug_exception{__FILE__, __LINE__, errc};   \
  } while (false)

/// Checks `a` always, regardless of `is_debug` (or `NDEBUG`).
#define DMITIGR_CHECK_GENERIC__(a, errc) do {   \
    if (!(a)) {                                 \
      DMITIGR_THROW_DEBUG(errc);                \
    }                                           \
  } while (false)

#define DMITIGR_CHECK(a) DMITIGR_CHECK_GENERIC__(a, dmitigr::Errc::generic)
#define DMITIGR_CHECK_ARG(a) DMITIGR_CHECK_GENERIC__(a, dmitigr::Errc::invalid_argument)
#define DMITIGR_CHECK_RANGE(a) DMITIGR_CHECK_GENERIC__(a, dmitigr::Errc::out_of_range)
#define DMITIGR_CHECK_LENGTH(a) DMITIGR_CHECK_GENERIC__(a, dmitigr::Errc::length_error)

#endif  // DMITIGR_ERROR_EXCEPTION_WITH_DEBUG_INFO_HPP
