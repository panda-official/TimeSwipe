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

/// A Timeswipe board.
class Timeswipe final {
public:
  /// An alias of the board state.
  using State = Timeswipe_state;

  /**
   * The destructor.
   *
   * @par Effects
   * Same as for Stop().
   */
  ~Timeswipe();

  /**
   * @returns The instance of this class.
   *
   * @par Effects
   * Restarts Timeswipe firmware on very first run!
   */
  static Timeswipe& instance();

  /// Non copy-constructible.
  Timeswipe(const Timeswipe&) = delete;

  /// Non copy-assignable.
  Timeswipe& operator=(const Timeswipe&) = delete;

  /// Non move-constructible.
  Timeswipe(Timeswipe&&) = delete;

  /// Non move-assignable.
  Timeswipe& operator=(Timeswipe&&) = delete;

  /// @returns Max possible sample rate.
  int get_max_sample_rate() const noexcept;

  /**
   * Set sample rate. Default value is max_sample_rate().
   *
   * @param rate - new sample rate
   *
   * @par Requires
   * `(!is_busy() && (1 <= rate && rate <= max_sample_rate())).
   *
   * @warning It's highly recommended not to use the rate for which
   * `(max_sample_rate() % rate != 0)` for best performance! In other words
   * the lower the value of `std::gcd(max_sample_rate(), rate)`, the worse
   * the performance of the resampling.
   */
  void set_sample_rate(int rate);

  /**
   * Sets the burst buffer size.
   *
   * @param size The number of records that the driver should deliver into the
   * callback.
   *
   * @see start().
   */
  void set_burst_size(std::size_t size);

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

  /**
   * An alias of a function to handle the incoming sensor data.
   *
   * @param data Portion of incoming data to consume.
   * @param error_count The number of errors (data losts).
   *
   * @see start().
   */
  using Sensor_data_handler = std::function<void(Sensors_data data, int error_count)>;

  /**
   * Initiates the start of measurement.
   *
   * @par Effects
   * Repeatedly calls `handler`. The call frequency of the handler is depends on
   * the burst_size() - the greater it's value, the less frequent `handler` is
   * called. If the `burst_size() == sample_rate()` then the `handler` is called
   * `1` time per second.
   *
   * @warning The `handler` **must** spend no more than `burst_size() / sample_rate()`
   * seconds on processing the incoming data! Otherwise, the driver will throttle
   * and some the sensor data will be skipped.
   *
   * @warning This method cannot be called from `handler`!
   *
   * @par Effects
   * `is_busy()`.
   */
  void start(Sensor_data_handler handler);

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
   */
  void stop();

  /**
   * An alias of a function to handle the incoming events.
   *
   * @see start().
   */
  using Event_handler = std::function<void(Event&&)>;

  /**
   * Sets the event handler.
   *
   * @par Requires
   * `!is_busy()`.
   */
  void set_event_handler(Event_handler&& handler);

  /**
   * An alias of a function to handle sensor data read errors.
   *
   * @see start().
   */
  using Error_handler = std::function<void(std::uint64_t)>;

  /**
   * Sets the error handler.
   *
   * @par Requires
   * `!is_busy()`.
   */
  void set_error_handler(Error_handler&& handler);

  /**
   * Sets the board state.
   *
   * @returns The actual board state after applying the given `state`.
   */
  void set_state(const State& state);

  /**
   * Gets the board state.
   *
   * @returns The actual board state.
   */
  const State& get_state() const;

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
  inline static std::unique_ptr<Timeswipe> instance_;

  Timeswipe();
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_TIMESWIPE_HPP
