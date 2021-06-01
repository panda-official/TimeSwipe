/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "../../common/HatsMemMan.h"
#include "base/DMSchannel.h"
#include "control/nodeControl.h"

void CDMSchannel::SetAmpGain(float GainValue)
{
    constexpr float ogain_factor{1.375};
    constexpr float GainTab[]={
        1,
        1*ogain_factor,
        2,
        2*ogain_factor,
        4,
        4*ogain_factor,
        8,
        8*ogain_factor,
        16,
        16*ogain_factor,
        32,
        32*ogain_factor,
        64,
        64*ogain_factor,
        128,
        128*ogain_factor,
        256,
        256*ogain_factor,
        512,
        512*ogain_factor,
        1024,
        1024*ogain_factor
    };
    constexpr size_t tsize{sizeof(GainTab)/sizeof(float)};
    static_assert(!(tsize % 2));
    size_t el{};
    for(; el<tsize; el++)
    {
        if(GainValue<GainTab[el])
            break;
    }
    if(el>0)
        el--;


    if(m_pPGA->SetGains( static_cast<CPGA280::igain>(el/2), static_cast<CPGA280::ogain>(el%2) ))
    {
        m_nGainIndex=el;
        m_ActualAmpGain=GainTab[el];
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
    cdata.GetCalPair( (mes_mode::Voltage==m_MesMode ? CCalAtom::atom_type::V_In1 : CCalAtom::atom_type::C_In1) + static_cast<size_t>(m_nChanInd),

                      m_nGainIndex, pair, strError);


    m_pDAC->SetRawOutput(pair.b);

}
