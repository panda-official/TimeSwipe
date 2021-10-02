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

#ifndef PANDA_TIMESWIPE_BOARD_SETTINGS_HPP
#define PANDA_TIMESWIPE_BOARD_SETTINGS_HPP

#include "basics.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe {

/**
 * Board-level settings.
 *
 * @see Driver::set_board_settings().
 */
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
  explicit Board_settings(std::string_view json_text);

  /// Swaps this instance with the `other` one.
  void swap(Board_settings& other) noexcept;

  /// @returns The result of conversion of this instance to a JSON text.
  std::string to_json_text() const;

  /// Sets settings from `other` instance.
  void set(const Board_settings& other);

  /// @name General control
  ///
  /// @brief This API provides a way to control of general board parameters.
  ///
  /// @{

  /**
   * Sets the signal mode.
   *
   * @warning This setting can be applied with Driver::set_board_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   */
  Board_settings& set_signal_mode(Signal_mode value);

  /// @returns The value of signal mode.
  std::optional<Signal_mode> signal_mode() const;

  /// @}

  /// @name Channel control
  ///
  /// @brief This API provides a way to control of data channels.
  ///
  /// Channel indexes must be in range `[0, Driver::instance().max_channel_count())`.
  ///
  /// @{

  /**
   * Sets measurement modes for all channels.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_channel_count())`.
   *
   * @warning This setting can be applied with Driver::set_board_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   */
  Board_settings& set_channel_measurement_modes(const std::vector<Measurement_mode>& values);

  /**
   * @returns Measurement mode of all channels, or `std::nullopt` if it's
   * not available for at least one channel.
   */
  std::optional<std::vector<Measurement_mode>> channel_measurement_modes() const;

  /**
   * Sets gains for all channels.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_channel_count())`.
   */
  Board_settings& set_channel_gains(const std::vector<float>& values);

  /**
   * @returns Gains of all channels, or `std::nullopt` if it's
   * not available for at least one channel.
   */
  std::optional<std::vector<float>> channel_gains() const;

  /**
   * Sets IEPE flags for all channels.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_channel_count())`.
   */
  Board_settings& set_channel_iepes(const std::vector<bool>& values);

  /**
   * @returns IEPE flags of all channels, or `std::nullopt` if it's
   * not available for at least one channel.
   */
  std::optional<std::vector<bool>> channel_iepes() const;

  /// @}

  /// @name PWM control
  ///
  /// @brief This API provides a way to control of PWM.
  ///
  /// PWM indexes must be in range `[0, 2)`.
  ///
  /// @{

  /**
   * Sets start-flags for all PWM generators.
   *
   * PWM generator `i` will run for `(pwm_repeat_counts()[i] / pwm_frequencies()[i])`
   * seconds and then stop.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_pwm_count())`.
   */
  Board_settings& set_pwms(const std::vector<bool>& values);

  /**
   * @returns Start-flag of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   */
  std::optional<std::vector<bool>> pwms() const;

  /**
   * Sets frequencies for all PWMs.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_pwm_count())`.
   */
  Board_settings& set_pwm_frequencies(const std::vector<int>& values);

  /**
   * @returns Frequency of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   */
  std::optional<std::vector<int>> pwm_frequencies() const;

  /**
   * Sets signal levels for all PWMs.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_pwm_count()) &&
   *  (0 <= values[i].first && values[i].first <= 4095) &&
   *  (0 <= values[i].second && values[i].second <= 4095) &&
   *  (values[i].first <= values[i].second)` for PWM `i`.
   */
  Board_settings& set_pwm_signal_levels(const std::vector<std::pair<int, int>>& values);

  /**
   * @returns Signal levels of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   */
  std::optional<std::vector<std::pair<int, int>>> pwm_signal_levels() const;

  /**
   * Sets the number of repeat periods for all PWMs.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_pwm_count()) &&
   *  (values[i] >= 0)` for PWM `i`.
   *
   * @remarks Zero value means "infinity".
   */
  Board_settings& set_pwm_repeat_counts(const std::vector<int>& values);

  /**
   * @returns Repeat counts of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   */
  std::optional<std::vector<int>> pwm_repeat_counts() const;

  /**
   * Sets the length of the period when signal is in high state for all PWMs.
   *
   * @par Requires
   * `(values.size() >= Driver::instance().max_pwm_count()) &&
   *  (0 < values[i] && values[i] < 1)` for PWM `i`.
   */
  Board_settings& set_pwm_duty_cycles(const std::vector<float>& values);

  /**
   * @returns Length of the period when signal is in high state of all PWMs, or
   * `std::nullopt` if it's not available for at least one PWM.
   */
  std::optional<std::vector<float>> pwm_duty_cycles() const;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_BOARD_SETTINGS_HPP
