/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "../../common/gain.hpp"
#include "../../common/HatsMemMan.h"
#include "base/DMSchannel.h"
#include "control/nodeControl.h"

/// FIXME: namespace
using namespace panda::timeswipe;

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
    CHatAtomCalibration cdata;
    m_pCont->GetCalibrationData(cdata, strError);

    CCalAtomPair pair;
    const auto atom = mes_mode::Voltage == m_MesMode ? CCalAtom::atom_type::V_In1 : CCalAtom::atom_type::C_In1;
    cdata.GetCalPair(atom + static_cast<std::size_t>(m_nChanInd), m_nGainIndex, pair, strError);
    m_pDAC->SetRawOutput(pair.b);
}
