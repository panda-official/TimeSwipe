/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "SamTempSensor.h"
#include"SamPORT.h"
#include "os.h"

class CFanControlSimple
{
protected:
    float m_TempOnC0;
    float m_TempOffC0;

    CSamPORT::group m_PortGroup;
    CSamPORT::pin   m_PortPin;

    /*!
     * \brief Last time when update() method was called, mSec
     */
    unsigned long m_last_time_upd_mS=os::get_tick_mS();

    /*!
     * \brief Minimum time between two consecutive updates
     */
    unsigned long m_upd_quant_mS=200;


    std::shared_ptr<CSamTempSensor> m_pTempSens;


public:
    CFanControlSimple(std::shared_ptr<CSamTempSensor> &pTempSens, CSamPORT::group nGroup, CSamPORT::pin nPin,
                float TempOnC0=40.0f, float TempOffC0=35.0f);

    void Update();
};
