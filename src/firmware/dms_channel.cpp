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

#include "../gain.hpp"
#include "../hat.hpp"
#include "board.hpp"
#include "dms_channel.hpp"

Error Dms_channel::set_measurement_mode(const Measurement_mode mode)
{
  if (Board::instance().is_channels_adc_enabled())
    return Error{Errc::board_measurement_started,
      "cannot set channel measurement mode when measurement started"};

  measurement_mode_ = mode;
  pga_->SetMode(static_cast<CPGA280::mode>(mode));
  update_offsets();
  return {};
}

Error Dms_channel::set_iepe(const bool value)
{
  if (Board::instance().is_channels_adc_enabled())
    return Error{Errc::board_measurement_started,
      "cannot set channel IEPE mode when measurement started"};

  is_iepe_ = value;
  iepe_switch_pin_->write(value);
  return {};
}

Error Dms_channel::set_amplification_gain(const float value)
{
  if (Board::instance().is_channels_adc_enabled())
    return Error{Errc::board_measurement_started,
      "cannot set channel gain when measurement started"};

  const auto index = gain::ogain_table_index(value);
  const auto igain = static_cast<CPGA280::igain>(index / 2);
  const auto ogain = static_cast<CPGA280::ogain>(index % 2);
  if (pga_->SetGains(igain, ogain)) {
    gain_index_ = index;
    amplification_gain_ = gain::ogain_table[index];
    update_offsets();
  }
  return {};
}

void Dms_channel::update_offsets()
{
  int raw{2048};
  if (board()->is_calibration_data_enabled()) {
    if (const auto [err, map] = board()->calibration_data(); !err) {
      using Ct = hat::atom::Calibration::Type;
      const auto atom = Measurement_mode::voltage == measurement_mode() ?
        Ct::v_in1 : Ct::c_in1;
      const auto type = static_cast<Ct>(static_cast<int>(atom) + channel_index());
      raw = map.atom(type).entry(gain_index_).offset();
    }
  }
  dac()->set_raw(raw);
}
