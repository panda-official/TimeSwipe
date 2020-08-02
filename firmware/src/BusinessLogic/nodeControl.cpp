/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "nodeControl.h"
#include "DataVis.h"


nodeControl::nodeControl()
{
    m_pMesChans.reserve(4);
}

void nodeControl::Serialize(CStorage &st)
{
    m_OffsetSearch.Serialize(st);
    if(st.IsDefaultSettingsOrder())
    {
        SetGain(1);
        SetBridge(false);
        SetSecondary(0);
    }

    st.ser(m_GainSetting).ser(m_BridgeSetting).ser(m_SecondarySetting);

    if(st.IsDownloading())
    {
        SetGain(m_GainSetting);
        SetBridge(m_BridgeSetting);
        SetSecondary(m_SecondarySetting);
    }
}

void nodeControl::Update()
{
    for(auto &el : m_pMesChans) el->Update();

    m_PersistStorage.Update();
    m_OffsetSearch.Update();
}
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
    //update channels gain setting:
    float gval=val;
    m_GainSetting=val;
    for(auto &el : m_pMesChans) el->SetAmpGain(gval);


     //set old IEPE gain:
     if(typeBoard::IEPEBoard==m_BoardType)
     {
         int gset=val-1;
         m_pGain1pin->Set(gset>>1);
         m_pGain0pin->Set(gset&1);
     }


     //generate an event:
     nlohmann::json v=val;
     Instance().Fire_on_event("Gain", v);

     return val;
}
bool nodeControl::GetBridge()
{
    return m_BridgeSetting;
}
void nodeControl::SetBridge(bool how)
{
     m_BridgeSetting=how;

     if(typeBoard::IEPEBoard!=m_BoardType)
     {
        assert(m_pUBRswitch);
        m_pUBRswitch->Set(how);
     }


    //generate an event:
    nlohmann::json v=how;
    Instance().Fire_on_event("Bridge", v);
}

void nodeControl::SetSecondary(int nMode)
{
    nMode&=1; //fit the value

    m_SecondarySetting=nMode;
}
int nodeControl::GetSecondary()
{
    return m_SecondarySetting;
}


void nodeControl::SetMode(int nMode)
{
    m_OpMode=static_cast<MesModes>(nMode);
    if(m_OpMode<MesModes::IEPE) { m_OpMode=MesModes::IEPE; }
    if(m_OpMode>MesModes::Normsignal){ m_OpMode=MesModes::Normsignal; }


    if(typeBoard::IEPEBoard==m_BoardType) //old IEPE board setting
    {
        //SetBridge(m_OpMode);
        assert(m_pUBRswitch);
        m_pUBRswitch->Set(MesModes::IEPE==m_OpMode ? true:false);
    }

    //switch all channels to IEPE:
    for(auto &el : m_pMesChans) el->IEPEon(MesModes::IEPE==m_OpMode);

    SetSecondary(m_OpMode);

    //generate an event:

    nlohmann::json v=nMode;
    Instance().Fire_on_event("Mode", v);
}
int nodeControl::GetMode()
{
    return m_OpMode;
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
