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
class Error final {
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
struct Error_or final : std::pair<Error, T> {
  static_assert(!std::is_same_v<T, void> &&
    !std::is_convertible_v<T, Errc> &&
    !std::is_convertible_v<T, Error>);

  /// An alias of the base class.
  using Base = std::pair<Error, T>;

  /// Constructs to hold a default value.
  constexpr Error_or() noexcept = default;

  /// Constructs to hold an error.
  constexpr Error_or(Error error) noexcept
    : Base{std::move(error), T{}}
  {}

  /// @overload
  constexpr Error_or(const Errc errc) noexcept
    : Error_or{Error{errc}}
  {}

  /// Constructs to hold a value.
  constexpr Error_or(T value) noexcept
    : Base{Error{}, std::move(value)}
  {}
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_ERROR_HPP
