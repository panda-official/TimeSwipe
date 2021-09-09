// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_BASICS_HPP
#define PANDA_TIMESWIPE_BASICS_HPP

#include <optional>

namespace panda::timeswipe {

/// The absolute maximum possible number of data channels.
constexpr int max_data_channel_count{4};

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
