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

#ifndef PANDA_TIMESWIPE_DRIVER_TIMESWIPE_STATE_HPP
#define PANDA_TIMESWIPE_DRIVER_TIMESWIPE_STATE_HPP

#include "../common/basics.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace panda::timeswipe::driver {

/// Timeswipe board state.
class Timeswipe_state final {
public:
  /// The destructor.
  ~Timeswipe_state();

  /// Copy-constructible.
  Timeswipe_state(const Timeswipe_state&);

  /// Copy-assignable.
  Timeswipe_state& operator=(const Timeswipe_state&);

  /// Move-constructible.
  Timeswipe_state(Timeswipe_state&&);

  /// Move-assignable.
  Timeswipe_state& operator=(Timeswipe_state&&);

  /// The default constructor.
  Timeswipe_state();

  /// The constructor.
  explicit Timeswipe_state(std::string_view stringified_json);

  /// Swaps this instance with the `other` one.
  void swap(Timeswipe_state& other) noexcept;

  /// @returns The result of conversion of this instance to a stringified JSON.
  std::string to_stringified_json() const;

  /// @name General control
  ///
  /// @brief This API provides a way to control of general board parameters.
  ///
  /// @{

  /// Sets the signal mode.
  Timeswipe_state& set_signal_mode(Signal_mode mode);

  /// @returns The value of signal mode.
  std::optional<Signal_mode> signal_mode() const;

  /// @}

  /// @name Channel control
  ///
  /// @brief This API provides a way to control of PWM.
  ///
  /// Channel indexes must be in range `[0, 3]`.
  ///
  /// @{

  /// Sets the channel measurement mode.
  Timeswipe_state& set_channel_measurement_mode(int index, Measurement_mode value);

  /// @returns The value of channel measurement mode.
  std::optional<Measurement_mode> channel_measurement_mode(int index) const;

  /// Sets the channel gain.
  Timeswipe_state& set_channel_gain(int index, float value);

  /// @returns The value of channel gain.
  std::optional<float> channel_gain(int index) const;

  /// Sets the channel IEPE.
  Timeswipe_state& set_channel_iepe(int index, bool value);

  /// @returns The value of channel IEPE.
  std::optional<bool> channel_iepe(int index) const;

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
  Timeswipe_state& set_pwm_start(int index, bool value);

  /// @returns The value of PWM start flag.
  std::optional<bool> pwm_start(int index) const;

  /// Sets frequency.
  Timeswipe_state& set_pwm_frequency(int index, int value);

  /// @returns The value of PWM frequency parameter.
  std::optional<int> pwm_frequency(int index) const;

  /// Sets PWM signal low value.
  Timeswipe_state& set_pwm_low(int index, int value);

  /// @returns The value of PWM low parameter.
  std::optional<int> pwm_low(int index) const;

  /// Sets PWM signal high value.
  Timeswipe_state& set_pwm_high(int index, int value);

  /// @returns The value of PWM high parameter.
  std::optional<int> pwm_high(int index) const;

  /**
   * Sets the number of repeat periods.
   *
   * @param value Zero value means infinity.
   */
  Timeswipe_state& set_pwm_repeat_count(int index, int value);

  /// @returns The value of PWM repeat count parameter.
  std::optional<int> pwm_repeat_count(int index) const;

  /**
   * Sets the length of the PWM period when signal is in high state.
   *
   * @param value Reasonable value must be in range `(0, 1)`.
   */
  Timeswipe_state& set_pwm_duty_cycle(int index, float value);

  /// @returns The value of PWM duty cycle parameter.
  std::optional<float> pwm_duty_cycle(int index) const;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_TIMESWIPE_STATE_HPP
