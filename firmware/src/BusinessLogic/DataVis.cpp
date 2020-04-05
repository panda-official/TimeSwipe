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

CDataVis::CDataVis(const std::shared_ptr<CAdc> &pADC, CView::vischan nCh)
{
    m_pADC=pADC;
    m_nCh=nCh;
    last_time_vis=os::get_tick_mS();
}

void CDataVis::reset()
{
    int meas1=m_pADC->GetRawBinVal();

    meas_max = meas1 + min_wind/2;
    if(meas_max > 4095){
        meas_max = 4095;
    }

    meas_min = meas1 - min_wind/2;
    if(meas_min < 0){
        meas_min = 0;
    }
    senscon_chan=false;
}
void CDataVis::Update()
{
    //quataion:
    if( (os::get_tick_mS()-last_time_vis)<m_upd_tspan_mS )
        return;
    last_time_vis=os::get_tick_mS();

    if(first_update == true){
        CDataVis::reset();
        first_update = false;
        m_upd_tspan_mS=17;
    }

    //obtaining color val:
    int meas1=m_pADC->GetRawBinVal();
    int cur_window=meas_max-meas_min+1;

    //check drop-out:
    int drop_out_trhold=static_cast<int>(cur_window*drop_out_factor);
    if( ((meas_max-meas1)>drop_out_trhold) || ((meas1-meas_min)>drop_out_trhold) )
    {
        reset();
        return;
    }

    if(meas1 > meas_max){
        meas_max = meas1;
        senscon_chan=true;
    }
    else if(meas1 < meas_min){
        meas_min = meas1;
        senscon_chan=true;
    }
    if(senscon_chan)
    {
        if((meas_max-meas1) > min_dist)
             meas_max = static_cast<int>(meas_max - (k_range *(meas_max - meas1)));
        if((meas1-meas_min) > min_dist)
             meas_min = static_cast<int>(meas_min + (k_range *(meas1 - meas_min)));
    }
    else return;

    float Inorm=( pow(b_brght, static_cast<float>(meas1-meas_min)/cur_window ) -1.0f)*bright_factor;
    if(Inorm<ILowLim)
    {
        Inorm=ILowLim; //prevent flickering
    }
    if(Inorm>1.0f)
    {
        Inorm=1.0f;
    }
    CView::Instance().GetChannel(m_nCh).SetSensorIntensity(Inorm);
}

