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

/**
 * @file
 *
 * @remarks The code of this file is exception-free!
 */

#ifndef PANDA_TIMESWIPE_DRIVER_BASICS_HPP
#define PANDA_TIMESWIPE_DRIVER_BASICS_HPP

#include <optional>
#include <string_view>

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Resampler_mode
// -----------------------------------------------------------------------------

/// Resampler mode.
enum class Resampler_mode {
  disabled,
  fir
};

/**
 * @returns The value of type Resampler_mode converted from `value`, or
 * `std::nullopt` if `value` doesn't corresponds to any member of Resampler_mode.
 */
constexpr std::optional<Resampler_mode> to_resampler_mode(const
  std::string_view value) noexcept
{
  if (value == "disabled") return Resampler_mode::disabled;
  else if (value == "fir") return Resampler_mode::fir;
  else return {};
}

/**
 * @returns The character literal converted from `value`, or `nullptr`
 * if `value` doesn't corresponds to any member of Resampler_mode.
 */
constexpr const char* to_literal(const Resampler_mode value) noexcept
{
  switch (value) {
  case Resampler_mode::disabled: return "disabled";
  case Resampler_mode::fir: return "fir";
  }
  return nullptr;
}

// -----------------------------------------------------------------------------
// Filter_mode
// -----------------------------------------------------------------------------

/// Filter mode.
enum class Filter_mode {
  disabled,
  iir
};

/**
 * @returns The value of type Filter_mode converted from `value`, or
 * `std::nullopt` if `value` doesn't corresponds to any member of Filter_mode.
 */
constexpr std::optional<Filter_mode> to_filter_mode(const
  std::string_view value) noexcept
{
  if (value == "disabled") return Filter_mode::disabled;
  else if (value == "iir") return Filter_mode::iir;
  else return {};
}

/**
 * @returns The character literal converted from `value`, or `nullptr`
 * if `value` doesn't corresponds to any member of Filter_mode.
 */
constexpr const char* to_literal(const Filter_mode value) noexcept
{
  switch (value) {
  case Filter_mode::disabled: return "disabled";
  case Filter_mode::iir: return "iir";
  }
  return nullptr;
}

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_DRIVER_BASICS_HPP
