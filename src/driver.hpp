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

#ifndef PANDA_TIMESWIPE_DRIVER_HPP
#define PANDA_TIMESWIPE_DRIVER_HPP

#include "board_settings.hpp"
#include "data_vector.hpp"
#include "driver_settings.hpp"
#include "event.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe {

/**
 * A Timeswipe driver.
 *
 * @note This class is designed by following the singleton pattern.
 */
class Driver final {
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
  using Data_handler = std::function<void(Data_vector data, int error_marker)>;

  /**
   * The destructor. Calls stop().
   *
   * @see stop().
   */
  ~Driver();

  /// Non copy-constructible.
  Driver(const Driver&) = delete;

  /// Non copy-assignable.
  Driver& operator=(const Driver&) = delete;

  /// Non move-constructible.
  Driver(Driver&&) = delete;

  /// Non move-assignable.
  Driver& operator=(Driver&&) = delete;

  /**
   * @returns The instance of this class.
   *
   * @par Effects
   * Restarts firmware on very first call!
   */
  static Driver& instance();

  /// @returns The driver version.
  int version() const;

  /// @returns Min possible sample rate per second the driver can handle.
  int min_sample_rate() const;

  /// @returns Max possible sample rate per second the driver can handle.
  int max_sample_rate() const;

  /// @returns Max possible number of data channels the board provides.
  int max_data_channel_count() const;

  /**
   * Sets the board settings.
   *
   * @see board_settings().
   */
  void set_board_settings(const Board_settings& settings);

  /**
   * @returns The actual board settings.
   *
   * @see set_board_settings().
   */
  const Board_settings& board_settings() const;

  /**
   * Sets the driver settings.
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @see settings();
   */
  void set_settings(Settings settings);

  /**
   * @returns The driver settings.
   *
   * @see set_settings();
   */
  const Settings& settings() const;

  /// @name Measurement control
  ///
  /// @brief This API provides a way to control measurement process.
  ///
  /// @{

  /**
   * Initiates the start of measurement.
   *
   * @par Effects
   * Repeatedly calls the `data_handler` with frequency (Hz) that depends on the
   * burst size, specified in the measurement options: the greater it's value,
   * the less frequent the handler is called. When `burst_size == sample_rate`
   * the frequency is `1`.
   *
   * @warning The `data_handler` must not take more than `burst_size / sample_rate`
   * seconds of runtime! Otherwise, the driver will throttle by skipping the
   * incoming sensor data and `data_handler` will be called with positive error
   * marker.
   *
   * @warning This method cannot be called from neither `data_handler` nor
   * `event_handler`!
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @par Effects
   * `is_busy()`.
   *
   * @see set_measurement_options(), set_state(), stop().
   */
  void start(Data_handler data_handler, Event_handler event_handler = {});

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
  /// measurement hardware when making long term measurements.
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
   * `!is_busy() && drift_references()`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see drift_references(), clear_drift_references(), calculate_drift_deltas().
   */
  std::vector<float> calculate_drift_references();

  /**
   * Clears drift references if any.
   *
   * @par Requires
   * `!is_busy()`.
   *
   * @par Effects
   * `!drift_references() && !drift_deltas()`. Removes the file
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
   * `drift_references() && !is_busy()`.
   *
   * @par Effects
   * `!is_busy() && drift_deltas()`.
   * After calling the `start()`, calculated deltas will be substracted from
   * each input value of the corresponding channel.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see drift_deltas(), calculate_drift_references(), start().
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
   * @see calculate_drift_references(), clear_drift_references(), drift_deltas().
   */
  std::optional<std::vector<float>> drift_references(bool force = {}) const;

  /**
   * @returns The calculated drift deltas.
   *
   * @see calculate_drift_deltas().
   */
  std::optional<std::vector<float>> drift_deltas() const;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
  inline static std::unique_ptr<Driver> instance_;

  Driver();
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_DRIVER_HPP
