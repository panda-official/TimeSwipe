// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

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

#ifndef PANDA_TIMESWIPE_DRIVER_BOARD_SETTINGS_HPP
#define PANDA_TIMESWIPE_DRIVER_BOARD_SETTINGS_HPP

#include "../common/basics.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace panda::timeswipe::driver {

/// Board-level settings.
class Board_settings final {
public:
  /// The destructor.
  ~Board_settings();

  /// Copy-constructible.
  Board_settings(const Board_settings&);

  /// Copy-assignable.
  Board_settings& operator=(const Board_settings&);

  /// Move-constructible.
  Board_settings(Board_settings&&);

  /// Move-assignable.
  Board_settings& operator=(Board_settings&&);

  /// The default constructor.
  Board_settings();

  /// The constructor.
  explicit Board_settings(std::string_view stringified_json);

  /// Swaps this instance with the `other` one.
  void swap(Board_settings& other) noexcept;

  /// @returns The result of conversion of this instance to a stringified JSON.
  std::string to_stringified_json() const;

  /// @name General control
  ///
  /// @brief This API provides a way to control of general board parameters.
  ///
  /// @{

  /// Sets the signal mode.
  Board_settings& set_signal_mode(Signal_mode mode);

  /// @returns The value of signal mode.
  std::optional<Signal_mode> get_signal_mode() const;

  /// @}

  /// @name Channel control
  ///
  /// @brief This API provides a way to control of PWM.
  ///
  /// Channel indexes must be in range `[0, 3]`.
  ///
  /// @{

  /// Sets the channel measurement mode.
  Board_settings& set_channel_measurement_mode(int index, Measurement_mode value);

  /// @returns The value of channel measurement mode.
  std::optional<Measurement_mode> get_channel_measurement_mode(int index) const;

  /// Sets the channel gain.
  Board_settings& set_channel_gain(int index, float value);

  /// @returns The value of channel gain.
  std::optional<float> get_channel_gain(int index) const;

  /// Sets the channel IEPE.
  Board_settings& set_channel_iepe(int index, bool value);

  /// @returns The value of channel IEPE.
  std::optional<bool> get_channel_iepe(int index) const;

  /// @}

  /// @name PWM control
  ///
  /// @brief This API provides a way to control of PWM.
  ///
  /// PWM indexes must be in range `[0, 1]`.
  ///
  /// @{

  /**
   * Sets the flag to start the PWM generator.
   *
   * PWM generator will run for `(pwm_repeat_count(index) / pwm_frequency(index))`
   * seconds and stop.
   */
  Board_settings& set_pwm_start(int index, bool value);

  /// @returns The value of PWM start flag.
  std::optional<bool> get_pwm_start(int index) const;

  /// Sets frequency.
  Board_settings& set_pwm_frequency(int index, int value);

  /// @returns The value of PWM frequency parameter.
  std::optional<int> get_pwm_frequency(int index) const;

  /// Sets PWM signal low value.
  Board_settings& set_pwm_low(int index, int value);

  /// @returns The value of PWM low parameter.
  std::optional<int> get_pwm_low(int index) const;

  /// Sets PWM signal high value.
  Board_settings& set_pwm_high(int index, int value);

  /// @returns The value of PWM high parameter.
  std::optional<int> get_pwm_high(int index) const;

  /**
   * Sets the number of repeat periods.
   *
   * @param value Zero value means infinity.
   */
  Board_settings& set_pwm_repeat_count(int index, int value);

  /// @returns The value of PWM repeat count parameter.
  std::optional<int> get_pwm_repeat_count(int index) const;

  /**
   * Sets the length of the PWM period when signal is in high state.
   *
   * @param value Reasonable value must be in range `(0, 1)`.
   */
  Board_settings& set_pwm_duty_cycle(int index, float value);

  /// @returns The value of PWM duty cycle parameter.
  std::optional<float> get_pwm_duty_cycle(int index) const;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_BOARD_SETTINGS_HPP
