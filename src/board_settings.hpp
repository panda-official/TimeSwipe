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
#include "types_fwd.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe {

/**
 * @brief Board-level settings.
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

  /**
   * @brief The constructor.
   *
   * @details Parses the JSON input. Possible JSON members are:
   *   - `channel[i]Mode` - an integer (see channel_measurement_modes());
   *   - `channel[i]Gain` - a float (see channel_gains());
   *   - `channel[i]Iepe` - a boolean (see channel_iepes());
   *   - `pwm[j]Enabled` - a boolean (see pwm_enabled()));
   *   - `pwm[j]Frequency` - an integer (see pwm_frequencies());
   *   - `pwm[j]LowBoundary` - an integer (see pwm_boundaries());
   *   - `pwm[i]HighBoundary` - an integer (see pwm_boundaries());
   *   - `pwm[j]RepeatCount` - an integer (see pwm_repeat_counts());
   *   - `pwm[j]DutyCycle` - a float (see pwm_duty_cycles()).
   * Suffix `[i]` of the members listed above is an integer in range
   * `[1, Driver::instance().max_channel_count()]`.
   * Suffix `[j]` of the members listed above is an integer in range
   * `[1, Driver::instance().max_pwm_count()]`.
   *
   * @see to_json_text().
   */
  explicit Board_settings(std::string_view json_text);

  /// @returns The vector of setting names.
  std::vector<std::string> names() const;

  /// @returns The vector of setting names which cannot be applied directly.
  std::vector<std::string> inapplicable_names() const;

  /// Swaps this instance with the `other` one.
  void swap(Board_settings& other) noexcept;

  /// Sets settings from `other` instance.
  void set(const Board_settings& other);

  /// @returns The result of conversion of this instance to a JSON text.
  std::string to_json_text() const;

  /// @returns `true` if this instance has no settings.
  bool is_empty() const;

  /// @name Channel control
  ///
  /// @brief This API provides a way to control of data channels.
  ///
  /// Channel indexes must be in range `[0, Driver::instance().max_channel_count())`.
  ///
  /// @{

  /**
   * @brief Sets measurement modes for all channels.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_channel_count())`.
   *
   * @warning This setting can be applied with Driver::set_board_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see channel_measurement_modes().
   */
  Board_settings& set_channel_measurement_modes(const std::vector<Measurement_mode>& values);

  /**
   * @returns Measurement mode of all channels, or `std::nullopt` if it's
   * not available for at least one channel.
   *
   * @see set_channel_measurement_modes().
   */
  std::optional<std::vector<Measurement_mode>> channel_measurement_modes() const;

  /**
   * @brief Sets gains for all channels.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_channel_count()`.
   * `values` must be in range `[Driver::instance().min_channel_gain(), Driver::instance().max_channel_gain()]`.
   *
   * @warning This setting can be applied with Driver::set_board_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see channel_gains().
   */
  Board_settings& set_channel_gains(const std::vector<float>& values);

  /**
   * @returns Gains of all channels, or `std::nullopt` if it's
   * not available for at least one channel.
   *
   * @see set_channel_gains().
   */
  std::optional<std::vector<float>> channel_gains() const;

  /**
   * @brief Sets IEPE flags for all channels.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_channel_count())`.
   *
   * @warning This setting can be applied with Driver::set_board_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see channel_iepes().
   */
  Board_settings& set_channel_iepes(const std::vector<bool>& values);

  /**
   * @returns IEPE flags of all channels, or `std::nullopt` if it's
   * not available for at least one channel.
   *
   * @see set_channel_iepes().
   */
  std::optional<std::vector<bool>> channel_iepes() const;

  /// @}

  /// @name PWM control
  ///
  /// @brief This API provides a way to control of PWM.
  ///
  /// @warning PWM indexes must be in range `[0, 2)`.
  ///
  /// @{

  /**
   * @brief Sets enable-flags for all PWM generators.
   *
   * @details PWM generator `i` will run for
   * `(pwm_repeat_counts()[i] / pwm_frequencies()[i])` seconds and then stop.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_pwm_count())`.
   *
   * @see pwm_enabled().
   */
  Board_settings& set_pwm_enabled(const std::vector<bool>& values);

  /**
   * @returns Enable-flag of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   *
   * @see set_pwm_enabled().
   */
  std::optional<std::vector<bool>> pwm_enabled() const;

  /**
   * @brief Sets frequencies for all PWMs.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_pwm_count())`.
   * `values` must be in range `[1, 1000]`.
   *
   * @see pwm_frequencies().
   */
  Board_settings& set_pwm_frequencies(const std::vector<int>& values);

  /**
   * @returns Frequency of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   *
   * @see set_pwm_frequencies().
   */
  std::optional<std::vector<int>> pwm_frequencies() const;

  /**
   * @brief Sets boundaries for all PWMs.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_pwm_count())`.
   * `values` must be in range `[0, 4095]`.
   * Truth of `(values[i].first <= values[i].second)` for PWM `i`.
   *
   * @see pwm_boundaries().
   */
  Board_settings& set_pwm_boundaries(const std::vector<std::pair<int, int>>& values);

  /**
   * @returns Boundaries of all PWMs, or `std::nullopt` if it's not available
   * for at least one PWM.
   *
   * @see set_pwm_boundaries().
   */
  std::optional<std::vector<std::pair<int, int>>> pwm_boundaries() const;

  /**
   * @brief Sets the number of repeat periods for all PWMs.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_pwm_count())`.
   * Truth of `(values[i] >= 0)` for PWM `i`.
   *
   * @remarks Zero value means "infinity".
   *
   * @see pwm_repeat_counts().
   */
  Board_settings& set_pwm_repeat_counts(const std::vector<int>& values);

  /**
   * @returns Repeat counts of all PWMs, or `std::nullopt` if it's
   * not available for at least one PWM.
   *
   * @see set_pwm_repeat_counts().
   */
  std::optional<std::vector<int>> pwm_repeat_counts() const;

  /**
   * @brief Sets the length of the period when signal is in the high state for
   * all PWMs.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_pwm_count())`.
   * `values` must be in range `(0, 1)`.
   *
   * @see pwm_duty_cycles().
   */
  Board_settings& set_pwm_duty_cycles(const std::vector<float>& values);

  /**
   * @returns Length of the period when signal is in high state of all PWMs, or
   * `std::nullopt` if it's not available for at least one PWM.
   *
   * @see set_pwm_duty_cycles().
   */
  std::optional<std::vector<float>> pwm_duty_cycles() const;

  /// @}

private:
  friend detail::iDriver;

  struct Rep;

  explicit Board_settings(std::unique_ptr<Rep> rep);

  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_BOARD_SETTINGS_HPP
