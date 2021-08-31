// -*- C++ -*-

// PANDA TimeSwipe Project
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

#include "event.hpp"
#include "sensor_data.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe::driver {

/// A driver version.
struct Version final {
  int major{};
  int minor{};
  int patch{};
};

/// @returns The driver version.
Version version() noexcept;

/**
 * TimeSwipe interface for Sensor
 */
class TimeSwipe final {
public:
  /**
   * The destructor.
   *
   * @par Effects
   * Same as for Stop().
   */
  ~TimeSwipe();

  /**
   * @returns The instance of this class.
   *
   * @par Effects
   * Restarts TimeSwipe firmware on very first run!
   */
  static TimeSwipe& instance();

  /// Non move-constructible.
  TimeSwipe(TimeSwipe&&) = delete;

  /// Non move-assignable.
  TimeSwipe& operator=(TimeSwipe&&) = delete;

  /// Non copy-constructible.
  TimeSwipe(const TimeSwipe&) = delete;

  /// Non copy-assignable.
  TimeSwipe& operator=(const TimeSwipe&) = delete;

  /// Input mode.
  enum class Mode {
    /// IEPE.
    iepe,
    /// Normal signal.
    normal,
    /// Digital.
    digital
  };

  /**
   * @returns The value of type `Mode` converted from `value`.
   *
   * @throws `std::invalid_argument` if `value` doesn't corresponds to any
   * member of Mode.
   */
  static Mode to_mode(const std::string_view value);

  /**
   * @returns The value of type `std::string_view` converted from `value`.
   *
   * @throws `std::invalid_argument` if `value` doesn't corresponds to any
   * member of Mode.
   */
  static std::string_view to_string_view(const Mode value);

  /// Measurement mode.
  enum class Measurement_mode {
    Voltage,
    Current
  };

  /// Sets the input mode.
  void set_mode(Mode mode);

  /// @returns Input mode.
  Mode mode() const noexcept;

