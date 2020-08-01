/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "os.h"
#include "DataVis.h"
#include "View.h"
#include <math.h>

#include "board_type.h"

CDataVis::CDataVis(CView::vischan nCh)
{
   // m_pADC=pADC;
    m_nCh=nCh;
    last_time_vis=os::get_tick_mS()+1000;
    m_MA.SetPeriod(120);
}

void CDataVis::Update(float InputValue)
{
    //quataion:
    if( static_cast<int>(os::get_tick_mS()-last_time_vis)<m_upd_tspan_mS )
        return;
    last_time_vis=os::get_tick_mS();

    //----------pre-averaging: reduces overall calculation work-------
    m_AvSumm+=InputValue;

    if(++m_MesCounter<m_AvPeriod)
        return;


    float rawval=m_AvSumm/m_AvPeriod;
    m_MesCounter=0;
    m_AvSumm=0;
    //-----------------------------------------------------------------


    //--------------obtain MA/StdDev----------------------------------
    float ma=m_MA.ObtainMA(rawval);
    if(m_MA.GetCurSize()<m_StdDevPer)
        return;

    float ds=rawval-ma;
    if(--m_StdDevRecalcCountDown<=0)
    {
        m_StdDevRecalcCountDown=m_StdDevPer;
        m_CurStdDev=m_MA.ObtainStdDev(m_StdDevPer);
        m_HalfRange=m_CurStdDev*m_InflationFactor;
        if(m_HalfRange<m_DetectThrhold)
            m_HalfRange=m_DetectThrhold;
    }
    //-----------------------------------------------------------------


    //-------------------sensor detection------------------------------
    if(!m_bSensorDetected)
    {
        if( std::abs(ds) < m_DetectThrhold )
        {
           // m_ZeroLevel=ma; //set zero level
            return;
        }

        m_bSensorDetected=true;
    }
    //-----------------------------------------------------------------

    //--------------------drop detection-------------------------------
    /*if( std::abs(m_ZeroLevel-ma) < m_DropThrhold &&  m_CurStdDev<m_DropThrhold )
    {
        m_bSensorDetected=false;
        CView::Instance().GetChannel(m_nCh).SetSensorIntensity(0);
        return;
    }*/
    //-----------------------------------------------------------------


    //--------------calculate the signal level-------------------------
    float out=(ds/m_HalfRange)*0.5f + 0.5f;

    float Inorm=( pow(b_brght, out) -1.0f)*bright_factor;
    if(Inorm<ILowLim)
    {
        Inorm=ILowLim; //prevent flickering
    }
    if(Inorm>1.0f)
    {
        Inorm=1.0f;
    }
    CView::Instance().GetChannel(m_nCh).SetSensorIntensity(Inorm);
    //-----------------------------------------------------------------
}


