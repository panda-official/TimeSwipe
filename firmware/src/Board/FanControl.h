/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "SamTempSensor.h"
#include "PINPWM.h"
#include "os.h"

class CFanControl
{
protected:
    float m_TempMinC0;
    float m_TempMaxC0;
    float m_TempRangeC0;

    unsigned int m_MinFreqHz;
    unsigned int m_MaxFreqHz;
    unsigned int m_FreqRangeHz;

    unsigned int m_FanSpeeds=4;
    unsigned int m_CurSpeed=0;

    /*!
     * \brief Last time when update() method was called, mSec
     */
    unsigned long m_last_time_upd_mS=os::get_tick_mS();

    /*!
     * \brief Minimum time between two consecutive updates
     */
    unsigned long m_upd_quant_mS=200;


    std::shared_ptr<CSamTempSensor> m_pTempSens;
    std::shared_ptr<CPinPWM>        m_pPWM;


public:
    CFanControl(std::shared_ptr<CSamTempSensor> &pTempSens, std::shared_ptr<CPinPWM> &pPWM,
                float MinTempC0=20.0f, float MaxTempC0=60.0f,
                unsigned int MinFreqHz=1000, unsigned int MaxFreqHz=9000);

    void Update();
};
