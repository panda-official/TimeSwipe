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

#include "error.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <iostream> // included here because std::cerr can be used by
                    // TimeSwipe::TimeSwipe() and the instance of TimeSwipe
                    // can be defined as a static object
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
} // namespace panda::timeswipe::driver

/**
 * \brief Sensors container
 */
class SensorsData final {
  static constexpr std::size_t SENSORS = 4;
  using CONTAINER = std::array<std::vector<float>, SENSORS>;
public:
  using Value = std::vector<float>;

  /**
   * \brief Get number of sensors
   *
   *
   * @return number of sensors
   */
  static constexpr std::size_t SensorsSize() noexcept
  {
    static_assert(SENSORS > 0);
    return SENSORS;
  }

  /**
   * \brief Get number of data entries
   *
   *
   * @return number of data entries each sensor has
   */
  std::size_t DataSize() const noexcept;

  /**
   * \brief Access sensor data
   *
   * @param num - sensor number. Valid values from 0 to @ref SensorsSize-1
   *
   * @return number of data entries each sensor has
   */
  std::vector<float>& operator[](std::size_t num) noexcept;

  /// @overload
  const Value& operator[](const std::size_t index) const noexcept
  {
    return data_[index];
  }

  CONTAINER& data();
  void reserve(std::size_t num);
  void resize(std::size_t new_size);
  void clear() noexcept;
  bool empty() const noexcept;
  void append(const SensorsData& other);
  void append(const SensorsData& other, std::size_t count);
  void erase_front(std::size_t count) noexcept;
  void erase_back(std::size_t count) noexcept;

  /// @name Iterators
  /// @{

  /// @returns Iterator that points to a first channel.
  auto begin() noexcept
  {
    return data_.begin();
  }

  /// @returns Constant iterator that points to a first channel.
  auto begin() const noexcept
  {
    return data_.begin();
  }

  /// @returns Constant iterator that points to a first channel.
  auto cbegin() const noexcept
  {
    return data_.cbegin();
  }

