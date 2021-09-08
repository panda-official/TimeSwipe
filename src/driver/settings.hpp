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

#ifndef PANDA_TIMESWIPE_DRIVER_SETTINGS_HPP
#define PANDA_TIMESWIPE_DRIVER_SETTINGS_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace panda::timeswipe::driver {

/// Driver-level settings.
class Settings final {
public:
  /// The destructor.
  ~Settings();

  /// Copy-constructible.
  Settings(const Settings&);

  /// Copy-assignable.
  Settings& operator=(const Settings&);

  /// Move-constructible.
  Settings(Settings&&);

  /// Move-assignable.
  Settings& operator=(Settings&&);

  /// The default constructor.
  Settings();

  /// The constructor.
  explicit Settings(std::string_view stringified_json);

  /// Swaps this instance with the `other` one.
  void swap(Settings& other) noexcept;

  /// @returns The result of conversion of this instance to a stringified JSON.
  std::string to_stringified_json() const;

  /**
   * Set sample rate.
   *
   * The default value is equals to Timeswipe::get_instance().get_max_sample_rate().
   *
   * @param rate - new sample rate
   *
   * @returns The reference to this instance.
   *
   * @par Requires
   * `(min_sample_rate <= rate && rate <= max_sample_rate)`.
   *
   * @warning It's highly recommended not to use the rate for which
   * `(max_sample_rate % rate != 0)` for best performance! In other words, the
   * lower the value of `std::gcd(max_sample_rate, rate)`, the worse the
   * performance of the resampling.
   *
   * @see get_sample_rate(), Timeswipe::get_min_sample_rate(),
   * Timeswipe::get_max_sample_rate().
   */
  Settings& set_sample_rate(int rate);

  /**
   * @returns The current sample rate.
   *
   * @see set_sample_rate().
   */
  int get_sample_rate() const;

  /**
   * Sets the burst buffer size.
   *
   * @param size The number of records that the driver should deliver upon
   * of Timeswipe::Data_handler call.
   */
  Settings& set_burst_buffer_size(std::size_t size);

  /// @returns The burst buffer size.
  std::size_t get_burst_buffer_size() const;

  /**
   * Sets the data translation offset.
   *
   * @param index The data channel index.
   * @param value The value of the translation offset to set.
   *
   * @returns The reference to this instance.
   */
  Settings& set_data_translation_offset(int index, int value);

  /// @returns The value of data translation offset.
  int get_data_translation_offset(int index) const;

  /**
   * Sets the data translation slope.
   *
   * @param index The data channel index.
   * @param value The value of the translation slope to set.
   *
   * @returns The reference to this instance.
   */
  Settings& set_data_translation_slope(int index, float value);

  /// @returns The value of data translation slope.
  float get_data_translation_slope(int index) const;

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_SETTINGS_HPP
