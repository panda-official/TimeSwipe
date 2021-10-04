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

#ifndef PANDA_TIMESWIPE_DRIVER_HPP
#define PANDA_TIMESWIPE_DRIVER_HPP

// All the public API headers.
#include "board_settings.hpp"
#include "data_vector.hpp"
#include "driver_settings.hpp"
#include "error.hpp"
#include "types_fwd.hpp"

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
class Driver {
public:
  /// An alias of driver settings.
  using Settings = Driver_settings;

  /**
   * An alias of a function to handle the incoming channel data.
   *
   * @param data Portion of the incoming data to process.
   * @param error_marker The error marker:
   *   - zero indicates "no error";
   *   - positive value indicates the number of data losts;
   *   - negative value indicates the negated error code (some value of
   *   panda::timeswipe::Generic_errc with the minus sign) in case of fatal
   *   error when the measurement process is about to stop.
   *
   * @see start_measurement().
   */
  using Data_handler = std::function<void(Data_vector data, int error_marker)>;

  /**
   * The destructor. Calls stop_measurement().
   *
   * @see stop_measurement().
   */
  virtual ~Driver() = default;

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
   * @remarks Doesn't initialize the driver implicitly.
   *
   * @see initialize().
   */
  static Driver& instance();

  /**
   * Explicitly initializes the driver.
   *
   * @par Effects
   * Restarts the firmware on very first call!
   *
   * @warning Firmware developers must remember, that restarting the firmware
   * causes reset of all the settings the firmware keeps in the on-board RAM!
   */
  virtual void initialize() = 0;

  /// @returns `true` if the driver initialized.
  virtual bool is_initialized() const = 0;

  /// @returns The driver version value as `major*10000 + minor*100 + patch`.
  virtual int version() const = 0;

  /// @returns Min possible sample rate per second the driver can handle.
  virtual int min_sample_rate() const = 0;

  /// @returns Max possible sample rate per second the driver can handle.
  virtual int max_sample_rate() const = 0;

  /// @returns Max possible number of data channels the board provides.
  virtual int max_channel_count() const = 0;

  /// @returns Max possible number of PWM the board provides.
  virtual int max_pwm_count() const = 0;

  /// @returns Min possible channel gain value.
  virtual float min_channel_gain() const = 0;

  /// @returns Max possible channel gain value.
  virtual float max_channel_gain() const = 0;

  /**
   * Sets the board-level settings.
   *
   * @warning Some of the board-level settings can be applied only when
   * `!is_measurement_started()` as explained in the documentation of
   * Board_settings class.
   *
   * @par Requires
   * `is_initialized()`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see board_settings(), Board_settings.
   */
  virtual void set_board_settings(const Board_settings& settings) = 0;

  /**
   * @returns The board-level settings.
   *
   * @see set_board_settings().
   */
  virtual const Board_settings& board_settings() const = 0;

  /**
   * Sets the driver-level settings.
   *
   * Each call of this method affects only a subset of current settings(). I.e.,
   * it's not necessary to pass all possible settings per call - the settings()
   * can be constructed piecewise.
   *
   * @warning Some of the driver-level settings can be applied only when
   * `!is_measurement_started()` as explained in the documentation of
   * Driver_settings class.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see settings(), Driver_settings.
   */
  virtual void set_settings(const Settings& settings) = 0;

  /**
   * @returns The driver-level settings.
   *
   * @see set_settings().
   */
  virtual const Settings& settings() const = 0;

  /// @name Measurement control
  ///
  /// @brief This API provides a way to control measurement process.
  ///
  /// @{

  /**
   * Initiates the start of measurement.
   *
   * Repeatedly calls the `data_handler` with frequency (Hz) that depends on the
   * burst size, specified in the measurement options: the greater it's value,
   * the less frequent the handler is called. When `burst_size == sample_rate`
   * the frequency is `1`.
   *
   * @warning The `data_handler` must not take more than `burst_size / sample_rate`
   * seconds of runtime! Otherwise, the driver will throttle by skipping the
   * incoming channel data and `data_handler` will be called with positive error
   * marker.
   *
   * @warning This method cannot be called from `data_handler`.
   *
   * @par Requires
   * `(data_handler &&
   *   is_initialized() &&
   *   !is_measurement_started() &&
   *   board_settings().channel_measurement_modes() &&
   *   board_settings().channel_gains() &&
   *   driver_settings().sample_rate())`.
   *
   * @par Effects
   * `is_measurement_started()`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see set_measurement_options(), set_state(), stop_measurement().
   */
  virtual void start_measurement(Data_handler data_handler) = 0;

  /**
   * @returns `true` if the measurement in progress.
   *
   * @remarks Implies is_initialized().
   *
   * @see calculate_drift_references(), calculate_drift_deltas(),
   * start_measurement().
   */
  virtual bool is_measurement_started() const noexcept = 0;

  /**
   * Stops the measurement.
   *
   * @par Effects
   * `!is_measurement_started()`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see start_measurement().
   */
  virtual void stop_measurement() = 0;

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
  /// either the references or deltas the measurement must not be in progress.
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
   * `is_initialized() && !is_measurement_started()`.
   *
   * @par Effects
   * `!is_measurement_started() && drift_references()`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see drift_references(), clear_drift_references(), calculate_drift_deltas().
   */
  virtual std::vector<float> calculate_drift_references() = 0;

  /**
   * Clears drift references if any.
   *
   * @par Requires
   * `!is_measurement_started()`.
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
  virtual void clear_drift_references() = 0;

  /**
   * @brief Calculates drift deltas based on calculated drift references.
   *
   * @par Requires
   * `is_initialized() && drift_references() && !is_measurement_started()`.
   *
   * @par Effects
   * `!is_measurement_started() && drift_deltas()`.
   * After calling the `start_measurement()`, calculated deltas will be
   * substracted from each input value of the corresponding data channel.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see drift_deltas(), calculate_drift_references(), start_measurement().
   */
  virtual std::vector<float> calculate_drift_deltas() = 0;

  /**
   * @brief Clears drift deltas if any.
   *
   * @par Requires
   * `!is_measurement_started()`.
   *
   * @par Effects
   * Input values of the corresponding channel will not be affected by deltas.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see calculate_drift_deltas(), clear_drift_references().
   */
  virtual void clear_drift_deltas() = 0;

  /**
   * @returns The calculated drift references.
   *
   * @param force Forces the reading of references from a filesystem if `true`.
   * Otherwise, the last cached value will be returned.
   *
   * @throws An Exception with code `Generic_errc::drift_comp_refs_invalid` if
   * file `<CWD>/.panda/timeswipe/drift_references` contains a junk.
   *
   * @see calculate_drift_references(), clear_drift_references(), drift_deltas().
   */
  virtual std::optional<std::vector<float>> drift_references(bool force = {}) const = 0;

  /**
   * @returns The calculated drift deltas.
   *
   * @see calculate_drift_deltas().
   */
  virtual std::optional<std::vector<float>> drift_deltas() const = 0;

  /// @}

private:
  friend detail::iDriver;

  inline static std::unique_ptr<Driver> instance_;

  Driver() = default;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_DRIVER_HPP
