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

/// The absolute maximum possible number of data channels.
constexpr int max_channel_count{4};

/// The absolute maximum possible number of PWMs.
constexpr int max_pwm_count{2};

/// Measurement mode.
enum class Measurement_mode {
  Voltage,
  Current
};

/// Signal mode.
enum class Signal_mode {
  /// IEPE.
  iepe,
  /// Normal signal.
  normal,
  /// Digital.
  digital
};

/**
 * @returns The value of type `Signal_mode` converted from `value`, or
 * `std::nullopt` if `value` doesn't corresponds to any member of Signal_mode.
 */
constexpr std::optional<Signal_mode> to_signal_mode(const std::string_view value) noexcept
{
  if (value == "iepe") return Signal_mode::iepe;
  else if (value == "normal") return Signal_mode::normal;
  else if (value == "digital") return Signal_mode::digital;
  else return {};
}

/**
 * @returns The character literal converted from `value`, or
 * `nullptr` if `value` doesn't corresponds to any member of Signal_mode.
 */
constexpr const char* to_literal(const Signal_mode value) noexcept
{
  switch (value) {
  case Signal_mode::iepe: return "iepe";
  case Signal_mode::normal: return "normal";
  case Signal_mode::digital: return "digital";
  }
  return nullptr;
}

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_BASICS_HPP
