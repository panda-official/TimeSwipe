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

/**
 * @brief Analog-to-Digital or Digital-to-Analog measurement/control channel.
 *
 * `ADC` and `DAC` devices usually contain a number of measurement/controlling
 * units called channels. Measured/control values are stored in real units, such
 * as *Volts*, *A/mA* etc. The range of the channel is also in real units, for
 * example, `[-10, +10]` Volts.
 */
class CADchan {
public:
  /// The destructor.
  virtual ~CADchan() = default;

  /// @returns The raw value.
  int GetRawBinVal() const noexcept
  {
    return raw_;
  }

  /**
   * @brief Sets the raw value (native for the ADC/DAC chip or board).
   *
   * @details If the `value` is out of range `[min_raw_, max_raw_]` it will
   * be narrowed by the edge of that range.
   */
  void SetRawBinVal(const int value) noexcept
  {
    raw_ = valid_raw(value);
  }

  /// @returns The result of conversion from GetRawBinVal() to a value in real units.
  float GetRealVal() const noexcept
  {
    return (raw_ - offset_) / slope_;
  }

  /**
   * @brief Sets the `value` in real units.
   *
   * @details `raw = value*k + b`.
   */
  void SetRealVal(const float value) noexcept
  {
    SetRawBinVal(value * slope_ + offset_);
  }

  /// Sets the conversion factors directly.
  void SetLinearFactors(const float slope, const float offset) noexcept
  {
    const auto real = GetRealVal();
    slope_ = slope;
    offset_ = offset;
    // Update the raw value by using current real and the new coefs.
    SetRealVal(real);
  }

protected:
  void set_raw_range(const int min_raw, const int max_raw)
  {
    min_raw_ = min_raw;
    max_raw_ = max_raw;
  }

private:
  /// Proportional convertion factor.
  float slope_{1};

  /// Zero offset.
  float offset_{};

  /// An actual value of the channel in the raw-binary format (native chip format).
  int raw_{};

  /// The minumum raw value of the chip in discrets (raw-binary fromat).
  int min_raw_{};

  /// The maximum raw value of the chip in discrets (raw-binary fromat).
  int max_raw_{};

  /// @returns A valid raw value in range `[0, max_raw_]`.
  int valid_raw(const int value) const noexcept
  {
    return value < min_raw_ ? min_raw_ : value > max_raw_ ? max_raw_ : value;
  }
};

// -----------------------------------------------------------------------------
// Class CAdc
// -----------------------------------------------------------------------------

/**
 * @brief An ADC (Analog-to-Digital-Converter) channel.
 *
 * @remarks Uses only ADC functionality from CADchan.
 */
class CAdc : public CADchan {
public:
  /**
   * @brief Force direct measurement for this channel on ADC device without queuing.
   *
   * @returns Immediately measured analog value in raw-binary format.
   */
  virtual int DirectMeasure() const noexcept
  {
    return GetRawBinVal();
  }
};

// -----------------------------------------------------------------------------
// Class CDac
// -----------------------------------------------------------------------------

/**
 * @brief A DAC (Digital-to-Analog-Converter) channel.
 *
 * @remarks Uses only DAC functionality of CADchan.
 */
class CDac : public CADchan {
public:
  /// Set the current control value for this channel.
  void SetVal() noexcept
  {
    DriverSetVal(GetRealVal(), GetRawBinVal());
  }

  /**
   * @brief Set control value in a real unit format for this channel.
   *
   * @param value A value in a real unit format
   */
  void SetVal(const float value) noexcept
  {
    SetRealVal(value);
    DriverSetVal(GetRealVal(), GetRawBinVal());
  }

  /**
   * @brief Set control value in a raw binary format for this channel.
   *
   * @param value A value in a raw binary format.
   */
  void SetRawOutput(const int value) noexcept
  {
    SetRawBinVal(value);
    DriverSetVal(GetRealVal(), GetRawBinVal());
  }

private:
  /**
   * @brief Sets the output value to the real DAC device.
   *
   * The function is used to transfer a control value from the abstract DAC
   * channel to the real DAC device and must be overriden in a real device
   * control class.
   *
   * @param val A value to set in a real-unit format for devices that can accept
   * it (some PCI boards for example).
   * @param out_bin A value to set in a raw-binary format - most common format
   * for DAC devices.
   */
  virtual void DriverSetVal(float val, int out_bin) = 0;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_ADCDAC_HPP
