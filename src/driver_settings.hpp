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

#ifndef PANDA_TIMESWIPE_DRIVER_SETTINGS_HPP
#define PANDA_TIMESWIPE_DRIVER_SETTINGS_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe {

/**
 * @brief Driver-level settings.
 *
 * @see Driver::set_settings().
 */
class Driver_settings final {
public:
  /// The destructor.
  ~Driver_settings();

  /// Copy-constructible.
  Driver_settings(const Driver_settings&);

  /// Copy-assignable.
  Driver_settings& operator=(const Driver_settings&);

  /// Move-constructible.
  Driver_settings(Driver_settings&&);

  /// Move-assignable.
  Driver_settings& operator=(Driver_settings&&);

  /// The default constructor.
  Driver_settings();

  /**
   * @brief The constructor.
   *
   * @details Parses the JSON input. Possible JSON members are:
   *   - `sampleRate` - an integer (see sample_rate());
   *   - `burstBufferSize` - an integer (see burst_buffer_size());
   *   - `frequency` - an integer (see frequency());
   *   - `translationOffsets` - an array of integers (see translation_offsets());
   *   - `translationSlopes` - an array of floats (see translation_slopes()).
   * The exception with code `Errc::driver_settings_invalid` will be thrown if
   * both `burstBufferSize` and `frequency` are presents in the same JSON input.
   *
   * @see to_json_text().
   */
  explicit Driver_settings(std::string_view json_text);

  /// Swaps this instance with the `other` one.
  void swap(Driver_settings& other) noexcept;

  /// Sets settings from `other` instance.
  void set(const Driver_settings& other);

  /// @returns The result of conversion of this instance to a JSON text.
  std::string to_json_text() const;

  /// @returns `true` if this instance has no settings.
  bool is_empty() const;

  /**
   * @brief Set sample rate.
   *
   * @details If this setting isn't set, the driver will use
   * `Driver::instance().max_sample_rate()`.
   *
   * @param rate The value of sample rate.
   *
   * @returns The reference to this instance.
   *
   * @par Requires
   * `(Driver::instace().min_sample_rate() <= rate &&
   *  rate <= Driver::instance().max_sample_rate())`.
   *
   * @warning It's highly recommended to use `rate` for which
   * `(Driver::instance().max_sample_rate() % rate == 0)` for best performance!
   * In other words, the lower the value of
   * `std::gcd(Driver::instance().max_sample_rate(), rate)`, the worse the
   * performance of the resampling.
   *
   * @warning This setting can be applied with Driver::set_driver_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see sample_rate(), Driver::min_sample_rate(), Driver::max_sample_rate().
   */
  Driver_settings& set_sample_rate(int rate);

  /**
   * @returns The current sample rate.
   *
   * @see set_sample_rate().
   */
  std::optional<int> sample_rate() const;

  /**
   * @brief Sets the burst buffer size.
   *
   * @par Requires
   * `(Driver::instance().min_sample_rate() <= size &&
   *  size <= Driver::instance().max_sample_rate())`.
   *
   * @par Effects
   * Affects the values returned by frequency() and to_json_text().
   * (The later will be without the `frequency` member.)
   *
   * @param size The number of records that the driver should deliver to
   * Driver::Data_handler.
   *
   * @warning This setting can be applied with Driver::set_driver_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see burst_buffer_size().
   */
  Driver_settings& set_burst_buffer_size(std::size_t size);

  /**
   * @returns The burst buffer size.
   *
   * @see set_burst_buffer_size(), frequency().
   */
  std::optional<std::size_t> burst_buffer_size() const;

  /**
   * @brief Indirect way to set the burst buffer size.
   *
   * @par Requires
   * `(1 <= frequency && frequency <= sample_rate())`.
   *
   *  @par Effects
   *  Affects the value returned by burst_buffer_size() and to_json_text().
   *  (The later will be without the `burstBufferSize` member.)
   *
   * @param size The number of records that the driver should deliver to
   * Driver::Data_handler.
   *
   * @warning This setting can be applied with Driver::set_driver_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see frequency().
   */
  Driver_settings& set_frequency(int frequency);

  /**
   * @returns The frequency value: `sample_rate() / burst_buffer_size()`.
   *
   * @see set_frequency(), burst_buffer_size(), sample_rate().
   */
  std::optional<int> frequency() const;

  /// @name Measured values transformation control
  ///
  /// @brief This API allows to control how the values, measured in `mV` must
  /// be transformed.
  ///
  /// @details Parameters `translationOffsets` and `translationSlopes` can be
  /// used to provide values for transformations of the values, measured in
  /// `mV`, by applying the following formula for that purpose:
  /// ```
  /// data[i] = (value[i] - translationOffsets[i]) * translationSlopes[i]
  /// ```
  /// where: `i` - is a sensor number, `value` - is a value, measured in `mV`
  /// by the sensor `i`.
  ///
  /// @{

  /**
   * @brief Sets translation offsets for all channels.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_channel_count())`.
   *
   * @returns The reference to this instance.
   *
   * @warning This setting can be applied with Driver::set_driver_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see translation_offsets().
   */
  Driver_settings& set_translation_offsets(const std::vector<int>& values);

  /**
   * @returns The translation offsets for all channels.
   *
   * @see set_translation_offsets().
   */
  std::optional<std::vector<int>> translation_offsets() const;

  /**
   * @brief Sets translation slopes for all channels.
   *
   * @par Requires
   * `(values.size() == Driver::instance().max_channel_count())`.
   *
   * @returns The reference to this instance.
   *
   * @warning This setting can be applied with Driver::set_driver_settings()
   * only if `!Driver::instance().is_measurement_started()`.
   *
   * @see translation_slopes().
   */
  Driver_settings& set_translation_slopes(const std::vector<float>& values);

  /**
   * @returns The translation slope for all channels.
   *
   * @see set_translation_slopes().
   */
  std::optional<std::vector<float>> translation_slopes() const;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_DRIVER_SETTINGS_HPP