  /**
   * \brief Setup Sensor offsets
   *
   * Default offsets are all 0
   *
   * @param offset1
   * @param offset2
   * @param offset3
   * @param offset4
   *
   * @warning THIS METHOD WILL BE REMOVED!
   */
  [[deprecated]]
  void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);

  /**
   * \brief Setup Sensor gains
   *
   * It is mandatory to setup gains before @ref Start
   *
   * @param gain1
   * @param gain2
   * @param gain3
   * @param gain4
   *
   * @warning THIS METHOD WILL BE REMOVED!
   */
  [[deprecated]]
  void SetSensorGains(float gain1, float gain2, float gain3, float gain4);

  /**
   * \brief Setup Sensor transmissions
   *
   * It is mandatory to setup transmissions before @ref Start
   *
   * @param trans1
   * @param trans2
   * @param trans3
   * @param trans4
   *
   * @warning THIS METHOD WILL BE REPLACED!
   */
  [[deprecated]]
  void SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4);

  /**
   * Starts the PWM generator.
   *
   * This method can be called even after the call of start().
   *
   * @param index PWM index. Must be in range `[0, 1]`.
   * @param frequency Periods per second. Must be in range `[1, 1000]`.
   * @param low PWM signal low value. Must be in range `[0, 4095]`.
   * @param high PWM signal high value. Must be in range `[0, 4095]`.
   * @param repeat_count The number of repeat periods. PWM generator will work
   * `(repeat_count / frequency)` seconds and after that goes to stop state and
   * this method can be called again. The value of `0` means infinity.
   * @param duty_cycle The length of the PWM period when signal is in high state.
   * Must be in range `(0, 1)`.
   *
   * @par Requires
   * `(0 <= index && index <= 1) && (1 <= frequency && frequency <= 1000) &&
   * (0 <= low && low <= 4095) && (0 <= high && high <= 4095) && (low <= high) &&
   * (repeat_count >= 0) && (0 < duty_cycle && duty_cycle < 1)`.
   *
   * @return false if at least one wrong parameter given or generator already in start state
   */
  bool start_pwm(int index, int frequency,
    int low = 0, int high = 4095, int repeat_count = 0, float duty_cycle = 0.5);

  /**
   * Stops the PWM generator.
   *
   * This method can be called even after the call of start().
   *
   * @param index PWM index. Must be in range `[0, 1]`.
   *
   * @return false if at least wrong parameter given or generator already in stop state
   */
  bool stop_pwm(int index);

  /**
   * \brief Get PWM generator state if it is in a Start state
   * Method can be called in any time.
   *
   * @param[in] num - output number - possible values 0 or 1
   * @param[out] active - pwm active flag, if active is false other parameters can not be considered valid
   * other parameters are output references to paramteres with same names in @ref StartPWM
   * they are valid only if true returned
   *
   * @return false if num parameter is wrong or generator is in stop state
   */

  bool GetPWM(std::uint8_t num, bool& active,
    std::uint32_t& frequency, std::uint32_t& high,
    std::uint32_t& low, std::uint32_t& repeats, float& duty_cycle);


  /**
   * @brief Sets the channel measurement mode: Voltage or Current
   * @param[in] channel The channel to be set
   * @param[in] nMode - the measurement mode: Voltage or Current
   *
   * @return true on operation success, false otherwise
   */
  bool SetChannelMode(int channel, Measurement_mode nMode);


  /**
   * @brief Requests the channel measurement mode
   * @param[in] nCh - the channel to be requested
   * @param[out] nMode - the actual measurement mode: Voltage or Current
   *
   * @return true on operation success, false otherwise
   */
  bool GetChannelMode(int channel, Measurement_mode &nMode);


  /**
   * @brief Sets the channel gain value
   * @details Sets the gain value in the range [1/8 : 128*1.375].
   *  If the value doesn't correspond to available gain it will be fitted to the closest available gain
   *
   * @param[in] channel The channel to be set
   * @param[in] Gain - the gain value to be set
   *
   * @return true on operation success, false otherwise
   */
  bool SetChannelGain(int channel, float Gain);


  /**
   * @brief Requests the channel gain value
   *
   * @param[in] channel The channel to be requested
   * @param[out] Gain - the actual gain of the channel
   *
   * @return true on operation success, false otherwise
   */
  bool GetChannelGain(int channel, float &Gain);


  /**
   * @brief Switches channel IEPE mode ON/OFF
   *
   * @param[in] channel The channel to be set
   * @param[in] bIEPEon - the state of the IEPE switch to be set
   *
   * @return true on operation success, false otherwise
   */
  bool SetChannelIEPE(int channel, bool bIEPEon);


  /**
   * @brief Requests the channel IEPE mode
   *
   * @param[in] channel The channel to be requested
   * @param[out] bIEPEon - the actual state of the IEPE switch
   *
   * @return true on operation success, false otherwise
   */
  bool GetChannelIEPE(int channel, bool &bIEPEon);

  /// @returns Max possible sample rate.
  int MaxSampleRate() const noexcept;

  /**
   * \brief Setup Burst buffer size
   *
   * This method notifies the driver to return at least burstNum records to the cb of @ref Start function per each call
   *
   * @param burstNum - number of records in burst buffer
   */
  void SetBurstSize(std::size_t burstNum);

  /**
   * @brief Set sample rate. Default value is MaxSampleRate().
   *
   * @param rate - new sample rate
   * @return false on wrong rate value requested
   *
   * @par Requires
   * `(!IsBusy() && (1 <= rate && rate <= MaxSampleRate())).
   *
   * @warning It's highly recommended not to use the rate for which
   * `(MaxSampleRate() % rate != 0)` for best performance. In other words
   * the lower the value of `std::gcd(MaxSampleRate(), rate)`, the worse
   * the performance of the underlying resampler.
   */
  bool SetSampleRate(int rate);

  /// @name Drift Compensation
  ///
  /// @brief This API provides a way to compensate the long term drift of the
  /// measurement hardware when making long term measurements with the TimeSwipe
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
   * @brief Calculates drift references.
   *
   * The calculated references are stored to
   * `<CWD>/.panda/timeswipe/drift_reference` for persistent storage until
   * either it deleted directly or by calling ClearDriftReferences().
   *
   * @par Requires
   * `!IsBusy()`.
   *
   * @par Effects
   * `!IsBusy() && DriftReferences()`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see DriftReferences(), ClearDriftReferences(), CalculateDriftDeltas().
   */
  std::vector<float> CalculateDriftReferences();

  /**
   * @brief Clears drift references if any.
   *
   * @par Requires
   * `!IsBusy()`.
   *
   * @par Effects
   * `!DriftReferences() && !DriftDeltas()`. Removes the file
   * `<CWD>/.panda/timeswipe/drift_references`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see CalculateDriftReferences(), ClearDriftDeltas().
   */
  void ClearDriftReferences();

  /**
   * @brief Calculates drift deltas based on calculated drift references.
   *
   * @par Requires
   * `DriftReferences() && !IsBusy()`.
   *
   * @par Effects
   * `!IsBusy() && DriftDeltas()`.
   * After calling the `Start()`, calculated deltas will be substracted from
   * each input value of the corresponding channel.
   *
   * @remarks Blocks the current thread for a while (~5ms).
   *
   * @see DriftDeltas(), CalculateDriftReferences(), Start().
   */
  std::vector<float> CalculateDriftDeltas();

  /**
   * @brief Clears drift deltas if any.
   *
   * @par Requires
   * `!IsBusy()`.
   *
   * @par Effects
   * Input values of the corresponding channel will not be affected by deltas.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see CalculateDriftDeltas(), ClearDriftReferences().
   */
  void ClearDriftDeltas();

  /**
   * @returns The calculated drift references.
   *
   * @param force Forces the reading of references from a filesystem if `true`.
   * Otherwise, the last cached value will be returned.
   *
   * @throws An Exception with the code `Errc::kInvalidDriftReference` if
   * file `<CWD>/.panda/timeswipe/drift_references` contains a junk.
   *
   * @see CalculateDriftReferences(), ClearDriftReferences(), DriftDeltas().
   */
  std::optional<std::vector<float>> DriftReferences(bool force = {}) const;

  /**
   * @returns The calculated drift deltas.
   *
   * @see CalculateDriftDeltas().
   */
  std::optional<std::vector<float>> DriftDeltas() const;

  /// @}

  /**
   * \brief Read sensors callback function pointer
   */
  using ReadCallback = std::function<void(Sensors_data, std::uint64_t errors)>;

  /**
   * \brief Start reading Sensor loop
   *
   * Only one instance of @ref TimeSwipe can be running each moment of the time
   *
   * After each sensor read complete cb called with vector of @ref Sensors_data
   *
   * Buffer is for 1 second data if \p cb works longer than 1 second, next data can be loosed and next callback called with non-zero errors
   *
   * Function starts two threads: one thread reads sensor values to the ring buffer, second thread polls ring buffer and calls @ref cb
   *
   * Function can not be called from callback
   *
   * @param cb
   * @return false if reading procedure start failed, otherwise true
   *
   * @par Effects
   * `IsBusy()`.
   */
  bool Start(ReadCallback cb);

  /**
   * @returns `true` if this instance is busy on read the sensor input.
   *
   * @see CalculateDriftReferences(), CalculateDriftDeltas(), Start().
   */
  bool IsBusy() const noexcept;

  /**
   * \brief Send SPI SetSettings request and receive the answer
   *
   * @param request - request json string
   * @param error - output error
   * @return json-formatted answer, set error if error occured
   */
  std::string SetSettings(const std::string& request, std::string& error);

  /**
   * \brief Send SPI GetSettings request and receive the answer
   *
   * @param request - request json string
   * @param error - output error
   * @return json-formatted answer, set error if error occured
   */
  std::string GetSettings(const std::string& request, std::string& error);

  using OnEventCallback = std::function<void(Event&& event)>;
  /**
   * \brief Register callback for event
   *
   * OnEvent must be called before @ref Start called, otherwise register fails
   *
   * @param cb callback called with event received
   * @return false if register callback failed, true otherwise
   */
  bool OnEvent(OnEventCallback cb);

  using OnErrorCallback = std::function<void(std::uint64_t)>;
  /**
   * \brief Register error callback
   *
   * OnError must be called before @ref Start called, otherwise register fails
   *
   * @param cb callback called once read error occurred
   * @return false if register callback failed, true otherwise
   */
  bool OnError(OnErrorCallback cb);

  /**
   * \brief Stop reading Sensor loop
   *
   * @return true is stop succeeded, false otherwise
   */
  bool Stop();

  /// Enables or disables SPI command tracing by using standard error output stream.
  void TraceSPI(bool val);

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;
  inline static std::unique_ptr<TimeSwipe> instance_;

  TimeSwipe();
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_TIMESWIPE_HPP