  /// @returns Iterator that points to an one-past-the-last channel.
  auto end() noexcept
  {
    return data_.end();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto end() const noexcept
  {
    return data_.end();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto cend() const noexcept
  {
    return data_.cend();
  }

  /// @}
private:
  CONTAINER data_;
};

class TimeSwipeEventImpl;

/**
 * TimeSwipe events
 */
class TimeSwipeEvent final {
public:

  /**
   * \brief Button press event
   */
  class Button {
  public:
    Button() = default;
    Button(bool _pressed, unsigned _count);
    /**
     * \brief returns true when pressed and false if released
     */
    bool pressed() const;
    /**
     * \brief returns press/release counter,
     * odd value is pressed, even value is released
     */
    unsigned count() const;
  private:
    bool _pressed;
    unsigned _count;
  };

  /**
   * \brief Gain value event
   */
  class Gain {
  public:
    Gain() = default;
    Gain(int _value);
    /**
     * \brief returns Gain value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief SetSecondary value event
   */
  class SetSecondary {
  public:
    SetSecondary() = default;
    SetSecondary(int _value);
    /**
     * \brief returns SetSecondary value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Bridge value event
   */
  class Bridge {
  public:
    Bridge() = default;
    Bridge(int _value);
    /**
     * \brief returns Bridge value as number
     */
    int value() const;
  private:
    int _value;
  };


  /**
   * \brief Record value event
   */
  class Record {
  public:
    Record() = default;
    Record(int _value);
    /**
     * \brief returns Record value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Offset value event
   */
  class Offset {
  public:
    Offset() = default;
    Offset(int _value);
    /**
     * \brief returns Offset value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Mode value event
   */
  class Mode {
  public:
    Mode() = default;
    Mode(int _value);
    /**
     * \brief returns Mode value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Check for interested event
   */
  template <class EVENT>
  bool is() const;

  /**
   * \brief get interested event
   */
  template <class EVENT>
  const EVENT& get() const;

  TimeSwipeEvent();
  ~TimeSwipeEvent();

  template <class EVENT>
  TimeSwipeEvent(EVENT&& ev);

  template <class EVENT>
  TimeSwipeEvent(const EVENT& ev);

  TimeSwipeEvent(TimeSwipeEvent&& ev) = default;
  TimeSwipeEvent(const TimeSwipeEvent& ev) = default;
  TimeSwipeEvent& operator=(const TimeSwipeEvent&) = default;
private:
  std::shared_ptr<TimeSwipeEventImpl> impl_;
};


class TimeSwipeImpl;

/**
 * TimeSwipe interface for Sensor
 */
class TimeSwipe final {
public:
  TimeSwipe();
  ~TimeSwipe();
  TimeSwipe(const TimeSwipe&) = delete;

  /** @enum TimeSwipe::Mode
   *
   * \brief Input mode
   *
   */
  enum class Mode {
    Primary,
    Norm,
    Digital
  };

  /**
   * @returns The value of type `Mode` converted from `value`.
   *
   * @throws `std::invalid_argument` if `value` doesn't corresponds to any
   * member of Mode.
   */
  static Mode ToMode(const std::string_view value)
  {
    if (value == "primary") return TimeSwipe::Mode::Primary;
    else if (value == "norm") return TimeSwipe::Mode::Norm;
    else if (value == "digital") return TimeSwipe::Mode::Digital;
    else throw std::invalid_argument{"invalid text representation of TimeSwipe::Mode"};
  }

  /**
   * @returns The value of type `std::string_view` converted from `value`.
   *
   * @throws `std::invalid_argument` if `value` doesn't corresponds to any
   * member of Mode.
   */
  static std::string_view ToStringView(const Mode value)
  {
    switch (value) {
    case Mode::Primary: return "primary";
    case Mode::Norm: return "norm";
    case Mode::Digital: return "digital";
    }
    throw std::invalid_argument{"invalid value of TimeSwipe::Mode"};
  }

  /** @enum TimeSwipe::Channels
   *
   * \brief Board channel index
   *
   */
  enum class Channel : int {
    CH1,
    CH2,
    CH3,
    CH4
  };

  /** @enum TimeSwipe::ChannelMesMode
   *
   * @brief The channel measurement mode
   *
   */
  enum class ChannelMesMode : int {
    Voltage,
    Current
  };


  /**
   * \brief Setup hardware mode
   *
   * @param number - one of @ref Mode
   */
  void SetMode(Mode number);

  /**
   * \brief Get current hardware mode
   *
   * @return current mode
   */
  Mode GetMode() const noexcept;

  /**
   * \brief Setup Sensor offsets
   *
   * Default offsets are all 0
   *
   * @param offset1
   * @param offset2
   * @param offset3
   * @param offset4
   */
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
   */
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
   */
  void SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4);

  /**
   * \brief Start PWM generator
   * Method can be called in any time.
   *
   * @param num - output number - possible values are 0 or 1
   * @param frequency - periods per second - possible values between 1 and 1000
   * @param high - PWM signal high value - possible values are 0..4095 high >= low, default is 4095
   * @param low - PWM signal low value - possible values are 0..4095 low <= high, default is 0
   * @param repeats - number of periods to repeat. PWM generator will work (repeats/frequency) seconds
   *                  after repeats number get exhausted PWM goes to stop state and StartPWM can be called again
   *                  0 is for unlimited repeats (default)
   * @param duty_cycle - part of PWM period when signal is in high state. 0.001 duty_cycle <= 0.999. default value is 0.5
   *
   * @return false if at least one wrong parameter given or generator already in start state
   */
  bool StartPWM(std::uint8_t num, std::uint32_t frequency,
    std::uint32_t high = 4095, std::uint32_t low = 0,
    std::uint32_t repeats = 0, float duty_cycle = 0.5);

  /**
   * \brief Stop PWM generator
   * Method can be called in any time.
   *
   * @param numb - output number - possible values 0 or 1
   *
   * @return false if at least wrong parameter given or generator already in stop state
   */
  bool StopPWM(std::uint8_t num);

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
   * @param[in] nCh - the channel to be set
   * @param[in] nMode - the measurement mode: Voltage or Current
   *
   * @return true on operation success, false otherwise
   */
  bool SetChannelMode(Channel nCh, ChannelMesMode nMode);


  /**
   * @brief Requests the channel measurement mode
   * @param[in] nCh - the channel to be requested
   * @param[out] nMode - the actual measurement mode: Voltage or Current
   *
   * @return true on operation success, false otherwise
   */
  bool GetChannelMode(Channel nCh, ChannelMesMode &nMode);


  /**
   * @brief Sets the channel gain value
   * @details Sets the gain value in the range [1/8 : 128*1.375].
   *  If the value doesn't correspond to available gain it will be fitted to the closest available gain
   *
   * @param[in] nCh - the channel to be set
   * @param[in] Gain - the gain value to be set
   *
   * @return true on operation success, false otherwise
   */
  bool SetChannelGain(Channel nCh, float Gain);


  /**
   * @brief Requests the channel gain value
   *
   * @param[in] nCh - the channel to be requested
   * @param[out] Gain - the actual gain of the channel
   *
   * @return true on operation success, false otherwise
   */
  bool GetChannelGain(Channel nCh, float &Gain);


  /**
   * @brief Switches channel IEPE mode ON/OFF
   *
   * @param[in] nCh - the channel to be set
   * @param[in] bIEPEon - the state of the IEPE switch to be set
   *
   * @return true on operation success, false otherwise
   */
  bool SetChannelIEPE(Channel nCh, bool bIEPEon);


  /**
   * @brief Requests the channel IEPE mode
   *
   * @param[in] nCh - the channel to be requested
   * @param[out] bIEPEon - the actual state of the IEPE switch
   *
   * @return true on operation success, false otherwise
   */
  bool GetChannelIEPE(Channel nCh, bool &bIEPEon);

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
   * `<CWD>/.pandagmbh/timeswipe/drift_reference` for persistent storage until
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
   * `<CWD>/.pandagmbh/timeswipe/drift_references`.
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
   * file `<CWD>/.pandagmbh/timeswipe/drift_references` contains a junk.
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
  using ReadCallback = std::function<void(SensorsData, std::uint64_t errors)>;

  /**
   * \brief Start reading Sensor loop
   *
   * Only one instance of @ref TimeSwipe can be running each moment of the time
   *
   * After each sensor read complete cb called with vector of @ref SensorsData
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

  using OnEventCallback = std::function<void(TimeSwipeEvent&& event)>;
  /**
   * \brief Register callback for event
   *
   * onEvent must be called before @ref Start called, otherwise register fails
   *
   * @param cb callback called with event received
   * @return false if register callback failed, true otherwise
   */
  bool onEvent(OnEventCallback cb);

  using OnErrorCallback = std::function<void(std::uint64_t)>;
  /**
   * \brief Register error callback
   *
   * onError must be called before @ref Start called, otherwise register fails
   *
   * @param cb callback called once read error occurred
   * @return false if register callback failed, true otherwise
   */
  bool onError(OnErrorCallback cb);

  /**
   * \brief Stop reading Sensor loop
   *
   * @return true is stop succeeded, false otherwise
   */
  bool Stop();

  /*!
   * \brief TraceSPI
   * \param val true=on
   */
  void TraceSPI(bool val);

  inline static bool resample_log;
private:
  std::unique_ptr<TimeSwipeImpl> impl_;
};

#endif  // PANDA_TIMESWIPE_DRIVER_TIMESWIPE_HPP
