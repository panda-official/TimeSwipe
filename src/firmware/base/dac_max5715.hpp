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

#ifndef PANDA_TIMESWIPE_FIRMWARE_BASE_DAC_MAX5715_HPP
#define PANDA_TIMESWIPE_FIRMWARE_BASE_DAC_MAX5715_HPP

#include "../../spi.hpp"
#include "../adcdac.hpp"
#include "../pin.hpp"

/// MAX5715 DAC's channel abstraction.
class Dac_max5715 final : public Dac_channel {
public:
  /// MAX5715 channel.
  enum class Channel {
    /// Channel A (0).
    a,
    /// Channel B (1).
    b,
    /// Channel C (2).
    c,
    /// Channel D (3).
    d
  };

  /**
   * @brief The constructor.
   * @param min_raw The value in range `[0, 4095]` (per datasheet).
   * @param max_raw The value in range `[0, 4095]` (per datasheet).
   *
   * @par Requires
   * `(min_raw <= max_raw)`.
   */
  Dac_max5715(CSPI* spi_bus, std::shared_ptr<Pin> pin, Channel channel,
    const int min_raw, const int max_raw);

  /// @see Adcdac_channel::GetRawBinVal().
  int GetRawBinVal() const noexcept override
  {
    return raw_;
  }

  /// @see Dac_channel::SetRawBinVal().
  void SetRawBinVal(int raw) override;

  /// @see Dac_channel::raw_range().
  std::pair<int, int> raw_range() const noexcept override
  {
    return {min_raw_, max_raw_};
  }

private:
  CSPI* spi_bus_{};
  std::shared_ptr<Pin> pin_;
  Channel channel_{};
  const int min_raw_{};
  const int max_raw_{};
  int raw_{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_BASE_DAC_MAX5715_HPP
