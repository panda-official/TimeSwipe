/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


//implementing emulation of node control:

#include "nodeControl.h"

std::shared_ptr<CADmux>  nodeControl::m_pMUX;
std::shared_ptr<CCalMan> nodeControl::m_pZeroCal;

//CADmux *m_pMUX;
//CCalMan *m_pZeroCal;

static bool brecord=false;

bool nodeControl::IsRecordStarted(){ return brecord;}
void nodeControl::StartRecord(const bool how)
{
   // brecord=how;

    //make a stamp:
    typeLEDcol rstamp=nodeLED::gen_rnd_col();

    //19.06.2019: generate an event:
    nlohmann::json v=rstamp;
    Instance().Fire_on_event("Record", v);

    //blink: here???
    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, rstamp, 3, 300);
}

int nodeControl::gain_out(int val)
{
    typeADgain sgain=typeADgain::gainX1;
     switch(val)
     {
       // case 1: sgain=typeADgain::gainX1; break;
        case 2: sgain=typeADgain::gainX2; break;
        case 3: sgain=typeADgain::gainX4; break;
        case 4: sgain=typeADgain::gainX8;
     }
     m_pMUX->SetGain(sgain);
     int rv=(int)m_pMUX->GetGain();

     //19.06.2019: generate an event:
     nlohmann::json v=rv;
     Instance().Fire_on_event("Gain", v);

     return rv;
}
int nodeControl::GetGain(){ return (int)m_pMUX->GetGain(); }
bool nodeControl::GetBridge()
{
    return m_pMUX->GetUBRVoltage();
}
void nodeControl::SetBridge(bool how)
{
    m_pMUX->SetUBRvoltage(how);

    //19.06.2019: generate an event:
    nlohmann::json v=how;
    Instance().Fire_on_event("Bridge", v);
}
void nodeControl::SetZero(bool how)
{
    //put an event inside zero proc???
    //no, just reflect the action...

   /* nlohmann::json v=how;
    Instance().Fire_on_event("Zero", v);*/ //15.07.2019 ->moved to zero_cal_man

    if(how)
        m_pZeroCal->Start();
    else
        m_pZeroCal->StopReset();
}
