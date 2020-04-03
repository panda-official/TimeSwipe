/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "nodeControl.h"
#include "DataVis.h"


std::shared_ptr<CADmux>  nodeControl::m_pMUX;
std::shared_ptr<CCalMan> nodeControl::m_pZeroCal;

static bool brecord=false;
static std::vector<CDataVis>  m_DataVis;

void nodeControl::CreateDataVis(const std::shared_ptr<CAdc> &pADC, CView::vischan nCh) //const std::shared_ptr<CLED> &pLED)
{
    m_DataVis.emplace_back(pADC, nCh); //pLED);
}
void nodeControl::Update()
{
    for(auto &el : m_DataVis) el.Update();
}

bool nodeControl::IsRecordStarted(){ return brecord;}
void nodeControl::StartRecord(const bool how)
{
    //make a stamp:
    static unsigned long count_mark=0;
    count_mark++;

    //generate an event:
    nlohmann::json v=count_mark;
    Instance().Fire_on_event("Record", v);
    //CView::Instance().SetRecordMarker();
}

int nodeControl::gain_out(int val)
{
    typeADgain sgain=typeADgain::gainX1;
     switch(val)
     {
        case 2: sgain=typeADgain::gainX2; break;
        case 3: sgain=typeADgain::gainX4; break;
        case 4: sgain=typeADgain::gainX8;
     }
     m_pMUX->SetGain(sgain);
     int rv=(int)m_pMUX->GetGain();

     //generate an event:
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

    //generate an event:
    nlohmann::json v=how;
    Instance().Fire_on_event("Bridge", v);
}

void nodeControl::SetSecondary(int nMode)
{
    nMode&=1; //fit the value

    m_pMUX->SetUBRvoltage(nMode);

    //generate an event:
    nlohmann::json v=nMode;
    Instance().Fire_on_event("SetSecondary", v);

}
int nodeControl::GetSecondary()
{
    return m_pMUX->GetUBRVoltage();
}

void nodeControl::SetZero(bool how)
{
    if(how)
        m_pZeroCal->Start();
    else
        m_pZeroCal->StopReset();
}
