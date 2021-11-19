/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "FanControl.h"

CFanControl::CFanControl(std::shared_ptr<CSamTempSensor> &pTempSens, std::shared_ptr<CPinPWM> &pPWM, float MinTempC0, float MaxTempC0, float MinDuty, float MaxDuty)
{

    m_TempMinC0=MinTempC0;
    m_TempMaxC0=MaxTempC0;
    m_TempRangeC0=MaxTempC0-MinTempC0;

    m_MinDuty=MinDuty;
    m_MaxDuty=MaxDuty;
    m_DutyRange=MaxDuty-MinDuty;

    m_pTempSens=pTempSens;
    m_pPWM=pPWM;
    m_pPWM->SetFrequency(100);

}

void CFanControl::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool CFanControl::GetEnabled() const noexcept
{
    return m_enabled;
}

void CFanControl::Update()
{
    //! check if minimum updation time has elapsed since last updation
    unsigned long elapsed=os::get_tick_mS()-m_last_time_upd_mS;
    if(elapsed<m_upd_quant_mS)
        return;

    m_pTempSens->Update();

    int speed= static_cast<int>(m_FanSpeeds*(m_pTempSens->GetTempCD()-m_TempMinC0)/m_TempRangeC0);
    if(speed<0)
        speed=0;
    else if(speed>=m_FanSpeeds)
        speed=m_FanSpeeds-1;

    m_CurSpeed=speed;

    if(0==speed)
    {
        m_pPWM->Start(false);
        return;
    }

    float duty=((float)m_CurSpeed/(m_FanSpeeds-1))*m_DutyRange + m_MinDuty;

    m_pPWM->SetDutyCycle(duty);
    if(m_enabled == true)
    {
        m_pPWM->Start(true);
    } else {
        m_pPWM->Start(false);
    }
}
