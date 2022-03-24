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

#ifndef PANDA_TIMESWIPE_EXCEPTIONS_HPP
#define PANDA_TIMESWIPE_EXCEPTIONS_HPP

#include "errc.hpp"

#include <cstring> // std::strlen
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

// -----------------------------------------------------------------------------
// Integration with std::system_error
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief Full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_condition_enum<panda::timeswipe::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Generic_error_category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief A generic category of errors.
 */
class Generic_error_category final : public std::error_category {
public:
  /// @returns The literal `panda_timeswipe_generic_error`.
  const char* name() const noexcept override
  {
    return "panda_timeswipe_generic_error";
  }

  /**
   * @returns The string that describes the error condition denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Errc.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    const char* const desc{to_literal_anyway(static_cast<Errc>(ev))};
    constexpr const char* const sep{": "};
    std::string result;
    result.reserve(std::strlen(name()) + std::strlen(sep) + std::strlen(desc));
    return result.append(name()).append(sep).append(desc);
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Generic_error_category.
 */
inline const Generic_error_category& generic_error_category() noexcept
{
  static const Generic_error_category instance;
  return instance;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), generic_error_category())`.
 */
inline std::error_condition make_error_condition(const Errc errc) noexcept
{
  return {static_cast<int>(errc), generic_error_category()};
}

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The generic exception class.
 */
class Exception : public std::exception {
public:
  /**
   * @brief The constructor.
   *
   * @param errc The error condition.
   * @param what The what-string.
   */
  Exception(const std::error_condition& errc, const std::string& what)
    : what_{what}
    , condition_{errc}
  {}

  /**
   * @brief Constructs an instance associated with Errc::generic.
   *
   * @param what The what-string.
   */
  explicit Exception(const std::string& what)
    : Exception{Errc::generic, what}
  {}

  /// @returns The what-string.
  const char* what() const noexcept override
  {
    return what_.what();
  }

  /// @returns The error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

private:
  std::runtime_error what_;
  std::error_condition condition_;
};

// -----------------------------------------------------------------------------
// Sys_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception thrown on system error.
 */
class Sys_exception final : public Exception {
public:
  /// The constructor.
  Sys_exception(const int ev, const std::string& what)
    : Exception{std::system_category().default_error_condition(ev), what}
  {}
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_EXCEPTIONS_HPP
