/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "DataVis.h"
#include <math.h>

unsigned long get_tick_mS(void);
CDataVis::CDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED)
{
    m_pADC=pADC;
    m_pLED=pLED;
    last_time_vis=get_tick_mS(); //set initial delay of 1 sec...
}

void CDataVis::reset()
{
    unsigned int meas1=m_pADC->DirectMeasure();

    meas_max = meas1 + min_wind/2;
    meas_min = meas1 - min_wind/2;
}

void CDataVis::Start(bool bHow, unsigned long nDelay_mS)
{
    m_bStarted=bHow;
    m_upd_tspan_mS=nDelay_mS;

    if(bHow)
    {
        m_bStartInitOder=true;
    }
    else
    {
        m_pLED->ON(false);
    }
}

void CDataVis::Update()
{
    //quataion:
    if( (get_tick_mS()-last_time_vis)<m_upd_tspan_mS )
        return;

    m_upd_tspan_mS=20; //some default value for a fast updation
    last_time_vis=get_tick_mS();

    if(!m_bStarted)
        return;

    if(m_bStartInitOder)                //dbg...this all should be moved into the "View" instance
    {
        m_pLED->SetBlinkMode(false);
        m_pLED->SetColor(0);
        m_pLED->ON(true);
        m_bStartInitOder=false;
    }


    //obtaining color val:
    unsigned int meas1=m_pADC->DirectMeasure();

    if(meas1 > meas_max){
        meas_max = meas1;
    }
    if(meas1 < meas_min){
        meas_min = meas1;
    }

    unsigned int intens1=static_cast<unsigned int>((pow(b_brght, static_cast<float>(meas1-meas_min)/(meas_max-meas_min+1))-1)/(b_brght-1) * 256.0);
    m_pLED->SetColor(intens1*65536);
}
