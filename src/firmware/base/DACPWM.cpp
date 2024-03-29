/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "DACPWM.h"

CDacPWM::CDacPWM(const std::shared_ptr<Dac_channel> &pDAC, const std::shared_ptr<Pin> &pDACsw)
{
    m_pDAC=pDAC;
    m_pDACsw=pDACsw;
}

void CDacPWM::on_obtain_half_periods()
{

}

void CDacPWM::impl_Start(const bool bHow)
{
  if (bHow) {
    m_pDACsw->write(true);
    m_pDAC->set_raw(m_prmHighLevel);
  } else
    m_pDAC->set_raw(0); //DAC off
}

void CDacPWM::impl_LoadNextHalfPeriod()
{
    m_pDAC->set_raw(m_CurHalfPeriodIndex ? m_prmLowLevel : m_prmHighLevel);
}
