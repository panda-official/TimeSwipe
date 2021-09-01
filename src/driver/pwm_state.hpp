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

#ifndef PANDA_TIMESWIPE_DRIVER_PWM_STATE_HPP
#define PANDA_TIMESWIPE_DRIVER_PWM_STATE_HPP

namespace panda::timeswipe::driver {

/// PWM state.
class Pwm_state final {
public:
  /// The default constructor.
  Pwm_state() noexcept = default;

  /// Sets frequency. (Periods per second.) Must be in range `[1, 1000]`.
  Pwm_state& frequency(int value);

  /// @returns The value of `frequency` parameter.
  int frequency() const noexcept
  {
    return frequency_;
  }

  /// Sets PWM signal low value. Must be in range `[0, 4095]`.
  Pwm_state& low(int value);

  /// @returns The value of `low` parameter.
  int low() const noexcept
  {
    return low_;
  }

  /// Sets PWM signal high value. Must be in range `[0, 4095]`.
  Pwm_state& high(int value);

  /// @returns The value of `high` parameter.
  int high() const noexcept
  {
    return high_;
  }

  /**
   * Sets the number of repeat periods. Must be greater or equal to `0`.
   * The value of `0` means infinity.
   */
  Pwm_state& repeat_count(int value);

  /// @returns The value of `repeat_count` parameter.
  int repeat_count() const noexcept
  {
    return repeat_count_;
  }

  /**
   * Sets the length of the PWM period when signal is in high state.
   * Must be in range `(0, 1)`.
   */
  Pwm_state& duty_cycle(float value);

  /// @returns The value of `duty_cycle` parameter.
  float duty_cycle() const noexcept
  {
    return duty_cycle_;
  }

private:
  int frequency_{};
  int low_{};
  int high_{4095};
  int repeat_count_{};
  float duty_cycle_{.5};
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_PWM_STATE_HPP
