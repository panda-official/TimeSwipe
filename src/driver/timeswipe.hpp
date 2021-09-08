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

#ifndef PANDA_TIMESWIPE_DRIVER_TIMESWIPE_HPP
#define PANDA_TIMESWIPE_DRIVER_TIMESWIPE_HPP

#include "types_fwd.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe::driver {

/**
 * A Timeswipe driver.
 *
 * @note This class is designed by following the singleton pattern.
 */
class Timeswipe final {
public:
  /// An alias of driver settings.
  using Settings = Driver_settings;

  /**
   * An alias of a function to handle the incoming events.
   *
   * @see start().
   */
  using Event_handler = std::function<void(Event&&)>;

  /**
   * An alias of a function to handle the incoming sensor data.
   *
   * @param data Portion of the incoming data to process.
   * @param error_marker The error marker:
   *   - zero indicates "no error";
   *   - positive value indicates the number of data losts;
   *   - negative value indicates the negated error code (some value of
   *   panda::timeswipe::Errc with the minus sign) in case of fatal error when
   *   the measurement process is about to stop.
   *
   * @see start().
   */
  using Sensor_data_handler = std::function<void(Sensors_data data, int error_marker)>;

  /**
   * The destructor. Calls stop().
   *
   * @see stop().
   */
  ~Timeswipe();

  /// Non copy-constructible.
  Timeswipe(const Timeswipe&) = delete;

  /// Non copy-assignable.
  Timeswipe& operator=(const Timeswipe&) = delete;

  /// Non move-constructible.
  Timeswipe(Timeswipe&&) = delete;

  /// Non move-assignable.
  Timeswipe& operator=(Timeswipe&&) = delete;

  /**
   * @returns The instance of this class.
   *
   * @par Effects
   * Restarts Timeswipe firmware on very first call!
   */
  static Timeswipe& get_instance();

  /// @returns Min possible sample rate per second.
  int get_min_sample_rate() const;

  /// @returns Max possible sample rate per second.
  int get_max_sample_rate() const;

  /// @returns The number of data channels.
  int get_data_channel_count() const;

  /**
   * Sets the board settings.
   *
   * @see get_board_settings().
   */
  void set_board_settings(const Board_settings& settings);

  /**
   * @returns The actual board settings.
   *
   * @see set_board_settings().
   */
  const Board_settings& get_board_settings() const;

  /**
   * Sets the driver settings.
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @see get_settings();
   */
  void set_settings(Settings settings);

  /**
   * @returns The driver settings.
   *
   * @see set_settings();
   */
  const Settings& get_settings() const;

  /// @name Measurement control
  ///
  /// @brief This API provides a way to control measurement process.
  ///
  /// @{

  /**
   * Initiates the start of measurement.
   *
   * @par Effects
   * Repeatedly calls the `sdh` with frequency (Hz) that depends on the burst
   * size, specified in the measurement options: the greater it's value, the
   * less frequent the handler is called. When `burst_size == sample_rate` the
   * frequency is `1`.
   *
   * @warning The `sdh` must not take more than `burst_size / sample_rate`
   * seconds of runtime! Otherwise, the driver will throttle by skipping the
   * incoming sensor data and `sdh` will be called with positive error marker.
   *
   * @warning This method cannot be called from neither `sdh` nor `evh`!
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @par Effects
   * `is_busy()`.
   *
   * @see set_measurement_options(), set_state(), stop().
   */
  void start(Sensor_data_handler sdh, Event_handler evh = {});

  /**
   * @returns `true` if the board is busy (measurement in progress).
   *
   * @see calculate_drift_references(), calculate_drift_deltas(), start().
   */
  bool is_busy() const noexcept;

  /**
   * Stops the measurement.
   *
   * @par Effects
   * `!is_busy()`.
   *
   * @see start().
   */
  void stop();

  /// @}

  /// @name Drift Compensation
  ///
  /// @brief This API provides a way to compensate the long term drift of the
  /// measurement hardware when making long term measurements with the Timeswipe
  /// board.
  ///
  /// @detail The approach assumes the calculation for each channel of the
  /// reference values (*references*) and deviations from these values
  /// (*deltas*). The later are used for correction (by subtraction) of all the
  /// values which comes from the hardware.
  /// The calculated references are saved to a file for long-term storage which
  /// can be useful in cases like power failures. The deltas can only be
  /// calculated if there are references available, otherwise and exception
  /// will be thrown. Unlike the references, the deltas are not saved to a file.
  /// Either the references or the deltas can be recalculated an arbitrary number
  /// of times. It's also possible to clear either the references or the deltas
  /// in order to stop correcting the input values and pass them to the user
  /// read callback unmodified. Please note, that in order to calculate or clear
  /// either the references or deltas the board must not be busy (started).
  ///
  /// @{

  /**
   * Calculates drift references.
   *
   * The calculated references are stored to
   * `<CWD>/.panda/timeswipe/drift_reference` for persistent storage until
   * either it deleted directly or by calling clear_drift_references().
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @par Effects
   * `!is_busy() && get_drift_references()`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see get_drift_references(), clear_drift_references(), calculate_drift_deltas().
   */
  std::vector<float> calculate_drift_references();

  /**
   * Clears drift references if any.
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @par Effects
   * `!get_drift_references() && !get_drift_deltas()`. Removes the file
   * `<CWD>/.panda/timeswipe/drift_references`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see calculate_drift_references(), clear_drift_deltas().
   */
  void clear_drift_references();

  /**
   * @brief Calculates drift deltas based on calculated drift references.
   *
   * @par Requires
   * `get_drift_references() && !is_busy()`.
   *
   * @par Effects
   * `!is_busy() && get_drift_deltas()`.
   * After calling the `start()`, calculated deltas will be substracted from
   * each input value of the corresponding channel.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see get_drift_deltas(), calculate_drift_references(), start().
   */
  std::vector<float> calculate_drift_deltas();

  /**
   * @brief Clears drift deltas if any.
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @par Effects
   * Input values of the corresponding channel will not be affected by deltas.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see calculate_drift_deltas(), clear_drift_references().
   */
  void clear_drift_deltas();

  /**
   * @returns The calculated drift references.
   *
   * @param force Forces the reading of references from a filesystem if `true`.
   * Otherwise, the last cached value will be returned.
   *
   * @throws An Exception with the code `Errc::invalid_drift_reference` if
   * file `<CWD>/.panda/timeswipe/drift_references` contains a junk.
   *
   * @see calculate_drift_references(), clear_drift_references(), get_drift_deltas().
   */
  std::optional<std::vector<float>> get_drift_references(bool force = {}) const;

  /**
   * @returns The calculated drift deltas.
   *
   * @see calculate_drift_deltas().
   */
  std::optional<std::vector<float>> get_drift_deltas() const;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
  inline static std::unique_ptr<Timeswipe> instance_;

  Timeswipe();
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_TIMESWIPE_HPP
