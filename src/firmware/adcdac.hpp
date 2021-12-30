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

#ifndef PANDA_TIMESWIPE_FIRMWARE_ADCDAC_HPP
#define PANDA_TIMESWIPE_FIRMWARE_ADCDAC_HPP

#include "../debug.hpp"
#include "../util.hpp"
using namespace panda::timeswipe::detail; // FIXME: remove

#include <cstdint>
#include <memory>
#include <utility>

/**
 * @brief Analog-to-Digital or Digital-to-Analog measurement (control) channel.
 *
 * @details `ADC` and `DAC` devices usually contains various measurement
 * (controlling) units called channels. Measured/control values are interpreted
 * as real units, such as *Volts*, *A/mA* etc.
 */
class Adcdac_channel {
public:
  /// The destructor.
  virtual ~Adcdac_channel() = default;

  /// @returns The raw content of a data register of the underlying hardware.
  virtual int GetRawBinVal() const noexcept = 0;
};

/// An ADC (Analog-to-Digital-Converter) channel.
class Adc_channel : public Adcdac_channel {};

/// A DAC (Digital-to-Analog-Converter) channel.
class Dac_channel : public Adcdac_channel {
private:
  friend class Calibratable_dac;

  /**
   * @brief Sets the raw content of a data register of the underlying hardware.
   *
   * @param raw The value that is guaranteed to be in range raw_range().
   *
   * @remarks This function is called by set_raw().
   *
   * @see set_raw().
   */
  virtual void SetRawBinVal(const int raw) = 0;

public:
  /**
   * @brief Sets the raw content of a data register of the underlying hardware.
   *
   * @param raw The value that will be narrowed to range raw_range().
   *
   * @see SetRawBinVal().
   */
  void set_raw(const int raw)
  {
    const auto [min_raw, max_raw] = raw_range();
    SetRawBinVal(clamp(raw, min_raw, max_raw));
  }

  /// @returns The defined range of the underlying hardware data register.
  virtual std::pair<int, int> raw_range() const noexcept = 0;
};

/**
 * @brief Decorator for the Dac_channel class.
 *
 * @details This class is follows the Decorator design pattern, allowing to
 * associate the DAC with the calibration data and provides the API for working
 * with the real values (Volts, Amperes etc) instead of raw values. The
 * calibration data is used to convert from real values to raw values.
 */
class Calibratable_dac final : public Dac_channel {
private:
  /// @see Dac_channel::SetRawBinVal().
  void SetRawBinVal(const int raw) override
  {
    return dac_->SetRawBinVal(raw);
  }

public:
  /**
   * @brief The constructor.
   *
   * @param dac The decoratable object.
   * @param min_value The minimum allowed real value (Volts, Amperes etc).
   * @param max_value The maximum allowed real value (Volts, Amperes etc).
   *
   * @par Requires
   * `(dac && min_value <= max_value)`.
   */
  Calibratable_dac(std::shared_ptr<Dac_channel> dac,
    const float min_value, const float max_value)
    : dac_{std::move(dac)}
    , min_value_{min_value}
    , max_value_{max_value}
  {
    PANDA_TIMESWIPE_ASSERT(dac_ && min_value_ <= max_value_);
  }

  /// @see Dac_channel::GetRawBinVal().
  int GetRawBinVal() const noexcept override
  {
    return dac_->GetRawBinVal();
  }

  /// @see Dac_channel::raw_range().
  std::pair<int, int> raw_range() const noexcept override
  {
    return dac_->raw_range();
  }

  /// @returns The stored real value.
  float value() const noexcept
  {
    return value_;
  }

  /**
   * @brief Sets the real value.
   *
   * @details Stores the real value, converts it to the raw value by using
   * linear_factors() and calls set_raw().
   *
   * @param value The value that will be narrowed to range value_range().
   */
  void set_value(float value)
  {
    value = clamp(value, min_value_, max_value_);
    set_raw(value * slope_ + offset_);
    value_ = value;
  }

  /// @returns The defined value range.
  std::pair<float, float> value_range() const noexcept
  {
    return {min_value_, max_value_};
  }

  /// @returns The defined linear factors which are used to convert value to raw.
  std::pair<float, std::int16_t> linear_factors() const noexcept
  {
    return {slope_, offset_};
  }

  /// Sets the conversion factors directly.
  void set_linear_factors(const float slope, const std::int16_t offset) noexcept
  {
    slope_ = slope;
    offset_ = offset;
    set_value(value_); // update with new slope_ and offset_
  }

private:
  std::shared_ptr<Dac_channel> dac_;
  const float min_value_{};
  const float max_value_{};
  float value_{};
  float slope_{1};
  std::int16_t offset_{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_ADCDAC_HPP
