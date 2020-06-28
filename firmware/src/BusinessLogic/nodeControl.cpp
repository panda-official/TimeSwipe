/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "nodeControl.h"
#include "DataVis.h"


/*std::shared_ptr<CADmux>  nodeControl::m_pMUX;
std::shared_ptr<CCalMan> nodeControl::m_pZeroCal;
std::shared_ptr<IPin>    nodeControl::m_pUBRswitch;
std::shared_ptr<CDac>    nodeControl::m_pVoltageDAC;


static bool brecord=false;
static std::vector<CDataVis>  m_DataVis;
nodeControl::MesModes nodeControl::m_OpMode=nodeControl::IEPE;


//float nodeControl::m_Voltage=0;
float nodeControl::m_Current=0;
float nodeControl::m_MaxCurrent=1000;  //mA*/

nodeControl::nodeControl()
{
    //m_DataVis.reserve(4);

    m_pMesChans.reserve(4);

    Instance().m_PersistStorage.AddItem(this->shared_from_this());
}

void nodeControl::Serialize(CStorage &st)
{
    m_OffsetSearch.Serialize(st);
    if(st.IsDefaultSettingsOrder())
    {
    //    SetGain(typeADgain::gainX1);
      //  SetUBRvoltage(false);
    }

    //st.ser( *((int*)&m_CurGain) ).ser(m_UBRVoltage);

    if(st.IsDownloading())
    {
      //  SetGain(m_CurGain);
        //SetUBRvoltage(m_UBRVoltage);
    }
}


/*void nodeControl::CreateDataVis(const std::shared_ptr<CAdc> &pADC, CView::vischan nCh)
{
    m_DataVis.emplace_back(pADC, nCh);
}*/
void nodeControl::Update()
{
    //for(auto &el : m_DataVis) el.Update();

    Instance().m_PersistStorage.Update();
    m_OffsetSearch.Update();

    //m_pZeroCal->Update();
}

//bool nodeControl::IsRecordStarted(){ return brecord;}
void nodeControl::StartRecord(bool how)
{
    //make a stamp:
    static unsigned long count_mark=0;
    count_mark++;

    //generate an event:
    nlohmann::json v=count_mark;
    Instance().Fire_on_event("Record", v);
}

int nodeControl::gain_out(int val)
{
    /*typeADgain sgain=typeADgain::gainX1;
     switch(val)
     {
        case 2: sgain=typeADgain::gainX2; break;
        case 3: sgain=typeADgain::gainX4; break;
        case 4: sgain=typeADgain::gainX8;
     }
     m_pMUX->SetGain(sgain);
     int rv=(int)m_pMUX->GetGain();*/

     //int rv=val;

     //set old IEPE gain:
     if(nodeControl::IEPE==m_OpMode)
     {
         int gset=val-1;
         m_pGain1pin->Set(gset>>1);
         m_pGain0pin->Set(gset&1);
     }
     else //DMS board
     {
         //set individual gain for each chan:
         //.................................

     }


     //generate an event:
     nlohmann::json v=val;
     Instance().Fire_on_event("Gain", v);

     return val;
}
//int nodeControl::GetGain(){ return (int)m_pMUX->GetGain(); }
bool nodeControl::GetBridge()
{
    //return m_pMUX->GetUBRVoltage();
    assert(m_pUBRswitch);
    return m_pUBRswitch->Get();

}
void nodeControl::SetBridge(bool how)
{
    //m_pMUX->SetUBRvoltage(how);
     assert(m_pUBRswitch);
     m_pUBRswitch->Set(how);


    //generate an event:
    nlohmann::json v=how;
    Instance().Fire_on_event("Bridge", v);
}

void nodeControl::SetSecondary(int nMode)
{
    nMode&=1; //fit the value

    //m_pMUX->SetUBRvoltage(nMode ? false:true);
    assert(m_pUBRswitch);
    m_pUBRswitch->Set(nMode);

    //generate an event:
    nlohmann::json v=nMode;
    Instance().Fire_on_event("Mode", v);

}
int nodeControl::GetSecondary()
{
    //return m_pMUX->GetUBRVoltage() ? 0:1;
    return GetBridge();
}


void nodeControl::SetMode(int nMode)
{
    m_OpMode=static_cast<MesModes>(nMode);
    if(m_OpMode<MesModes::IEPE) { m_OpMode=MesModes::IEPE; }
    if(m_OpMode>MesModes::Normsignal){ m_OpMode=MesModes::Normsignal; }

    SetSecondary(m_OpMode);
}
int nodeControl::GetMode()
{
    return GetSecondary();
}


void nodeControl::SetOffset(int nOffs)
{
    switch(nOffs)
    {
        case 1: //negative
            m_OffsetSearch.Start(4000);
        break;

        case 2: //zero
            m_OffsetSearch.Start();
        break;

        case 3: //positive
            m_OffsetSearch.Start(100);
        break;

        default:
            nOffs=0;
            m_OffsetSearch.StopReset();
        return;
    }

    nlohmann::json v=nOffs;
    Instance().Fire_on_event("Offset", v);
}
