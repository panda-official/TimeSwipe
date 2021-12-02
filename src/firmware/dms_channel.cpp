/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "control/nodeControl.h"
#include "dms_channel.hpp"
#include "../gain.hpp"
#include "../hat.hpp"

void CDMSchannel::set_amplification_gain(const float value)
{
  const auto index = gain::ogain_table_index(value);
  const auto igain = static_cast<CPGA280::igain>(index / 2);
  const auto ogain = static_cast<CPGA280::ogain>(index % 2);
  if (pga_->SetGains(igain, ogain)) {
    gain_index_ = index;
    amplification_gain_ = gain::ogain_table[index];
    update_offsets();
  }
}

void CDMSchannel::update_offsets()
{
  // Apply offsets iif calibration is enabled.
  if (!node_control()->IsCalEnabled()) return;

  std::string strError;
  hat::Calibration_map cmap;
  node_control()->GetCalibrationData(cmap, strError);

  using Type = hat::atom::Calibration::Type;
  const auto atom = Measurement_mode::voltage == measurement_mode() ?
    Type::v_in1 : Type::c_in1;
  const auto type = static_cast<Type>(static_cast<int>(atom) + channel_index());
  const auto& entry = cmap.atom(type).entry(gain_index_);
  dac()->SetRawOutput(entry.offset());
}
