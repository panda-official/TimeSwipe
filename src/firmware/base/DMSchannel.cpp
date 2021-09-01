/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "../../common/gain.hpp"
#include "../../common/hat.hpp"
#include "base/DMSchannel.h"
#include "control/nodeControl.h"

void CDMSchannel::SetAmpGain(const float GainValue)
{
    const auto index = OgainTableIndex(GainValue);
    const auto igain = static_cast<CPGA280::igain>(index / 2);
    const auto ogain = static_cast<CPGA280::ogain>(index % 2);
    if (m_pPGA->SetGains(igain, ogain)) {
        m_nGainIndex = index;
        m_ActualAmpGain = ogain_table[index];
        UpdateOffsets();
    }
}
void CDMSchannel::UpdateOffsets()
{
    //apply offsets only if calibration is enabled
    if(!m_pCont->IsCalEnabled())
        return;

    std::string strError;
    hat::Calibration_map cmap;
    m_pCont->GetCalibrationData(cmap, strError);

    using Type = hat::atom::Calibration::Type;
    const auto atom = mes_mode::Voltage == m_MesMode ? Type::v_in1 : Type::c_in1;
    const hat::atom::Calibration::Type type{static_cast<std::uint16_t>(atom) +
      static_cast<std::uint16_t>(m_nChanInd)};
    const auto& entry = cmap.get_atom(type).get_entry(m_nGainIndex, strError);
    m_pDAC->SetRawOutput(entry.get_b());
}
