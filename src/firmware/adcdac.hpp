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
protected:
  /// Proportional convertion factor k: Raw = Real*k + b.
  float m_k{};

  /// Zero offset: Raw = Real*k + b.
  float m_b{};

  /// The range of the chip in discrets(raw-binary fromat).
  int m_IntRange{}; // ADCrange

  /// The minimum range of the channel in real units (V, a, mA...).
  float m_RangeMin{};

  /// The maximum range of the channel in real units (V, a, mA...).
  float m_RangeMax{};

  /// An actual value of the channel in the raw-binary format (native chip format).
  int m_Raw{};

  /// @returns A valid raw value in range `[0, m_IntRange]`.
  int ValidRaw(const int value) const noexcept
  {
    return value < 0 ? 0 : value > m_IntRange ? m_IntRange : value;
  }

  /// @returns A valid real value in range `[m_RangeMin, m_RangeMax]`.
  float ValidReal(const float value) const noexcept
  {
    return value < m_RangeMin ? m_RangeMin : value > m_RangeMax ? m_RangeMax : value;
  }

  /**
   * @brief Conversion from raw value (native for the ADC/DAC chip or
   * board) to real units value.
   *
   * @param value The value in a raw-binary format.
   *
   * @return Real value in defined units.
   */
  float RawToReal(const int value) const noexcept
  {
    return (ValidRaw(value) - m_b) / m_k;
  }

  /**
   * @brief Conversion from real value to raw value (native for the
   * ADC/DAC chip or board).
   *
   * @param value The value in a real units.
   *
   * @return Raw-binary value.
   */
  int RealToRaw(const float value) const noexcept
  {
    return ValidReal(value) * m_k + m_b;
  }

public:
  /// The destructor.
  virtual ~CADchan() = default;

  /// The default constructor.
  CADchan() noexcept
  {
    SetRange(0, 1);
  }

  /// @returns An actual measured/controlled value in real units.
  float GetRealVal() const noexcept
  {
    return RawToReal(m_Raw);
  }

  /// @returns An actual measured/controlled value in raw-binary format
  int GetRawBinVal() // FIXME: const noexcept
  {
    return m_Raw;
  }

  /**
   * Sets the actual measured/controlled `value` in real units. If the `value`
   * is not in range `[m_RangeMin, m_RangeMax]` it will be adjusted to the
   * nearest border of that range.
   */
  void SetRealVal(const float value) noexcept
  {
    m_Raw = RealToRaw(ValidReal(value));
  }

  /// Sets the actual measured/controlled value in raw-binary format.
  void SetRawBinVal(const int value) noexcept
  {
    m_Raw = ValidRaw(value);
  }

  /**
   * @brief Gets the real value range.
   *
   * @param min Minimum range.
   * @param max Maximum range.
   */
  void GetRange(float& min, float& max) noexcept
  {
    min = m_RangeMin;
    max = m_RangeMax;
  }

  /**
   * @brief Sets the real value range.
   *
   * @param min Minimum range.
   * @param max Maximum range.
   */
  void SetRange(const float min, const float max) noexcept
  {
    const auto old_real{GetRealVal()};
    m_RangeMin = min;
    m_RangeMax = max;
    // Update the stored value according to new range.
    SetRealVal(old_real);
  }

  /// Sets the conversion factors directly.
  void SetLinearFactors(const float k, const float b) noexcept
  {
    const auto old_real{GetRealVal()};
    m_k = k;
    m_b = b;
    // Update the stored value according to new coefs.
    SetRealVal(old_real);
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
  virtual int DirectMeasure() // FIXME: const noexcept
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
protected:
  /**
   * @brief Sets the output value to the real DAC device
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
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_ADCDAC_HPP
