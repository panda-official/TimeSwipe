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

#ifndef PANDA_TIMESWIPE_FIRMWARE_DMS_CHANNEL_HPP
#define PANDA_TIMESWIPE_FIRMWARE_DMS_CHANNEL_HPP

#include "channel.hpp"
#include "pga280.hpp"
#include "shiftreg.hpp"

/// The DMS measurement channel.
class Dms_channel final : public Channel {
public:
  /// The constructor.
  Dms_channel(const int channel_index,
    const std::shared_ptr<Adc_channel>& adc,
    const std::shared_ptr<Dac_channel>& dac,
    const CView::vischan visualization_index,
    const std::shared_ptr<Pin>& iepe_switch_pin,
    const std::shared_ptr<CPGA280>& pga,
    const bool is_visualization_enabled)
    : channel_index_{channel_index}
    , visualization_index_{visualization_index}
    , is_visualization_enabled_{is_visualization_enabled}
    , adc_{adc}
    , dac_{dac}
    , iepe_switch_pin_{iepe_switch_pin}
    , pga_{pga}
    {}

  /// @see Channel::measurement_mode();
  std::optional<Measurement_mode> measurement_mode() const noexcept override
  {
    return measurement_mode_;
  }

  /// @see Channel::set_measurement_mode();
  Error set_measurement_mode(Measurement_mode mode) override;

  /// @see Channel::is_iepe();
  bool is_iepe() const noexcept override
  {
    return is_iepe_;
  }

  /// @see Channel::set_iepe();
  Error set_iepe(bool value) override;

  /// @see Channel::amplification_gain();
  std::optional<float> amplification_gain() const noexcept override
  {
    return amplification_gain_;
  }

  /// @see Channel::set_amplification_gain();
  Error set_amplification_gain(float GainValue) override;

  int channel_index() const noexcept override
  {
    return channel_index_;
  }

  /// @see Channel::visualization_index();
  const CDataVis& visualization_index() const noexcept override
  {
    return visualization_index_;
  }

  /// @overload
  CDataVis& visualization_index() noexcept override
  {
    return visualization_index_;
  }

  /// @see Channel::is_visualization_enabled();
  bool is_visualization_enabled() const noexcept override
  {
    return is_visualization_enabled_;
  }

  /// @see Channel::adc();
  std::shared_ptr<const Adc_channel> adc() const noexcept override
  {
    return adc_;
  }

  /// @overload
  std::shared_ptr<Adc_channel> adc() noexcept
  {
    return adc_;
  }

  /// @see Channel::dac();
  std::shared_ptr<const Dac_channel> dac() const noexcept
  {
    return dac_;
  }

  /// @overload
  std::shared_ptr<Dac_channel> dac() noexcept
  {
    return dac_;
  }

  /// @see Channel::update_offsets();
  void update_offsets() override;

private:
  bool is_iepe_{};
  std::optional<Measurement_mode> measurement_mode_;
  std::optional<float> amplification_gain_;
  std::size_t gain_index_{};
  int channel_index_{-1};

  CDataVis visualization_index_;
  bool is_visualization_enabled_{};

  std::shared_ptr<Adc_channel> adc_;
  std::shared_ptr<Dac_channel> dac_;

  std::shared_ptr<Pin> iepe_switch_pin_;
  std::shared_ptr<CPGA280> pga_;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_DMS_CHANNEL_HPP
