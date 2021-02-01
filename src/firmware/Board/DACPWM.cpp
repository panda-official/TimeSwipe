/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "DACPWM.h"

CDacPWM::CDacPWM(const std::shared_ptr<CDac> &pDAC, const std::shared_ptr<CPin> &pDACsw)
{
    m_pDAC=pDAC;
    m_pDACsw=pDACsw;
}

void CDacPWM::on_obtain_half_periods()
{

}
void CDacPWM::impl_Start(bool bHow)
{
    if(bHow)
    {
        m_pDACsw->Set(true);
        m_pDAC->SetRawOutput(m_prmHighLevel);
    }
    else
    {
        m_pDAC->SetRawOutput(0); //DAC off
    }
}
void CDacPWM::impl_LoadNextHalfPeriod()
{
    m_pDAC->SetRawOutput( m_CurHalfPeriodIndex ? m_prmLowLevel:m_prmHighLevel );
}

