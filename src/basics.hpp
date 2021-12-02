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

#ifndef PANDA_TIMESWIPE_BASICS_HPP
#define PANDA_TIMESWIPE_BASICS_HPP

#include <optional>
#include <string_view>

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Measurement_mode
// -----------------------------------------------------------------------------

/// Measurement mode.
enum class Measurement_mode {
  voltage,
  current
};

/**
 * @returns The value of type `Measurement_mode` converted from `value`, or
 * `std::nullopt` if `value` doesn't corresponds to any member of Measurement_mode.
 */
constexpr std::optional<Measurement_mode> to_measurement_mode(const
  std::string_view value) noexcept
{
  if (value == "voltage") return Measurement_mode::voltage;
  else if (value == "current") return Measurement_mode::current;
  else return {};
}

/**
 * @returns The character literal converted from `value`, or `nullptr`
 * if `value` doesn't corresponds to any member of Measurement_mode.
 */
constexpr const char* to_literal(const Measurement_mode value) noexcept
{
  switch (value) {
  case Measurement_mode::voltage: return "voltage";
  case Measurement_mode::current: return "current";
  }
  return nullptr;
}

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_BASICS_HPP
