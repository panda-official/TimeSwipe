/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "FanControlSimple.h"
#include "sam.h"

CFanControlSimple::CFanControlSimple(std::shared_ptr<CSamTempSensor> &pTempSens, CSamPORT::group nGroup, CSamPORT::pin nPin,
                                     float TempOnC0, float TempOffC0)
{
    m_pTempSens=pTempSens;

    m_TempOnC0      =TempOnC0;
    m_TempOffC0     =TempOffC0;

    m_PortGroup     =nGroup;
    m_PortPin       =nPin;

    PORT->Group[nGroup].DIRSET.reg=(1L<<nPin);
    PORT->Group[nGroup].OUTCLR.reg=(1L<<nPin);
}

void CFanControlSimple::Update()
{
    //! check if minimum updation time has elapsed since last updation
    unsigned long elapsed=os::get_tick_mS()-m_last_time_upd_mS;
    if(elapsed<m_upd_quant_mS)
        return;

    m_pTempSens->Update();
    float MesTemp=m_pTempSens->GetTempCD();
    if(MesTemp>=m_TempOnC0)
    {
        PORT->Group[m_PortGroup].OUTSET.reg=(1L<<m_PortPin);
    }
    else if(MesTemp<=m_TempOffC0)
    {
        PORT->Group[m_PortGroup].OUTCLR.reg=(1L<<m_PortPin);
    }
}
