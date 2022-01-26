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

#ifndef PANDA_TIMESWIPE_ERROR_HPP
#define PANDA_TIMESWIPE_ERROR_HPP

#include "errc.hpp"

#include <string>
#include <type_traits>
#include <utility>

namespace panda::timeswipe::detail {

// -----------------------------------------------------------------------------
// Error
// -----------------------------------------------------------------------------

/// An error.
class Error {
public:
  /// Constructs not an error.
  Error() noexcept = default;

  /// The constructor.
  Error(const Errc errc, std::string what = {}) noexcept
    : condition_{errc}
    , what_{std::move(what)}
  {}

  /// @returns `true` if the instance represents an error.
  explicit operator bool() const noexcept
  {
    return static_cast<bool>(condition_);
  }

  /// @returns The error condition as Errc.
  Errc errc() const noexcept
  {
    return condition_;
  }

  /// @returns The what-string.
  const std::string& what() const noexcept
  {
    return what_;
  }

private:
  Errc condition_{};
  std::string what_;
};

/// For using when Error represents a normal result.
class Error_result final : public Error {
public:
  /// The default constructor.
  Error_result() noexcept = default;

  /// The constructor.
  explicit Error_result(Error error) noexcept
    : Error{std::move(error)}
  {}
};

/// @returns `true` if `lhs` is equals to `rhs`.
inline bool operator==(const Error& lhs, const Error& rhs) noexcept
{
  return lhs.errc() == rhs.errc();
}

/// @returns `true` if `lhs` is not equals to `rhs`.
inline bool operator!=(const Error& lhs, const Error& rhs) noexcept
{
  return !(lhs == rhs);
}

// -----------------------------------------------------------------------------
// Error_or
// -----------------------------------------------------------------------------

/**
 * @brief A pair of Error and T.
 *
 * @details This template struct is designed for using as a return type of
 * functions which must not throw exceptions.
 */
template<typename T>
struct Error_or final {
  static_assert(!std::is_same_v<T, void> &&
    !std::is_convertible_v<T, Errc> &&
    !std::is_same_v<T, Error>);

  /// Constructs to hold not an error and a default value.
  constexpr Error_or() noexcept = default;

  /// Constructs to hold an error and a default value.
  constexpr Error_or(Error err) noexcept
    : error{std::move(err)}
  {}

  /// @overload
  constexpr Error_or(const Errc errc) noexcept
    : Error_or{Error{errc}}
  {}

  /// Constructs to hold not an error and `value`.
  constexpr Error_or(T val) noexcept
    : value{std::move(val)}
  {}

  /**
   * @brief Constructs to hold the both error and `value`.
   *
   * @details This constructor could be useful is rare cases to return partially
   * valid value with an error in assumption that the caller could know how to
   * handle it.
   */
  constexpr Error_or(Errc errc, T val) noexcept
    : error{Error{errc}}
    , value{std::move(value)}
  {}

  Error error;
  T value{};
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_ERROR_HPP
