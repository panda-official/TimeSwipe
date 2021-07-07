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

#ifndef DMITIGR_ASSERT_ASSERT_HPP
#define DMITIGR_ASSERT_ASSERT_HPP

#include <iostream>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace dmitigr {

/// The debug mode indicator.
#ifndef NDEBUG
constexpr bool is_debug{true};
#else
constexpr bool is_debug{false};
#endif

/**
 * An exception class derived from `std::exception` which provides the
 * information about the source from where the exception is thrown.
 *
 * @tparam Base The base exception class derived from `std::exception`.
 */
template<class Base>
class Sourced_exception : public Base {
public:
  /**
   * Lifted constructors.
   *
   * @par Effects
   * file() returns `nullptr`, line() returns `0`.
   *
   * @see file(), line().
   */
  using Base::Base;

  /**
   * The constructor.
   *
   * @param file The name of file from where the exception thrown.
   * @param line The line of file from where the exception thrown.
   * @param desc The error description (what string) or error code/condition.
   */
  template<typename T>
  Sourced_exception(const char* const file, const int line, const T desc)
    : Base{desc}
    , file_{file}
    , line_{line}
  {
    static_assert(std::is_base_of_v<std::exception, Sourced_exception>);
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

} // namespace dmitigr

// Helpers
#define DMITIGR_ASSERT_STR(s) #s
#define DMITIGR_ASSERT_XSTR(s) DMITIGR_ASSERT_STR(s)

/// Checks `a` always, regardless of `NDEBUG`.
#define DMITIGR_ASSERT(a) do {                                          \
    if (!(a)) {                                                         \
      std::cerr<<"assertion ("<<#a<<") failed at "<<__FILE__<<":"<<__LINE__<<"\n"; \
      std::terminate();                                                 \
    }                                                                   \
  } while (false)

/// Checks `a` always, regardless of `NDEBUG`.
#define DMITIGR_CHECK_GENERIC(a, Base) do {                             \
    if (!(a)) {                                                         \
      throw dmitigr::Sourced_exception<Base>{__FILE__, __LINE__,        \
        "check (" #a ") failed at " __FILE__ ":" DMITIGR_ASSERT_XSTR(__LINE__)}; \
    }                                                                   \
  } while (false)

#define DMITIGR_CHECK(a) DMITIGR_CHECK_GENERIC(a, std::logic_error)
#define DMITIGR_CHECK_ARG(a) DMITIGR_CHECK_GENERIC(a, std::invalid_argument)
#define DMITIGR_CHECK_DOMAIN(a) DMITIGR_CHECK_GENERIC(a, std::domain_error)
#define DMITIGR_CHECK_LENGTH(a) DMITIGR_CHECK_GENERIC(a, std::length_error)
#define DMITIGR_CHECK_RANGE(a) DMITIGR_CHECK_GENERIC(a, std::out_of_range)

#endif  // DMITIGR_ASSERT_ASSERT_HPP
