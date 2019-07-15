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

    last_time_vis=get_tick_mS()-1000; //set initial delay of 1 sec...
}

void CDataVis::reset()
{
    unsigned int meas1=m_pADC->DirectMeasure();

    meas_max = meas1 + min_wind/2;
    meas_min = meas1 - min_wind/2;
}

void CDataVis::Update()
{
    //quataion:
    if( (get_tick_mS()-last_time_vis)<1000 )
        return;
    last_time_vis=get_tick_mS();

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
