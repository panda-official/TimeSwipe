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

#ifndef PANDA_TIMESWIPE_FIRMWARE_ERROR_HPP
#define PANDA_TIMESWIPE_FIRMWARE_ERROR_HPP

#include "../errc.hpp"
using namespace panda::timeswipe; // FIXME: remove

#include <exception> // std::terminate

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

  /// @returns The error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

  /// @returns The what-string.
  const std::string& what() const noexcept
  {
    return what_;
  }

private:
  std::error_condition condition_;
  std::string what_;
};

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

/**
 * @brief Checks the assertion `a`.
 *
 * @details Always active regardless of `NDEBUG`.
 *
 * @par Effects
 * Terminates the process if `!a`.
 */
#define PANDA_TIMESWIPE_FIRMWARE_ASSERT(a) do { \
    if (!(a)) {                                 \
      std::terminate();                         \
    }                                           \
  } while (false)

#endif  // PANDA_TIMESWIPE_FIRMWARE_ERROR_HPP
